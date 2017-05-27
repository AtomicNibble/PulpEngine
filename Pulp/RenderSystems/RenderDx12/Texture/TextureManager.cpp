#include "stdafx.h"
#include "TextureManager.h"
#include "Texture.h"
#include "TextureUtil.h"

#include <ICi.h>
#include <IConsole.h>
#include <IFileSys.h>

// Img Lib
#include <../../tools/ImgLib/ImgLib.h>

#include "Allocators\DescriptorAllocator.h"
#include "CommandContex.h"

// buffers
#include "Buffers\ColorBuffer.h"
#include "Buffers\DepthBuffer.h"
#include "Buffers\ShadowBuffer.h"



X_NAMESPACE_BEGIN(texture)



	TextureManager::TextureManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
			texture::TextureVars texVars, render::ContextManager& contextMan,
			render::DescriptorAllocator& descriptorAlloc, DXGI_FORMAT depthFmt, bool reverseZ) :
		contextMan_(contextMan),
		vars_(texVars),
		pDevice_(pDevice),
		descriptorAlloc_(descriptorAlloc),
		depthFmt_(depthFmt),
		arena_(arena),
		textures_(arena, sizeof(TextureResource), core::Max(64_sz, X_ALIGN_OF(TextureResource))),
		clearDepthVal_(reverseZ ? 0.f : 1.f),
		pTexDefault_(nullptr),
		pTexDefaultBump_(nullptr),
		ptexMipMapDebug_(nullptr)
	{
		X_ENSURE_LE(sizeof(TextureResource), 128, "Texture with ref count should be 128 bytes or less");

	}

	TextureManager::~TextureManager()
	{

	}

	bool TextureManager::init(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pConsole);
		X_LOG1("TextureManager", "Starting");
		X_PROFILE_NO_HISTORY_BEGIN("TextureMan", core::profiler::SubSys::RENDER);

		if (!loadDefaultTextures()) {
			X_ERROR("TextureManager", "Failed to load default textures");
			return false;
		}

		return true;
	}

	bool TextureManager::shutDown(void)
	{
		X_LOG1("TextureManager", "Shutting Down");

		releaseDefaultTextures();
		releaseDanglingTextures();

		return true;
	}


	void TextureManager::registerCmds(void)
	{

		ADD_COMMAND_MEMBER("imgReloadAll", this, TextureManager, &TextureManager::Cmd_ReloadTextures, core::VarFlag::SYSTEM,
			"Reload all textures");
		ADD_COMMAND_MEMBER("imgReload", this, TextureManager, &TextureManager::Cmd_ReloadTexture, core::VarFlag::SYSTEM,
			"Reload a textures <name>");

		ADD_COMMAND_MEMBER("listImages", this, TextureManager, &TextureManager::Cmd_ListTextures, core::VarFlag::SYSTEM,
			"List all the loaded textures");

		
	}

	DXGI_FORMAT TextureManager::getDepthFmt(void) const
	{
		return depthFmt_;
	}

	Texture* TextureManager::forName(const char* pName, TextureFlags flags)
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
			pTexRes = textures_.createAsset(name, name, flags);
			threadPolicy.Leave();
		}

		return pTexRes;
	}

	Texture* TextureManager::createTexture(const char* pNickName, Vec2i dim,
		texture::Texturefmt::Enum fmt, const uint8_t* pInitialData)
	{
		core::string name(pNickName);

		auto& threadPolicy = textures_.getThreadPolicy();
		threadPolicy.Enter();

		TexRes* pTexRes = textures_.findAsset(name);

		if (pTexRes)
		{
			threadPolicy.Leave();

			pTexRes->addReference();

			X_WARNING("Texture", "Created texture with matching name of exsisting texture, returning original: \"%s\"", pNickName);
			if (pInitialData) {
				// update?
			}
		}
		else
		{
			pTexRes = textures_.createAsset(name, name, TexFlag::DONT_STREAM | TexFlag::DONT_RESIZE | TexFlag::NOMIPS);

			threadPolicy.Leave();

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

	Texture* TextureManager::createPixelBuffer(const char* pNickName, Vec2i dim, uint32_t numMips,
		render::PixelBufferType::Enum type)
	{
		core::string name(pNickName);

		//
		//	For pixel buffers we store them in the same texture pool / hash
		//	This has a number of benfits in that pixel buffers are listed in texture list.
		//	it also makes it more simple to read from a pixel buffer as they can be passed to the pipeline as if they just normal textures.
		//	
		//
		auto& threadPolicy = textures_.getThreadPolicy();
		threadPolicy.Enter();

		TexRes* pTexRes = textures_.findAsset(name);
		if (pTexRes)
		{
			threadPolicy.Leave();

			// do we want to allow ref counted pixelBuffers?
			X_WARNING("TexMan", "Pixel buffer already exsists: \"%s\"", name.c_str());
			pTexRes->addReference();
		}
		else
		{
			pTexRes = textures_.createAsset(name, name, TextureFlags());

			threadPolicy.Leave();

			// okay now we create the buffer resource.
			if (type == render::PixelBufferType::DEPTH)
			{
				render::DepthBuffer* pDethBuf = X_NEW(render::DepthBuffer, arena_, "DepthBuffer")(*pTexRes, clearDepthVal_, 0);
				pDethBuf->create(pDevice_, descriptorAlloc_, dim.x, dim.y, depthFmt_);
				pTexRes->setPixelBuffer(type, pDethBuf);
			}
			else if (type == render::PixelBufferType::COLOR)
			{
				render::ColorBuffer* pColBuffer = X_NEW(render::ColorBuffer, arena_, "ColorBuffer")(*pTexRes);
				pTexRes->setPixelBuffer(type, pColBuffer);
			}
			else if (type == render::PixelBufferType::SHADOW)
			{

				X_ASSERT_NOT_IMPLEMENTED();
			}
			
			// check we have a pixel buf.
			X_ASSERT_NOT_NULL(pTexRes->pPixelBuffer_);
			X_ASSERT(pTexRes->getBufferType() != render::PixelBufferType::NONE, "Type not set")();
		}


		return pTexRes;
	}


	Texture* TextureManager::getByID(TexID texId) const
	{
		core::ScopedLock<TextureContainer::ThreadPolicy> lock(textures_.getThreadPolicy());

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
			
			X_ASSERT(pTex->getBufferType() == render::PixelBufferType::NONE, "PixelBuffers should not be released here")();

			textures_.releaseAsset(pTexRes);
		}
	}

	void TextureManager::releasePixelBuffer(render::IPixelBuffer* pPixelBuf)
	{
		TexRes* pTexRes = static_cast<TexRes*>(pPixelBuf);

		if (pTexRes->removeReference() == 0)
		{
			core::string name(pTexRes->getName());

			releasePixelBuffer_internal(pTexRes);

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

	X_INLINE void TextureManager::releasePixelBuffer_internal(render::IPixelBuffer* pPixelBuf)
	{
		Texture* pTex = static_cast<Texture*>(pPixelBuf);

		static_assert(render::PixelBufferType::ENUM_COUNT == 4, "pixel buffer enum changed, this code need updating?");

		if (pTex->getBufferType() != render::PixelBufferType::NONE)
		{
			switch (pTex->getBufferType())
			{
				case render::PixelBufferType::COLOR:
					X_DELETE(&pTex->getColorBuf(), arena_);
					break;
				case render::PixelBufferType::DEPTH:
					X_DELETE(&pTex->getDepthBuf(), arena_);
					break;
				case render::PixelBufferType::SHADOW:
					X_DELETE(&pTex->getShadowBuf(), arena_);
					break;
				default:
					X_ASSERT_UNREACHABLE();
					break;
			}

#if X_DEBUG
			pTex->setPixelBuffer(render::PixelBufferType::NONE, nullptr);
#endif // !X_DEBUG
		}
	}

	TextureManager::TexRes* TextureManager::findTexture(const char* pName)
	{
		core::string name(pName);
		return findTexture(name);
	}

	TextureManager::TexRes* TextureManager::findTexture(const core::string& name)
	{
		core::ScopedLock<TextureContainer::ThreadPolicy> lock(textures_.getThreadPolicy());

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
		{
			core::ScopedLock<TextureContainer::ThreadPolicy> lock(textures_.getThreadPolicy());

			auto it = textures_.begin();
			for (; it != textures_.end(); ++it) {
				auto* pTexRes = it->second;
				releasePixelBuffer_internal(pTexRes);
				X_WARNING("Texture", "\"%s\" was not deleted. refs: %" PRIi32, pTexRes->getName(), pTexRes->getRefCount());
			}
		}

		textures_.free();
	}


#if 0
	bool TextureManager::load(Texture* pTex)
	{
		X_ASSERT_NOT_NULL(pTex);

		XTextureFile imgFile(arena_);

//		if (!loadFromFile(imgFile, pTex->getName())) {
//			return false;
//		}

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
			X_ERROR("Texture", "Failed to upload texture data");
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
#endif

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

#if 0
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
#endif

	// -----------------------------------

	void TextureManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
	{
		X_UNUSED(jobSys);
		X_UNUSED(name);

	}

	// -----------------------------------

	void TextureManager::listTextures(const char* pSearchPattern)
	{
		core::ScopedLock<TextureContainer::ThreadPolicy> lock(textures_.getThreadPolicy());

		core::Array<TextureContainer::Resource*> sorted_texs(arena_);
		sorted_texs.reserve(textures_.size());

		for (const auto& mat : textures_)
		{
			if (!pSearchPattern || core::strUtil::WildCompare(pSearchPattern, mat.first))
			{
				sorted_texs.push_back(mat.second);
			}
		}

		std::sort(sorted_texs.begin(), sorted_texs.end(), [](TextureContainer::Resource* a, TextureContainer::Resource* b) {
			const auto& nameA = a->getName();
			const auto& nameB = b->getName();
			return nameA.compareInt(nameB) < 0;
		}
		);

		X_LOG0("Texture", "------------- ^8Textures(%" PRIuS ")^7 ------------", sorted_texs.size());

		for (const auto* pTex : sorted_texs)
		{
			X_LOG0("Texture", "^2\"%s\"^7 dim: ^2%" PRIi32 "x%" PRIi32 " ^7mips: ^2%" PRIi32 " ^7fmt: ^2%s ^7refs: ^2%" PRIi32,
				pTex->getName(), pTex->getWidth(), pTex->getHeight(), pTex->getNumMips(), 
				Texturefmt::ToString(pTex->getFormat()), pTex->getRefCount());
		}

		X_LOG0("Texture", "------------ ^8Textures End^7 ------------");
	}

	// -----------------------------------

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

	void TextureManager::Cmd_ListTextures(core::IConsoleCmdArgs* pCmd)
	{
		const char* pSearchPattern = nullptr;

		if (pCmd->GetArgCount() >= 2) {
			pSearchPattern = pCmd->GetArg(1);
		}

		listTextures(pSearchPattern);
	}




X_NAMESPACE_END