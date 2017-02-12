#pragma once

#include <Containers\HashMap.h>
#include <IDirectoryWatcher.h>

#include "TextureVars.h"

#include <Assets\AssertContainer.h>

X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_DECLARE(render,
	class ContextManager;
	class CommandContext;
	class CommandListManger;
	class DescriptorAllocator;
	class GpuResource;
);

X_NAMESPACE_BEGIN(texture)

class Texture;
class XTextureFile;

class TextureManager : public core::IXHotReload
{
	typedef core::AssetContainer<Texture, TEX_MAX_LOADED_IMAGES, core::MultiThreadPolicy<core::Spinlock>> TextureContainer;
	typedef TextureContainer::Resource TextureResource;
	typedef TextureContainer::Resource TexRes;

	typedef core::FixedArray<ITextureFmt*, 8> TextureLoadersArr;

public:
	TextureManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice, 
		render::ContextManager& contextMan, render::DescriptorAllocator& descriptorAlloc, DXGI_FORMAT depthFmt);
	~TextureManager();

	bool init(void);
	bool shutDown(void);

	void registerVars(void);
	void registerCmds(void);

	DXGI_FORMAT getDepthFmt(void) const;

	Texture* forName(const char* pName, TextureFlags flags);
	Texture* createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, const uint8_t* pInitialData = nullptr);
	
	// this is used for creating textures that can be used for more than just SRV's
	Texture* createPixelBuffer(const char* pNickName, Vec2i dim, uint32_t numMips,
		render::PixelBufferType::Enum type);

	Texture* getByID(TexID texId) const;
	Texture* getDefault(void) const;

	// must not be null.
	void releaseTexture(texture::ITexture* pTex);
	void releaseTexture(Texture* pTex);
	void releasePixelBuffer(render::IPixelBuffer* pPixelBuf);

	bool updateTexture(render::CommandContext& contex, TexID texId, const uint8_t* pSrc, uint32_t srcSize) const;


private:
	TexRes* findTexture(const char* pName);
	TexRes* findTexture(const core::string& name);
	bool reloadForName(const char* pName);

	bool loadDefaultTextures(void);
	void releaseDefaultTextures(void);
	void releaseDanglingTextures(void);

	bool stream(Texture* pTex);
	bool load(Texture* pTex);
	bool loadFromFile(XTextureFile& imgFile, const char* pPath);
	bool processImgFile(Texture* pTex, XTextureFile& imgFile);
	bool createDeviceTexture(Texture* pTex);
	bool initializeTexture(render::GpuResource& dest, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* pSubData);

	uint64_t getRequiredIntermediateSize(ID3D12Resource* pDestinationResource,
		uint32_t firstSubresource, uint32_t numSubresources) const;

	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

	void listTextures(const char* pSearchPattern);

private:
	void Cmd_ReloadTextures(core::IConsoleCmdArgs* pCmd);
	void Cmd_ReloadTexture(core::IConsoleCmdArgs* pCmd);
	void Cmd_ListTextures(core::IConsoleCmdArgs* pCmd);

private:
	ID3D12Device* pDevice_;
	render::ContextManager& contextMan_;
	render::DescriptorAllocator& descriptorAlloc_;
	DXGI_FORMAT depthFmt_;

	core::MemoryArenaBase* arena_;
	TextureContainer textures_;
	TextureVars vars_;

	ITextureFmt* pCILoader_;
	TextureLoadersArr textureLoaders_;

private:
	Texture* pTexDefault_;
	Texture* pTexDefaultBump_;
	Texture* ptexMipMapDebug_;
};


X_NAMESPACE_END