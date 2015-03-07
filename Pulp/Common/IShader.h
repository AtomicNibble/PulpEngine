#pragma once

#ifndef X_SHADER_I_H_
#define X_SHADER_I_H_


#include <Util\EnumMacros.h>
#include <String\StrRef.h>
#include <Math\XColor.h>

X_NAMESPACE_DECLARE(texture, class XTexture);

#include <ITexture.h>

X_NAMESPACE_BEGIN(shader)

struct BlendType
{
	enum Enum
	{
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
	};

	static Enum typeFromStr(const char* _str);
};

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


X_DECLARE_ENUM(ShaderType)(UnKnown, Vertex, Pixel, Geometry);
X_DECLARE_ENUM(ShaderTextureIdx)(DIFFUSE, BUMP, SPEC);


// a shader can have flags
// Eg:
//	"Fill(Textured)"
//
//	this means we compile it with thoses flags.
//  
//	"Fill(Textured)" -> compiled with X_TEXTURED
//	"Fill(Color)" -> compiled with X_COLOR
//	"Fill(Color, Textured)" -> compiled with X_COLOR then X_TEXTURED
//
//	how can the render system make use of this?
//  what if a technique has flags that saying it supports Color, Textured
//
//



// I support diffrent vertex formats
// for use by the engine, not so much assets.
struct VertexFormat
{
	// P = Position
	// C = Color
	// T = TexCord
	// N = Normal
	// TB = tanget & bi-normal
	//
	// Layout:
	//         Type | Num | Format
	// Eg: P3F -> Position 3 floats.
	enum Enum
	{		
		P3F_T3F, // what is this used for?

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
		P3F_T4F_C4B_N3F,

		Num
	};

	// REMEBER: if you add new types that the input layout sytem 
	// will need to check for it's flag.
	static const char* toString(Enum type) {
		switch (type) {
			case P3F_T3F:
				return "P3F_T3F";

			case P3F_T2S:
				return "P3F_T2S";
			case P3F_T2S_C4B:
				return "P3F_T2S_C4B";
			case P3F_T2S_C4B_N3F:
				return "P3F_T2S_C4B_N3F";
			case P3F_T2S_C4B_N3F_TB3F:
				return "P3F_T2S_C4B_N3F_TB3F";

			case P3F_T2S_C4B_N10:
				return "P3F_T2S_C4B_N10";
			case P3F_T2S_C4B_N10_TB10:
				return "P3F_T2S_C4B_N10_TB10";

			case P3F_T2F_C4B:
				return "P3F_T2F_C4B";

			case P3F_T4F_C4B_N3F:
				return "P3F_T4F_C4B_N3F";

			case Num:
				return "";
#if X_DEBUG		
			default:
				X_ASSERT_UNREACHABLE();
#else
				X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
		}

		return "VertexFormat::Ukn";
	}
};



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
		new(this)XTexState(src);
		return *this;
	}

	X_INLINE friend bool operator==(const XTexState &lhs, const XTexState &rhs)
	{
		return 
			*(uint64 *)&lhs == *(uint64 *)&rhs &&
			lhs.m_dwBorderColor == rhs.m_dwBorderColor &&
			lhs.m_bComparison == rhs.m_bComparison &&
			lhs.m_bSRGBLookup == rhs.m_bSRGBLookup;
	}

	X_INLINE void Release() {
		delete this;
	}

	X_INLINE void* getDeviceState() {
		return m_pDeviceState;
	}

	bool setFilterMode(FilterMode::Enum filter);
	bool setClampMode(TextureAddressMode::Enum addressU,
		TextureAddressMode::Enum addressV,
		TextureAddressMode::Enum addressW);
	void setBorderColor(Color8u color);
	void setComparisonFilter(bool bEnable);
	void postCreate();

private:

	void clearDevice(void);

	X_PUSH_WARNING_LEVEL(3)
	struct
	{
		signed char m_nMinFilter : 8;
		signed char m_nMagFilter : 8;
		signed char m_nMipFilter : 8;
		signed char m_nAddressU : 8;
		signed char m_nAddressV : 8;
		signed char m_nAddressW : 8;
		signed char m_nAnisotropy : 8;
		signed char padding : 8;
	};
	X_POP_WARNING_LEVEL

	Color8u m_dwBorderColor;

	void* m_pDeviceState;
	bool m_bActive;
	bool m_bComparison;
	bool m_bSRGBLookup;
};



class XTextureResource
{
public:
	XTextureResource() : pTex(nullptr) {}

	~XTextureResource() {
		cleanUp();
	}

	X_INLINE void cleanUp(void) {	
		core::SafeRelease(pITex);
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

	virtual ShaderID getID() X_ABSTRACT;
	virtual const int addRef() X_ABSTRACT;
	virtual const int release() X_ABSTRACT;


	virtual const char* getName() const X_ABSTRACT;
	virtual VertexFormat::Enum getVertexFmt() X_ABSTRACT;
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


X_NAMESPACE_END

#endif // X_SHADER_I_H_