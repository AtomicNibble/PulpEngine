#include "stdafx.h"
#include "XTexture.h"

#include "Hashing\Fnva1Hash.h"
#include "Containers\HashMap.h"
#include "Util\ReferenceCountedOwner.h"

#include "IFileSys.h"

#include "../XRender.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(texture)

// temp just so they link.
namespace
{
    CI::XTexLoaderCI ci;
    DDS::XTexLoaderDDS dds;
    JPG::XTexLoaderJPG jpg;
    PNG::XTexLoaderPNG png;
    PSD::XTexLoaderPSD psd;
    TGA::XTexLoaderTGA tga;

    static_assert(ImgFileFormat::ENUM_COUNT == 7, "Added additional img src fmts? this code needs updating.");

    static ITextureFmt* image_file_loaders[] = {
        &ci,
        &dds,
        &jpg,
        &png,
        &psd,
        &tga};

    struct eqstr
    {
        bool operator()(const char* s1, const char* s2) const
        {
            return core::strUtil::IsEqual(s1, s2);
        }
    };

    struct strhash
    {
        size_t operator()(const char* s1) const
        {
            return core::Hash::Fnv1aHash(s1, core::strUtil::strlen(s1));
        }
    };

    class ImgHotReload : public core::IXHotReload
    {
        void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE
        {
            X_UNUSED(jobSys);
            X_UNUSED(name);
            //	return XTexture::reloadForName(name);
        }
    };

    ImgHotReload g_ImgHotReload;
} // namespace

int XTexture::s_Var_SaveToCI = 0;

render::XRenderResourceContainer* XTexture::s_pTextures = nullptr;

bool XTexture::s_DefaultTexturesLoaded = false;

int XTexture::s_GlobalDefaultTexStateId = -1;
shader::XTexState XTexture::s_GlobalDefaultTexState;
shader::XTexState XTexture::s_DefaultTexState;
core::Array<shader::XTexState> XTexture::s_TexStates(nullptr);

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
    this->ReleaseDeviceTexture();
}

const int XTexture::release()
{
    int ref = XBaseAsset::release();
    if (ref == 0) {
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
    switch (fmt) {
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

        default:
            X_NO_SWITCH_DEFAULT_ASSERT;
    }
    return DXGI_FORMAT_UNKNOWN;
}

bool XTexture::Load()
{
    bool bRes = this->LoadFromFile(this->FileName);

    if (!bRes) {
        X_WARNING("Texture", "Failed to load: \"%s\"", this->FileName.c_str());
        flags.Set(TextureFlags::LOAD_FAILED);
    }
    else {
        flags.Remove(TextureFlags::LOAD_FAILED);
    }

    return bRes;
}

bool XTexture::LoadFromFile(const char* path_)
{
    int i;
    bool bRes;
    XTextureFile* image_data;
    core::Path<char> path(path_);
    core::IFileSys::fileModeFlags mode;
    mode.Set(core::IFileSys::fileMode::READ);

    bRes = false;
    image_data = nullptr;

    path.toLower(); // lower case file names only.

    for (i = 0; i < 6; i++) {
        if (image_file_loaders[i]->canLoadFile(path)) {
            core::XFile* file = gEnv->pFileSys->openFile(path.c_str(), mode);
            if (file) {
                //	image_data = image_file_loaders[i]->loadTexture(file);
                gEnv->pFileSys->closeFile(file);
            }
            break;
        }
    }

    if (image_data) {
        bRes = createTexture(image_data);
    }

    return bRes;
}

bool XTexture::createTexture(XTextureFile* pImagedata)
{
#if 1
    X_UNUSED(pImagedata);
    return false;
#else

    core::ReferenceCountedOwner<XTextureFile> image_data(pImagedata, g_textureDataArena);
    bool res = false;
    {
        if (!image_data->isValid()) {
            return false;
        }

        if (image_data->getType() == TextureType::TCube) {
            if (image_data->getNumMips() != 1) {
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

        res = createDeviceTexture(image_data);
    }
    return res;
#endif
}

void XTexture::preProcessImage(core::ReferenceCountedOwner<XTextureFile>& image_data)
{
#if 1
    X_UNUSED(image_data);
    X_ASSERT_NOT_IMPLEMENTED();
#else
    // check if we need to change the format or make it power of 2.
    if (image_data->pFaces[0] == nullptr)
        return;

    // also build the sub info.
    int i, size;
    int Offset = 0;

    int width = getWidth();
    int height = getHeight();

    // might merge this when i support faces + mips.
    if (image_data->getType() == TextureType::TCube) {
        for (i = 0; i < image_data->getNumFaces(); i++) {
            XTextureFile::MipInfo& sub = image_data->SubInfo[i];

            size = texture::Util::dataSize(width, height, 1, 1, getFormat());

            sub.pSysMem = &image_data->pFaces[i][Offset];
            sub.SysMemPitch = get_data_size(width, 1, 1, 1, getFormat());
            sub.SysMemSlicePitch = size;
        }
    }
    else {
        for (i = 0; i < image_data->getNumMips(); i++) {
            XTextureFile::MipInfo& sub = image_data->SubInfo[i];

            size = texture::Util::dataSize(width,
                height, 1, 1, getFormat());

            sub.pSysMem = &image_data->pFaces[0][Offset];
            sub.SysMemPitch = get_data_size(width, 1, 1, 1, getFormat());
            sub.SysMemSlicePitch = size;

            Offset += size;

            width >>= 1;
            height >>= 1;
        }
    }

    if (s_Var_SaveToCI == 1 && !image_data->getFlags().IsSet(TexFlag::CI_IMG)) {
        X_LOG0("Texture", "Compiling image to CI: ^5%s", this->FileName.c_str());

        core::Path<char> outPath;
        outPath = "compiled_images/";
        outPath /= FileName.c_str();

        X_ASSERT_NOT_IMPLEMENTED();
        //	CI::WriteCIImgAsync(outPath, image_data, g_textureDataArena);
    }
#endif
}

// Static

XTexture* XTexture::NewTexture(const char* name, const Vec2i& size,
    TextureFlags Flags, Texturefmt::Enum fmt)
{
    X_ASSERT_NOT_NULL(s_pTextures);
    X_ASSERT_NOT_NULL(name);

    XTexture* pTex = nullptr;

    pTex = static_cast<XTexture*>(s_pTextures->findAsset(name));

    if (pTex) {
        pTex->addRef(); // add a ref.
    }
    else {
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

XTexture* XTexture::Create2DTexture(const char* name, const Vec2i& size, size_t numMips,
    TextureFlags Flags, byte* pData, Texturefmt::Enum textureFmt)
{
    X_ASSERT_NOT_NULL(name);
    X_ASSERT_NOT_NULL(pData);

    XTexture* pTex;

    pTex = XTexture::NewTexture(name, size, Flags, textureFmt);

#if 1
    X_UNUSED(numMips);
    X_UNUSED(Flags);
    X_UNUSED(pData);
#else
    XTextureFile file;
    file.pFaces[0] = pData;
    file.depth_ = 1;
    file.numFaces_ = 1;
    file.format_ = textureFmt;
    file.numMips_ = safe_static_cast<uint8_t, size_t>(numMips);
    file.flags_ = Flags;
    file.size_ = size;
    file.type_ = TextureType::T2D;
    file.datasize_ = texture::Util::dataSize(size[0], size[1], 1,
        safe_static_cast<uint32_t, size_t>(numMips), textureFmt);
    file.bDontDelete_ = true;
    file.addReference();

    if (file.isValid()) {
        pTex->createTexture(&file);
    }
#endif

    return pTex;
}

XTexture* XTexture::CreateRenderTarget(const char* name, uint32_t width, uint32_t height,
    Texturefmt::Enum fmt, TextureType::Enum type)
{
    X_ASSERT_NOT_NULL(name);

    XTexture* pTex;
    TextureFlags Flags = TextureFlags::TEX_FONT | TextureFlags::DONT_STREAM | TextureFlags::DONT_RESIZE | TextureFlags::RENDER_TARGET;

    Vec2i size(width, height);

    pTex = XTexture::NewTexture(name, size, Flags, fmt);
    X_ASSERT_NOT_NULL(pTex);

#if 1
    X_UNUSED(type);
#else
    XTextureFile file(g_textureDataArena);
    file.pFaces[0] = nullptr;
    file.depth_ = 1;
    file.numFaces_ = 1;
    file.format_ = fmt;
    file.numMips_ = 1;
    file.flags_ = Flags;
    file.size_ = size;
    file.type_ = type;
    file.datasize_ = texture::Util::dataSize(size[0], size[1], 1, 1, fmt);
    file.bDontDelete_ = true;
    file.addReference();

    if (file.isValid()) {
        pTex->createTexture(&file);
    }
    else {
        X_ASSERT_UNREACHABLE();
    }
#endif

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
    XTexture* pTex = static_cast<XTexture*>(s_pTextures->findAsset(name));

    if (pTex) {
        pTex->addRef(); // add a ref.
    }
    else {
        // add a new texture.
        pTex = X_NEW_ALIGNED(XTexture, g_rendererArena, "Texture", X_ALIGN_OF(XTexture));
        pTex->flags = Flags;
        pTex->FileName = name;
        // Add it.
        s_pTextures->AddAsset(name, pTex);

        if (!pTex->Load()) {
            //	i now return the correct texture object, so when loading fails.
            //  a valid hot reload will correctly update the broken ones.
            //  the use of default texture is now done and bind stage.
            //	return XTexture::s_pTexDefault;
        }
    }

    return pTex;
}

XTexture* XTexture::getByID(TexID id)
{
    XTexture* pTex = static_cast<XTexture*>(s_pTextures->findAsset(id));

    if (!pTex)
        return s_pTexDefault;

    return pTex;
}

int XTexture::getTexStateId(const shader::XTexState& TS)
{
    size_t i;
    size_t num = s_TexStates.size();

    for (i = 0; i < num; i++) {
        shader::XTexState* pTS = &s_TexStates[i];
        if (*pTS == TS)
            break;
    }

    if (i == s_TexStates.size()) {
        s_TexStates.push_back(TS);
        s_TexStates[i].postCreate();
    }

    return safe_static_cast<int, size_t>(i);
}

void XTexture::applyDefault(void)
{
    s_pTexDefault->apply(0);
    s_pTexDefaultBump->apply(1);
}

void XTexture::init(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pConsole);

    ADD_CVAR_REF("image_autoConvert", s_Var_SaveToCI, 0, 0, 1, core::VarFlag::SYSTEM,
        "Save unconverted images as ci automatically");

    ADD_COMMAND("imageReloadAll", Command_ReloadTextures, core::VarFlag::SYSTEM,
        "Reload all textures");
    ADD_COMMAND("imageReload", Command_ReloadTexture, core::VarFlag::SYSTEM,
        "Reload a textures <name>");

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
    X_LOG0("Textures", "Shutting Down");
    X_LOG_BULLET;

    gEnv->pHotReload->addfileType(nullptr, "ci");
    gEnv->pHotReload->addfileType(nullptr, "dds");
    gEnv->pHotReload->addfileType(nullptr, "png");
    gEnv->pHotReload->addfileType(nullptr, "jpg");
    gEnv->pHotReload->addfileType(nullptr, "psd");
    gEnv->pHotReload->addfileType(nullptr, "tga");

    releaseDefaultTextures();

    s_TexStates.free();

    // either we did not full start up or there is a issue.
    X_ASSERT_NOT_NULL(s_pTextures);

    if (s_pTextures) {
        // list any textures still lurking
        render::XRenderResourceContainer::ResourceItor it = s_pTextures->begin();
        for (; it != s_pTextures->end();) {
            XTexture* pTex = static_cast<XTexture*>(it->second);

            ++it;

            if (!pTex)
                continue;

            X_WARNING("Texture", "\"%s\" was not deleted", pTex->getName());

            pTex->forceRelease();
        }

        X_DELETE_AND_NULL(s_pTextures, g_rendererArena);
    }
}

void XTexture::update(void)
{
    X_PROFILE_BEGIN("TextureUpdate", core::profiler::SubSys::RENDER);
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

    s_ptexMipMapDebug = XTexture::FromName("Textures/Debug/MipMapDebug", default_flags | TextureFlags::FILTER_BILINEAR);
    s_ptexColorBlue = XTexture::FromName("Textures/Debug/color_Blue", default_flags);
    s_ptexColorCyan = XTexture::FromName("Textures/Debug/color_Cyan", default_flags);
    s_ptexColorGreen = XTexture::FromName("Textures/Debug/color_Green", default_flags);
    s_ptexColorPurple = XTexture::FromName("Textures/Debug/color_Purple", default_flags);
    s_ptexColorRed = XTexture::FromName("Textures/Debug/color_Red", default_flags);
    s_ptexColorWhite = XTexture::FromName("Textures/Debug/color_White", default_flags);
    s_ptexColorYellow = XTexture::FromName("Textures/Debug/color_Yellow", default_flags);
    s_ptexColorOrange = XTexture::FromName("Textures/Debug/color_Orange", default_flags);
    s_ptexColorMagenta = XTexture::FromName("Textures/Debug/color_Magenta", default_flags);

    Recti rect;
    render::gRenDev->GetViewport(rect);

    uint32_t width = rect.getWidth();
    uint32_t height = rect.getHeight();

    s_GBuf_Albedo = XTexture::CreateRenderTarget("$G_Albedo", width, height, Texturefmt::R8G8B8A8, TextureType::T2D);
    s_GBuf_Depth = XTexture::CreateRenderTarget("$G_Depth", width, height, Texturefmt::R16G16_FLOAT, TextureType::T2D);
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
    core::Path<char> path(name);
    path.replaceAll('\\', '/');

    if (core::strUtil::IsEqualCaseInsen(path.extension(), ".ci")) {
        return true;
    }

    XTexture* pTex = static_cast<XTexture*>(s_pTextures->findAsset(path.c_str()));

    if (pTex) {
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

void XTexture::Command_ReloadTextures(core::IConsoleCmdArgs* Cmd)
{
    X_UNUSED(Cmd);
    // TODO
}

void XTexture::Command_ReloadTexture(core::IConsoleCmdArgs* Cmd)
{
    if (Cmd->GetArgCount() < 2) {
        X_ERROR("Texture", "imageReload <filename>");
        return;
    }

    const char* pName = Cmd->GetArg(1);

    XTexture::reloadForName(pName);
}

X_NAMESPACE_END
