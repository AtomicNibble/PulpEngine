#include "stdafx.h"
#include "IMaterial.h"
#include "MaterialManager.h"

#include <IFileSys.h>
#include <IShader.h>
#include <IRender.h>
#include <IConsole.h>
#include <ITexture.h>

#include "TechDefStateManager.h"
#include "Drawing\VariableStateManager.h"
#include "Drawing\CBufferManager.h"
#include "Texture\Texture.h"
#include "Texture\TextureManager.h"

#include <CBuffer.h>
#include <Sampler.h>
#include <Texture.h>


X_NAMESPACE_BEGIN(engine)

using namespace render::shader;


XMaterialManager::XMaterialManager(core::MemoryArenaBase* arena, VariableStateManager& vsMan, CBufferManager& cBufMan) :
	arena_(arena),
	pTechDefMan_(nullptr),
	cBufMan_(cBufMan),
	vsMan_(vsMan),
	materials_(arena, sizeof(MaterialResource), core::Max<size_t>(8u,X_ALIGN_OF(MaterialResource))),
	pDefaultMtl_(nullptr)
{
	pTechDefMan_ = X_NEW(TechDefStateManager, arena, "TechDefStateManager")(arena);
}

XMaterialManager::~XMaterialManager()
{
	if (pTechDefMan_) {
		X_DELETE(pTechDefMan_, arena_);
	}
}


void XMaterialManager::registerCmds(void)
{

	ADD_COMMAND_MEMBER("listMaterials", this, XMaterialManager, &XMaterialManager::Cmd_ListMaterials,
		core::VarFlag::SYSTEM, "List all the loaded materials");

}

void XMaterialManager::registerVars(void)
{


}

bool XMaterialManager::init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);

	if (!initDefaults()) {
		return false;
	}

	// hotreload support.
	gEnv->pHotReload->addfileType(this, MTL_FILE_EXTENSION);
	gEnv->pHotReload->addfileType(this, MTL_B_FILE_EXTENSION);

	return true;
}

void XMaterialManager::shutDown(void)
{
	X_LOG0("MtlManager", "Shutting Down");

	// hotreload support.
	gEnv->pHotReload->unregisterListener(this);

	if (pDefaultMtl_) {
		releaseMaterial(pDefaultMtl_);
	}

	freeDanglingMaterials();

	if (pTechDefMan_) {
		pTechDefMan_->shutDown();
	}
}

bool XMaterialManager::asyncInitFinalize(void)
{

	return true;
}


// IMaterialManager

Material* XMaterialManager::findMaterial(const char* pMtlName) const
{
	core::string name(pMtlName);

	Material* pMtl = findMaterial_Internal(name);
	if (pMtl) {
		return pMtl;
	}

	X_WARNING("MatMan", "Failed to find material: \"%s\"", pMtlName);
	return nullptr;
}

Material* XMaterialManager::loadMaterial(const char* pMtlName)
{
	X_ASSERT_NOT_NULL(pMtlName);
	X_ASSERT(core::strUtil::FileExtension(pMtlName) == nullptr, "Extension not allowed")(pMtlName);

	core::string name(pMtlName);
	core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

	MaterialResource* pMatRes = materials_.findAsset(name);
	if (pMatRes)
	{
		// inc ref count.
		pMatRes->addReference();
		return pMatRes;
	}

	pMatRes = materials_.createAsset(name, g_3dEngineArena);
	pMatRes->setName(name);

	// now we want to dispatch a load!
	// do we want to have the loading login in the manager or 

	return pMatRes;
}

void XMaterialManager::releaseMaterial(Material* pMat)
{
	MaterialResource* pMatRes = reinterpret_cast<MaterialResource*>(pMat);
	if (pMatRes->removeReference() == 0)
	{
		releaseMaterialResources(pMatRes);

		materials_.releaseAsset(pMatRes);
	}
}

Material::Tech* XMaterialManager::getTechForMaterial(Material* pMat, core::StrHash techNameHash, render::shader::VertexFormat::Enum vrtFmt,
	PermatationFlags permFlags)
{
	X_ASSERT_NOT_NULL(pMat);

	// the material holds all it's techs like a cache, but when it don't have one we must create it.
	auto* pTech = pMat->getTech(techNameHash, vrtFmt, permFlags);
	if (pTech) {
		return pTech;
	}

	if (pMat->isDefault())
	{
		// if this is not real material and we want to return default material techs.
		pTech = pDefaultMtl_->getTech(techNameHash, vrtFmt, permFlags);
		if (!pTech)
		{
			pTech = getTechForMaterial_int(pDefaultMtl_, techNameHash, vrtFmt, permFlags);
		}

		return pTech;
	}

	return getTechForMaterial_int(pMat, techNameHash, vrtFmt, permFlags);

}

Material::Tech* XMaterialManager::getTechForMaterial_int(Material* pMat, core::StrHash techNameHash, render::shader::VertexFormat::Enum vrtFmt,
	PermatationFlags permFlags)
{
	X_ASSERT_NOT_NULL(pMat);

	// we must get the techDef so we can select the tech definition.
	TechDefState* pTechDefState = pMat->getTechDefState();

	X_ASSERT_NOT_NULL(pTechDefState);

	// now we have the tech we wnat to create a permatation of it supporting what we want.
	TechDef* pTechDef = pTechDefState->getTech(techNameHash);
	if (!pTechDef) {
		X_ERROR("MatMan", "Tech not found");
		return nullptr;
	}

	// we now have a permatation of the shader that we want.
	// this gives us the pipeline state handle.
	// we just now need to make variable state.
	TechDefPerm* pPerm = pTechDef->getOrCreatePerm(vrtFmt, permFlags);
	if (!pPerm) {
		X_ERROR("MatMan", "Failed to get tech perm");
		return nullptr;
	}


	render::shader::IShaderPermatation* pShaderPerm = pPerm->pShaderPerm;

	// from the shader perm we can see how many const buffers we need to provide.
	const auto& cbLinks = pShaderPerm->getCbufferLinks();
	const auto& permSamplers = pShaderPerm->getSamplers();
	const auto& permTextures = pShaderPerm->getTextures();

	// we need to know how many textures we are going to be sending.
	// it may be less than what the material has.
	// the tech should tell us O_O
	// him -> pTechDef
	const size_t numTex = permTextures.size();
	const size_t numSamplers = permSamplers.size();
	const size_t numCb = cbLinks.size();

	render::Commands::ResourceStateBase* pVariableState = vsMan_.createVariableState(
		numTex, 
		numSamplers, 
		numCb
	);

	// we should create the const buffers we need and set them in the variable state.
#if X_ENABLE_ASSERTIONS
	{
		auto* pCBHandles = pVariableState->getCBs();

		for (size_t i = 0; i < numCb; i++) {
			pCBHandles[i] = render::INVALID_BUF_HANLDE;
		}
	}
#endif // !X_ENABLE_ASSERTIONS

	{
		// need to map material textures to perm textures.
		// some textures have default values.
		// but we have the compiler set them, so always provided in material.
		// but some textures are set by code.
		auto* pTexStates = pVariableState->getTexStates();
		const auto& matTextures = pMat->getTextures();

		X_UNUSED(matTextures);

		auto* pTexMan = gEngEnv.pTextureMan_;
		auto* pDefaultTex = pTexMan->getDefault(render::TextureSlot::DIFFUSE);

		for (size_t i = 0; i < numTex; i++)
		{
			auto& texState = pTexStates[i];
			const auto& permTexture = permTextures[i];

			// find texture that matches from material.
			size_t j;
			for (j = 0; j < matTextures.size(); j++)
			{
				auto& t = matTextures[j];
				if (t.name == permTexture.getName())
				{
					// create the texture instance.
					// might get default back, etc..
					texState.textureId = t.pTexture->getDeviceID();
					break;
				}
			}

			if (pTechDef->getNumAliases())
			{
				const auto& aliases = pTechDef->getAliases();
				for (const auto& alias : aliases)
				{
					// find a alias that points to the persm resource.
					if (alias.resourceName == permTexture.getName())
					{
						if (alias.isCode)
						{
							// if we must assign the texture with code, make it default.
							texState.textureId = pDefaultTex->getDeviceID();
							goto texSet;
						}
						else
						{

							// okay so now we know the name of the material sampler that we want.
							for (j = 0; j < matTextures.size(); j++)
							{
								auto& t = matTextures[j];
								if (t.name == alias.name)
								{
									texState.textureId = t.pTexture->getDeviceID();
									break;
								}
							}
						}
					}
				}
			}

			// find one?
			if (j == matTextures.size())
			{
				X_ERROR("Material", "Failed to find texture values for perm texture: \"%s\" using default", permTexture.getName().c_str());
				
				// really we should know what type of texture would be set 
				// eg TextureSlot::Enum
				// this way we can return defaults that would actually not look retarded.


				texState.textureId = pDefaultTex->getDeviceID();
			}

		texSet:;
		}
	}

	{
		// so we need to map material samplers to perm samplers
		auto* pSamplers = pVariableState->getSamplers();
		const auto& matSamplers = pMat->getSamplers();

		for (size_t i = 0; i < numSamplers; i++)
		{
			auto& sampler = pSamplers[i];
			const auto& permSampler = permSamplers[i];

			// find a sampler that matches from material
			size_t j;
			for (j = 0; j < matSamplers.size(); j++)
			{
				auto& s = matSamplers[j];
				if (s.name == permSampler.getName())
				{
					sampler = s.sate;
					break;
				}
			}

			// do we want to lookup aliases even if we matched?
			if (pTechDef->getNumAliases())
			{
				const auto& aliases = pTechDef->getAliases();
				for (const auto& alias : aliases)
				{
					// find a alias that points to the persm resource.
					if (alias.resourceName == permSampler.getName())
					{
						// okay so now we know the name of the material sampler that we want.
						for (j = 0; j < matSamplers.size(); j++)
						{
							auto& s = matSamplers[j];
							if (s.name == alias.name)
							{
								sampler = s.sate;
								break;
							}
						}
					}
				}
			}

			if (j == matSamplers.size())
			{
				X_ERROR("Material", "Failed to find sampler values for perm sampler: \"%s\" using defaults", permSampler.getName().c_str());
				sampler.filter = render::FilterType::LINEAR_MIP_LINEAR;
				sampler.repeat = render::TexRepeat::TILE_BOTH;
			}
		}
	}


	// we now have a material tech that contains the pipeline state needed and what variable state also needs to be set 
	// we just add this to the materials local store, so it don't have to ask us for this next time.
	// so once everything has it's state to render anything we just have to check
	Material::Tech matTech(arena_);
	matTech.hashVal = techNameHash;
	matTech.pPerm = pPerm;
	matTech.pVariableState = pVariableState;

	// now we scan the cbuffers and try to match any material params to cbuffer params (todo: take into account techDef aliases).
	{
		auto* pCBHandles = pVariableState->getCBs();

		core::FixedArray<const render::shader::XCBuffer*, render::shader::MAX_SHADER_CB_PER_PERM> cbuffers;
		for (auto& cb : cbLinks) {
			cbuffers.push_back(cb.pCBufer);
		}

		const auto& params = pMat->getParams();

		if (params.isNotEmpty())
		{
			auto& paramLinks = matTech.paramLinks;

			core::FixedArray<int32_t, render::shader::MAX_SHADER_CB_PER_PERM> materialCBIdxs;

			for (size_t i = 0; i < params.size(); i++)
			{
				auto& param = params[i];
				auto& name = param.name;
				core::StrHash nameHash(name.begin(), name.end());

				for (size_t j = 0; j < cbLinks.size(); j++)
				{
					auto& cbLink = cbLinks[j];
					auto& cb = *cbLink.pCBufer;

					for (int32_t p = 0; p < cb.getParamCount(); p++)
					{
						auto& cbParam = cb[p];

						if (cbParam.getNameHash() == nameHash && cbParam.getName() == name)
						{
							ParamLink link;
							link.paramIdx = safe_static_cast<int32_t>(i);
							link.cbIdx = safe_static_cast<int32_t>(j);
							link.cbParamIdx = safe_static_cast<int32_t>(p);

							paramLinks.push_back(link);

							if (std::find(materialCBIdxs.begin(), materialCBIdxs.end(), link.cbIdx) == materialCBIdxs.end()) {
								materialCBIdxs.append(link.cbIdx);
							}
						}
					}
				}
			}

			if (paramLinks.isNotEmpty())
			{
				// sort them so in cb order then param order.
				std::sort(paramLinks.begin(), paramLinks.end(), [](const ParamLink& lhs, const ParamLink& rhs) -> bool {			
					if (lhs.cbIdx != rhs.cbIdx) {
						return lhs.cbIdx < rhs.cbIdx;
					}
					return lhs.paramIdx < rhs.paramIdx;
				});

				// reserver so the memory address don't change when appending as we keep pointers.
				matTech.materialCbs.setGranularity(materialCBIdxs.size());
				matTech.materialCbs.reserve(materialCBIdxs.size());

				int32_t currentCbIdx = -1;  
				render::shader::XCBuffer matCb(arena_);

				auto addCB = [&](int32_t cbIdx, render::shader::XCBuffer& matCb) {
					matCb.postParamModify();
					X_ASSERT(matCb.containsUpdateFreqs(render::shader::UpdateFreq::MATERIAL), "Should contain per material params")();
					matTech.materialCbs.append(std::move(matCb));

					cbuffers[cbIdx] = &matTech.materialCbs.back();
				};

				for (auto& link : paramLinks)
				{
					if (currentCbIdx != link.cbIdx)
					{
						if (currentCbIdx != -1) {
							addCB(link.cbIdx, matCb);
						}

						currentCbIdx = link.cbIdx;
						auto& cbLink = cbLinks[link.cbIdx];
						auto& cb = *cbLink.pCBufer;

						matCb = cb; // make a copy.
					}

					// change the update freq.
					// potentially and param that's unkown could just be default marked as material param.
					// would make some things more simple.
					auto& cpuData = matCb.getCpuData();
					auto& cbParam = matCb[link.cbParamIdx];
					const auto& matParam = params[link.paramIdx];

					cbParam.setUpdateRate(render::shader::UpdateFreq::MATERIAL);

					// always vec4? humm.
					X_ASSERT(cpuData.size() >= (cbParam.getBindPoint() + sizeof(matParam.value)), "Overflow when writing mat param value to cbuffer")();
					std::memcpy(&cpuData[cbParam.getBindPoint()], &matParam.value, sizeof(matParam.value));
				}

				// might be better to not store these material cb instance for each tech and instead share them within a material
				// so if a material has multiple techs that have identical material cbuffers they could share them.
				addCB(currentCbIdx, matCb);
			}
		}

		// the 'cbuffers' arr can contain a mix of cbuffer link instances and material instances.
		X_ASSERT(cbLinks.size() == cbuffers.size(), "Links and cbuffer list should be same size")(cbLinks.size(), cbuffers.size());
		for (size_t i = 0; i < cbuffers.size(); i++)
		{
			auto* pCB = cbuffers[i];
			X_ASSERT_NOT_NULL(pCB);
			pCBHandles[i] = cBufMan_.createCBuffer(*pCB);
		}
	}

#if X_ENABLE_ASSERTIONS
	{
		auto* pCBHandles = pVariableState->getCBs();

		for (size_t i = 0; i < numCb; i++) {
			X_ASSERT(pCBHandles[i] != render::INVALID_BUF_HANLDE, "Cbuffer handle is invalid")();
		}
	}
#endif // !X_ENABLE_ASSERTIONS

	pMat->addTech(std::move(matTech));

	auto* pTech = pMat->getTech(techNameHash, vrtFmt, permFlags);

	return pTech;
}

bool XMaterialManager::setTextureID(Material* pMat, Material::Tech* pTech, core::StrHash texNameHash, texture::TexID id)
{
	TechDefState* pTechDefState = pMat->getTechDefState();

	TechDef* pTechDef = pTechDefState->getTech(core::StrHash(pTech->hashVal));

	if (pTechDef->getNumAliases() < 1) {
		return false;
	}

	auto& als = pTechDef->getAliases();

	for (size_t i=0; i<als.size(); i++)
	{
		const auto& alias = als[i];
		if (alias.nameHash == texNameHash)
		{
			if (!alias.isCode)
			{
				// this was just a plain name alias not a code one! TWAT!
				return false;
			}

			auto* pTextureStates = pTech->pVariableState->getTexStates();
			pTextureStates[i].textureId = id;
			return true;
		}
	}

	return false;
}

XMaterialManager::MaterialResource* XMaterialManager::loadMaterialCompiled(const core::string& matName)
{
	core::Path<char> path;
	path /= "materials/";
	path.setFileName(matName);
	path.setExtension(MTL_B_FILE_EXTENSION);

	MaterialHeader hdr;

	core::XFileMemScoped file;
	if (!file.openFile(path.c_str(), core::fileMode::READ)) {
		return false;
	}

	if (file.readObj(hdr) != sizeof(hdr)) {
		return false;
	}

	if (!hdr.isValid()) {
		return false;
	}

	if (hdr.numSamplers > MTL_MAX_SAMPLERS) {
		return false;
	}

	// we need to poke the goat.
	// so now materials store the cat and type which we can use to get techdef info.
	core::string catType;
	file.readString(catType);

	Material::SamplerArr samplers(arena_, hdr.numSamplers);
	Material::ParamArr params(arena_, hdr.numParams);
	Material::TextureArr textures(arena_, hdr.numTextures);

	// now samplers.
	for (uint8_t i = 0; i < hdr.numSamplers; i++)
	{
		MaterialSampler& sampler = samplers[i];

		file.readObj(sampler.sate);
		file.readString(sampler.name);
	}

	// now params.
	for (uint8_t i = 0; i < hdr.numParams; i++)
	{
		MaterialParam& param = params[i];
		file.readObj(param.value);
		file.readObj(param.type);
		file.readString(param.name);
	}

	// now textures.
	for (uint8_t i = 0; i < hdr.numTextures; i++)
	{
		MaterialTexture& tex = textures[i];
		file.readObj(tex.texSlot);
		file.readString(tex.name);
		file.readString(tex.val);
	}


#if X_DEBUG
	const auto left = file.remainingBytes();
	X_WARNING_IF(left > 0, "Material", "potential read fail, bytes left in file: %" PRIu64, left);
#endif // !X_DEBUG


	// so my fat one legged camel.
	// we get the techDefState for this material.
	// which tells us the possible techs this material supports.
	// the material can request 'permatations' from this like ones with diffrent vertex formats (if supported).
	// when you have a perm you will have a state handle that you can render with.
	// this is so for a given material you can render it:
	// 1: either instanced or none-instanced.
	// 2: with less vertex streams
	// 3: etc..
	// But when you get a diffrent perm it's likley you will need to make a diffrent variableState to pass with 
	// that perm for rendering.
	// also once you have a perm you can ask it what cbuffers it needs.
	TechDefState* pTechDefState = pTechDefMan_->getTechDefState(hdr.cat, catType);
	if (!pTechDefState) {
		return false;
	}

	// we want to load all the textures now.
	// so that we are not 'creating' refrences for every tech.
	texture::TextureFlags texFlags = texture::TextureFlags();

	for (auto& tex : textures)
	{
		auto* pTexture = gEngEnv.pTextureMan_->forName(tex.name.c_str(), texFlags);

		tex.pTexture = pTexture;
	}


	MaterialResource* pMatRes = createMaterial_Internal(matName);
	pMatRes->setTechDefState(pTechDefState);
	pMatRes->setParams(std::move(params));
	pMatRes->setSamplers(std::move(samplers));
	pMatRes->setTextures(std::move(textures));

	return pMatRes;
}

XMaterialManager::MaterialResource* XMaterialManager::createMaterial_Internal(const core::string& name)
{
	// internal create expects you to know no duplicates
	X_ASSERT(findMaterial_Internal(name) == nullptr, "Creating a material that already exsists")();
	
	core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

	auto* pMatRes = materials_.createAsset(name, g_3dEngineArena);
	pMatRes->setName(name);

	return pMatRes;
}

XMaterialManager::MaterialResource* XMaterialManager::findMaterial_Internal(const core::string& name) const
{
	core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

	return materials_.findAsset(name);
}


void XMaterialManager::releaseMaterialResources(Material* pMat)
{
	// when we release the material we need to clean up somethings.
	X_UNUSED(pMat);

}



bool XMaterialManager::initDefaults(void)
{
	if (pDefaultMtl_ == nullptr)
	{
		// this will be data driven soon.
		pDefaultMtl_ = loadMaterialCompiled(core::string(MTL_DEFAULT_NAME));
		if (!pDefaultMtl_) {
			return false;
		}

		// it's default :|
		pDefaultMtl_->setFlags(pDefaultMtl_->getFlags() | MaterialFlag::DEFAULT);
	}

	return true;
}


// ~IMaterialManager


void XMaterialManager::freeDanglingMaterials(void)
{
	{
		core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

		for (const auto& m : materials_)
		{
			auto* pMatRes = m.second;
			const auto& name = pMatRes->getName();

			releaseMaterialResources(pMatRes);
			X_WARNING("MtlManager", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pMatRes->getRefCount());
		}
	}

	materials_.free();
}

void XMaterialManager::listMaterials(const char* pSearchPatten) const
{
	core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

	core::Array<MaterialResource*> sorted_mats(g_3dEngineArena);
	sorted_mats.setGranularity(materials_.size());

	for (const auto& mat : materials_)
	{
		if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, mat.first))
		{
			sorted_mats.push_back(mat.second);
		}
	}

	std::sort(sorted_mats.begin(), sorted_mats.end(), [](MaterialResource* a, MaterialResource* b) {
			const auto& nameA = a->getName();
			const auto& nameB = b->getName();
			return nameA.compareInt(nameB) < 0;
		}
	);

	X_LOG0("Material", "------------ ^8Materials(%" PRIuS ")^7 ------------", sorted_mats.size());

	for(const auto* pMat : sorted_mats)
	{
		X_LOG0("Material", "^2\"%s\"^7 refs: %" PRIi32, pMat->getName(), pMat->getRefCount());
	}

	X_LOG0("Material", "------------ ^8Materials End^7 -----------");
}



// ICoreEventListener
void XMaterialManager::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
	X_UNUSED(event);
	X_UNUSED(wparam);
	X_UNUSED(lparam);



}
// ~ICoreEventListener


void XMaterialManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys);
#if 0
	const char* fileExt;

	fileExt = core::strUtil::FileExtension(name);
	if (fileExt)
	{
		if (core::strUtil::IsEqual(MTL_FILE_EXTENSION, fileExt))
		{
			X_LOG0("Material", "reload material: \"%s\"", name);


		}
		else if (core::strUtil::IsEqual(MTL_B_FILE_EXTENSION, fileExt))
		{
			//	X_LOG0("Material", "reload material: \"%s\"", name);


		}
	}
	return true;
#else
	X_UNUSED(name);
#endif
}




void XMaterialManager::Cmd_ListMaterials(core::IConsoleCmdArgs* Cmd)
{
	// optional search criteria
	const char* pSearchPatten = nullptr;

	if (Cmd->GetArgCount() >= 2) {
		pSearchPatten = Cmd->GetArg(1);
	}

	listMaterials(pSearchPatten);
}



X_NAMESPACE_END
