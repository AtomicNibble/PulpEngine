#include "stdafx.h"
#include "TextureManager.h"
#include "Texture.h"

#include <ICi.h>
#include <IConsole.h>

X_NAMESPACE_BEGIN(texture)


	TextureManager::TextureManager(core::MemoryArenaBase* arena) :
		arena_(arena),
		textures_(arena, TEX_MAX_LOADED_IMAGES),
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


		pCILoader_ = X_NEW(CI::XTexLoaderCI, arena_, "CILoader");

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

	bool TextureManager::loadFromFile(const char* pPath)
	{
		X_ASSERT_NOT_NULL(pCILoader_);
		X_ASSERT_NOT_NULL(pPath);

		core::IFileSys::fileModeFlags mode;
		mode.Set(core::IFileSys::fileMode::READ);

		core::Path<char> path(pPath);
		path.toLower(); // lower case file names only.
		path.setExtension(CI_FILE_EXTENSION);

		XTextureFile imgFile(arena_);
		core::XFileScoped file;

		if (file.openFile(path.c_str(), mode)) 	
		{
			if (!pCILoader_->loadTexture(file.GetFile(), imgFile, arena_)) {
				X_WARNING("Texture", "Failed to load: \"%s\"", pPath);
				return false;
			}

			return true;
		}

		if (!vars_.allowRawImgLoading()) {
			return false;
		}

		// try loading none compiled.
		core::IFileSys* pFileSys = gEnv->pFileSys;
		for (auto pLoader : textureLoaders_)
		{
			path.setExtension(pLoader->getExtension());

			if (pFileSys->fileExists(path.c_str()))
			{
				if (!pLoader->loadTexture(file.GetFile(), imgFile, arena_)) {
					X_WARNING("Texture", "Failed to load: \"%s\"", pPath);
					return false;
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

	DXGI_FORMAT TextureManager::DXGIFormatFromTexFmt(Texturefmt::Enum fmt)
	{
		switch (fmt)
		{
		case Texturefmt::R8G8B8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Texturefmt::R8G8B8A8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;

		case Texturefmt::B8G8R8A8:
			return DXGI_FORMAT_B8G8R8A8_UNORM;

		case Texturefmt::A8:
			return DXGI_FORMAT_A8_UNORM;

		case Texturefmt::A8R8G8B8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;

		case Texturefmt::BC1:
			return DXGI_FORMAT_BC1_UNORM;
		case Texturefmt::BC2:
			return DXGI_FORMAT_BC2_UNORM;
		case Texturefmt::BC3:
			return DXGI_FORMAT_BC3_UNORM;
		case Texturefmt::BC4:
			return DXGI_FORMAT_BC4_UNORM;
		case Texturefmt::BC4_SNORM:
			return DXGI_FORMAT_BC4_SNORM;

		case Texturefmt::BC5:
		case Texturefmt::ATI2:
			return DXGI_FORMAT_BC5_UNORM;
		case Texturefmt::BC5_SNORM:
			return DXGI_FORMAT_BC5_SNORM;

		case Texturefmt::BC6:
			return DXGI_FORMAT_BC6H_UF16; // HDR BAbbbbbbbbby!
		case Texturefmt::BC6_SF16:
			return DXGI_FORMAT_BC6H_SF16;
		case Texturefmt::BC6_TYPELESS:
			return DXGI_FORMAT_BC6H_TYPELESS;

		case Texturefmt::BC7:
			return DXGI_FORMAT_BC7_UNORM;
		case Texturefmt::BC7_SRGB:
			return DXGI_FORMAT_BC7_UNORM_SRGB;
		case Texturefmt::BC7_TYPELESS:
			return DXGI_FORMAT_BC7_TYPELESS;

		case Texturefmt::R16G16_FLOAT:
			return DXGI_FORMAT_R16G16_FLOAT;
		case Texturefmt::R10G10B10A2:
			return DXGI_FORMAT_R10G10B10A2_TYPELESS;

#if X_DEBUG
		default:
			X_ASSERT_UNREACHABLE();
			break;
#else
			X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
		}
		return DXGI_FORMAT_UNKNOWN;
	}

X_NAMESPACE_END