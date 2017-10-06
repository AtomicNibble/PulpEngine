#include "stdafx.h"
#include "IMaterial.h"
#include "MaterialManager.h"

#include <IFileSys.h>
#include <IShader.h>
#include <IRender.h>
#include <IConsole.h>
#include <ITexture.h>

#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>

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
	blockArena_(arena),
	pTechDefMan_(nullptr),
	cBufMan_(cBufMan),
	vsMan_(vsMan),
	materials_(arena, sizeof(MaterialResource), core::Max<size_t>(8u,X_ALIGN_OF(MaterialResource)), "MaterialPool"),
	pDefaultMtl_(nullptr),
	requestQueue_(arena),
	pendingRequests_(arena),
	failedLoads_(arena)
{
	pTechDefMan_ = X_NEW(TechDefStateManager, arena, "TechDefStateManager")(arena);

	requestQueue_.reserve(64);
	pendingRequests_.setGranularity(32);
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
	vars_.registerVars();

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
	X_LOG0("Material", "Shutting Down");

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
	dispatchPendingLoads();

	if (!pDefaultMtl_) {
		X_ERROR("Material", "Default Material is not valid");
		return false;
	}

	if (!waitForLoad(pDefaultMtl_)) {
		X_ERROR("Material", "Failed to load default Material");
		return false;
	}

	pDefaultMtl_->setFlags(pDefaultMtl_->getFlags() | MaterialFlag::DEFAULT);

	// anything that failed to load, while default material was loading
	// assing default to it now.
	for (auto* pMat : failedLoads_)
	{
		X_ASSERT(pMat->getStatus() == core::LoadStatus::Error, "Unexpected status")();
		pMat->assignProps(*pDefaultMtl_);
	}

	failedLoads_.free();
	return true;
}


// IMaterialManager

Material* XMaterialManager::findMaterial(const char* pMtlName) const
{
	core::string name(pMtlName);

	core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

	Material* pMtl = materials_.findAsset(name);
	if (pMtl) {
		return pMtl;
	}

	X_WARNING("Material", "Failed to find material: \"%s\"", pMtlName);
	return nullptr;
}

Material* XMaterialManager::loadMaterial(const char* pMtlName)
{
	X_ASSERT_NOT_NULL(pMtlName);
	X_ASSERT(core::strUtil::FileExtension(pMtlName) == nullptr, "Extension not allowed")(pMtlName);

	core::string name(pMtlName);

	{
	}

	MaterialResource* pMatRes = nullptr;
	{
		core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

		pMatRes = materials_.findAsset(name);
		if (pMatRes)
		{
			// inc ref count.
			pMatRes->addReference();
			return pMatRes;
		}

		pMatRes = materials_.createAsset(name, name, arena_);
	}

	// add to list of Materials that need loading.
	addLoadRequest(pMatRes);

	return pMatRes;
}

void XMaterialManager::releaseMaterial(Material* pMat)
{
	MaterialResource* pMatRes = reinterpret_cast<MaterialResource*>(pMat);
	if (pMatRes->removeReference() == 0)
	{
		releaseResources(pMatRes);

		materials_.releaseAsset(pMatRes);
	}
}


bool XMaterialManager::initDefaults(void)
{
	if (pDefaultMtl_ == nullptr)
	{
		pDefaultMtl_ = loadMaterial(MTL_DEFAULT_NAME);
		if (!pDefaultMtl_) {
			X_ERROR("Material", "Failed to create default material");
			return false;
		}

		dispatchPendingLoads();
	}

	return true;
}


void XMaterialManager::freeDanglingMaterials(void)
{
	{
		core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

		for (const auto& m : materials_)
		{
			auto* pMatRes = m.second;
			const auto& name = pMatRes->getName();

			releaseResources(pMatRes);
			X_WARNING("Material", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pMatRes->getRefCount());
		}
	}

	materials_.free();
}

void XMaterialManager::releaseResources(Material* pMat)
{
	// when we release the material we need to clean up somethings.
	X_UNUSED(pMat);

}


void XMaterialManager::listMaterials(const char* pSearchPatten) const
{
	core::ScopedLock<MaterialContainer::ThreadPolicy> lock(materials_.getThreadPolicy());

	core::Array<MaterialResource*> sorted_mats(arena_);
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

	for (const auto* pMat : sorted_mats)
	{
		X_LOG0("Material", "^2\"%s\"^7 refs: %" PRIi32, pMat->getName(), pMat->getRefCount());
	}

	X_LOG0("Material", "------------ ^8Materials End^7 -----------");
}



void XMaterialManager::addLoadRequest(MaterialResource* pMaterial)
{
	core::CriticalSection::ScopedLock lock(loadReqLock_);

	pMaterial->addReference(); // prevent instance sweep
	pMaterial->setStatus(core::LoadStatus::Loading);

	// queue it if over 16 active requests.
	int32_t maxReq = vars_.maxActiveLoadReq();
	if (maxReq == 0 || safe_static_cast<int32_t>(pendingRequests_.size()) < maxReq)
	{
		dispatchLoad(pMaterial, lock);
	}
	else
	{
		queueLoadRequest(pMaterial, lock);
	}
}


void XMaterialManager::queueLoadRequest(MaterialResource* pMaterial, core::CriticalSection::ScopedLock&)
{
	X_ASSERT(pMaterial->getName().isNotEmpty(), "Material has no name")();

	// can we know it's not in this queue just from status?
	// like if it's complete it could be in this status
	auto status = pMaterial->getStatus();
	if (status == core::LoadStatus::Complete || status == core::LoadStatus::Loading)
	{
		X_WARNING("Material", "Redundant load request requested: \"%s\"", pMaterial->getName().c_str());
		return;
	}

	X_ASSERT(!requestQueue_.contains(pMaterial), "Queue already contains asset")(pMaterial);

	requestQueue_.push(pMaterial);
}


void XMaterialManager::dispatchLoad(Material* pMaterial, core::CriticalSection::ScopedLock&)
{
	X_ASSERT(pMaterial->getStatus() == core::LoadStatus::Loading || pMaterial->getStatus() == core::LoadStatus::NotLoaded, "Incorrect status")();

	auto loadReq = core::makeUnique<MaterialLoadRequest>(arena_, pMaterial);

	// dispatch IO 
	dispatchLoadRequest(loadReq.get());

	pendingRequests_.emplace_back(loadReq.release());
}

bool XMaterialManager::dispatchPendingLoad(core::CriticalSection::ScopedLock& lock)
{
	int32_t maxReq = vars_.maxActiveLoadReq();

	if (requestQueue_.isNotEmpty() && (maxReq == 0 || safe_static_cast<int32_t>(pendingRequests_.size()) < maxReq))
	{
		X_ASSERT(requestQueue_.peek()->getStatus() == core::LoadStatus::Loading, "Incorrect status")();
		dispatchLoad(requestQueue_.peek(), lock);
		requestQueue_.pop();
		return true;
	}

	return false;
}


void XMaterialManager::dispatchPendingLoads(void)
{
	core::CriticalSection::ScopedLock lock(loadReqLock_);

	while (dispatchPendingLoad(lock));
}


bool XMaterialManager::waitForLoad(Material* pMaterial)
{
	if (pMaterial->getStatus() == core::LoadStatus::Complete) {
		return true;
	}

	{
		// we lock to see if loading as the setting of loading is performed inside this lock.
		core::CriticalSection::ScopedLock lock(loadReqLock_);
		while (pMaterial->getStatus() == core::LoadStatus::Loading)
		{
			loadCond_.Wait(loadReqLock_);
		}

	}

	// did we fail? or never sent a dispatch?
	auto status = pMaterial->getStatus();
	if (status == core::LoadStatus::Complete)
	{
		return true;
	}
	else if (status == core::LoadStatus::Error)
	{
		return false;
	}
	else if (status == core::LoadStatus::NotLoaded)
	{
		// request never sent?
	}

	X_ASSERT_UNREACHABLE();
	return false;
}

void XMaterialManager::dispatchLoadRequest(MaterialLoadRequest* pLoadReq)
{
	pLoadReq->dispatchTime = core::StopWatch::GetTimeNow();

	core::Path<char> path;
	path /= "materials";
	path /= pLoadReq->pMaterial->getName();
	path.setExtension(MTL_B_FILE_EXTENSION);

	// dispatch a read request baby!
	core::IoRequestOpen open;
	open.callback.Bind<XMaterialManager, &XMaterialManager::IoRequestCallback>(this);
	open.pUserData = pLoadReq;
	open.mode = core::fileMode::READ;
	open.path = path;

	gEnv->pFileSys->AddIoRequestToQue(open);
}

void XMaterialManager::onLoadRequestFail(MaterialLoadRequest* pLoadReq)
{
	auto* pMaterial = pLoadReq->pMaterial;

	if (pDefaultMtl_ != pMaterial)
	{
		// what if default Material not loaded :| ?
		if (!pDefaultMtl_->isLoaded())
		{
			// we can't wait for fucking IO load, since this is called by the IO thread.
			// if it's not loaded it will never load.
			core::CriticalSection::ScopedLock lock(loadReqLock_);

			pMaterial->setStatus(core::LoadStatus::Error);
			failedLoads_.push_back(pMaterial);
		}
		else
		{
			pMaterial->assignProps(*pDefaultMtl_);
		}
	}
	else
	{
		pMaterial->setStatus(core::LoadStatus::Error);
	}


	loadRequestCleanup(pLoadReq);
}

void XMaterialManager::loadRequestCleanup(MaterialLoadRequest* pLoadReq)
{
	pLoadReq->loadTime = core::StopWatch::GetTimeNow();

	auto status = pLoadReq->pMaterial->getStatus();
	X_ASSERT(status == core::LoadStatus::Complete || status == core::LoadStatus::Error, "Unexpected load status")(status);

	X_LOG0("Material", "Material loaded in: ^6%fms^7 io:^6%fms",
		(pLoadReq->loadTime - pLoadReq->dispatchTime).GetMilliSeconds(),
		(pLoadReq->ioTime - pLoadReq->dispatchTime).GetMilliSeconds()
	);
	
	{
		core::CriticalSection::ScopedLock lock(loadReqLock_);
		pendingRequests_.remove(pLoadReq);

		// dispatch another?
		dispatchPendingLoad(lock);
	}

	if (pLoadReq->pFile) {
		gEnv->pFileSys->AddCloseRequestToQue(pLoadReq->pFile);
	}

	// release our ref.
	releaseMaterial(pLoadReq->pMaterial);

	X_DELETE(pLoadReq, arena_);

	loadCond_.NotifyAll();
}


void XMaterialManager::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	core::IoRequest::Enum requestType = pRequest->getType();
	MaterialLoadRequest* pLoadReq = pRequest->getUserData<MaterialLoadRequest>();
	Material* pMaterial = pLoadReq->pMaterial;

	if (requestType == core::IoRequest::OPEN)
	{
		pLoadReq->ioTime = core::StopWatch::GetTimeNow();

		if (!pFile)
		{
			X_ERROR("Material", "Failed to load: \"%s\"", pMaterial->getName().c_str());
			onLoadRequestFail(pLoadReq);
			return;
		}

		pLoadReq->pFile = pFile;

		// read the whole file.
		uint32_t fileSize = safe_static_cast<uint32_t>(pFile->fileSize());
		X_ASSERT(fileSize > 0, "Datasize must be positive")(fileSize);
		pLoadReq->data = core::makeUnique<uint8_t[]>(blockArena_, fileSize, 16);
		pLoadReq->dataSize = fileSize;

		core::IoRequestRead read;
		read.callback.Bind<XMaterialManager, &XMaterialManager::IoRequestCallback>(this);
		read.pUserData = pLoadReq;
		read.pFile = pFile;
		read.dataSize = fileSize;
		read.offset = 0;
		read.pBuf = pLoadReq->data.ptr();
		fileSys.AddIoRequestToQue(read);
	}
	else if (requestType == core::IoRequest::READ)
	{
		const core::IoRequestRead* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);

		X_ASSERT(pLoadReq->data.ptr() == pReadReq->pBuf, "Buffers don't match")();

		if (bytesTransferred != pReadReq->dataSize)
		{
			X_ERROR("Material", "Failed to read material data. Got: 0x%" PRIx32 " need: 0x%" PRIx32, bytesTransferred, pReadReq->dataSize);
			onLoadRequestFail(pLoadReq);
			return;
		}

		gEnv->pJobSys->CreateMemberJobAndRun<XMaterialManager>(this, &XMaterialManager::ProcessData_job,
			pLoadReq JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));	
	}
}

void XMaterialManager::ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys);
	X_UNUSED(threadIdx);
	X_UNUSED(pJob);
	X_UNUSED(pData);

	MaterialLoadRequest* pLoadReq = static_cast<MaterialLoadRequest*>(X_ASSERT_NOT_NULL(pData));
	Material* pMaterial = pLoadReq->pMaterial;

	core::XFileFixedBuf file(pLoadReq->data.ptr(), pLoadReq->data.ptr() + pLoadReq->dataSize);
	

	if (!processData(pMaterial, &file))
	{
		onLoadRequestFail(pLoadReq);
	}
	else
	{
		loadRequestCleanup(pLoadReq);
	}
}


bool XMaterialManager::processData(Material* pMaterial, core::XFile* pFile)
{
	MaterialHeader hdr;
	if (pFile->readObj(hdr) != sizeof(hdr)) {
		return false;
	}

	if (!hdr.isValid()) {
		X_ERROR("Material", "Header is invalid: \"%s\"", pMaterial->getName().c_str());
		return false;
	}

	// we need to poke the goat.
	// so now materials store the cat and type which we can use to get techdef info.
	core::string catType;
	pFile->readString(catType);

	Material::SamplerArr samplers(arena_, hdr.numSamplers);
	Material::ParamArr params(arena_, hdr.numParams);
	Material::TextureArr textures(arena_, hdr.numTextures);

	// now samplers.
	for (uint8_t i = 0; i < hdr.numSamplers; i++)
	{
		MaterialSampler& sampler = samplers[i];

		pFile->readObj(sampler.sate);
		pFile->readString(sampler.name);
	}

	// now params.
	for (uint8_t i = 0; i < hdr.numParams; i++)
	{
		MaterialParam& param = params[i];
		pFile->readObj(param.value);
		pFile->readObj(param.type);
		pFile->readString(param.name);
	}

	// now textures.
	for (uint8_t i = 0; i < hdr.numTextures; i++)
	{
		MaterialTexture& tex = textures[i];
		pFile->readObj(tex.texSlot);
		pFile->readString(tex.name);
		pFile->readString(tex.val);
	}


#if X_DEBUG
	const auto left = pFile->remainingBytes();
	X_WARNING_IF(left > 0, "Material", "potential read fail, bytes left in file: %" PRIu64, left);
#endif // !X_DEBUG

	TechDefState* pTechDefState = pTechDefMan_->getTechDefState(hdr.cat, catType);
	if (!pTechDefState) {
		X_ERROR("Material", "Failed to get techdefstate");
		return false;
	}

	// we want to load all the textures now.
	// so that we are not 'creating' refrences for every tech.
	texture::TextureFlags texFlags = texture::TextureFlags();

	for (auto& tex : textures)
	{
		if (tex.val.isEmpty()) {
			X_ERROR("Material", "missing texture value");
			return false;
		}

		auto* pTexture = gEngEnv.pTextureMan_->forName(tex.val.c_str(), texFlags);
		tex.pTexture = pTexture;
	}

	pMaterial->setTechDefState(pTechDefState);
	pMaterial->setParams(std::move(params));
	pMaterial->setSamplers(std::move(samplers));
	pMaterial->setTextures(std::move(textures));
	pMaterial->setStatus(core::LoadStatus::Complete);
	return true;
}



Material::Tech* XMaterialManager::getTechForMaterial(Material* pMat, core::StrHash techNameHash, 
	render::shader::VertexFormat::Enum vrtFmt, PermatationFlags permFlags)
{
	X_ASSERT_NOT_NULL(pMat);

	if (!waitForLoad(pMat)) {
		return nullptr;
	}

	if (pMat->getTextures().isNotEmpty()) {
		permFlags.Set(render::shader::Permatation::Textured);
	}

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
	const auto& buffers = pShaderPerm->getBuffers();
	const auto& permSamplers = pShaderPerm->getSamplers();
	const auto& permTextures = pShaderPerm->getTextures();

	// we need to know how many textures we are going to be sending.
	// it may be less than what the material has.
	// the tech should tell us O_O
	// him -> pTechDef
	const size_t numTex = permTextures.size();
	const size_t numSamplers = permSamplers.size();
	const size_t numCb = cbLinks.size();
	const size_t numBuffers = buffers.size();

	render::Commands::ResourceStateBase* pVariableState = vsMan_.createVariableState(
		numTex, 
		numSamplers, 
		numCb,
		numBuffers
	);

	// we should create the const buffers we need and set them in the variable state.
#if X_ENABLE_ASSERTIONS
	{
		auto* pCBHandles = pVariableState->getCBs();
		auto* pBuffers = pVariableState->getBuffers();

		for (size_t i = 0; i < numCb; i++) {
			pCBHandles[i] = render::INVALID_BUF_HANLDE;
		}
		for (size_t i = 0; i < numBuffers; i++) {
			pBuffers[i].buf = render::INVALID_BUF_HANLDE;
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

		matTech.cbs.reserve(cbLinks.size());
		for (auto& cb : cbLinks) {
			matTech.cbs.push_back(cb.pCBufer);
		}

		const auto& matParams = pMat->getParams();

		if (matParams.isNotEmpty())
		{
			auto& paramLinks = matTech.paramLinks;

			// a list of all the cb's that contain material params.
			core::FixedArray<int32_t, render::shader::MAX_SHADER_CB_PER_PERM> materialCBIdxs;

			for (size_t i = 0; i < matParams.size(); i++)
			{
				auto& param = matParams[i];
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
				X_ASSERT(materialCBIdxs.isNotEmpty(), "Must have atleast one material cb idx, if we have param links")();

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

				auto addCBToMatTech = [&](int32_t cbIdx, render::shader::XCBuffer& matCb) {
					matCb.postParamModify();
					X_ASSERT(matCb.containsUpdateFreqs(render::shader::UpdateFreq::MATERIAL), "Should contain per material params")();
					matTech.materialCbs.append(std::move(matCb));

					// change the pointer to mat instance, instead of the instance from the shader perm which is shared.
					matTech.cbs[cbIdx] = &matTech.materialCbs.back();
				};

				for (auto& link : paramLinks)
				{
					if (currentCbIdx != link.cbIdx)
					{
						if (currentCbIdx != -1) {
							addCBToMatTech(link.cbIdx, matCb);
						}

						currentCbIdx = link.cbIdx;
						const auto& cbLink = cbLinks[link.cbIdx];
						const auto& cb = *cbLink.pCBufer;

						matCb = cb; // make a copy.
					}

					// change the update freq.
					// potentially and param that's unkown could just be default marked as material param.
					// would make some things more simple.
					auto& cpuData = matCb.getCpuData();
					auto& cbParam = matCb[link.cbParamIdx];
					const auto& matParam = matParams[link.paramIdx];

					cbParam.setUpdateRate(render::shader::UpdateFreq::MATERIAL);

					
					// copy the material params value into the 
					// always vec4? humm.
					X_ASSERT(cpuData.size() >= (cbParam.getBindPoint() + sizeof(matParam.value)), "Overflow when writing mat param value to cbuffer")();
					std::memcpy(&cpuData[cbParam.getBindPoint()], &matParam.value, sizeof(matParam.value));
				}

				// optermisation:
				// might be better to not store these material cb instance for each tech and instead share them within a material
				// so if a material has multiple techs that have identical material cbuffers they could share them.
				// since the material params will be the same for all techs.
				addCBToMatTech(currentCbIdx, matCb);
			}
		}

		// the 'cbuffers' arr can contain a mix of cbuffer's pointers that can point at material instances or ones from the shader perm.
		X_ASSERT(cbLinks.size() == matTech.cbs.size(), "Links and cbuffer list should be same size")(cbLinks.size(), matTech.cbs.size());
		for (size_t i = 0; i < matTech.cbs.size(); i++)
		{
			auto* pCB = matTech.cbs[i];
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


void XMaterialManager::Cmd_ListMaterials(core::IConsoleCmdArgs* pCmd)
{
	// optional search criteria
	const char* pSearchPatten = nullptr;

	if (pCmd->GetArgCount() >= 2) {
		pSearchPatten = pCmd->GetArg(1);
	}

	listMaterials(pSearchPatten);
}



X_NAMESPACE_END
