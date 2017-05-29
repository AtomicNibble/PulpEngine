#pragma once

#include <Assets\AssertContainer.h>
#include <ITexture.h>

#include "Vars\TextureVars.h"


X_NAMESPACE_DECLARE(texture,
	namespace CI {
		class XTexLoaderCI;
	}	
)

X_NAMESPACE_BEGIN(engine)

class Texture;

class TextureManager : public core::IXHotReload
{
	typedef core::AssetContainer<Texture, texture::TEX_MAX_LOADED_IMAGES, core::MultiThreadPolicy<core::Spinlock>> TextureContainer;
	typedef TextureContainer::Resource TextureResource;
	typedef TextureContainer::Resource TexRes;

	typedef core::FixedArray<texture::ITextureFmt*, 8> TextureLoadersArr;

public:
	TextureManager(core::MemoryArenaBase* arena);
	~TextureManager();

	void registerCmds(void);
	void registerVars(void);
	
	bool init(void);
	void shutDown(void);


	Texture* forName(const char* pName, texture::TextureFlags flags);

private:
	bool loadFromFile(texture::XTextureFile& imgFile, const char* pPath);
	bool processImgFile(Texture* pTex, texture::XTextureFile& imgFile);


private:
	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

private:
	core::MemoryArenaBase* arena_;
	TextureVars vars_;
	TextureContainer textures_;

	texture::CI::XTexLoaderCI* pCILoader_;
	TextureLoadersArr textureLoaders_;
};



X_NAMESPACE_END