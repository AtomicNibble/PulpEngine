#pragma once

#ifndef _X_MATERIAL_I_H_
#define _X_MATERIAL_I_H_

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif // !TRANSPARENT
#ifdef OPAQUE
#undef OPAQUE
#endif // !OPAQUE

#include <IShader.h>
#include <IConverterModule.h>
#include "Util\GenericUtil.h"

X_NAMESPACE_BEGIN(engine)


static const uint32_t	 MTL_MATERIAL_MAX_LEN = 64;
static const uint32_t	 MTL_B_VERSION = 3;
static const uint32_t	 MTL_B_FOURCC = X_TAG('m', 't', 'l', 'b');
static const char*		 MTL_B_FILE_EXTENSION = "mtlb";
static const char*		 MTL_FILE_EXTENSION = "mtl";
static const char*		 MTL_DEFAULT_NAME = "default";


static const float POLY_DECAL_OFFSET = 0.05f;
static const float POLY_WEAPON_IMPACT_OFFSET = 0.1f;

static const int16_t AUTO_TILING = -1;

struct IMaterialLib : public IConverter
{

};


X_DECLARE_FLAGS(MtlXmlFlags)(NAME, FLAGS, SURFACETYPE, COVERAGE);


X_DECLARE_FLAGS(MaterialFlag)(
	NODRAW,			// not visable
	EDITOR_VISABLE, // makes nodraw visable in editor modes.

	SOLID,			// eye/view can't be in a solid

	STRUCTURAL,		// collision, used to buold area's also.
	DETAIL,			// no collision

	PORTAL,			// for creating render cells

	MOUNT,			// can mount

	PLAYER_CLIP,	// players can't go through this
	AI_CLIP,		// AI can't go throught this
	BULLET_CLIP,
	MISSLE_CLIP,
	VEHICLE_CLIP,

	NO_FALL_DMG,	// no dmg given on fall
	NO_IMPACT,		// impacts not shown
	NO_PENNETRATE,	// bullets can't pass through.
	NO_STEPS,		// don't create footsteps.

	LOAD_FAILED
);




X_DECLARE_ENUM8(MaterialMountType)(
	NONE,
	LADDER,
	MANTLEON,
	MANTLEOVER,
	CLIMBWALL,
	CLIMBPIPE
);


typedef Flags<MaterialFlag> MaterialFlags;

X_DECLARE_FLAGS8(MaterialStateFlag)(
	DEPTHWRITE,
	WIREFRAME,

	// dunno if i want to keep these here.
	UV_SCROLL,
	UV_ROTATE,
	UV_CLAMP_U,
	UV_CLAMP_V
);

typedef Flags8<MaterialStateFlag> MaterialStateFlags;


// cat used to refine avaliable subtypes.
X_DECLARE_ENUM8(MaterialCat)(
	GEO,
	DECAL,
	UI,
	TOOL,
	CODE,
	FILTERS,
	WEAPON,
	EFFECT,
	UNKNOWN
);

X_DECLARE_ENUM8(MaterialCoverage)(
	BAD,
	OPAQUE,
	PERFORATED,
	TRANSLUCENT
);

// offset types.
X_DECLARE_ENUM8(MaterialPolygonOffset)(
	NONE,
	STATIC_DECAL,
	WEAPON_IMPACT
);

X_DECLARE_ENUM8(MaterialFilterType)(
	NEAREST_MIP_NONE,
	NEAREST_MIP_NEAREST,
	NEAREST_MIP_LINEAR,

	LINEAR_MIP_NONE,
	LINEAR_MIP_NEAREST,
	LINEAR_MIP_LINEAR,

	ANISOTROPIC_X2,
	ANISOTROPIC_X4,
	ANISOTROPIC_X8,
	ANISOTROPIC_X16
);

X_DECLARE_ENUM8(MaterialTexRepeat)(
	NO_TILE,
	TILE_BOTH,
	TILE_HOZ,
	TILE_VERT
);

X_DECLARE_ENUM8(MaterialSurType)(
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
	WATER
);

// used for grouping in editor etc
// not used at runtime.
X_DECLARE_ENUM8(MaterialUsage)(
	NONE,

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
	DECAL
);



X_DECLARE_ENUM8(MaterialCullType)(
	FRONT_SIDED,
	BACK_SIDED,
	TWO_SIDED
);

X_DECLARE_ENUM8(MaterialBlendType)(
	ZERO,
	ONE,

	SRC_COLOR,
	SRC_ALPHA,
	SRC_ALPHA_SAT,
	SRC1_COLOR,
	SRC1_ALPHA,

	INV_SRC_COLOR,
	INV_SRC_ALPHA,

	INV_SRC1_COLOR,
	INV_SRC1_ALPHA,

	DEST_COLOR,
	DEST_ALPHA,

	INV_DEST_COLOR,
	INV_DEST_ALPHA,

	BLEND_FACTOR,
	INV_BLEND_FACTOR,

	INVALID
);

X_DECLARE_ENUM8(StencilOperation)(
	KEEP, 
	ZERO, 
	REPLACE, 
	INCR_SAT, 
	DECR_SAT, 
	INVERT, 
	INCR, 
	DECR
);

X_DECLARE_ENUM8(StencilFunc)(
	NEVER, 
	LESS, 
	EQUAL, 
	LESS_EQUAL, 
	GREATER,
	NOT_EQUAL, 
	GREATER_EQUAL, 
	ALWAYS
);



/*
what o do for the shader input system that i need todo for my engine.
ineed to also add in the engine system that dose the culling and deteriming what ld to render for each model
that means i should prbos add in amodel to be rendered into the 3d engine and then 
it decdes what lod to render.
*/


struct IMaterial
{
	virtual ~IMaterial(){};


	// materials are shared, we ref count them so we know when we are done.
	virtual const int addRef() X_ABSTRACT;
	virtual const int release() X_ABSTRACT;
	virtual const int forceRelease() X_ABSTRACT;

	virtual const char* getName() const X_ABSTRACT;
	virtual void setName(const char* pName) X_ABSTRACT;

	virtual const MaterialFlags getFlags() const X_ABSTRACT;
	virtual void setFlags(const MaterialFlags flags) X_ABSTRACT;

	virtual MaterialSurType::Enum getSurfaceType() const X_ABSTRACT;
	virtual void setSurfaceType(MaterialSurType::Enum type) X_ABSTRACT;

	virtual MaterialCullType::Enum getCullType() const X_ABSTRACT;
	virtual void setCullType(MaterialCullType::Enum type) X_ABSTRACT;

	virtual MaterialTexRepeat::Enum getTexRepeat(void) const X_ABSTRACT;
	virtual void setTexRepeat(MaterialTexRepeat::Enum texRepeat) X_ABSTRACT;

	virtual MaterialPolygonOffset::Enum getPolyOffsetType(void) const X_ABSTRACT;
	virtual void setPolyOffsetType(MaterialPolygonOffset::Enum polyOffsetType) X_ABSTRACT;

	virtual MaterialFilterType::Enum getFilterType(void) const X_ABSTRACT;
	virtual void setFilterType(MaterialFilterType::Enum filterType) X_ABSTRACT;

//	virtual MaterialType::Enum getType(void) const X_ABSTRACT;
//	virtual void setType(MaterialType::Enum type) X_ABSTRACT;

	virtual MaterialCoverage::Enum getCoverage(void) const X_ABSTRACT;
	virtual void setCoverage(MaterialCoverage::Enum coverage) X_ABSTRACT;

//	virtual void setShaderItem(render::shader::XShaderItem& item) X_ABSTRACT;
//	virtual render::shader::XShaderItem& getShaderItem(void) X_ABSTRACT;

	virtual bool isDefault() const X_ABSTRACT;

	// util.
	X_INLINE bool isDrawn(void) const {
		return !getFlags().IsSet(MaterialFlag::NODRAW);
	}
};


X_DECLARE_FLAGS8(MaterialTexFlags)(
	DIFFUSE,
	NORMAL,
	DETAIL,
	SPECCOL
);

struct MaterialHeader
{
	// 4
	uint32_t fourCC;
	// 4
	uint8_t version;
	uint8_t numTextures;
	MaterialCat::Enum cat;
	MaterialSurType::Enum surfaceType;

	// 4
	MaterialUsage::Enum usage;
	MaterialCullType::Enum cullType;
	MaterialPolygonOffset::Enum polyOffsetType;
	MaterialCoverage::Enum coverage;

	// 4: blend ops.
	MaterialBlendType::Enum srcBlendColor;
	MaterialBlendType::Enum dstBlendColor;
	MaterialBlendType::Enum srcBlendAlpha;
	MaterialBlendType::Enum dstBlendAlpha;

	// 4
	MaterialMountType::Enum mountType;
	StencilFunc::Enum depthTest;
	MaterialStateFlags stateFlags;
	bool _pad;

	// 4
	MaterialFlags flags;

	// used for custom texture repeat.
	// if AUTO_TILING the textures dim's are used.
	Vec2<int16_t> tiling;

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

		return version == MTL_B_VERSION &&
			fourCC == MTL_B_FOURCC;
	}

};


struct MaterialTexture
{
	uint8_t nameLen;
	MaterialFilterType::Enum filterType;
	MaterialTexRepeat::Enum texRepeat;
	uint8_t _pad;
};


X_ENSURE_SIZE(MaterialCat::Enum, 1);
X_ENSURE_SIZE(MaterialCoverage::Enum, 1);
X_ENSURE_SIZE(MaterialPolygonOffset::Enum, 1);
X_ENSURE_SIZE(MaterialFilterType::Enum, 1);
X_ENSURE_SIZE(MaterialTexRepeat::Enum, 1);
X_ENSURE_SIZE(MaterialSurType::Enum, 1);
X_ENSURE_SIZE(MaterialUsage::Enum, 1);
X_ENSURE_SIZE(MaterialCullType::Enum, 1);
X_ENSURE_SIZE(MaterialMountType::Enum, 1);
X_ENSURE_SIZE(MaterialBlendType::Enum, 1);
X_ENSURE_SIZE(StencilOperation::Enum, 1);
X_ENSURE_SIZE(StencilFunc::Enum, 1);
X_ENSURE_SIZE(MaterialStateFlag::Bits, 1);
X_ENSURE_SIZE(MaterialStateFlags, 1);


X_ENSURE_SIZE(MaterialHeader, 48);
X_ENSURE_SIZE(MaterialTexture, 4);


struct IMaterialManagerListener
{
	virtual ~IMaterialManagerListener(){}
	virtual  IMaterial* OnLoadMaterial(const char* MtlName) X_ABSTRACT;
	virtual  IMaterial* OnCreateMaterial(IMaterial* pMat) X_ABSTRACT;
	virtual  IMaterial* OnDeleteMaterial(IMaterial* pMat) X_ABSTRACT;
};

struct IMaterialManager
{
	virtual ~IMaterialManager(){}

	// if mat of this name exsists returns and adds refrence
	// dose not load anything.
	virtual IMaterial* createMaterial(const char* MtlName) X_ABSTRACT;
	// returns null if not found, ref count unaffected
	virtual IMaterial* findMaterial(const char* MtlName) const X_ABSTRACT;
	// if material is found adds ref and returns, if not try's to load the material file.
	// if file can't be loaded or error it return the default material.
	virtual IMaterial* loadMaterial(const char* MtlName) X_ABSTRACT;

	virtual IMaterial* getDefaultMaterial() X_ABSTRACT;

	virtual void setListener(IMaterialManagerListener* pListner) X_ABSTRACT;
};

X_NAMESPACE_END

#endif // _X_MATERIAL_I_H_