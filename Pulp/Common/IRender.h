#pragma once

#ifndef _X_RENDER_I_H_
#define _X_RENDER_I_H_

#include <ITexture.h>
#include <IShader.h>

#include <Containers\Array.h>

#include <Util\Flags.h>
#include <Math\VertexFormats.h>


typedef void* WIN_HWND;


X_NAMESPACE_DECLARE(font,
	struct IXFont_RenderProxy;
	struct XTextDrawConect;
)

X_NAMESPACE_DECLARE(model,
	struct MeshHeader;
)

X_NAMESPACE_DECLARE(render,
	struct IRenderAux;

	template<typename>
	class CommandBucket;

	namespace shader {
		struct IShader;

		class XCBuffer;
		class ShaderPermatation;
	} // namespace shader
)


X_NAMESPACE_BEGIN(render)

static const uint32_t MAX_RENDER_TARGETS = 8;
static const uint32_t MAX_CONST_BUFFERS_BOUND = 8;
static const uint32_t MAX_TEXTURES_BOUND = 8;

typedef uintptr_t Handle;
typedef Handle VertexBufferHandle;
typedef Handle IndexBufferHandle;
typedef Handle ConstantBufferHandle;
typedef Handle PassStateHandle;
typedef Handle StateHandle;

typedef std::array<VertexBufferHandle, VertexStream::ENUM_COUNT> VertexBufferHandleArr;

static const Handle INVALID_BUF_HANLDE = 0;
static const Handle INVALID_STATE_HANLDE = 0;


X_DECLARE_ENUM8(CullType)(
	FRONT_SIDED,
	BACK_SIDED,
	NONE
);

X_DECLARE_ENUM8(TopoType)(
	TRIANGLELIST,
	TRIANGLESTRIP,
	LINELIST,
	LINESTRIP,
	POINTLIST
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

struct StencilDesc
{
	StencilFunc::Enum stencilFunc;
	StencilOperation::Enum failOp;
	StencilOperation::Enum zFailOp;
	StencilOperation::Enum passOp;
};

struct StencilState
{
	StencilDesc front;
	StencilDesc back;
};

X_ENSURE_SIZE(StencilDesc, 4);
X_ENSURE_SIZE(StencilState, 8);


X_DECLARE_ENUM8(DepthFunc)(
//	NEVER, // just don't enable depth test?
	LEQUAL,
	EQUAL,
	GREAT,
	LESS,
	GEQUAL,
	NOTEQUAL,
	ALWAYS
);


X_DECLARE_ENUM8(BlendType)(
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

X_DECLARE_ENUM8(BlendOp)(
	OP_ADD,
	OP_SUB,
	OP_REB_SUB,
	OP_MIN,
	OP_MAX,
	DISABLE // disables blend 
);

X_DECLARE_FLAGS8(WriteMask)(
	RED,
	GREEN,
	BLUE,
	ALPHA
);

typedef Flags8<WriteMask> WriteMaskFlags;

struct BlendState
{
	BlendType::Enum srcBlendColor;
	BlendType::Enum dstBlendColor;
	BlendType::Enum srcBlendAlpha;
	BlendType::Enum dstBlendAlpha;
	BlendOp::Enum colorOp;
	BlendOp::Enum alphaOp;
	WriteMaskFlags writeMask;
};

X_ENSURE_SIZE(BlendState, 7);


X_DECLARE_ENUM8(TextureSlot)(
	DIFFUSE,
	NORMAL,
	DETAIL,
	SPECCOL,
	CAMO,
	OCCLUSION,
	OPACITY,
	ROUGTHNESS
);

static_assert(MAX_TEXTURES_BOUND >= TextureSlot::ENUM_COUNT, "Slots bound must be greater or equal to num slots");

// LINEAR is linera with none mip sampling aka LINEAR_MIP_NONE
// BILINEAR is linera with point mip sampling aka LINEAR_MIP_NEAREST
// TRILINEAR is linera with linear mip sampling aka LINEAR_MIP_LINEAR
X_DECLARE_ENUM8(FilterType)(
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

X_DECLARE_ENUM8(TexRepeat)(
	NO_TILE,
	TILE_BOTH,
	TILE_HOZ,
	TILE_VERT
);

struct SamplerState
{
	FilterType::Enum filter;
	TexRepeat::Enum repeat;
};

X_ENSURE_SIZE(SamplerState, 2);

// do we want to explicitly group these?
struct TextureState
{
	// 4
	texture::TexID textureId; 
	// 2
	SamplerState sampler;
	// do i want to pass this?
	// or try be fancy and pass arrays in TextureSlot index order.
	TextureSlot::Enum slot; 
	uint8_t _pad[1];
};

#if X_64
X_ENSURE_SIZE(TextureState, 8);
#else
X_ENSURE_SIZE(TextureState, 8);
#endif

X_DECLARE_FLAGS8(StateFlag)(
	DEPTHWRITE,
	WIREFRAME,

	NO_DEPTH_TEST,
	STENCIL,
	BLEND,
	ALPHATEST
);

typedef Flags8<StateFlag> StateFlags;

struct StateDesc
{
	// 8
	StencilState stencil;
	// 7
	BlendState blend;
	// 1
	shader::VertexFormat::Enum vertexFmt;

	// 4
	CullType::Enum cullType;
	TopoType::Enum topo;
	DepthFunc::Enum depthFunc;
	StateFlags stateFlags;

};

X_ENSURE_SIZE(StateDesc, 20);


struct CBState
{
	ConstantBufferHandle cb;
	shader::ShaderStageFlags visibility;
};

typedef core::FixedArray<texture::Texturefmt::Enum, MAX_RENDER_TARGETS> RenderTargetFmtsArr;


struct IRenderTarget
{
	virtual ~IRenderTarget() {};


	virtual texture::Texturefmt::Enum getFmt(void) X_ABSTRACT;
};

X_DECLARE_ENUM(AuxRenderer)(MISC, PHYSICS);
X_DECLARE_FLAGS(CpuAccess)(WRITE, READ);
X_DECLARE_ENUM(BufUsage)(
	IMMUTABLE,  // never changes
	STATIC,	    // changes sometimes not every frame.
	DYNAMIC		// stuff that changes every frame
);

typedef Flags<CpuAccess> CpuAccessFlags;


struct IRender
{
	// physics has it's own Aux render que so to speak, other que's can be added.
	// they are not thread safe, but it's fine to populate diffrent aux instances in diffrent threads.
	virtual ~IRender() {};

	virtual bool init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height) X_ABSTRACT;
	virtual void shutDown(void) X_ABSTRACT;
	virtual void freeResources(void) X_ABSTRACT;

	virtual void release(void) X_ABSTRACT;

	virtual void registerVars(void) X_ABSTRACT;
	virtual void registerCmds(void) X_ABSTRACT;

	virtual void renderBegin(void) X_ABSTRACT;
	virtual void renderEnd(void) X_ABSTRACT;

	virtual void submitCommandPackets(CommandBucket<uint32_t>& cmdBucket) X_ABSTRACT;

	// each enum has a instance, and you don't own the pointer.
	virtual IRenderAux* getAuxRender(AuxRenderer::Enum user) X_ABSTRACT;

	// display res is stored as int, if you want float cast this.
	virtual Vec2<uint32_t> getDisplayRes(void) const X_ABSTRACT;

	// should the render system not know about the render targets?
	// and just do as it's told?
	// how will we handle buffer resize?

	virtual IRenderTarget* createRenderTarget(void) X_ABSTRACT;
	virtual void destoryRenderTarget(IRenderTarget* pRT) X_ABSTRACT;
	virtual IRenderTarget* getCurBackBuffer(uint32_t* pIdx = nullptr) X_ABSTRACT;

	virtual VertexBufferHandle createVertexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag = 0) X_ABSTRACT;
	virtual VertexBufferHandle createVertexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag = 0) X_ABSTRACT;
	virtual IndexBufferHandle createIndexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag = 0) X_ABSTRACT;
	virtual IndexBufferHandle createIndexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag = 0) X_ABSTRACT;

	virtual void destoryVertexBuffer(VertexBufferHandle handle) X_ABSTRACT;
	virtual void destoryIndexBuffer(IndexBufferHandle handle) X_ABSTRACT;

	virtual void getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_ABSTRACT;
	virtual void getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_ABSTRACT;

	// cb's
	virtual ConstantBufferHandle createConstBuffer(shader::XCBuffer& cbuffer, BufUsage::Enum usage) X_ABSTRACT;
	virtual void destoryConstBuffer(ConstantBufferHandle handle) X_ABSTRACT;


	virtual texture::ITexture* getTexture(const char* pName, texture::TextureFlags flags) X_ABSTRACT;
	// creates a texture for dynamic content, no data loaded from disk.
	virtual texture::ITexture* createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, const uint8_t* pInitialData = nullptr) X_ABSTRACT;
	
	// shaders
	// new api for creating techs in 3dengine
	virtual shader::IShaderSource* getShaderSource(const char* pName) X_ABSTRACT;
	virtual shader::IHWShader* createHWShader(shader::ShaderType::Enum type, const core::string& entry, shader::IShaderSource* pSourceFile) X_ABSTRACT;
	virtual shader::IShaderPermatation* createPermatation(shader::IHWShader* pVertex, shader::IHWShader* pPixel) X_ABSTRACT;
	virtual shader::IShaderPermatation* createPermatation(const shader::ShaderStagesArr& stages) X_ABSTRACT;


	virtual void releaseTexture(texture::ITexture* pTex) X_ABSTRACT;

	// state
	virtual PassStateHandle createPassState(const RenderTargetFmtsArr& rtfs) X_ABSTRACT;
	virtual void destoryPassState(PassStateHandle handle) X_ABSTRACT;

	virtual StateHandle createState(PassStateHandle passHandle, const shader::IShaderPermatation* pPerm, const StateDesc& state, const TextureState* pTextStates, size_t numStates) X_ABSTRACT;
	virtual void destoryState(StateHandle handle) X_ABSTRACT;

	// =============================================
	// ============== OLD API ======================
	// =============================================

	// AuxGeo
	virtual IRenderAux* GetIRenderAuxGeo(void) X_ABSTRACT;
	// ~AuxGeo
};


X_NAMESPACE_END

#endif // !_X_RENDER_I_H_
