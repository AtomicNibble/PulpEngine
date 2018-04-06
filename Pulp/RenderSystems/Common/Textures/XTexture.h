#pragma once

#ifndef _X_TEXTURE_H_
#define _X_TEXTURE_H_

#include <Util\PointerUtil.h>
#include <Util\ReferenceCountedOwner.h>
#include <Threading\AtomicInt.h>
#include "String\StrRef.h"

#include "DeviceManager\DeviceManager.h"
#include <../Common/Resources/BaseRenderAsset.h>

#include <ITexture.h>
#include <IShader.h>

X_NAMESPACE_DECLARE(core,
                    struct IConsoleCmdArgs;)

X_NAMESPACE_BEGIN(texture)

class XTextureFile;

static const size_t TEX_MAX_SLOTS = 8;

X_DISABLE_WARNING(4324)
class X_ALIGNED_SYMBOL(XTexture, 64) :
    public ITexture,
    public core::XBaseAsset
{
public:
    virtual ~XTexture();

public:
    XTexture();

    static void init(void);
    static void shutDown(void);
    static void update(void);
    static void loadDefaultTextures(void);
    static void releaseDefaultTextures(void);

    X_INLINE const int addRef() X_OVERRIDE
    {
        return XBaseAsset::addRef();
    }
    X_INLINE const int release() X_OVERRIDE;
    X_INLINE const int forceRelease() X_OVERRIDE
    {
        for (;;) {
            if (release() <= 0)
                break;
        }
        return 0;
    }

    // the resource id.
    X_INLINE const TexID getTexID() const X_OVERRIDE
    {
        return XBaseAsset::getID();
    };

    X_INLINE const char* getName() const X_OVERRIDE
    {
        return FileName.c_str();
    }
    X_INLINE const Vec2<uint16_t> getDimensions() const X_OVERRIDE
    {
        return dimensions;
    }
    X_INLINE const int getWidth() const X_OVERRIDE
    {
        return dimensions.x;
    }
    X_INLINE const int getHeight() const X_OVERRIDE
    {
        return dimensions.y;
    }
    X_INLINE const int getNumFaces() const X_OVERRIDE
    {
        return numFaces;
    }
    X_INLINE const int getDepth() const X_OVERRIDE
    {
        return depth;
    }
    X_INLINE const int getNumMips() const X_OVERRIDE
    {
        return static_cast<int>(numMips);
    }
    X_INLINE const int getDataSize() const X_OVERRIDE
    {
        return 0;
    } // TODO

    X_INLINE const bool isLoaded() const X_OVERRIDE
    {
        return !flags.IsSet(TextureFlags::LOAD_FAILED);
    }
    X_INLINE const bool IsShared() const X_OVERRIDE
    {
        return false;
    }
    X_INLINE const bool IsStreamable() const X_OVERRIDE
    {
        return false;
    }

    X_INLINE const TextureType::Enum getTextureType() const X_OVERRIDE
    {
        return type;
    }
    X_INLINE const TextureFlags getFlags() const X_OVERRIDE
    {
        return flags;
    }
    X_INLINE const Texturefmt::Enum getFormat() const X_OVERRIDE
    {
        return format;
    }

    static DXGI_FORMAT DCGIFormatFromTexFmt(Texturefmt::Enum fmt);

    static XTexture* FromName(const char* name, TextureFlags Flags);
    static XTexture* getByID(TexID tex_id);

    static int getTexStateId(const shader::XTexState& TS);
    static bool setDefaultFilterMode(shader::FilterMode::Enum filter);

    static void applyDefault(void);

    static void applyFromId(int texUnit, TexID tex_id, int state_id)
    {
        XTexture* pTex = getByID(tex_id);
        X_ASSERT_NOT_NULL(pTex);

        pTex->apply(texUnit, state_id);
    }

    static bool reloadForName(const char* name);
    bool reload(void);

    bool Load();
    bool LoadFromFile(const char* path);
    bool createTexture(XTextureFile* image_data);
    void preProcessImage(core::ReferenceCountedOwner<XTextureFile>& image_data);

public:
    // Defined in the Render specific code.
    bool createDeviceTexture(core::ReferenceCountedOwner<XTextureFile>& image_data);
    bool RT_CreateDeviceTexture(XTextureFile* image_data);
    bool RT_ReleaseDevice(void);
    bool ReleaseDeviceTexture(void);

    ID3D11RenderTargetView* getRenderTargetView(void);

    void unbind(void);
    void apply(int tex_sampler_id, int state_id = -1,
        shader::ShaderType::Enum type = shader::ShaderType::Pixel);

    void updateTextureRegion(byte* data, int nX, int nY,
        int USize, int VSize, Texturefmt::Enum srcFmt);

    void RT_UpdateTextureRegion(byte* data, int nX, int nY,
        int USize, int VSize, Texturefmt::Enum srcFmt);

    // -------------
    void setTexStates();

    void setSamplerState(int tex_sampler_id, int state_id,
        shader::ShaderType::Enum shader_type);

    static XTexture* NewTexture(const char* name, const Vec2i& size, TextureFlags Flags,
        Texturefmt::Enum fmt);

    static XTexture* Create2DTexture(const char* name, const Vec2i& size, size_t numMips, TextureFlags Flags,
        byte* pData, Texturefmt::Enum fmt);

    static XTexture* CreateRenderTarget(const char* name, uint32_t width, uint32_t height,
        Texturefmt::Enum fmt, TextureType::Enum type);

    bool setClampMode(shader::TextureAddressMode::Enum addressU,
        shader::TextureAddressMode::Enum addressV,
        shader::TextureAddressMode::Enum addressW);

    bool setFilterMode(shader::FilterMode::Enum filter);

private:
    static void Command_ReloadTextures(core::IConsoleCmdArgs* Cmd);
    static void Command_ReloadTexture(core::IConsoleCmdArgs* Cmd);

private:
    static int s_Var_SaveToCI;

    static render::XRenderResourceContainer* s_pTextures;

    static bool s_DefaultTexturesLoaded;

    static int s_GlobalDefaultTexStateId;
    static shader::XTexState s_GlobalDefaultTexState;
    static shader::XTexState s_DefaultTexState;
    static core::Array<shader::XTexState> s_TexStates;

    static XTexture* s_pTexDefault;
    static XTexture* s_pTexDefaultBump;

    static XTexture* s_ptexMipMapDebug;
    static XTexture* s_ptexColorBlue;
    static XTexture* s_ptexColorCyan;
    static XTexture* s_ptexColorGreen;
    static XTexture* s_ptexColorPurple;
    static XTexture* s_ptexColorRed;
    static XTexture* s_ptexColorWhite;
    static XTexture* s_ptexColorYellow;
    static XTexture* s_ptexColorMagenta;
    static XTexture* s_ptexColorOrange;

    static XTexture* s_pCurrentTexture[TEX_MAX_SLOTS];

public:
    static XTexture* s_GBuf_Depth;
    static XTexture* s_GBuf_Albedo;
    static XTexture* s_GBuf_Normal;
    static XTexture* s_GBuf_Spec;

private:
    // original name.
    core::string FileName;

    XDeviceTexture DeviceTexture;

    Vec2<uint16_t> dimensions;
    uint32_t datasize; // size of the higest mip in bytes.
    TextureType::Enum type;
    TextureFlags flags;
    Texturefmt::Enum format;
    uint8_t numMips;
    uint8_t depth;
    uint8_t numFaces;

    int defaultTexStateId_;

    void* pDeviceShaderResource_;
    void* pDeviceRenderTargetView_;
};

X_ENABLE_WARNING(4324)
// X_ENSURE_SIZE(XTexture, 64);

X_NAMESPACE_END

#endif // !_X_TEXTURE_H_
