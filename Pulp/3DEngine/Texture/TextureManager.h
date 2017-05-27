#pragma once

#include <Assets\AssertContainer.h>
#include <ITexture.h>

#include "Vars\TextureVars.h"

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

	bool init(void);
	void shutDown(void);

	void registerCmds(void);
	void registerVars(void);

	Texture* forName(const char* pName, texture::TextureFlags flags);

private:
	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

private:
	core::MemoryArenaBase* arena_;
	TextureVars vars_;
	TextureContainer textures_;

	texture::ITextureFmt* pCILoader_;
	TextureLoadersArr textureLoaders_;
};



X_NAMESPACE_END