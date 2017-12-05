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
	struct TextDrawContext;
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

static const bool DEPTH_REVERSE_Z = true; // setting this to false 'should' be enougth to turn reverse z on/off
// make these small as possible, reducdes pool size.
static const uint32_t MAX_RENDER_TARGETS = 8;
static const uint32_t MAX_CONST_BUFFERS_BOUND = 8;
static const uint32_t MAX_TEXTURES_BOUND = 8;
static const uint32_t MAX_BUFFERS_BOUND = 8;

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
};

#if X_64
X_ENSURE_SIZE(TextureState, 4);
#else
X_ENSURE_SIZE(TextureState, 4);
#endif

X_DECLARE_FLAGS(StateFlag)(
	DEPTHWRITE,
	WIREFRAME,

	NO_DEPTH_TEST,
	STENCIL,
	BLEND,
	ALPHATEST,

	VERTEX_STREAMS, // vertex format is made from streams
	INSTANCED_POS_COLOR, // a stream of postions and colors is provided.
	HWSKIN // a stream of index and weights is provided.
);

typedef Flags<StateFlag> StateFlags;

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
	uint8_t _goat;

	// 4
	StateFlags stateFlags;
};

X_ENSURE_SIZE(StateDesc, 24);


struct CBState
{
	ConstantBufferHandle cb;
	shader::ShaderStageFlags visibility;
};

struct BufferState
{
	
	VertexBufferHandle buf;
};

typedef core::FixedArray<texture::Texturefmt::Enum, MAX_RENDER_TARGETS> RenderTargetFmtsArr;


X_DECLARE_ENUM8(PixelBufferType)(
	NONE, // don't use.
	DEPTH,
	COLOR, // can be used for render targets
	SHADOW
);

typedef int32_t TexID;

struct IDeviceTexture
{
	virtual ~IDeviceTexture() {};

	// the resource id.
	virtual const TexID getTexID(void) const X_ABSTRACT;

	virtual const core::string& getName(void) const X_ABSTRACT;
	virtual const Vec2<uint16_t> getDimensions(void) const X_ABSTRACT;
	virtual const int getWidth(void) const X_ABSTRACT;
	virtual const int getHeight(void) const X_ABSTRACT;
	virtual const int getNumFaces(void) const X_ABSTRACT;
	virtual const int getNumMips(void) const X_ABSTRACT;
	virtual const int getDepth(void) const X_ABSTRACT;
	virtual const int getDataSize(void) const X_ABSTRACT;
	virtual const bool isLoaded(void) const X_ABSTRACT;

	virtual const texture::TextureType::Enum getTextureType(void) const X_ABSTRACT;
	virtual const texture::TextureFlags getFlags(void) const X_ABSTRACT;
	virtual const texture::Texturefmt::Enum getFormat(void) const X_ABSTRACT;

	virtual void setProperties(const texture::XTextureFile& imgFile) X_ABSTRACT;
};


struct IPixelBuffer : public IDeviceTexture
{
	virtual ~IPixelBuffer() {};

	virtual PixelBufferType::Enum getBufferType(void) const X_ABSTRACT;
};


typedef IPixelBuffer IRenderTarget;
typedef IPixelBuffer IDepthBuffer;

struct Stats
{
	typedef core::StackString512 Str;

	X_INLINE Stats() {
		core::zero_this(this);
	}

	X_INLINE const Str::char_type* toString(Str& buf) const {

		core::HumanSize::Str str;

		buf.clear();
		buf.appendFmt("NumPassState: ^6%" PRIi32 "^~\n", numPassStates);
		buf.appendFmt("NumState: ^6%" PRIi32 "^~\n", numStates);
		return buf.c_str();
	}

	int32_t numPassStates;
	int32_t maxPassStates;

	int32_t numStates;
	int32_t maxStates;
};

X_DECLARE_FLAGS(CpuAccess)(WRITE, READ);
X_DECLARE_ENUM(BufUsage)(
	IMMUTABLE,		// never changes
	DYNAMIC,	    // changes sometimes not every frame.
	PER_FRAME		// stuff that changes every frame
);

typedef Flags<CpuAccess> CpuAccessFlags;


struct IRender
{
	// physics has it's own Aux render que so to speak, other que's can be added.
	// they are not thread safe, but it's fine to populate diffrent aux instances in diffrent threads.
	virtual ~IRender() {};

	virtual void registerVars(void) X_ABSTRACT;
	virtual void registerCmds(void) X_ABSTRACT;

	virtual bool init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height, texture::Texturefmt::Enum depthFmt, bool reverseZ) X_ABSTRACT;
	virtual void shutDown(void) X_ABSTRACT;
	virtual void freeResources(void) X_ABSTRACT;
	virtual void release(void) X_ABSTRACT;


	virtual void renderBegin(void) X_ABSTRACT;
	virtual void renderEnd(void) X_ABSTRACT;

	virtual void submitCommandPackets(CommandBucket<uint32_t>& cmdBucket) X_ABSTRACT;

	// display res is stored as int, if you want float cast this.
	virtual Vec2<uint32_t> getDisplayRes(void) const X_ABSTRACT;

	// this is used for creating buffers for rendering to.
	// for example creating depth buffers, or shadow map buffers that can then be bound to pipeline.
	// they also support been passed to shaders, so you can take the texture id of a pixel buffer and place it in variable state.
	// this allows you todo things like populate a depth buffer then read it from a shader (SRV)
	virtual IPixelBuffer* createDepthBuffer(const char* pNickName, Vec2i dim) X_ABSTRACT;
	virtual IPixelBuffer* createColorBuffer(const char* pNickName, Vec2i dim, uint32_t numMips, texture::Texturefmt::Enum fmt) X_ABSTRACT;

	virtual IRenderTarget* getCurBackBuffer(uint32_t* pIdx = nullptr) X_ABSTRACT;

	virtual VertexBufferHandle createVertexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag = CpuAccessFlags()) X_ABSTRACT;
	virtual VertexBufferHandle createVertexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag = CpuAccessFlags()) X_ABSTRACT;
	virtual IndexBufferHandle createIndexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag = CpuAccessFlags()) X_ABSTRACT;
	virtual IndexBufferHandle createIndexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag = CpuAccessFlags()) X_ABSTRACT;


	// cb's
	virtual ConstantBufferHandle createConstBuffer(const shader::XCBuffer& cbuffer, BufUsage::Enum usage) X_ABSTRACT;

	// this creates a texture for a given textureFile, the cpu data is only uploaded if requested.
	virtual IDeviceTexture* getDeviceTexture(int32_t id) X_ABSTRACT;
	virtual bool initDeviceTexture(IDeviceTexture* pTex) X_ABSTRACT;
	virtual bool initDeviceTexture(IDeviceTexture* pTex, const texture::XTextureFile& imgFile) X_ABSTRACT;

	// creates a texture for dynamic content, no data loaded from disk.
	virtual IDeviceTexture* createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, BufUsage::Enum usage, const uint8_t* pInitialData = nullptr) X_ABSTRACT;
	
	// shaders
	// new api for creating techs in 3dengine
	virtual shader::IShaderSource* getShaderSource(const core::string& name) X_ABSTRACT;
	virtual shader::IHWShader* createHWShader(shader::ShaderType::Enum type, const core::string& entry, const core::string& customDefines,
		shader::IShaderSource* pSourceFile, shader::PermatationFlags permFlags, render::shader::VertexFormat::Enum vertFmt) X_ABSTRACT;
	virtual shader::IShaderPermatation* createPermatation(const shader::ShaderStagesArr& stages) X_ABSTRACT;

	virtual PassStateHandle createPassState(const RenderTargetFmtsArr& rtfs) X_ABSTRACT;
	virtual StateHandle createState(PassStateHandle passHandle, const shader::IShaderPermatation* pPerm, const StateDesc& state, const TextureState* pTextStates, size_t numStates) X_ABSTRACT;

	// Will relesse all the HWShaders in the perm for you.
	virtual void releaseShaderPermatation(shader::IShaderPermatation* pPerm) X_ABSTRACT;
	virtual void releaseTexture(IDeviceTexture* pTex) X_ABSTRACT;
	virtual void releasePixelBuffer(render::IPixelBuffer* pPixelBuf) X_ABSTRACT;
	virtual void destoryPassState(PassStateHandle handle) X_ABSTRACT;
	virtual void destoryState(StateHandle handle) X_ABSTRACT;

	virtual void destoryVertexBuffer(VertexBufferHandle handle) X_ABSTRACT;
	virtual void destoryIndexBuffer(IndexBufferHandle handle) X_ABSTRACT;
	virtual void destoryConstBuffer(ConstantBufferHandle handle) X_ABSTRACT;

	virtual void getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_ABSTRACT;
	virtual void getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_ABSTRACT;

	virtual Stats getStats(void) const X_ABSTRACT;
};


X_NAMESPACE_END

#endif // !_X_RENDER_I_H_
