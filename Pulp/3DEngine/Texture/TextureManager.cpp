#include "stdafx.h"
#include "TextureManager.h"
#include "Texture.h"

#include <Threading\JobSystem2.h>
#include <IFileSys.h>
#include <ICi.h>

X_NAMESPACE_BEGIN(engine)

namespace
{
	struct CIFileJobData
	{
		Texture* pTexture;
		const uint8_t* pData;
		size_t length;
	};


} // namespace 


TextureManager::TextureManager(core::MemoryArenaBase* arena) :
	arena_(arena),
	textures_(arena, sizeof(TextureResource), core::Max(64_sz, X_ALIGN_OF(TextureResource))),
	pCILoader_(nullptr),

	blockAlloc_(),
	blockArena_(&blockAlloc_, "TextureBlockAlloc"),
	streamQueue_(arena),
	loadComplete_(true),
	currentDeviceTexId_(0),
	pTexDefault_(nullptr),
	pTexDefaultBump_(nullptr)
{
	defaultLookup_.fill(nullptr);
}

TextureManager::~TextureManager()
{

}


void TextureManager::registerCmds(void)
{

}

void TextureManager::registerVars(void)
{
	vars_.registerVars();
}


bool TextureManager::init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);
	X_LOG1("TextureManager", "Starting");
	X_PROFILE_NO_HISTORY_BEGIN("TextureMan", core::profiler::SubSys::ENGINE3D);

	auto hotReload = gEnv->pHotReload;
	hotReload->addfileType(this, "ci");
	hotReload->addfileType(this, "dds");
	hotReload->addfileType(this, "png");
	hotReload->addfileType(this, "jpg");
	hotReload->addfileType(this, "psd");
	hotReload->addfileType(this, "tga");

	pCILoader_ = X_NEW(texture::CI::XTexLoaderCI, arena_, "CILoader");

	static_assert(texture::ImgFileFormat::ENUM_COUNT == 7, "Added additional img src fmts? this code needs updating.");

	textureLoaders_.append(pCILoader_);

	if (vars_.allowFmtDDS()) {
		textureLoaders_.append(X_NEW(texture::DDS::XTexLoaderDDS, arena_, "DDSLoader"));
	}
	if (vars_.allowFmtJPG()) {
		textureLoaders_.append(X_NEW(texture::JPG::XTexLoaderJPG, arena_, "JPGLoader"));
	}
	if (vars_.allowFmtPNG()) {
		textureLoaders_.append(X_NEW(texture::PNG::XTexLoaderPNG, arena_, "PNGLoader"));
	}
	if (vars_.allowFmtPSD()) {
		textureLoaders_.append(X_NEW(texture::PSD::XTexLoaderPSD, arena_, "PSDLoader"));
	}
	if (vars_.allowFmtTGA()) {
		textureLoaders_.append(X_NEW(texture::TGA::XTexLoaderTGA, arena_, "TGALoader"));
	}

	if (!loadDefaultTextures()) {
		return false;
	}

	return true;
}

void TextureManager::shutDown(void)
{
	X_LOG0("TextureManager", "Shutting Down");

	auto hotReload = gEnv->pHotReload;
	hotReload->addfileType(nullptr, "ci");
	hotReload->addfileType(nullptr, "dds");
	hotReload->addfileType(nullptr, "png");
	hotReload->addfileType(nullptr, "jpg");
	hotReload->addfileType(nullptr, "psd");
	hotReload->addfileType(nullptr, "tga");

	for (auto* pTexLoader : textureLoaders_) {
		X_DELETE(pTexLoader, arena_);
	}
	textureLoaders_.clear();
}

bool TextureManager::asyncInitFinalize(void)
{
	// we need to know that the default textures have finished loading and are ready.
	while(std::any_of(defaultLookup_.begin(), defaultLookup_.end(), [](Texture* pTex) { return pTex->isLoading(); }))
	{
		loadComplete_.wait();
	}

	if (pTexDefault_->loadFailed()) {
		X_ERROR("Texture", "failed to load default texture: %s", texture::TEX_DEFAULT_DIFFUSE);
		return false;
	}
	if (pTexDefaultBump_->loadFailed()) {
		X_ERROR("Texture", "failed to load default bump texture: %s", texture::TEX_DEFAULT_BUMP);
		return false;
	}

	return std::all_of(defaultLookup_.begin(), defaultLookup_.end(), [](Texture* pTex) { return pTex->isLoaded(); });
}


void TextureManager::scheduleStreaming(void)
{
	// we work out what to start streaming.
	if (streamQueue_.isEmpty()) {
		return;
	}

}


Texture* TextureManager::forName(const char* pName, texture::TextureFlags flags)
{
	core::string name(pName);

	auto& threadPolicy = textures_.getThreadPolicy();
	threadPolicy.Enter();

	TexRes* pTexRes = textures_.findAsset(name);

	if (pTexRes)
	{
		threadPolicy.Leave();
		pTexRes->addReference();
	}
	else
	{
		auto* pDevicTex = gEnv->pRender->getDeviceTexture(currentDeviceTexId_++);
		if (!pDevicTex) {
			return nullptr;
		}

		pTexRes = textures_.createAsset(name, name, flags, pDevicTex);
		threadPolicy.Leave();
	}

	return pTexRes;
}

Texture* TextureManager::getDefault(render::TextureSlot::Enum slot) const
{
	return X_ASSERT_NOT_NULL(defaultLookup_[slot]);
}

void TextureManager::releaseTexture(Texture* pTex)
{
	TexRes* pTexRes = static_cast<TexRes*>(X_ASSERT_NOT_NULL(pTex));

	if (pTexRes->removeReference() == 0)
	{
		textures_.releaseAsset(pTexRes);
	}
}

void TextureManager::dispatchRead(Texture* pTexture)
{
	X_UNUSED(pTexture);

	// lets dispatch a IO request to read the image data.
	// where to we store the data?
	// we want fixed buffers for streaming.
	core::Path<char> path(pTexture->getName());
	path.toLower(); // lower case file names only.
	path.setExtension(texture::CI_FILE_EXTENSION);

	core::IFileSys::fileModeFlags mode;
	mode.Set(core::IFileSys::fileMode::READ);
	mode.Set(core::IFileSys::fileMode::SHARE);

	core::IoCallBack del;
	del.Bind<TextureManager, &TextureManager::IoRequestCallback>(this);

	core::IoRequestOpenRead req;
	req.callback = del;
	req.pUserData = pTexture;
	req.mode = mode;
	req.path = path;
	req.arena = &blockArena_;
	
	gEnv->pFileSys->AddIoRequestToQue(req);
}

void TextureManager::IoRequestCallback(core::IFileSys&, const core::IoRequestBase* pReqBase, core::XFileAsync* pFile, uint32_t bytesRead)
{
	// who is this?
	if (pReqBase->getType() == core::IoRequest::OPEN_READ_ALL)
	{
		auto* pReq = static_cast<const core::IoRequestOpenRead*>(pReqBase);
		Texture* pTexture = static_cast<Texture*>(pReq->pUserData);
		auto& flags = pTexture->flags();

		// if we have loaded the data, we now need to process it.
		if (!pFile || !bytesRead)
		{
			flags.Set(texture::TexFlag::LOAD_FAILED);
			loadComplete_.raise();
		}

		// YOU WANT A JOB?
		// 3 fiddy a hour.
		CIFileJobData data{ pTexture, pReq->pBuf, pReq->dataSize };

		gEnv->pJobSys->CreateMemberJobAndRun<TextureManager>(this, &TextureManager::processCIFile_job,
			data JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
	}

}

void TextureManager::processCIFile_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys, threadIdx, pJob);
	CIFileJobData* pJobData = static_cast<CIFileJobData*>(pData);

	processCIFile(pJobData->pTexture, pJobData->pData, pJobData->length);

	X_DELETE_ARRAY(pJobData->pData, &blockArena_);

	loadComplete_.raise();
}

void TextureManager::processCIFile(Texture* pTexture, const uint8_t* pData, size_t length)
{
	auto& flags = pTexture->flags();

	core::XFileFixedBuf file(pData, pData + length);
	texture::XTextureFile imgFile(&blockArena_);

	if (!pCILoader_->loadTexture(&file, imgFile))
	{
		flags.Set(texture::TexFlag::LOAD_FAILED);
		return;
	}
	
	pTexture->setProperties(imgFile);

	// we need to upload the texture data.
	gEnv->pRender->initDeviceTexture(pTexture->getDeviceTexture(), imgFile);

	flags.Set(texture::TexFlag::LOADED);
}


#if 0
bool TextureManager::loadFromFile(texture::XTextureFile& imgFile, const char* pPath)
{
	X_ASSERT_NOT_NULL(pCILoader_);
	X_ASSERT_NOT_NULL(pPath);

	core::IFileSys* pFileSys = gEnv->pFileSys;

	core::IFileSys::fileModeFlags mode;
	mode.Set(core::IFileSys::fileMode::READ);
	mode.Set(core::IFileSys::fileMode::SHARE);

	core::Path<char> path(pPath);
	path.toLower(); // lower case file names only.
	path.setExtension(texture::CI_FILE_EXTENSION);

	if (pFileSys->fileExists(path.c_str()))
	{
		core::XFileScoped file;

		if (file.openFile(path.c_str(), mode))
		{
			if (!pCILoader_->loadTexture(file.GetFile(), imgFile, arena_)) {
				X_ERROR("Texture", "Error loading: \"%s\"", pPath);
				return false;
			}

			return true;
		}
	}

	if (!vars_.allowRawImgLoading()) {
		return false;
	}

	// try loading none compiled.
	for (auto pLoader : textureLoaders_)
	{
		path.setExtension(pLoader->getExtension());

		if (pFileSys->fileExists(path.c_str()))
		{
			core::XFileScoped file;

			if (file.openFile(path.c_str(), mode))
			{
				if (!pLoader->loadTexture(file.GetFile(), imgFile, arena_)) {
					X_ERROR("Texture", "Error loading: \"%s\"", pPath);
					return false;
				}
			}

			return true;
		}
	}

	return false;
}
#endif


bool TextureManager::loadDefaultTextures(void)
{
	using namespace texture;

	TextureFlags default_flags = TextureFlags::DONT_RESIZE | TextureFlags::DONT_STREAM;

	pTexDefault_ = forName(TEX_DEFAULT_DIFFUSE, default_flags);
	pTexDefaultBump_ = forName(TEX_DEFAULT_BUMP, default_flags);

	defaultLookup_.fill(pTexDefault_);
	defaultLookup_[render::TextureSlot::NORMAL] = pTexDefaultBump_;


	// start reading the texture data.
	dispatchRead(pTexDefault_);
	dispatchRead(pTexDefaultBump_);

	return true;
}

void TextureManager::releaseDefaultTextures(void)
{
	if (pTexDefault_) {
		releaseTexture(pTexDefault_);
	}
	if (pTexDefaultBump_) {
		releaseTexture(pTexDefaultBump_);
	}

	pTexDefault_ = nullptr;
	pTexDefaultBump_ = nullptr;
}

void TextureManager::releaseDanglingTextures(void)
{
	{
		core::ScopedLock<TextureContainer::ThreadPolicy> lock(textures_.getThreadPolicy());

		auto it = textures_.begin();
		for (; it != textures_.end(); ++it) {
			auto* pTexRes = it->second;
			releaseTexture(pTexRes);
			X_WARNING("Texture", "\"%s\" was not deleted. refs: %" PRIi32, pTexRes->getName(), pTexRes->getRefCount());
		}
	}

	textures_.free();
}

void TextureManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys, name);

}


X_NAMESPACE_END