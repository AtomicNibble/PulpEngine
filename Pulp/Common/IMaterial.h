#pragma once

#ifndef _X_MATERIAL_I_H_
#define _X_MATERIAL_I_H_

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




struct IMaterialLib : public IConverter
{

};



X_DECLARE_FLAGS(MtlXmlFlags)(NAME, FLAGS, SURFACETYPE, COVERAGE);


X_DECLARE_FLAGS(MaterialFlag)(
	NODRAW,			// 1 not visable
	EDITOR_VISABLE, // 2 makes nodraw visable in editor modes.

	SOLID,			// 4 eye/view can't be in a solid

	STRUCTURAL,		// 8 collision, used to buold area's also.
	DETAIL,			// 16 no collision

	PORTAL,			// 32 for creating render cells

	PLAYER_CLIP,	// 64 players can't go through this
	AI_CLIP,		// 128 AI can't go throught this

	NO_FALL_DMG,	// 256 no dmg given on fall
	NO_IMPACT,		// 512 impacts not shown
	NO_PENNETRATE	// 1024 bullets can't pass through.
);

// the type of material it is, changes nothing really currently.
X_DECLARE_ENUM8(MaterialType)(
	UI,
	WORLD,
	MODEL,
	TOOL,
	UNKNOWN
);

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif // !TRANSPARENT
#ifdef OPAQUE
#undef OPAQUE
#endif // !OPAQUE

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
	LINEAR,
	BILINEAR_X2,
	BILINEAR_X4,
	TRILINEAR_X2,
	TRILINEAR_X4
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

	FLESH,

	GLASS,
	GRASS,
	GRAVEL,

	ICE,

	METAL,
	MUD,

	PLASTIC,
	PAPER,
	ROCK,

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
	WINDOW,
	SKY
);



X_DECLARE_ENUM8(MaterialCullType)(
	FRONT_SIDED,
	BACK_SIDED,
	TWO_SIDED
);

typedef Flags<MaterialFlag> MaterialFlags;



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

	virtual MaterialType::Enum getType(void) const X_ABSTRACT;
	virtual void setType(MaterialType::Enum type) X_ABSTRACT;

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



struct MaterialHeader
{
	// 4
	uint32 fourCC;
	// 4
	uint8 version;
	uint8 numTextures;
	MaterialCullType::Enum cullType;
	MaterialSurType::Enum type;
	// 4
	MaterialTexRepeat::Enum texRepeat;
	MaterialPolygonOffset::Enum polyOffsetType;
	MaterialFilterType::Enum filterType;
	MaterialType::Enum matType;
	// 1
	MaterialCoverage::Enum coverage;
	uint8_t _pad[3];

	Color diffuse;
	Color specular;
	Color emissive;
	float shineness;
	float opacity;

	bool isValid(void) const;
};


struct MaterialTexture
{
	render::shader::ShaderTextureIdx::Enum type;
	core::StackString<MTL_MATERIAL_MAX_LEN> name;

};



X_ENSURE_SIZE(MaterialCullType::Enum, 1);
X_ENSURE_SIZE(MaterialSurType::Enum, 1);

X_ENSURE_SIZE(MaterialTexRepeat::Enum, 1);
X_ENSURE_SIZE(MaterialPolygonOffset::Enum, 1);
X_ENSURE_SIZE(MaterialFilterType::Enum, 1);
X_ENSURE_SIZE(MaterialType::Enum, 1);
X_ENSURE_SIZE(MaterialCoverage::Enum, 1);

X_ENSURE_SIZE(MaterialHeader, 72);


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