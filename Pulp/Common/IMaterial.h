#pragma once

#ifndef _X_MATERIAL_I_H_
#define _X_MATERIAL_I_H_

#include <IShader.h>
#include "Util\GenericUtil.h"

X_NAMESPACE_BEGIN(engine)


static const uint32_t	 MTL_MATERIAL_MAX_LEN = 64;
static const uint32_t	 MTL_B_VERSION = 1;
static const uint32_t	 MTL_B_FOURCC = X_TAG('m', 't', 'l', 'b');
static const char*		 MTL_B_FILE_EXTENSION = "mtlb";
static const char*		 MTL_FILE_EXTENSION = "mtl";


X_DECLARE_FLAGS(MaterialFlag)(TWO_SIDED, WIRE, ADDITIVE, NOSHADOW, NOLIGHTING, NODRAW, UI);

typedef Flags<MaterialFlag> MaterialFlags;

struct MaterialSurType
{
	enum Enum : uint8_t
	{
		NONE = 1,

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
		WATER,
	};
};

struct MaterialCullType
{
	enum Enum : uint8_t
	{
		FRONT_SIDED,
		BACK_SIDED,
		TWO_SIDED
	};
};

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

	virtual void setShaderItem(shader::XShaderItem& item) X_ABSTRACT;
	virtual shader::XShaderItem& getShaderItem(void) X_ABSTRACT;

	virtual bool isDefault() const X_ABSTRACT;

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

	Color diffuse;
	Color specular;
	Color emissive;
	float shineness;
	float opacity;

	bool isValid(void) const;
};


struct MaterialTexture
{
	shader::ShaderTextureIdx::Enum type;
	core::StackString<MTL_MATERIAL_MAX_LEN> name;

};



X_ENSURE_SIZE(MaterialCullType::Enum, 1);
X_ENSURE_SIZE(MaterialSurType::Enum, 1);
X_ENSURE_SIZE(MaterialHeader, 64);


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