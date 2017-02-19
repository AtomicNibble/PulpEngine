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
#include <CBuffer.h>

#include "Drawing\CBufferManager.h"


X_NAMESPACE_BEGIN(engine)

using namespace render::shader;

namespace 
{

	// Commands

	void Cmd_ListMaterials(core::IConsoleCmdArgs* Cmd)
	{
		// optional search criteria
		const char* pSearchPatten = nullptr;

		if (Cmd->GetArgCount() >= 2) {
			pSearchPatten = Cmd->GetArg(1);
		}

		(static_cast<XMaterialManager*>(XEngineBase::getMaterialManager()))->ListMaterials(pSearchPatten);
	}


} // namespace

XMaterialManager::XMaterialManager(core::MemoryArenaBase* arena, VariableStateManager& vsMan) :
	arena_(arena),
	pTechDefMan_(nullptr),
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


bool XMaterialManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);

	if (!InitDefaults()) {
		return false;
	}

	ADD_COMMAND("listMaterials", Cmd_ListMaterials, core::VarFlag::SYSTEM, "List all the loaded materials");

	// hotreload support.
	gEnv->pHotReload->addfileType(this, MTL_FILE_EXTENSION);
	gEnv->pHotReload->addfileType(this, MTL_B_FILE_EXTENSION);

	return true;
}

void XMaterialManager::ShutDown(void)
{
	X_LOG0("MtlManager", "Shutting Down");


	// hotreload support.
	gEnv->pHotReload->addfileType(nullptr, MTL_FILE_EXTENSION);
	gEnv->pHotReload->addfileType(nullptr, MTL_B_FILE_EXTENSION);


	releaseMaterial(pDefaultMtl_);

	freeDanglingMaterials();

	if (pTechDefMan_) {
		pTechDefMan_->shutDown();
	}
}

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


// IMaterialManager
Material* XMaterialManager::createMaterial(const char* pMtlName)
{
	core::string name(pMtlName);

	MaterialResource* pMtl = findMaterial_Internal(name);
	if (pMtl) {
		// add ref?
		pMtl->addReference();
		X_WARNING("MtlMan", "Create material called with name of exsisting material: \"%s\"", pMtlName);
		return pMtl;
	}

	// now I kinda want the material manager to handle the creation of variable state.
	// and the storing of all the cbuffers and permatation for the materials.
	// basically the things we need to render something with the material.
//	render::shader::IShader* pShader = nullptr;




	return createMaterial_Internal(name);
}

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

#if X_DEBUG
	const char* pExt = core::strUtil::FileExtension(pMtlName);
	if(pExt)
	{
		// engine should not make requests for materials with a extensiob
		X_ERROR("MtlMan", "Invalid mtl name extension was included: %s", pExt);
		return getDefaultMaterial();
	}
#endif // !X_DEBUG

	core::string name(pMtlName);


	// try find it.
	MaterialResource* pMatRes = findMaterial_Internal(name);

	if (pMatRes)
	{
		// inc ref count.
		pMatRes->addReference();
		return pMatRes;
	}

	// only support loading compile materials now bitch!
	pMatRes = loadMaterialCompiled(name);
	if (pMatRes) {
		return pMatRes;
	}

	// we want to create real instance but assign default props to it,
	X_ERROR("MatMan", "Failed to find material: %s", pMtlName);

	const auto* pDefault = getDefaultMaterial();

	pMatRes = createMaterial_Internal(name);
	pMatRes->assignProps(*pDefault);

	return pMatRes;
}

void XMaterialManager::releaseMaterial(Material* pMat)
{
	MaterialResource* pMatRes = reinterpret_cast<MaterialResource*>(pMat);
	if (pMatRes->removeReference() == 0)
	{
		materials_.releaseAsset(pMatRes);
	}
}

Material::Tech* XMaterialManager::getTechForMaterial(Material* pMat, core::StrHash techNameHash, render::shader::VertexFormat::Enum vrtFmt , bool vertStreams)
{
	X_ASSERT_NOT_NULL(pMat);

	// the material holds all it's techs like a cache, but when it don't have one we must create it.
	auto* pTech = pMat->getTech(techNameHash, vrtFmt);
	if (pTech) {
		return pTech;
	}

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
	PermatationFlags permFlags;
	if (vertStreams) {
		permFlags.Set(Permatation::VertStreams);
	}

	TechDefPerm* pPerm = pTechDef->getOrCreatePerm(vrtFmt, permFlags);
	if (!pPerm) {
		X_ERROR("MatMan", "Failed to get tech perm");
		return nullptr;
	}


	render::shader::IShaderPermatation* pShaderPerm = pPerm->pShaderPerm;

	// from the shader perm we can see how many const buffers we need to provide.
	const auto& cbLinks = pShaderPerm->getCbufferLinks();

	// we need to know how many textures we are going to be sending.
	// it may be less than what the material has.
	// the tech should tell us O_O
	// him -> pTechDef
	const size_t numTex = pTechDef->getNumAliases();

	render::Commands::ResourceStateBase* pVariableState = vsMan_.createVariableState(
		numTex, 
		numTex, // same as numTex for now as i refactor sampler out of texState.
		cbLinks.size()
	);

	// we should create the const buffers we need and set them in the variable state.
	auto* pCBHandles = pVariableState->getCBs();
	for (size_t i = 0; i < cbLinks.size(); i++)
	{
		auto& cb = cbLinks[i];
		
		pCBufMan_->autoUpdateBuffer(*cb.pCBufer);

		auto cbHandle = pCBufMan_->createCBuffer(*cb.pCBufer);

		pCBHandles[i] = cbHandle;
	}

	auto* pTexStates = pVariableState->getTexStates();
	for (size_t i = 0; i < numTex; i++)
	{
		auto& texState = pTexStates[i];

		// we need to select the correct texture from the material, and pass the texture id and states.


		texState.textureId = 0; // get FOOKED.
	}

	auto* pSamplers = pVariableState->getSamplers();
	for (size_t i = 0; i < numTex; i++)
	{
		auto& sampler = pSamplers[i];
		sampler.filter = render::FilterType::LINEAR_MIP_LINEAR;
		sampler.repeat = render::TexRepeat::TILE_BOTH;
	}


	// we now have a material tech that contains the pipeline state needed and what variable state also needs to be set 
	// we just add this to the materials local store, so it don't have to ask us for this next time.
	// so once everything has it's state to render anything we just have to check
	Material::Tech matTech;
	matTech.hash = techNameHash;
//	matTech.vertFmt = vrtFmt;
	matTech.pPerm = pPerm;
//	matTech.stateHandle = pPerm->stateHandle;
	matTech.pVariableState = pVariableState;

	pMat->addTech(matTech);
	return pMat->getTech(techNameHash, vrtFmt);
}

bool XMaterialManager::setTextureID(Material* pMat, Material::Tech* pTech, core::StrHash texNameHash, texture::TexID id)
{
	TechDefState* pTechDefState = pMat->getTechDefState();

	TechDef* pTechDef = pTechDefState->getTech(pTech->hash);

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

	core::XFileScoped file;
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

#if 1

	core::string catType;
	file.readString(catType);

	// now samplers.
	for (uint8_t i = 0; i < hdr.numSamplers; i++)
	{
		render::SamplerState sampler;
		core::string samplerName;

		file.readObj(sampler);
		file.readString(samplerName);
	}

	// now params.
	for (uint8_t i = 0; i < hdr.numParams; i++)
	{
		core::string name, val;

		file.readString(name);
		file.readString(val);
	}


#else

	// we also have texture info.
	core::FixedArray<MaterialTexture, MTL_MAX_TEXTURES> texInfo;
	core::FixedArray<const char*, MTL_MAX_TEXTURES> texNames;
	texInfo.resize(hdr.numTextures);

	if (file.readObjs(texInfo.data(), hdr.numTextures) != hdr.numTextures) {
		return false;
	}


#if 1
	core::string catType;

	file.readString(catType);

#else
	// now we have strings
	core::string catType;
	char nameBuffer[assetDb::ASSET_NAME_MAX_LENGTH * MTL_MAX_TEXTURES];

	file.read(nameBuffer, hdr.strDataSize);


	catType = nameBuffer;

	const char* pNameCur = nameBuffer + hdr.catTypeNameLen;

	for (const auto& tex : texInfo)
	{
		texNames.append(pNameCur);
		pNameCur += tex.nameLen;
	}
#endif
#endif


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

	// ok for a material we need to create all the techs now.

//	auto* pTech = pTechDefState->getTech(core::StrHash("unlit"));
//	pTech = nullptr;



	MaterialResource* pMatRes = createMaterial_Internal(matName);
	pMatRes->setTechDefState(pTechDefState);

#if 0
	// so my fine little goat muncher.
	// we need to slap textures for the goats that fly towards us.
	core::FixedArray<Material::Texture, MTL_MAX_TEXTURES> processedInfo;
	processedInfo.resize(texInfo.size());

	for (size_t i=0; i<texInfo.size(); i++)
	{
		auto* pITexture = pRender_->getTexture(texNames[i], texture::TextureFlags::STREAMABLE);


		Material::Texture tex;
		tex.texId = pITexture->getTexID();
		tex.filterType = texInfo[i].filterType;
		tex.texRepeat = texInfo[i].texRepeat;
	}

	pMatRes->setTextures(processedInfo);
#endif

	return pMatRes;
}

XMaterialManager::MaterialResource* XMaterialManager::createMaterial_Internal(const core::string& name)
{
	// internal create expects you to know no duplicates
	X_ASSERT(findMaterial_Internal(name) == nullptr, "Creating a material that already exsists")();

	auto pMatRes = materials_.createAsset(name, g_3dEngineArena);
	pMatRes->setName(name);

	return pMatRes;
}

XMaterialManager::MaterialResource* XMaterialManager::findMaterial_Internal(const core::string& name) const
{
	return materials_.findAsset(name);
}

bool XMaterialManager::InitDefaults(void)
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


// ICoreEventListener
void XMaterialManager::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
	X_UNUSED(event);
	X_UNUSED(wparam);
	X_UNUSED(lparam);



}
// ~ICoreEventListener


void XMaterialManager::freeDanglingMaterials(void)
{
	auto it = materials_.begin();
	for (; it != materials_.end(); ++it) {
		auto matRes = it->second;
		X_WARNING("MtlManager", "\"%s\" was not deleted. refs: %" PRIi32, matRes->getName(), matRes->getRefCount());
	}

	materials_.free();
}

void XMaterialManager::ListMaterials(const char* pSearchPatten) const
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


X_NAMESPACE_END
