#include "stdafx.h"
#include "TextureManager.h"

#include "Texture.h"

X_NAMESPACE_BEGIN(engine)


TextureManager::TextureManager(core::MemoryArenaBase* arena) :
	arena_(arena),
	textures_(arena, sizeof(TextureResource), core::Max(64_sz, X_ALIGN_OF(TextureResource))),
	pCILoader_(nullptr)
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


void TextureManager::registerCmds(void)
{

}

void TextureManager::registerVars(void)
{
	vars_.registerVars();
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
		pTexRes = textures_.createAsset(name, name, flags);

		threadPolicy.Leave();
	}

	return pTexRes;
}


void TextureManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys, name);

}


X_NAMESPACE_END