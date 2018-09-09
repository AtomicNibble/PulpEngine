#pragma once

#ifndef _X_MATERIAL_I_H_
#define _X_MATERIAL_I_H_

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif // !TRANSPARENT
#ifdef OPAQUE
#undef OPAQUE
#endif // !OPAQUE

#include <IAsyncLoad.h>
#include <IShader.h>
#include <IRender.h>
#include <IConverterModule.h>
#include "Util\GenericUtil.h"
#include <String\StringHash.h>

X_NAMESPACE_BEGIN(engine)

static const uint32_t MTL_MATERIAL_MAX_LEN = 64;
static const uint32_t MTL_B_VERSION = 5;
static const uint32_t MTL_B_FOURCC = X_TAG('m', 't', 'l', 'b');
static const char* MTL_B_FILE_EXTENSION = "mtlb";
static const char* MTL_DEFAULT_NAME = "default/$default";

// some limits for each material
static const uint32_t MTL_MAX_TECHS = 16;
static const uint32_t MTL_MAX_TEXTURES = 8;
static const uint32_t MTL_MAX_SAMPLERS = 8;
static const uint32_t MTL_MAX_PARAMS = 16;

static const uint32_t TECH_DEFS_MAX = 256;
static const char* TECH_DEFS_FILE_EXTENSION = "techsetdef";

static const uint32_t MTL_MAX_LOADED = 1 << 12;

static const float POLY_DECAL_OFFSET = 0.05f;
static const float POLY_WEAPON_IMPACT_OFFSET = 0.1f;

static const int16_t AUTO_TILING = -1;

struct IMaterialLib : public IConverter
{
};

X_DECLARE_ENUM(Register)
(
    CodeTexture0,
    CodeTexture1,
    CodeTexture2,
    CodeTexture3,
    Invalid);

struct RegisterCtx
{
    static_assert(Register::Invalid == Register::ENUM_COUNT - 1, "");

    typedef std::array<uint32_t, Register::ENUM_COUNT - 1> SlotArr;

    RegisterCtx()
    {
        core::zero_this(this);
    }

    SlotArr regs;
};

X_DECLARE_FLAGS(MaterialFlag)
(
    NODRAW,         // not visable
    EDITOR_VISABLE, // makes nodraw visable in editor modes.

    SOLID, // eye/view can't be in a solid

    STRUCTURAL, // collision, used to buold area's also.
    DETAIL,     // no collision

    PORTAL, // for creating render cells

    MOUNT, // can mount

    PLAYER_CLIP, // players can't go through this
    AI_CLIP,     // AI can't go throught this
    BULLET_CLIP,
    MISSLE_CLIP,
    VEHICLE_CLIP,

    NO_FALL_DMG,   // no dmg given on fall
    NO_IMPACT,     // impacts not shown
    NO_PENNETRATE, // bullets can't pass through.
    NO_STEPS,      // don't create footsteps.

    // might move these out later.
    UV_SCROLL,
    UV_ROTATE,
    UV_CLAMP_U,
    UV_CLAMP_V,

    DEFAULT // set if default material. (it will still be named as original material tho, so you can hotreload a material that failed to load initially)
);

typedef Flags<MaterialFlag> MaterialFlags;

X_DECLARE_FLAG_OPERATORS(MaterialFlags);

X_DECLARE_ENUM8(MaterialMountType)
(
    NONE,
    LADDER,
    MANTLEON,
    MANTLEOVER,
    CLIMBWALL,
    CLIMBPIPE);

// cat used to refine avaliable subtypes.
X_DECLARE_ENUM8(MaterialCat)
(
    GEO,
    DECAL,
    UI, // 2D
    TOOL,
    CODE,
    FILTERS,
    WEAPON,
    EFFECT,
    UNKNOWN);

X_DECLARE_ENUM8(MaterialCoverage)
(
    BAD,
    OPAQUE,
    PERFORATED,
    TRANSLUCENT);

// offset types.
X_DECLARE_ENUM8(MaterialPolygonOffset)
(
    NONE,
    STATIC_DECAL,
    WEAPON_IMPACT);

X_DECLARE_ENUM8(MaterialSurType)
(
    NONE,

    BRICK,
    CONCRETE,
    CLOTH,
    CARPET,
    CERAMIC,

    DIRT,

    FLESH,
    FOLIAGE,

    GLASS,
    GRASS,
    GRAVEL,

    ICE,

    METAL,
    METAL_THIN,
    METAL_HOLLOW,
    MUD,

    PLASTIC,
    PAPER,
    PLASTER,
    ROCK,
    RUBBER,

    SNOW,
    SAND,

    WOOD,
    WATER);

// used for grouping in editor etc
// not used at runtime.
X_DECLARE_ENUM8(MaterialUsage)
(
    NONE,
    TOOLS,
    CLIP,

    DOOR,
    FLOOR,
    CELING,
    ROOF,
    WALL_INTERIOR,
    WALL_EXTERIOR,
    TRIM_INTERIOR,
    TRIM_EXTERIOR,
    WINDOW,
    FOLIAGE,
    WATER,
    SKY,
    DECAL);

typedef render::shader::PermatationFlags PermatationFlags;

// the names of these are used directly as the define names (case sensitive)
X_DECLARE_ENUM(ParamType)
(
    Float1,
    Float2,
    Float4,
    Int,
    Bool,
    Color);

struct MaterialHeader
{
    // 4
    uint32_t fourCC;
    // 4
    uint8_t version;
    uint8_t numSamplers;
    uint16_t strDataSize;

    // 4
    uint8_t catTypeNameLen;
    uint8_t numParams;
    MaterialCat::Enum cat;
    MaterialUsage::Enum usage;

    // 4
    MaterialSurType::Enum surfaceType;
    MaterialCoverage::Enum coverage;
    MaterialMountType::Enum mountType;
    uint8_t numTextures;

    // 4
    MaterialFlags flags;

    // used for custom texture repeat.
    // if AUTO_TILING the textures dim's are used.
    Vec2<int16_t> tiling;
    Vec2<int16_t> atlas;

    // 12
    Color8u diffuse;
    Color8u specular;
    Color8u emissive;
    // 8
    float shineness;
    float opacity;

    X_INLINE bool isValid(void) const
    {
        if (version != MTL_B_VERSION) {
            X_ERROR("Mtl", "material version is invalid. FileVer: %i RequiredVer: %i",
                version, MTL_B_VERSION);
        }

        if (numSamplers > MTL_MAX_SAMPLERS || numTextures > MTL_MAX_TEXTURES || numParams > MTL_MAX_PARAMS) {
            X_ERROR("Mtl", "Error material exceeds limits");
            return false;
        }

        return version == MTL_B_VERSION && fourCC == MTL_B_FOURCC;
    }
};

struct MaterialTextureHdr
{
    uint8_t nameLen;
    render::FilterType::Enum filterType;
    render::TexRepeat::Enum texRepeat;
    uint8_t _pad;
};

X_ENSURE_SIZE(MaterialFlags, 4);
X_ENSURE_SIZE(MaterialCat::Enum, 1);
X_ENSURE_SIZE(MaterialCoverage::Enum, 1);
X_ENSURE_SIZE(MaterialPolygonOffset::Enum, 1);
X_ENSURE_SIZE(MaterialSurType::Enum, 1);
X_ENSURE_SIZE(MaterialUsage::Enum, 1);
X_ENSURE_SIZE(MaterialMountType::Enum, 1);

X_ENSURE_SIZE(MaterialHeader, 48);
X_ENSURE_SIZE(MaterialTextureHdr, 4);

class Material;
struct MaterialTech;

struct IMaterialManager : public core::IAssetLoader
{
    using core::IAssetLoader::waitForLoad;

    virtual ~IMaterialManager() = default;

    // returns null if not found, ref count unaffected
    virtual Material* findMaterial(const char* pMtlName) const X_ABSTRACT;
    // if material is found adds ref and returns, if not try's to load the material file.
    // if file can't be loaded or error it return the default material.
    virtual Material* loadMaterial(const char* pMtlName) X_ABSTRACT;

    virtual MaterialTech* getTechForMaterial(Material* pMat, core::StrHash hash, render::shader::VertexFormat::Enum vrtFmt,
        PermatationFlags permFlags) X_ABSTRACT;
    virtual bool setTextureID(Material* pMat, MaterialTech* pTech, core::StrHash texNameHash, texture::TexID id) X_ABSTRACT;
    virtual bool setRegisters(MaterialTech* pTech, const RegisterCtx& regs) X_ABSTRACT;

    virtual Material* getDefaultMaterial(void) const X_ABSTRACT;

    virtual bool waitForLoad(Material* pMaterial) X_ABSTRACT;
};

X_NAMESPACE_END

#endif // _X_MATERIAL_I_H_
