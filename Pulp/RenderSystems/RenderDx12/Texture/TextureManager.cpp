#include "stdafx.h"
#include "TextureManager.h"
#include "Texture.h"
#include "TextureUtil.h"

#include <ICi.h>
#include <IConsole.h>
#include <IFileSys.h>

#include "Allocators\DescriptorAllocator.h"
#include "CommandContex.h"

X_NAMESPACE_BEGIN(texture)


	TextureManager::TextureManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice, render::ContextManager& contextMan,
		render::DescriptorAllocator& descriptorAlloc) :
		contextMan_(contextMan),
		pDevice_(pDevice),
		descriptorAlloc_(descriptorAlloc),
		arena_(arena),
		textures_(arena, sizeof(TextureResource), X_ALIGN_OF(TextureResource)),
		pTexDefault_(nullptr),
		pTexDefaultBump_(nullptr),
		ptexMipMapDebug_(nullptr)
	{
	}

	TextureManager::~TextureManager()
	{

	}

	bool TextureManager::init(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pConsole);
		X_LOG1("TextureManager", "Starting");

		auto hotReload = gEnv->pHotReload;
		hotReload->addfileType(this, "ci");
		hotReload->addfileType(this, "dds");
		hotReload->addfileType(this, "png");
		hotReload->addfileType(this, "jpg");
		hotReload->addfileType(this, "psd");
		hotReload->addfileType(this, "tga");

		pCILoader_ = X_NEW(CI::XTexLoaderCI, arena_, "CILoader");
		
		static_assert(ImgFileFormat::ENUM_COUNT == 7, "Added additional img src fmts? this code needs updating.");

		textureLoaders_.append(pCILoader_);
		if (vars_.allowFmtDDS()) {
			textureLoaders_.append(X_NEW(DDS::XTexLoaderDDS, arena_, "DDSLoader"));
		}
		if (vars_.allowFmtJPG()) {
			textureLoaders_.append(X_NEW(JPG::XTexLoaderJPG, arena_, "JPGLoader"));
		}
		if (vars_.allowFmtPNG()) {
			textureLoaders_.append(X_NEW(PNG::XTexLoaderPNG, arena_, "PNGLoader"));
		}
		if (vars_.allowFmtPSD()) {
			textureLoaders_.append(X_NEW(PSD::XTexLoaderPSD, arena_, "PSDLoader"));
		}
		if (vars_.allowFmtTGA()) {
			textureLoaders_.append(X_NEW(TGA::XTexLoaderTGA, arena_, "TGALoader"));
		}

		if (!loadDefaultTextures()) {
			X_ERROR("TextureManager", "Failed to load default textures");
			return false;
		}

		return true;
	}

	bool TextureManager::shutDown(void)
	{
		X_LOG1("TextureManager", "Shutting Down");

		auto hotReload = gEnv->pHotReload;
		hotReload->addfileType(nullptr, "ci");
		hotReload->addfileType(nullptr, "dds");
		hotReload->addfileType(nullptr, "png");
		hotReload->addfileType(nullptr, "jpg");
		hotReload->addfileType(nullptr, "psd");
		hotReload->addfileType(nullptr, "tga");

		releaseDefaultTextures();
		releaseDanglingTextures();

		for (auto tl : textureLoaders_) {
			X_DELETE(tl, arena_);
		}

		return true;
	}


	void TextureManager::registerVars(void)
	{
		vars_.RegisterVars();
	}

	void TextureManager::registerCmds(void)
	{

		ADD_COMMAND_MEMBER("imageReloadAll", this, TextureManager, &TextureManager::Cmd_ReloadTextures, core::VarFlag::SYSTEM,
			"Reload all textures");
		ADD_COMMAND_MEMBER("imageReload", this, TextureManager, &TextureManager::Cmd_ReloadTexture, core::VarFlag::SYSTEM,
			"Reload a textures <name>");


		
	}


	Texture* TextureManager::forName(const char* pName, TextureFlags flags)
	{
		core::string name(pName);

		TexRes* pTexRes = findTexture(name);

		if (pTexRes)
		{
			pTexRes->addReference();
		}
		else
		{
			pTexRes = textures_.createAsset(name, name, flags);

			if (pTexRes->IsStreamable() && flags.IsSet(TexFlag::DONT_STREAM)) {
				stream(pTexRes);
			}
			else {
				if (!load(pTexRes)) {
					pTexRes->flags_.Set(TexFlag::LOAD_FAILED);

					X_WARNING("Texture", "Failed to load: \"%s\"", pTexRes->fileName_.c_str());
				}
			}
		}

		return pTexRes;
	}

	Texture* TextureManager::createTexture(const char* pNickName, Vec2i dim,
		texture::Texturefmt::Enum fmt, const uint8_t* pInitialData)
	{
		core::string name(pNickName);

		TexRes* pTexRes = findTexture(name);

		if (pTexRes)
		{
			pTexRes->addReference();

			X_WARNING("Texture", "Created texture with matching name of exsisting texture, returning original: \"%s\"", pNickName);
			if (pInitialData) {
				// update?
			}
		}
		else
		{
			pTexRes = textures_.createAsset(name, name, TexFlag::DONT_STREAM | TexFlag::DONT_RESIZE | TexFlag::NOMIPS);

			pTexRes->setDepth(1);
			pTexRes->setNumFaces(1);
			pTexRes->setNumMips(1);
			pTexRes->setWidth(dim.x);
			pTexRes->setHeight(dim.y);
			pTexRes->setFormat(fmt);
			pTexRes->setType(texture::TextureType::T2D);


			if (!createDeviceTexture(pTexRes)) {
				X_ERROR("Texture", "Failed to create device texture");
			}

			if (pInitialData) {
				// need to refactor to make this nicer todo without duplication of logic.

				D3D12_SUBRESOURCE_DATA texResource;
				{
					const size_t rowBytes = Util::rowBytes(pTexRes->getWidth(), 1, pTexRes->getFormat());

					texResource.pData = pInitialData;
					texResource.RowPitch = rowBytes;
					texResource.SlicePitch = texResource.RowPitch * pTexRes->getHeight();
				}

				auto& gpuResource = pTexRes->getGpuResource();

				if (!initializeTexture(gpuResource, 1, &texResource)) {
					// we should mark the texture as invalid.
				}
			}
		}

		return pTexRes;
	}


	Texture* TextureManager::getByID(TexID texId) const
	{
	//	X_UNUSED(texId);
	//	X_ASSERT_NOT_IMPLEMENTED();
		return textures_.findAsset(texId);
	}

	Texture* TextureManager::getDefault(void) const
	{
		return pTexDefault_;
	}

	void TextureManager::releaseTexture(texture::ITexture* pTex)
	{
		return releaseTexture(static_cast<Texture*>(pTex));
	}

	void TextureManager::releaseTexture(Texture* pTex)
	{
		X_ASSERT_NOT_NULL(pTex);

		TexRes* pTexRes = static_cast<TexRes*>(pTex);

		if (pTexRes->removeReference() == 0)
		{
			core::string name(pTex->getName());
			
			textures_.releaseAsset(pTexRes);
		}
	}

	bool TextureManager::updateTexture(render::CommandContext& contex, TexID texId, const uint8_t* pSrc, uint32_t srcSize) const
	{
		Texture* pTex = getByID(texId);
		auto& dest = pTex->getGpuResource();

		const size_t rowBytes = Util::rowBytes(pTex->getWidth(), 1, pTex->getFormat());

		D3D12_SUBRESOURCE_DATA texResource;
		texResource.pData = pSrc;
		texResource.RowPitch = rowBytes;
		texResource.SlicePitch = texResource.RowPitch * pTex->getHeight();
		
		const uint64_t uploadBufSize = getRequiredIntermediateSize(dest.getResource(), 0, 1);

		D3D12_HEAP_PROPERTIES heapProps;
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC bufferDesc;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = uploadBufSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ID3D12Resource* pUploadBuffer;

		HRESULT hr = pDevice_->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
			&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pUploadBuffer));

		if (FAILED(hr))
		{
			// if we fail we can handle it by just showing default texture, without much trouble.
			X_ERROR("Texture", "Failed to create commited resource for uploading. res: %" PRIu32, hr);
			return false;
		}

		contex.transitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
		contex.updateSubresources<1>(pDevice_, dest, pUploadBuffer, 0, 0, 1, &texResource);
		contex.transitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

		// we have to keep this..
	//	pUploadBuffer->Release();

		return true;
	}

	TextureManager::TexRes* TextureManager::findTexture(const char* pName)
	{
		core::string name(pName);
		return findTexture(name);
	}

	TextureManager::TexRes* TextureManager::findTexture(const core::string& name)
	{
		return textures_.findAsset(name);
	}


	bool TextureManager::reloadForName(const char* pName)
	{
		X_ASSERT_NOT_NULL(pName);

		// all asset names need forward slashes, for the hash.
		core::Path<char> path(pName);
		path.replaceAll('\\', '/');

		Texture* pTex = findTexture(path.c_str());
		if (!pTex) {
			X_WARNING("Texture", "Failed to find texture(%s) for reloading", pName);
			return false;
		}


		X_ASSERT_NOT_IMPLEMENTED();
		return false;
	}

	bool TextureManager::loadDefaultTextures(void)
	{
		TextureFlags default_flags = TextureFlags::DONT_RESIZE | TextureFlags::DONT_STREAM;

		pTexDefault_ = forName(TEX_DEFAULT_DIFFUSE, default_flags);
		pTexDefaultBump_ = forName(TEX_DEFAULT_BUMP, default_flags);

		// these are required.
		if (!pTexDefault_->isLoaded()) {
			X_ERROR("Texture", "failed to load default texture: %s", TEX_DEFAULT_DIFFUSE);
			return false;
		}
		if (!pTexDefaultBump_->isLoaded()) {
			X_ERROR("Texture", "failed to load default bump texture: %s", TEX_DEFAULT_BUMP);
			return false;
		}

		ptexMipMapDebug_ = forName("imgs/debug/mipmaplevels", default_flags | TextureFlags::FILTER_BILINEAR);


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
		if (ptexMipMapDebug_) {
			releaseTexture(ptexMipMapDebug_);
		}
	}

	void TextureManager::releaseDanglingTextures(void)
	{
		auto it = textures_.begin();
		for (; it != textures_.end(); ++it) {
			auto texRes = it->second;
			X_WARNING("Texture", "\"%s\" was not deleted. refs: %" PRIi32, texRes->getName(), texRes->getRefCount());
		}

		textures_.free();
	}

	bool TextureManager::stream(Texture* pTex)
	{
		// start streaming the texture.
		X_ASSERT_NOT_IMPLEMENTED();

		return true;
	}

	bool TextureManager::load(Texture* pTex)
	{
		X_ASSERT_NOT_NULL(pTex);

		XTextureFile imgFile(arena_);

		if (!loadFromFile(imgFile, pTex->getName())) {
			return false;
		}

		if (!processImgFile(pTex, imgFile)) {
			return false;
		}

		if (!createDeviceTexture(pTex)) {
			return false;
		}

		if (imgFile.getDepth() > 1 || imgFile.getNumFaces() > 1)
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		// upload img data to gpu.
		D3D12_SUBRESOURCE_DATA texResource[texture::TEX_MAX_MIPS];

		for(size_t i=0; i< imgFile.getNumMips(); i++)
		{
			const size_t rowBytes = imgFile.getLevelRowbytes(i);
			const size_t pitch = imgFile.getLevelSize(i);

			texResource[i].pData = imgFile.getLevel(0, i);
			texResource[i].RowPitch = rowBytes;
			texResource[i].SlicePitch = pitch;
		}

		auto& gpuResource = pTex->getGpuResource();

		if (!initializeTexture(gpuResource, imgFile.getNumMips(), texResource)) {
			// we should mark the texture as invalid.
			return false;
		}

		return true;
	}

	bool TextureManager::processImgFile(Texture* pTex, XTextureFile& imgFile)
	{
		// ummm check shit like generating mip maps and limits?
		// or converting a format if it's not support :S ?
		pTex->setFormat(imgFile.getFormat());
		pTex->setType(imgFile.getType());
		pTex->setWidth(imgFile.getWidth());
		pTex->setHeight(imgFile.getHeight());
		pTex->setDepth(imgFile.getDepth());
		pTex->setNumFaces(imgFile.getNumFaces());
		pTex->setNumMips(imgFile.getNumMips());

		const auto flags = pTex->getFlags();
		if (flags.IsSet(TextureFlags::FORCE_MIPS))
		{
			// we could just load the img lib and process this.
			// but fuck you.
			X_ASSERT_NOT_IMPLEMENTED();
			return false;
		}

		return true;
	}

	bool TextureManager::createDeviceTexture(Texture* pTex)
	{
		auto fmt = Util::DXGIFormatFromTexFmt(pTex->getFormat());
		auto& gpuResource = pTex->getGpuResource();

		D3D12_RESOURCE_DESC texDesc;
		core::zero_object(texDesc);
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Width = pTex->getWidth();
		texDesc.Height = pTex->getHeight();
		texDesc.DepthOrArraySize = pTex->getDepth();
		texDesc.MipLevels = pTex->getNumMips();
		texDesc.Format = fmt;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES heapProps;
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		HRESULT hr = pDevice_->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
			gpuResource.getUsageState(), nullptr, IID_PPV_ARGS(&gpuResource.getResourcePtrRef()));
		if (FAILED(hr)) {
			X_ERROR("Texture", "Failed to create resource for texture. err: %" PRIu32, hr);
			return false;
		}


		if (pTex->getSRV().ptr == render::D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
			D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor = descriptorAlloc_.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		
			pTex->setSRV(cpuDescriptor);
		}

		pDevice_->CreateShaderResourceView(gpuResource.getResource(), nullptr, pTex->getSRV());

		render::D3DDebug::SetDebugObjectName(gpuResource.getResource(), pTex->getName());

		return true;
	}

	bool TextureManager::initializeTexture(render::GpuResource& dest, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* pSubData)
	{
		const uint64_t uploadBufSize = getRequiredIntermediateSize(dest.getResource(), 0, numSubresources);

		D3D12_HEAP_PROPERTIES heapProps;
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC bufferDesc;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = uploadBufSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ID3D12Resource* pUploadBuffer;

		HRESULT hr = pDevice_->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
			&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pUploadBuffer));

		if (FAILED(hr))
		{
			// if we fail we can handle it by just showing default texture, without much trouble.
			X_ERROR("Texture", "Failed to create commited resource for uploading. res: %" PRIu32, hr);
			return false;
		}

		// we going todo this sync, but could keep hold of the resource and release when done.
		render::CommandContext* pContext = contextMan_.allocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);

		// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
		pContext->transitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
		pContext->updateSubresources<texture::TEX_MAX_MIPS>(pDevice_, dest, pUploadBuffer, 0, 0, numSubresources, pSubData);
		pContext->transitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);
		pContext->finishAndFree(true);

		pUploadBuffer->Release();

		return true;
	}

	uint64_t TextureManager::getRequiredIntermediateSize(ID3D12Resource* pDestinationResource,
		uint32_t firstSubresource, uint32_t numSubresources) const
	{
		const D3D12_RESOURCE_DESC desc = pDestinationResource->GetDesc();
		uint64_t requiredSize = 0;

		ID3D12Device* pDevice;
		pDestinationResource->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
		pDevice->GetCopyableFootprints(&desc, firstSubresource, numSubresources, 0, nullptr, nullptr, nullptr, &requiredSize);
		pDevice->Release();

		return requiredSize;
	}

	bool TextureManager::loadFromFile(XTextureFile& imgFile, const char* pPath)
	{
		X_ASSERT_NOT_NULL(pCILoader_);
		X_ASSERT_NOT_NULL(pPath);

		core::IFileSys* pFileSys = gEnv->pFileSys;

		core::IFileSys::fileModeFlags mode;
		mode.Set(core::IFileSys::fileMode::READ);
		mode.Set(core::IFileSys::fileMode::SHARE);

		core::Path<char> path(pPath);
		path.toLower(); // lower case file names only.
		path.setExtension(CI_FILE_EXTENSION);

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


	void TextureManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
	{
		X_UNUSED(jobSys);
		X_UNUSED(name);

	}


	void TextureManager::Cmd_ReloadTextures(core::IConsoleCmdArgs* pCmd)
	{
		X_UNUSED(pCmd);


	}

	void TextureManager::Cmd_ReloadTexture(core::IConsoleCmdArgs* pCmd)
	{
		if (pCmd->GetArgCount() < 2) {
			X_ERROR("Texture", "imageReload <filename>");
			return;
		}

		const char* pName = pCmd->GetArg(1);

		reloadForName(pName);
	}


X_NAMESPACE_END