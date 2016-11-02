#pragma once

#ifndef X_SHADER_I_H_
#define X_SHADER_I_H_


#include <Util\EnumMacros.h>
#include <String\StrRef.h>
#include <Math\XColor.h>

#include <Memory\NewAndDelete.h>

X_NAMESPACE_DECLARE(texture, class XTexture);

#include <ITexture.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{


static const uint32_t MAX_SHADERS = 256;
static const uint32_t MAX_HW_SHADERS = 256;
static const uint32_t MAX_SHADER_SOURCE = 256;


// we have shader params that will need to be updated at diffrent rates.
// so we should group them.
// 
// Some example shader parmas
//
//  genral matrix:
//		ModelMat
//		VeiwMat
//		ProjectionMat
// 
//  used for effects:
//		Time
//		Verte offset (for decals)

//  Deffered rendering:
//		width, height of viewport (used for calculating pos from depth)

/*
// GL = global
// PB = Per-Batch
// PI = Per-Instance
// SI = Per-Instance Static
// PF = Per-Frame
// PM = Per-Material
// SK = Skin data
// MP = Morph data
*/

struct ShaderParamFreq
{
	enum Enum
	{
		FRAME,
		INSTANCE
	};
};


// I support diffrent vertex formats
// for use by the engine, not so much assets.

// P = Position
// C = Color
// T = TexCord
// N = Normal
// TB = tanget & bi-normal
//
// Layout:
//         Type | Num | Format
// Eg: P3F -> Position 3 floats.

X_DECLARE_ENUM8(VertexFormat)(
	P3F_T3F, // used in Aux Geo.

			 // 16bit tex coords
	P3F_T2S,
	P3F_T2S_C4B,
	P3F_T2S_C4B_N3F,
	P3F_T2S_C4B_N3F_TB3F,

	// same as above but using compressed normals.
	P3F_T2S_C4B_N10,
	P3F_T2S_C4B_N10_TB10,

	// 32bit texcoords
	P3F_T2F_C4B,  // used by gui.

				  // double coords
	P3F_T4F_C4B_N3F
);


// -----------------------------------------------------------------------

X_DECLARE_ENUM(ShaderType)(UnKnown, Vertex, Pixel, Geometry, Hull, Domain);
X_DECLARE_ENUM(ShaderTextureIdx)(DIFFUSE, BUMP, SPEC);



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//
// These ILFlags are automatically detected whne parsing the hlsl source file.
// It looks for macros that start with IL_*(Normal, BiNormal, Color)
// 
// When it finds these flags it knows the source supports been compiled with AND without that flags.
// Which lets you define a shader that both works with and without Normal stream.
//
// The source will be compiled first with no flags defined which will be the minimum the shader requires as input.
// Then the shader works out what the required streams are for that compiled source.
//	
//  It then repeats this step each time adding a flag follwed by calculating the IL for that compiled source.	
//
//	Example:
//
//		if the hlsl contains:
//			#if IL_NORMAL
//			#if IL_BINORMAL
//
//
//		compile0:
//				flags: <none>
//				detectedIL:	POS_UV_COL
//
//		compile1:
//				flags: IL_NORMAL
//				detectedIL:	POS_UV_COL_NORM
//
//		compile2:
//				flags: IL_NORMAL, IL_BINORMAL
//				detectedIL:	POS_UV_COL_NORM_TAN_BI
//
//
//	the order the flags are define between each compile pass matches the order of the enum.
//	So you won't get: IL_BINORMAL defined before IL_NORMAL is defined, they accumulate in order.
//		
//	But it's totally possible to just have only IL_BINORMAL defined, if the shader requires normals but not binormals.
//


X_DECLARE_FLAGS(ILFlag)(Normal, BiNormal, Color);
typedef Flags<ILFlag> ILFlags;

X_DECLARE_ENUM(InputLayoutFormat)(
	Invalid,

	POS_UV,
	POS_UV_COL,
	POS_UV_COL_NORM,
	POS_UV_COL_NORM_TAN,
	POS_UV_COL_NORM_TAN_BI,

	POS_UV2_COL_NORM
);


extern InputLayoutFormat::Enum ILfromVertexFormat(const VertexFormat::Enum fmt);
extern ILFlags IlFlagsForVertexFormat(const VertexFormat::Enum fmt);


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//
//	These flags are to allow a shader to be compiled supporting difffrent features.
//
//	This means a shader can be select that supports skinning only
//	Or the same shader that supports skinning and instancing.
//	The flags are defined in the .shader file with the tech name currently.
//
//	Every permatation of the shader will be compiled:
//
//	"Fill(Textured)" -> compiled with: <none>, X_TEXTURED
//	"Fill(Color)" -> compiled with: <none>, X_COLOR
//	"Fill(Color, Textured)" -> compiled with: <none>, X_COLOR, X_TEXTURED, X_TEXTURED|X_COLOR
//
//
//


X_DECLARE_FLAGS(TechFlag)(
	Color, 
	Textured, 
	Skinned, 
	Instanced
);

typedef Flags<TechFlag> TechFlags;


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



struct TextureAddressMode
{
	enum Enum
	{
		WRAP,
		MIRROR,
		CLAMP,
		BORDER
	};
};

#define X_RENDER_ALLOW_ANISOTROPIC 1

struct FilterMode
{
	enum Enum
	{
		NONE = -1,
		POINT = 0,
		LINEAR = 1,
		BILINEAR,
		TRILINEAR,
#if X_RENDER_ALLOW_ANISOTROPIC
		ANISO2X,
		ANISO4X,
		ANISO8X,
		ANISO16X,
#endif // !X_RENDER_ALLOW_ANISOTROPIC
	};
};



struct XTexState
{
	XTexState() {
		core::zero_this(this);
	}

	XTexState(FilterMode::Enum filter, bool bClamp) {
		core::zero_this(this);

		TextureAddressMode::Enum Address = bClamp ?
			TextureAddressMode::CLAMP : TextureAddressMode::WRAP;

		setFilterMode(filter);
		setClampMode(Address, Address, Address);
	}
	XTexState(FilterMode::Enum filter,
		TextureAddressMode::Enum AddressU, 
		TextureAddressMode::Enum AddressV,
		TextureAddressMode::Enum AddressW, 
		Color8u borderColor)
	{
		core::zero_this(this);

		setFilterMode(filter);
		setClampMode(AddressU, AddressV, AddressW);
		setBorderColor(borderColor);
	}

	XTexState(const XTexState& src);
	~XTexState();

	X_INLINE XTexState& operator=(const XTexState& src)
	{
		this->~XTexState();
		core::Mem::Construct(this, src);
		return *this;
	}

	X_INLINE friend bool operator==(const XTexState &lhs, const XTexState &rhs)
	{
		return 
			*(uint64 *)&lhs == *(uint64 *)&rhs &&
			lhs.dwBorderColor_ == rhs.dwBorderColor_ &&
			lhs.bComparison_ == rhs.bComparison_ &&
			lhs.bSRGBLookup_ == rhs.bSRGBLookup_;
	}

	X_INLINE void Release(void) {
		delete this;
	}

	X_INLINE void* getDeviceState(void) {
		return pDeviceState_;
	}

	bool setFilterMode(FilterMode::Enum filter);
	bool setClampMode(TextureAddressMode::Enum addressU,
		TextureAddressMode::Enum addressV,
		TextureAddressMode::Enum addressW);
	void setBorderColor(Color8u color);
	void setComparisonFilter(bool bEnable);
	void postCreate(void);

private:

	void clearDevice(void);

	X_PUSH_WARNING_LEVEL(3)
	struct
	{
		signed char nMinFilter_ : 8;
		signed char nMagFilter_ : 8;
		signed char nMipFilter_ : 8;
		signed char nAddressU_ : 8;
		signed char nAddressV_ : 8;
		signed char nAddressW_ : 8;
		signed char nAnisotropy_ : 8;
		signed char padding_ : 8;
	};
	X_POP_WARNING_LEVEL

	Color8u dwBorderColor_;

	void* pDeviceState_;
	bool bActive_;
	bool bComparison_;
	bool bSRGBLookup_;
};



class XTextureResource
{
public:
	XTextureResource() : pTex(nullptr) {}

	~XTextureResource() {
		cleanUp();
	}

	X_INLINE void cleanUp(void) {	
	//	core::SafeRelease(pITex);
	}

	union {
		texture::XTexture* pTex;
		texture::ITexture* pITex;
	};

	core::string name;

	X_NO_COPY(XTextureResource);
	X_NO_ASSIGN(XTextureResource);
};


// textures + options
struct IRenderShaderResources
{
	virtual ~IRenderShaderResources() {}

	virtual void release(void) X_ABSTRACT;

	virtual Color& getDiffuseColor() X_ABSTRACT;
	virtual Color& getSpecularColor() X_ABSTRACT;
	virtual Color& getEmissiveColor() X_ABSTRACT;
	
	virtual float& getSpecularShininess() X_ABSTRACT;
	virtual float& getGlow() X_ABSTRACT;
	virtual float& getOpacity() X_ABSTRACT;

	virtual XTextureResource* getTexture(ShaderTextureIdx::Enum idx) const X_ABSTRACT;

};


struct XInputMaterial
{
	XInputMaterial() : 
		diffuse(Col_Black),
		specular(Col_Black), 
		emissive(Col_Black),
		specShininess(0.f)
	{}

public:
	Color diffuse;
	Color specular;
	Color emissive;
	float specShininess;
};

struct XInputShaderResources
{
	XInputShaderResources() {
		opacity = 1.0f;
	}

	XInputMaterial material;
	XTextureResource textures[ShaderTextureIdx::ENUM_COUNT];
	float opacity;

private:
	X_NO_COPY(XInputShaderResources);
	X_NO_ASSIGN(XInputShaderResources);
};



typedef int32_t ShaderID;

struct IShader
{
	virtual ~IShader() {}

//	virtual ShaderID getID(void) X_ABSTRACT;
//	virtual const int32_t addRef(void) X_ABSTRACT;
//	virtual const int32_t release(void) X_ABSTRACT;

	virtual const core::string& getName(void) const X_ABSTRACT;


	// tech stuff.

	
//	virtual VertexFormat::Enum getVertexFmt() X_ABSTRACT;
//	virtual ShaderType::Enum getType() X_ABSTRACT;

};


// a object that want's to rend has one of these.
// so the renderer know#s what shader / technique
// as well as the input resources. aka: textures.
struct XShaderItem
{
	XShaderItem() :
	pShader_(nullptr),
	pResources_(nullptr),
	technique_(0)
	{}

	// objects are ref counted so identical ones will point to the same memory.
	// meaning the standard compare operator will work fine
	// for checking if these are identical.

	IShader* pShader_;
	IRenderShaderResources* pResources_;
	int32_t	 technique_;
};

} // namespace shader

X_NAMESPACE_END

#endif // X_SHADER_I_H_