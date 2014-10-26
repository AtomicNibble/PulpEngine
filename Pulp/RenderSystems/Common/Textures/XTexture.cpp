#include "stdafx.h"
#include "XTexture.h"

#include "TextureLoaderCI.h"
#include "TextureLoaderDDS.h"
#include "TextureLoaderJPG.h"
#include "TextureLoaderPNG.h"
#include "TextureLoaderPSD.h"
#include "TextureLoaderTGA.h"

#include "Hashing\Fnva1Hash.h"
#include "Containers\HashMap.h"

#include "IFileSys.h"
#include "XTextureFile.h"

#include "../ReaderThread.h"
#include "../XRender.h"

X_NAMESPACE_BEGIN(texture)

// temp just so they link.
namespace {

	CI::XTexLoaderCI ci;
	DDS::XTexLoaderDDS dds;
	JPG::XTexLoaderJPG jpg;
	PNG::XTexLoaderPNG png;
	PSD::XTexLoaderPSD psd;
	TGA::XTexLoaderTGA tga;

	static ITextureLoader* image_file_loaders[] =
	{
		&ci,
		&dds,
		&jpg,
		&png,
		&psd,
		&tga
	};

	struct eqstr {
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) == 0;
		}
	};

	struct strhash {
		size_t operator()(const char* s1) const
		{
			return core::Hash::Fnv1aHash(s1, core::strUtil::strlen(s1));
		}
	};


	class ImgHotReload : public core::IXHotReload
	{
		bool OnFileChange(const char* name) X_OVERRIDE
		{
			return XTexture::reloadForName(name);
		}

	};

	ImgHotReload g_ImgHotReload;
}

render::XRenderResourceContainer*		XTexture::s_pTextures = nullptr;

bool							XTexture::s_DefaultTexturesLoaded = false;

int								XTexture::s_GlobalDefaultTexStateId = -1;
shader::XTexState				XTexture::s_GlobalDefaultTexState;
shader::XTexState				XTexture::s_DefaultTexState;
core::Array<shader::XTexState>	XTexture::s_TexStates(nullptr);


XTexture* XTexture::s_pTexDefault = nullptr;
XTexture* XTexture::s_pTexDefaultBump = nullptr;

XTexture* XTexture::s_ptexMipMapDebug = nullptr;
XTexture* XTexture::s_ptexColorBlue = nullptr;
XTexture* XTexture::s_ptexColorCyan = nullptr;
XTexture* XTexture::s_ptexColorGreen = nullptr;
XTexture* XTexture::s_ptexColorPurple = nullptr;
XTexture* XTexture::s_ptexColorRed = nullptr;
XTexture* XTexture::s_ptexColorWhite = nullptr;
XTexture* XTexture::s_ptexColorYellow = nullptr;
XTexture* XTexture::s_ptexColorMagenta = nullptr;
XTexture* XTexture::s_ptexColorOrange = nullptr;

XTexture* XTexture::s_GBuf_Depth = nullptr;
XTexture* XTexture::s_GBuf_Albedo = nullptr;
XTexture* XTexture::s_GBuf_Normal = nullptr;
XTexture* XTexture::s_GBuf_Spec = nullptr;


XTexture::XTexture()
{
	dimensions = Vec2<uint16_t>::zero();
	datasize = 0;
	type = TextureType::UNKNOWN;
	format = Texturefmt::UNKNOWN;
	numMips = 0;
	depth = 0;
	numFaces = 0;
	defaultTexStateId_ = -1;

	pDeviceShaderResource_ = nullptr;
	pDeviceRenderTargetView_ = nullptr;
}

XTexture::~XTexture()
{
	if (render::gRenDev->rThread())
		render::gRenDev->rThread()->RC_ReleaseDeviceTexture(this);
}

const int XTexture::release()
{
	int ref = XBaseAsset::release();
	if (ref == 0)
	{
		X_DELETE(this, g_rendererArena);
	}

	return ref;
}


bool XTexture::RT_ReleaseDevice(void)
{
	return ReleaseDeviceTexture();
}


DXGI_FORMAT XTexture::DCGIFormatFromTexFmt(Texturefmt::Enum fmt)
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
			return DXGI_FORMAT_BC6H_TYPELESS; // HDR BAbbbbbbbbby!
		case Texturefmt::BC7:
			return DXGI_FORMAT_BC7_UNORM;

		case Texturefmt::R16G16F:
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

Texturefmt::Enum XTexture::TexFmtFromStr(const char* pStr)
{

	if (core::strUtil::IsEqualCaseInsen("R8G8B8", pStr))
		return Texturefmt::R8G8B8;
	if (core::strUtil::IsEqualCaseInsen("R8G8B8A8", pStr))
		return Texturefmt::R8G8B8A8;

	if (core::strUtil::IsEqualCaseInsen("B8G8R8", pStr))
		return Texturefmt::B8G8R8;
	if (core::strUtil::IsEqualCaseInsen("B8G8R8A8", pStr))
		return Texturefmt::B8G8R8A8;

	if (core::strUtil::IsEqualCaseInsen("A8", pStr))
		return Texturefmt::A8;
	if (core::strUtil::IsEqualCaseInsen("ATI2", pStr))
		return Texturefmt::ATI2;
	if (core::strUtil::IsEqualCaseInsen("ATI2_XY", pStr))
		return Texturefmt::ATI2_XY;
	if (core::strUtil::IsEqualCaseInsen("BC1", pStr))
		return Texturefmt::BC1;
	if (core::strUtil::IsEqualCaseInsen("BC2", pStr))
		return Texturefmt::BC2;
	if (core::strUtil::IsEqualCaseInsen("BC3", pStr))
		return Texturefmt::BC3;
	if (core::strUtil::IsEqualCaseInsen("BC4", pStr))
		return Texturefmt::BC4;
	if (core::strUtil::IsEqualCaseInsen("BC4_SNORM", pStr))
		return Texturefmt::BC4_SNORM;
	if (core::strUtil::IsEqualCaseInsen("BC5", pStr))
		return Texturefmt::BC5;
	if (core::strUtil::IsEqualCaseInsen("BC5_SNORM", pStr))
		return Texturefmt::BC5_SNORM;
	if (core::strUtil::IsEqualCaseInsen("BC6", pStr))
		return Texturefmt::BC6;
	if (core::strUtil::IsEqualCaseInsen("BC7", pStr))
		return Texturefmt::BC7;
	if (core::strUtil::IsEqualCaseInsen("R16G16F", pStr))
		return Texturefmt::R16G16F;
	if (core::strUtil::IsEqualCaseInsen("R10G10B10A2", pStr))
		return Texturefmt::R10G10B10A2;

	// this is here incase i've forgot to add a new format
	// into this function.
	X_ASSERT_UNREACHABLE();
	return Texturefmt::UNKNOWN;
}

uint32_t XTexture::NumMipsMips(uint32_t Width, uint32_t Height)
{
	uint32_t Biggest = core::Max<uint32_t>(Width, Height);
	uint32_t mips = 0;
	while (Biggest > 0) {
		mips++;
		Biggest >>= 1;
	}
	return mips;
}

bool XTexture::is_dxt(Texturefmt::Enum fmt)
{
	switch (fmt)
	{
	case Texturefmt::BC1:
	case Texturefmt::BC2:
	case Texturefmt::BC3:
	case Texturefmt::BC4:
	case Texturefmt::BC4_SNORM:
	case Texturefmt::BC5:
	case Texturefmt::BC5_SNORM:
	case Texturefmt::BC6:
	case Texturefmt::BC7:
	case Texturefmt::ATI2:
	case Texturefmt::ATI2_XY:
		return true;
	default: break;
	}
	return false;
}

uint32_t XTexture::get_bpp(Texturefmt::Enum fmt)
{
	switch (fmt)
	{
	case Texturefmt::BC1:			return 4;
	case Texturefmt::BC2:			return 4;
	case Texturefmt::BC3:			return 8;
	case Texturefmt::BC4:			return 8;
	case Texturefmt::BC4_SNORM:     return 8;
	case Texturefmt::BC5:			return 8;
	case Texturefmt::BC5_SNORM:     return 8;
	case Texturefmt::BC6:			return 8;
	case Texturefmt::BC7:			return 8;

	case Texturefmt::ATI2:			return 8;
	case Texturefmt::ATI2_XY:		return 8;

	case Texturefmt::R8G8B8:		return 24;
	case Texturefmt::B8G8R8:		return 24;
	case Texturefmt::A8R8G8B8:		return 32;
	case Texturefmt::R8G8B8A8:		return 32;
	case Texturefmt::B8G8R8A8:		return 32;
	case Texturefmt::A8:			return 8;

	case Texturefmt::R16G16F:		return 32;
	case Texturefmt::R10G10B10A2:	return 32;

	default:
		break;
	}
	X_ASSERT_UNREACHABLE();
	return 0;
};


uint32_t XTexture::get_dxt_bytes_per_block(Texturefmt::Enum fmt)
{
	switch (fmt)
	{
	case Texturefmt::BC1:			return 8;

	case Texturefmt::BC2:			return 16;
	case Texturefmt::BC3:			return 16;
	case Texturefmt::BC4:			return 16;
	case Texturefmt::BC4_SNORM:     return 16;
	case Texturefmt::BC5:			return 16;
	case Texturefmt::BC5_SNORM:     return 16;
	case Texturefmt::BC6:			return 16;
	case Texturefmt::BC7:			return 16;

	case Texturefmt::ATI2:			return 16;
	case Texturefmt::ATI2_XY:       return 16;
	default:
		break;
	}

	return 0;
}

uint32_t XTexture::get_data_size(uint32_t width, uint32_t height,
	uint32_t depth, uint32_t mips, Texturefmt::Enum fmt)
{
	unsigned size = 0;
	unsigned i;

	const unsigned bits_per_pixel = get_bpp(fmt);
	const unsigned bytes_per_block = get_dxt_bytes_per_block(fmt);
	const bool isDXT = is_dxt(fmt);

	for (i = 0; i < mips; i++)
	{
		width = core::Max(1u, width);
		height = core::Max(1u, height);
		depth = core::Max(1u, depth);

		// work out total pixels.
		if (isDXT)
		{
			// scale to 4x4 pixel blocks.
			size += core::Max(bytes_per_block, ((width + 3) / 4) * ((height + 3) / 4) * bytes_per_block);
		}
		else
			size += ((bits_per_pixel * width) * height) / 8;

		// shift
		width >>= 1;
		height >>= 1;
		depth--;
	}

	return size;
}



bool XTexture::Load()
{
	bool bRes = this->LoadFromFile(this->FileName);

	if (!bRes) 
	{
		X_WARNING("Texture", "Failed to load: \"%s\"", this->FileName.c_str());
		flags.Set(TextureFlags::LOAD_FAILED);


		// can't do this since these are chnaged on reload.
		// we must actualy return the default texture object.
		// and add a refrence.
		// for hot reloading i'll probs make it check if it's default.
		// and if so just unrefrnce it.
#if 0
		// assing default.
		if(XTexture::s_DefaultTexturesLoaded)
		{
			XTexture* pDefault = XTexture::s_pTexDefault;

			this->DeviceTexture = pDefault->DeviceTexture;
			this->defaultTexStateId_ = pDefault->defaultTexStateId_;
			this->pDeviceShaderResource_ = pDefault->pDeviceShaderResource_;
			this->pDeviceRenderTargetView_ = pDefault->pDeviceRenderTargetView_;

			this->dimensions = pDefault->dimensions;
			this->datasize = pDefault->datasize;
			this->format = pDefault->format;
			this->numMips = pDefault->numMips;
			this->depth = pDefault->depth;
			this->numFaces = pDefault->numFaces;

		}
#endif
	}

	return bRes;
}


bool XTexture::LoadFromFile(const char* path_)
{
	int i;
	bool bRes;
	XTextureFile* image_data;
	core::Path path(path_);
	core::IFileSys::fileModeFlags mode;
	mode.Set(core::IFileSys::fileMode::READ);
	
	bRes = false;
	image_data = nullptr;

	path.toLower(); // lower case file names only.

	for (i = 0; i < 6; i++)
	{
		if (image_file_loaders[i]->canLoadFile(path))
		{
			core::XFile* file = gEnv->pFileSys->openFile(path.c_str(), mode);
			if (file)
			{
				image_data = image_file_loaders[i]->loadTexture(file);
				gEnv->pFileSys->closeFile(file);
			}
			break;
		}
	}

	if (image_data)
	{
		if (image_data->isValid())
		{
			bRes = createTexture(image_data);
		}

		if (!bRes) {
			X_DELETE( image_data, g_rendererArena);
		}
	}

	return bRes;
}


bool XTexture::createTexture(XTextureFile* image_data)
{
	if (image_data->getType() == TextureType::TCube)
	{
		if (image_data->getNumMips() != 1)
		{
			X_ERROR("Texture", "cubemaps with mips are not supported.");
			return false;
		}
	}


	this->numFaces = image_data->getNumFaces();
	this->numMips = image_data->getNumMips();
	this->depth = image_data->getDepth();
	this->format = image_data->getFormat();
	this->type = image_data->getType();
	this->dimensions = image_data->getSize();
	this->datasize = image_data->getDataSize();
	this->flags |= image_data->getFlags();

	preProcessImage(image_data);

	return createDeviceTexture(image_data);
}


void XTexture::preProcessImage(XTextureFile* image_data)
{
	// check if we need to change the format or make it power of 2.
	if (image_data->pFaces[0] == nullptr)
		return;

	// also build the sub info.
	int i, size;
	int Offset = 0;

	int width = getWidth();
	int height = getHeight();

	// might merge this when i support faces + mips.
	if (image_data->getType() == TextureType::TCube)
	{
		for (i = 0; i < image_data->getNumFaces(); i++)
		{
			XTextureFile::MipInfo& sub = image_data->SubInfo[i];

			size = get_data_size(width,height, 1, 1, getFormat());

			sub.pSysMem = &image_data->pFaces[i][Offset];
			sub.SysMemPitch = get_data_size(width, 1, 1, 1, getFormat());
			sub.SysMemSlicePitch = size;

		}
	}
	else
	{
		for (i = 0; i < image_data->getNumMips(); i++)
		{
			XTextureFile::MipInfo& sub = image_data->SubInfo[i];

			size = get_data_size(width,
				height, 1, 1, getFormat());


			sub.pSysMem = &image_data->pFaces[0][Offset];
			sub.SysMemPitch = get_data_size(width, 1, 1, 1, getFormat());
			sub.SysMemSlicePitch = size;

			Offset += size;

			width >>= 1;
			height >>= 1;
		}
	}

}



// Static

XTexture* XTexture::NewTexture(const char* name, const Vec2i& size, 
	TextureFlags Flags, Texturefmt::Enum fmt)
{
	X_ASSERT_NOT_NULL(s_pTextures);
	X_ASSERT_NOT_NULL(name);


	XTexture *pTex = NULL;

	pTex = (XTexture*)s_pTextures->findAsset(name);

	if (pTex)
	{
		pTex->addRef(); // add a ref.
	}
	else
	{
		// add a new texture.
		
		pTex = X_NEW_ALIGNED(XTexture, g_rendererArena, "Texture", X_ALIGN_OF(XTexture));
		pTex->depth = 1;
		pTex->dimensions = size;
		pTex->flags = Flags;
		pTex->FileName = name;
		pTex->format = fmt;

		// Add it.
		s_pTextures->AddAsset(name, pTex);
	}

	return pTex;
}

XTexture* XTexture::Create2DTexture(const char* name, const Vec2i& size, int numMips,
	TextureFlags Flags, byte* pData, Texturefmt::Enum textureFmt)
{
	X_ASSERT_NOT_NULL(name);
	X_ASSERT_NOT_NULL(pData);

	XTexture *pTex;

	pTex = XTexture::NewTexture(name, size, Flags, textureFmt);

	XTextureFile file;
	file.pFaces[0] = pData;
	file.depth = 1;
	file.numFaces = 1;
	file.format = textureFmt;
	file.numMips = numMips;
	file.flags = Flags;
	file.size = size;
	file.type = TextureType::T2D;
	file.datasize = get_data_size(size[0], size[1], 1, numMips, textureFmt);
	file.bDontDelete = true;

	if (file.isValid())
		pTex->createTexture(&file);

	return pTex;
}


XTexture* XTexture::CreateRenderTarget(const char* name, uint32_t width, uint32_t height,
	Texturefmt::Enum fmt, TextureType::Enum type)
{
	X_ASSERT_NOT_NULL(name);

	XTexture *pTex;
	TextureFlags Flags = TextureFlags::TEX_FONT | TextureFlags::DONT_STREAM | 
		TextureFlags::DONT_RESIZE | TextureFlags::RENDER_TARGET;

	Vec2i size(width, height);

	pTex = XTexture::NewTexture(name, size, Flags, fmt);
	X_ASSERT_NOT_NULL(pTex);

	XTextureFile file;
	file.pFaces[0] = nullptr;
	file.depth = 1;
	file.numFaces = 1;
	file.format = fmt;
	file.numMips = 1;
	file.flags = Flags;
	file.size = size;
	file.type = type;
	file.datasize = get_data_size(size[0], size[1], 1, 1, fmt);
	file.bDontDelete = true;

	if (file.isValid())
		pTex->createTexture(&file);
	else
	{
		X_ASSERT_UNREACHABLE();
	}

#if X_DEBUG
	// also add a debug name to the RTV
	ID3D11RenderTargetView* pRTV = pTex->getRenderTargetView();

	render::D3DDebug::SetDebugObjectName(pRTV, name);
#endif // !X_DEBUG

	return pTex;
}




XTexture* XTexture::FromName(const char* name, TextureFlags Flags)
{
	X_ASSERT_NOT_NULL(s_pTextures);
	X_ASSERT_NOT_NULL(name);

	// can we find it?
	// we store all image in a pool.
	// we make a hash of the name tho.
	XTexture* pTex = (XTexture*)s_pTextures->findAsset(name);

	if (pTex)
	{
		pTex->addRef(); // add a ref.
	}
	else
	{
		// add a new texture.
		pTex = X_NEW_ALIGNED(XTexture, g_rendererArena, "Texture", X_ALIGN_OF(XTexture));
		pTex->flags = Flags;
		pTex->FileName = name;
		// Add it.
		s_pTextures->AddAsset(name, pTex);

		if (!pTex->Load())
		{


			return XTexture::s_pTexDefault;
		}
	}

	return pTex;
}


XTexture* XTexture::getByID(TexID id)
{
	XTexture* pTex = (XTexture*)s_pTextures->findAsset(id);

	if (!pTex)
		return s_pTexDefault;

	return pTex;
}


int XTexture::getTexStateId(const shader::XTexState& TS)
{
	size_t i;
	size_t num = s_TexStates.size();

	for (i = 0; i<num; i++)
	{
		shader::XTexState *pTS = &s_TexStates[i];
		if (*pTS == TS)
			break;
	}

	if (i == s_TexStates.size())
	{
		s_TexStates.push_back(TS);
		s_TexStates[i].postCreate();
	}

	return safe_static_cast<int,size_t>(i);
}


void XTexture::init(void)
{
	s_pTextures = X_NEW_ALIGNED(render::XRenderResourceContainer, g_rendererArena, 
		"TexturesRes", X_ALIGN_OF(render::XRenderResourceContainer))(g_rendererArena, 4096);

	s_TexStates.setArena(g_rendererArena);
	s_TexStates.reserve(256);

//	s_pTextures->setArena(g_rendererArena, 4096);
	setDefaultFilterMode(shader::FilterMode::TRILINEAR);

	// Register reloaders.
	gEnv->pHotReload->addfileType(&g_ImgHotReload, "ci");
	gEnv->pHotReload->addfileType(&g_ImgHotReload, "dds");
	gEnv->pHotReload->addfileType(&g_ImgHotReload, "png");
	gEnv->pHotReload->addfileType(&g_ImgHotReload, "jpg");
	gEnv->pHotReload->addfileType(&g_ImgHotReload, "psd");
	gEnv->pHotReload->addfileType(&g_ImgHotReload, "tga");

	loadDefaultTextures();
}

void XTexture::shutDown(void)
{
	X_LOG0("Textures", "Shutting down");
	X_ASSERT_NOT_NULL(s_pTextures);


	gEnv->pHotReload->addfileType(nullptr, "ci");
	gEnv->pHotReload->addfileType(nullptr, "dds");
	gEnv->pHotReload->addfileType(nullptr, "png");
	gEnv->pHotReload->addfileType(nullptr, "jpg");
	gEnv->pHotReload->addfileType(nullptr, "psd");
	gEnv->pHotReload->addfileType(nullptr, "tga");

	releaseDefaultTextures();

	s_TexStates.free();

	// list any textures still lurking
	render::XRenderResourceContainer::ResourceItor it = s_pTextures->begin();
	for (; it != s_pTextures->end(); )
	{
		XTexture* pTex = (XTexture*)it->second;

		++it;

		if (!pTex)
			continue;

		X_WARNING("Texture", "\"%s\" was not deleted", pTex->getName());

		pTex->forceRelease();
	}

	X_DELETE_AND_NULL(s_pTextures, g_rendererArena);
}

void XTexture::update(void)
{
	X_PROFILE_BEGIN("TextureUpdate", core::ProfileSubSys::RENDER);


}

void XTexture::loadDefaultTextures(void)
{
	TextureFlags default_flags = TextureFlags::DONT_RESIZE | TextureFlags::DONT_STREAM;

	
	s_pTexDefault = XTexture::FromName(TEX_DEFAULT_DIFFUSE, default_flags);
	s_pTexDefaultBump = XTexture::FromName(TEX_DEFAULT_BUMP, default_flags);

	// these are required.
	if (!s_pTexDefault->isLoaded()) {
		X_FATAL("Texture", "failed to load default texture: %s", TEX_DEFAULT_DIFFUSE);
		return;
	}
	if (!s_pTexDefaultBump->isLoaded()) {
		X_FATAL("Texture", "failed to load default bump texture: %s", TEX_DEFAULT_BUMP);
		return;
	}

	s_ptexMipMapDebug = XTexture::FromName("core_assets/Textures/Debug/MipMapDebug.dds", default_flags | TextureFlags::FILTER_BILINEAR);
	s_ptexColorBlue = XTexture::FromName("core_assets/Textures/Debug/color_Blue.dds", default_flags);
	s_ptexColorCyan = XTexture::FromName("core_assets/Textures/Debug/color_Cyan.dds", default_flags);
	s_ptexColorGreen = XTexture::FromName("core_assets/Textures/Debug/color_Green.dds", default_flags);
	s_ptexColorPurple = XTexture::FromName("core_assets/Textures/Debug/color_Purple.dds", default_flags);
	s_ptexColorRed = XTexture::FromName("core_assets/Textures/Debug/color_Red.dds", default_flags);
	s_ptexColorWhite = XTexture::FromName("core_assets/Textures/Debug/color_White.dds", default_flags);
	s_ptexColorYellow = XTexture::FromName("core_assets/Textures/Debug/color_Yellow.dds", default_flags);
	s_ptexColorOrange = XTexture::FromName("core_assets/Textures/Debug/color_Orange.dds", default_flags);
	s_ptexColorMagenta = XTexture::FromName("core_assets/Textures/Debug/color_Magenta.dds", default_flags);


	Recti rect;
	render::gRenDev->GetViewport(rect);

	uint32_t width = rect.getWidth();
	uint32_t height = rect.getHeight();

	s_GBuf_Albedo = XTexture::CreateRenderTarget("$G_Albedo", width, height, Texturefmt::R8G8B8A8, TextureType::T2D);
	s_GBuf_Depth = XTexture::CreateRenderTarget("$G_Depth", width, height, Texturefmt::R16G16F, TextureType::T2D);
	s_GBuf_Normal = XTexture::CreateRenderTarget("$G_Normal", width, height, Texturefmt::R8G8B8A8, TextureType::T2D);
//	s_GBuf_Spec = XTexture::CreateRenderTarget("$G_spec", width, height, Texturefmt::R10G10B10A2, TextureType::T2D);


	s_DefaultTexturesLoaded = true;
}

void XTexture::releaseDefaultTextures(void)
{
	if (!s_DefaultTexturesLoaded) {
		X_WARNING("Texture", "failed to free default textures, they are not loaded.");
		return;
	}

	core::SafeRelease(s_pTexDefault);
	core::SafeRelease(s_pTexDefaultBump);

	core::SafeRelease(s_ptexMipMapDebug);
	core::SafeRelease(s_ptexColorBlue);
	core::SafeRelease(s_ptexColorCyan);
	core::SafeRelease(s_ptexColorGreen);
	core::SafeRelease(s_ptexColorPurple);
	core::SafeRelease(s_ptexColorRed);
	core::SafeRelease(s_ptexColorWhite);
	core::SafeRelease(s_ptexColorYellow);
	core::SafeRelease(s_ptexColorOrange);
	core::SafeRelease(s_ptexColorMagenta);

	core::SafeRelease(s_GBuf_Depth);
	core::SafeRelease(s_GBuf_Albedo);
	core::SafeRelease(s_GBuf_Normal);


	s_DefaultTexturesLoaded = false;
}


// ~Static

bool XTexture::setClampMode(shader::TextureAddressMode::Enum addressU,
	shader::TextureAddressMode::Enum addressV, 
	shader::TextureAddressMode::Enum addressW)
{
	return s_DefaultTexState.setClampMode(addressU, addressV, addressW);
}

bool XTexture::setFilterMode(shader::FilterMode::Enum filter)
{
	return s_DefaultTexState.setFilterMode(filter);
}


bool XTexture::reloadForName(const char* name)
{
	X_ASSERT_NOT_NULL(s_pTextures);
	X_ASSERT_NOT_NULL(name);

	// all asset names need forward slashes, for the hash.
	core::Path path(name);
	path.replaceAll('\\','/');

	XTexture* pTex = (XTexture*)s_pTextures->findAsset(path.c_str());

	if (pTex)
	{

		return pTex->reload();
	}

	X_WARNING("Texture", "Failed to find texture(%s) for reloading", name);
	return false;
}



bool XTexture::reload(void)
{
	X_LOG1("Texture", "reloading: \"%s\"", this->FileName.c_str());

	// how to reload it baby!
	// we shit on my tits, then load the new filedata.
	if (!Load())
		return false;

	return true; // such lies.
}





X_NAMESPACE_END

