#pragma once

#include <Containers\HashMap.h>

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

class TextureManager
{
	typedef core::AssetContainer<Texture, TEX_MAX_LOADED_IMAGES, core::MultiThreadPolicy<core::Spinlock>> TextureContainer;
	typedef TextureContainer::Resource TextureResource;
	typedef TextureContainer::Resource TexRes;

public:
	TextureManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice, texture::TextureVars texVars, render::ContextManager& contextMan,
		render::DescriptorAllocator& descriptorAlloc, DXGI_FORMAT depthFmt, bool reverseZ);
	~TextureManager();

	void registerCmds(void);

	bool init(void);
	bool shutDown(void);


	DXGI_FORMAT getDepthFmt(void) const;

	Texture* getDeviceTexture(int32_t id);

	Texture* createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, const uint8_t* pInitialData = nullptr);
	
	// this is used for creating textures that can be used for more than just SRV's
	Texture* createPixelBuffer(const char* pNickName, Vec2i dim, uint32_t numMips,
		render::PixelBufferType::Enum type);

	Texture* getByID(TexID texId) const;

	// must not be null.
	void releaseTexture(render::IDeviceTexture* pTex);
	void releaseTexture(Texture* pTex);
	void releasePixelBuffer(render::IPixelBuffer* pPixelBuf);

	bool initDeviceTexture(Texture* pTex) const;
	bool initDeviceTexture(Texture* pTex, const texture::XTextureFile& imgFile) const;
	
	bool updateTextureData(render::CommandContext& contex, TexID texId, const uint8_t* pSrc, uint32_t srcSize) const;

private:
	X_INLINE bool updateTextureData(Texture* pTex, const texture::XTextureFile& imgFile) const;
	X_INLINE bool updateTextureData(Texture* pTex, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* pSubData) const;
	bool updateTextureData(render::GpuResource& dest, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* pSubData) const;

	static uint64_t getRequiredIntermediateSize(ID3D12Resource* pDestinationResource,
		uint32_t firstSubresource, uint32_t numSubresources);

private:
	void releasePixelBuffer_internal(render::IPixelBuffer* pPixelBuf);

	TexRes* findTexture(const char* pName);
	TexRes* findTexture(const core::string& name);
	bool reloadForName(const char* pName);

	void releaseDanglingTextures(void);

	void listTextures(const char* pSearchPattern);

private:
	void Cmd_ListTextures(core::IConsoleCmdArgs* pCmd);

private:
	ID3D12Device* pDevice_;
	render::ContextManager& contextMan_;
	render::DescriptorAllocator& descriptorAlloc_;
	DXGI_FORMAT depthFmt_;

	core::MemoryArenaBase* arena_;
	TextureContainer textures_;
	TextureVars& vars_;

	float clearDepthVal_;
};


X_NAMESPACE_END