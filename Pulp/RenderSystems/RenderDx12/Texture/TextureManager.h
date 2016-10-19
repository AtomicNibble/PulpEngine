#pragma once

#include <Containers\HashMap.h>
#include <IDirectoryWatcher.h>

#include "TextureVars.h"

X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_DECLARE(render,
	class ContextManager;
	class CommandListManger;
	class DescriptorAllocator;
	class GpuResource;
);

X_NAMESPACE_BEGIN(texture)

class Texture;
class XTextureFile;

class TextureManager : public core::IXHotReload
{
	typedef core::HashMap<core::string, Texture*> TextureMap;
	typedef core::FixedArray<ITextureFmt*, 8> TextureLoadersArr;

public:
	TextureManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice, 
		render::ContextManager& contextMan, render::DescriptorAllocator& descriptorAlloc);
	~TextureManager();

	bool init(void);
	bool shutDown(void);

	void registerVars(void);
	void registerCmds(void);


	Texture* forName(const char* pName, TextureFlags flags);
	Texture* getByID(TexID texId);
	Texture* getDefault(void);

private:
	Texture* findTexture(const char* pName);
	Texture* findTexture(const core::string& name);
	bool reloadForName(const char* pName);

	bool loadDefaultTextures(void);
	void releaseDefaultTextures(void);
	void releaseDanglingTextures(void);

	bool stream(Texture* pTex);
	bool load(Texture* pTex);
	bool loadFromFile(XTextureFile& imgFile, const char* pPath);
	bool processImgFile(Texture* pTex, XTextureFile& imgFile);
	bool createDeviceTexture(Texture* pTex, XTextureFile& imgFile);
	bool initializeTexture(render::GpuResource& dest, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* pSubData);

	uint64_t getRequiredIntermediateSize(ID3D12Resource* pDestinationResource,
		uint32_t firstSubresource, uint32_t numSubresources);

	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload


private:
	void Cmd_ReloadTextures(core::IConsoleCmdArgs* pCmd);
	void Cmd_ReloadTexture(core::IConsoleCmdArgs* pCmd);

	static DXGI_FORMAT DXGIFormatFromTexFmt(Texturefmt::Enum fmt);

private:
	ID3D12Device* pDevice_;
	render::ContextManager& contextMan_;
	render::DescriptorAllocator& descriptorAlloc_;

	core::MemoryArenaBase* arena_;
	TextureMap textures_;
	TextureVars vars_;

	ITextureFmt* pCILoader_;
	TextureLoadersArr textureLoaders_;

private:
	Texture* pTexDefault_;
	Texture* pTexDefaultBump_;
	Texture* ptexMipMapDebug_;
};


X_NAMESPACE_END