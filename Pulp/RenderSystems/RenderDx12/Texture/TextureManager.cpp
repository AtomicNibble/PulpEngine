#include "stdafx.h"
#include "TextureManager.h"
#include "Texture.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(texture)


	TextureManager::TextureManager(core::MemoryArenaBase* arena) :
		arena_(arena),
		textures_(arena, 2048),
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

		registerVars();
		registerCmds();

		loadDefaultTextures();

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

		Texture* pTex = findTexture(name);

		if (pTex)
		{
		//	pTex->addRef(); // add a ref.
		}
		else
		{
			pTex = X_NEW(Texture, arena_, "Texture")(pName, flags);

			textures_.insert(std::make_pair(name, pTex));
		}

		return pTex;
	}


	Texture* TextureManager::getByID(TexID texId)
	{
		X_UNUSED(texId);
		return nullptr;
	}

	Texture* TextureManager::getDefault(void)
	{
		return pTexDefault_;
	}

	Texture* TextureManager::findTexture(const char* pName)
	{
		auto it = textures_.find(X_CONST_STRING(pName));
		if (it != textures_.end()) {
			return it->second;
		}

		return nullptr;
	}

	Texture* TextureManager::findTexture(const core::string& name)
	{
		auto it = textures_.find(name);
		if (it != textures_.end()) {
			return it->second;
		}

		return nullptr;
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

		ptexMipMapDebug_ = forName("Textures/Debug/MipMapDebug", default_flags | TextureFlags::FILTER_BILINEAR);


		return true;
	}
		
	void TextureManager::releaseDefaultTextures(void)
	{

		core::SafeRelease(pTexDefault_);
		core::SafeRelease(pTexDefaultBump_);
		core::SafeRelease(ptexMipMapDebug_);

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