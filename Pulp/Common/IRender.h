#pragma once

#ifndef _X_RENDER_I_H_
#define _X_RENDER_I_H_

#include <ITexture.h>
#include <IShader.h>
#include <IRenderCommands.h>

#include <Containers\Array.h>

#include <Util\Flags.h>

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
	} // namespace shader
)


X_NAMESPACE_BEGIN(render)

static const uint32_t MAX_RENDER_TARGETS = 8;


X_DECLARE_ENUM8(CullType)(
	FRONT_SIDED,
	BACK_SIDED,
	TWO_SIDED
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
	LEQUAL,
	EQUAL,
	GREAT,
	LESS,
	GEQUAL,
	NOTEQUAL
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


struct BlendState
{
	BlendType::Enum srcBlendColor;
	BlendType::Enum dstBlendColor;
	BlendType::Enum srcBlendAlpha;
	BlendType::Enum dstBlendAlpha;
};

X_ENSURE_SIZE(BlendState, 4);


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
	texture::ITexture* pTexture; // i will make this 16bit texture id making this whoel thing 4bytes.
	SamplerState sampler;
	// do i want to pass this?
	// or try be fancy and pass arrays in TextureSlot index order.
	TextureSlot::Enum slot; 
	uint8_t _pad[2];
};

#if X_64
X_ENSURE_SIZE(TextureState, 16);
#else
X_ENSURE_SIZE(TextureState, 12);
#endif

X_DECLARE_FLAGS8(StateNewFlag)(
	DEPTHWRITE,
	WIREFRAME,

	NO_DEPTH_TEST,
	STENCIL
);

typedef Flags8<StateNewFlag> StateNewFlags;

struct StateDesc
{
	StencilState stencil;
	BlendState blend;
	// 4
	CullType::Enum cullType;
	TopoType::Enum topo;
	DepthFunc::Enum depthFunc;
	StateNewFlags state;

	shader::VertexFormat::Enum vertexFmt;
	bool _pad[3];
};


X_ENSURE_SIZE(StateDesc, 20);


typedef core::FixedArray<texture::Texturefmt::Enum, MAX_RENDER_TARGETS> RenderTargetFmtsArr;


// ==================== OLD =========================================

X_DECLARE_FLAGS(DrawTextFlags)(CENTER, RIGHT, CENTER_VER, MONOSPACE, POS_2D, FIXED_SIZE, FRAMED);

struct XDrawTextInfo
{
	Flags<DrawTextFlags> flags;
	Color col;
};


struct States
{
	static const unsigned int FLAGS_COUNT = 8;

	enum Enum
	{
		BLEND_SRC_ZERO				= 0x1,
		BLEND_SRC_ONE				= 0x2,
		BLEND_SRC_DEST_COLOR		= 0x3,
		BLEND_SRC_INV_DEST_COLOR	= 0x4,
		BLEND_SRC_SRC_ALPHA			= 0x5,
		BLEND_SRC_INV_SRC_ALPHA		= 0x6,
		BLEND_SRC_DEST_ALPHA		= 0x7,
		BLEND_SRC_INV_DEST_ALPHA	= 0x8,
		BLEND_SRC_ALPHA_SAT			= 0x9,

		BLEND_DEST_ZERO				= 0x10,
		BLEND_DEST_ONE				= 0x20,
		BLEND_DEST_SRC_COLOR		= 0x30,
		BLEND_DEST_INV_SRC_COLOR	= 0x40,
		BLEND_DEST_SRC_ALPHA		= 0x50,
		BLEND_DEST_INV_SRC_ALPHA	= 0x60,
		BLEND_DEST_DEST_ALPHA		= 0x70,
		BLEND_DEST_INV_DEST_ALPHA	= 0x80,

		BLEND_OP_ADD		= 0x00001000,
		BLEND_OP_SUB		= 0x00002000,
		BLEND_OP_REB_SUB	= 0x00004000,
		BLEND_OP_MIN		= 0x00008000,
		BLEND_OP_MAX		= 0x00010000,

		CULL_BACK			= 0x00000000,
		CULL_FRONT			= 0x00020000,
		CULL_NONE			= 0x00040000,

		DEPTHFUNC_LEQUAL	= 0x00000000,
		DEPTHFUNC_EQUAL		= 0x00100000,
		DEPTHFUNC_GREAT		= 0x00200000,
		DEPTHFUNC_LESS		= 0x00300000,
		DEPTHFUNC_GEQUAL	= 0x00400000,
		DEPTHFUNC_NOTEQUAL	= 0x00500000,

		ALPHATEST_GREATER	= 0x10000000,
		ALPHATEST_LESS		= 0x20000000,
		ALPHATEST_GEQUAL	= 0x40000000,
		ALPHATEST_LEQUAL	= 0x80000000,

		// single flags
		DEPTHWRITE          = 0x00000100,
		WIREFRAME			= 0x00000200,
		NO_DEPTH_TEST		= 0x00000400,
		STENCIL             = 0x00000800,

		// Blend Masks.
		BLEND_SRC_MASK		= 0x0000000f,
		BLEND_DEST_MASK		= 0x000000f0,
		BLEND_MASK			= 0x000000ff,
		BLEND_OP_MASK		= 0x0001F000,

		CULL_MASK			= 0x00060000,

		DEPTHFUNC_MASK		= 0x00700000,
		ALPHATEST_MASK		= 0xf0000000,
	};



	struct Bits																															
	{																																	
		uint32_t BIT_blendSrc : 4;
		uint32_t BIT_blendDst : 4;
		uint32_t BIT_depthWrite : 1;
		uint32_t BIT_wireframe : 1;
		uint32_t BIT_no_depth_test : 1;
		uint32_t BIT_stencil : 1;
		uint32_t BIT_blendOP : 5;
		uint32_t BIT_cull : 2;
		uint32_t BIT_pad8 : 1;
		uint32_t BIT_depthFuncMask : 3;
		uint32_t BIT_pad1 : 5;
		uint32_t BIT_alphaTestMask : 4;
	};

	static const char* ToString(uint32_t value)																							
	{					
		X_UNUSED(value);
		return "not Supported"; // cba :D !																												
	}
};

typedef Flags<States> StateFlag;

// ==================== OLD  =========================================


#if 0
struct IRender
{
	virtual ~IRender(){};

	virtual bool Init(HWND hWnd, uint32_t width, uint32_t hieght) X_ABSTRACT;
	virtual void ShutDown() X_ABSTRACT;
	virtual void freeResources() X_ABSTRACT;

	virtual void RenderBegin() X_ABSTRACT;
	virtual void RenderEnd() X_ABSTRACT;

	virtual void SetState(StateFlag state) X_ABSTRACT;
	virtual void SetStencilState(StencilState::Value ss) X_ABSTRACT;
	virtual void SetCullMode(CullMode::Enum mode) X_ABSTRACT;
	virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_ABSTRACT;

	// ViewPort
	virtual void GetViewport(int* left, int* top, int* right, int* bottom) X_ABSTRACT;
	virtual void SetViewport(int left, int top, int right, int bottom) X_ABSTRACT;
	virtual void GetViewport(Recti& rect) X_ABSTRACT;
	virtual void SetViewport(const Recti& rect) X_ABSTRACT;

	virtual int getWidth(void) const X_ABSTRACT;
	virtual int getHeight(void) const X_ABSTRACT;
	virtual float getWidthf(void) const X_ABSTRACT;
	virtual float getHeightf(void) const X_ABSTRACT;
	// ~ViewPort

	// scales from 800x600 range to what ever res.
	// 400(x) on 1650x1050 becomes 825
	virtual float ScaleCoordX(float value) const X_ABSTRACT;
	virtual float ScaleCoordY(float value) const X_ABSTRACT;
	virtual void ScaleCoord(float& x, float& y) const X_ABSTRACT;
	virtual void ScaleCoord(Vec2f& xy) const X_ABSTRACT;


	virtual void  SetCamera(const XCamera& cam) X_ABSTRACT;
	virtual const XCamera& GetCamera() X_ABSTRACT;

	// AuxGeo
	virtual IRenderAux* GetIRenderAuxGeo(void) X_ABSTRACT;
	// ~AuxGeo

	// Textures 
	virtual texture::ITexture* LoadTexture(const char* path, texture::TextureFlags flags) X_ABSTRACT;

	virtual void ReleaseTexture(texture::TexID id) X_ABSTRACT;

	virtual bool SetTexture(texture::TexID texId) X_ABSTRACT;
	// ~Textures

	// Drawing util

	// Screen Space Draw: range 0-2 width / h is also scrrenspace size not pixels
	virtual void DrawQuadSS(float x, float y, float width, float height, const Color& col) X_ABSTRACT;
	virtual void DrawQuadSS(const Rectf& rect, const Color& col) X_ABSTRACT;
	virtual void DrawQuadSS(float x, float y, float width, float height, const Color& col, const Color& borderCol) X_ABSTRACT;
	virtual void DrawQuadImageSS(float x, float y, float width, float height, texture::TexID texture_id, const Color& col) X_ABSTRACT;
	virtual void DrawQuadImageSS(const Rectf& rect, texture::TexID texture_id, const Color& col) X_ABSTRACT;
	virtual void DrawRectSS(float x, float y, float width, float height, const Color& col) X_ABSTRACT;
	virtual void DrawRectSS(const Rectf& rect, const Color& col) X_ABSTRACT;
	virtual void DrawLineColorSS(const Vec2f& vPos1, const Color& color1,
		const Vec2f& vPos2, const Color& vColor2) X_ABSTRACT;

	virtual void DrawQuadImage(float x, float y, float width, float height, texture::TexID texture_id, const Color& col) X_ABSTRACT;
	virtual void DrawQuadImage(float x, float y, float width, float height, texture::ITexture* pTexutre, const Color& col) X_ABSTRACT;
	virtual void DrawQuadImage(const Rectf& rect, texture::ITexture* pTexutre, const Color& col) X_ABSTRACT;

	// for 2d, z is depth not position
	virtual void DrawQuad(float x, float y, float z, float width, float height, const Color& col) X_ABSTRACT;
	virtual void DrawQuad(float x, float y, float z, float width, float height, const Color& col, const Color& borderCol) X_ABSTRACT;
	virtual void DrawQuad(float x, float y, float width, float height, const Color& col) X_ABSTRACT;
	virtual void DrawQuad(float x, float y, float width, float height, const Color& col, const Color& borderCol) X_ABSTRACT;
	virtual void DrawQuad(Vec2<float> pos, float width, float height, const Color& col) X_ABSTRACT;
	// draw a quad in 3d z is position not depth.
	virtual void DrawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2, const Vec3f& pos3, const Color& col) X_ABSTRACT;

	virtual void DrawLines(Vec3f* points, uint32_t num, const Color& col) X_ABSTRACT;
	virtual void DrawLine(const Vec3f& pos1, const Vec3f& pos2) X_ABSTRACT;
	virtual void DrawLineColor(const Vec3f& vPos1, const Color& color1,
		const Vec3f& vPos2, const Color& vColor2) X_ABSTRACT;

	virtual void DrawRect(float x, float y, float width, float height, const Color& col) X_ABSTRACT;
	
	virtual void DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
		float padding, uint32_t max) X_ABSTRACT;

	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* format, va_list args) X_ABSTRACT;
	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* text) X_ABSTRACT;

	virtual void DrawAllocStats(Vec3f pos, const XDrawTextInfo& ti,
		const core::MemoryAllocatorStatistics& allocStats, const char* title) X_ABSTRACT;

	virtual void FlushTextBuffer(void) X_ABSTRACT;
	// ~Drawing

	// Font
	virtual int FontCreateTexture(const Vec2i& size, BYTE *pData,
		texture::Texturefmt::Enum eTF = texture::Texturefmt::R8G8B8A8, bool genMips = false) X_ABSTRACT;

	virtual bool FontUpdateTexture(int texId, int X, int Y, int USize, int VSize, uint8_t* pData) X_ABSTRACT;
	virtual bool FontSetTexture(int texId) X_ABSTRACT;
	virtual bool FontSetRenderingState() X_ABSTRACT;
	virtual void FontRestoreRenderingState() X_ABSTRACT;
	virtual void FontSetBlending() X_ABSTRACT;
	virtual void DrawStringW(font::IXFont_RenderProxy* pFont, const Vec3f& pos,
		const wchar_t* pStr, const font::XTextDrawConect& ctx) const X_ABSTRACT;

	// ~Font

	// used by font's mainly.
	virtual void DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
		PrimitiveTypePublic::Enum type) X_ABSTRACT;

	// Shader Stuff

	virtual shader::XShaderItem LoadShaderItem(shader::XInputShaderResources& res) X_ABSTRACT;
	
	virtual bool DefferedBegin(void) X_ABSTRACT;
	virtual bool DefferedEnd(void) X_ABSTRACT;
	virtual bool SetWorldShader(void) X_ABSTRACT;
	virtual bool setGUIShader(bool textured = false) X_ABSTRACT;
	// ~Shader Stuff

	// Model
	virtual model::IRenderMesh* createRenderMesh(void) X_ABSTRACT;
	virtual model::IRenderMesh* createRenderMesh(const model::MeshHeader* pMesh,
		shader::VertexFormat::Enum fmt, const char* name) X_ABSTRACT;
	virtual void freeRenderMesh(model::IRenderMesh* pMesh) X_ABSTRACT;
	 
	virtual void SetModelMatrix(const Matrix44f& mat) X_ABSTRACT;
	// ~Model
};
#else


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

	virtual void submitCommandPackets(CommandBucket<uint32_t>& cmdBucket, Commands::Key::Type::Enum keyType) X_ABSTRACT;

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

	virtual VertexBufferHandle createVertexBuffer(uint32_t size, BufUsage::Enum usage, CpuAccessFlags accessFlag = 0) X_ABSTRACT;
	virtual VertexBufferHandle createVertexBuffer(uint32_t size, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag = 0) X_ABSTRACT;
	virtual IndexBufferHandle createIndexBuffer(uint32_t size, BufUsage::Enum usage, CpuAccessFlags accessFlag = 0) X_ABSTRACT;
	virtual IndexBufferHandle createIndexBuffer(uint32_t size, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag = 0) X_ABSTRACT;

	virtual void destoryVertexBuffer(VertexBufferHandle handle) X_ABSTRACT;
	virtual void destoryIndexBuffer(IndexBufferHandle handle) X_ABSTRACT;

	virtual void getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_ABSTRACT;
	virtual void getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_ABSTRACT;


	virtual texture::ITexture* getTexture(const char* pName, texture::TextureFlags flags) X_ABSTRACT;
	virtual shader::IShader* getShader(const char* pName) X_ABSTRACT;

	virtual void releaseTexture(texture::ITexture* pTex) X_ABSTRACT;
	virtual void releaseShader(shader::IShader* pShader) X_ABSTRACT;

	// state
	virtual PassStateHandle createPassState(const RenderTargetFmtsArr& rtfs) X_ABSTRACT;
	virtual void destoryPassState(PassStateHandle handle) X_ABSTRACT;

	virtual StateHandle createState(PassStateHandle passHandle, const StateDesc& state, const TextureState* pTextStates, size_t numStates) X_ABSTRACT;
	virtual void destoryState(StateHandle handle) X_ABSTRACT;

	// =============================================
	// ============== OLD API ======================
	// =============================================

	// everying is depriciated.
//	virtual void SetState(StateFlag state) X_ABSTRACT;
//	virtual void SetStencilState(StencilState::Value ss) X_ABSTRACT;
//	virtual void SetCullMode(CullMode::Enum mode) X_ABSTRACT;
	virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_ABSTRACT;

	// ViewPort
	virtual void GetViewport(int* left, int* top, int* right, int* bottom) X_ABSTRACT;
	virtual void SetViewport(int left, int top, int right, int bottom) X_ABSTRACT;
	virtual void GetViewport(Recti& rect) X_ABSTRACT;
	virtual void SetViewport(const Recti& rect) X_ABSTRACT;

	virtual int getWidth(void) const X_ABSTRACT;
	virtual int getHeight(void) const X_ABSTRACT;
	virtual float getWidthf(void) const X_ABSTRACT;
	virtual float getHeightf(void) const X_ABSTRACT;
	// ~ViewPort

	// scales from 800x600 range to what ever res.
	// 400(x) on 1650x1050 becomes 825
	virtual float ScaleCoordX(float value) const X_ABSTRACT;
	virtual float ScaleCoordY(float value) const X_ABSTRACT;
	virtual void ScaleCoord(float& x, float& y) const X_ABSTRACT;
	virtual void ScaleCoord(Vec2f& xy) const X_ABSTRACT;


	virtual void  SetCamera(const XCamera& cam) X_ABSTRACT;
	virtual const XCamera& GetCamera() X_ABSTRACT;

	// AuxGeo
	virtual IRenderAux* GetIRenderAuxGeo(void) X_ABSTRACT;
	// ~AuxGeo

	// Textures 
//	virtual texture::ITexture* LoadTexture(const char* path, texture::TextureFlags flags) X_ABSTRACT;

	virtual void ReleaseTexture(texture::TexID id) X_ABSTRACT;

	virtual bool SetTexture(texture::TexID texId) X_ABSTRACT;
	// ~Textures

	// Drawing util

	// Screen Space Draw: range 0-2 width / h is also scrrenspace size not pixels
	virtual void DrawQuadSS(float x, float y, float width, float height, const Color& col) X_ABSTRACT;
	virtual void DrawQuadSS(const Rectf& rect, const Color& col) X_ABSTRACT;
	virtual void DrawQuadSS(float x, float y, float width, float height, const Color& col, const Color& borderCol) X_ABSTRACT;
	virtual void DrawQuadImageSS(float x, float y, float width, float height, texture::TexID texture_id, const Color& col) X_ABSTRACT;
	virtual void DrawQuadImageSS(const Rectf& rect, texture::TexID texture_id, const Color& col) X_ABSTRACT;
	virtual void DrawRectSS(float x, float y, float width, float height, const Color& col) X_ABSTRACT;
	virtual void DrawRectSS(const Rectf& rect, const Color& col) X_ABSTRACT;
	virtual void DrawLineColorSS(const Vec2f& vPos1, const Color& color1,
		const Vec2f& vPos2, const Color& vColor2) X_ABSTRACT;

	virtual void DrawQuadImage(float x, float y, float width, float height, texture::TexID texture_id, const Color& col) X_ABSTRACT;
	virtual void DrawQuadImage(float x, float y, float width, float height, texture::ITexture* pTexutre, const Color& col) X_ABSTRACT;
	virtual void DrawQuadImage(const Rectf& rect, texture::ITexture* pTexutre, const Color& col) X_ABSTRACT;

	// for 2d, z is depth not position
	virtual void DrawQuad(float x, float y, float z, float width, float height, const Color& col) X_ABSTRACT;
	virtual void DrawQuad(float x, float y, float z, float width, float height, const Color& col, const Color& borderCol) X_ABSTRACT;
	virtual void DrawQuad(float x, float y, float width, float height, const Color& col) X_ABSTRACT;
	virtual void DrawQuad(float x, float y, float width, float height, const Color& col, const Color& borderCol) X_ABSTRACT;
	virtual void DrawQuad(Vec2<float> pos, float width, float height, const Color& col) X_ABSTRACT;
	// draw a quad in 3d z is position not depth.
	virtual void DrawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2, const Vec3f& pos3, const Color& col) X_ABSTRACT;

	virtual void DrawLines(Vec3f* points, uint32_t num, const Color& col) X_ABSTRACT;
	virtual void DrawLine(const Vec3f& pos1, const Vec3f& pos2) X_ABSTRACT;
	virtual void DrawLineColor(const Vec3f& vPos1, const Color& color1,
		const Vec3f& vPos2, const Color& vColor2) X_ABSTRACT;

	virtual void DrawRect(float x, float y, float width, float height, const Color& col) X_ABSTRACT;

	virtual void DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
		float padding, uint32_t max) X_ABSTRACT;

	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* format, va_list args) X_ABSTRACT;
	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* text) X_ABSTRACT;

	virtual void DrawAllocStats(Vec3f pos, const XDrawTextInfo& ti,
		const core::MemoryAllocatorStatistics& allocStats, const char* title) X_ABSTRACT;

	virtual void FlushTextBuffer(void) X_ABSTRACT;
	// ~Drawing

	// Font
	virtual int FontCreateTexture(const Vec2i& size, BYTE *pData,
		texture::Texturefmt::Enum eTF = texture::Texturefmt::R8G8B8A8, bool genMips = false) X_ABSTRACT;

	virtual bool FontUpdateTexture(int texId, int X, int Y, int USize, int VSize, uint8_t* pData) X_ABSTRACT;
	virtual bool FontSetTexture(int texId) X_ABSTRACT;
	virtual bool FontSetRenderingState() X_ABSTRACT;
	virtual void FontRestoreRenderingState() X_ABSTRACT;
	virtual void FontSetBlending() X_ABSTRACT;
	virtual void DrawStringW(font::IXFont_RenderProxy* pFont, const Vec3f& pos,
		const wchar_t* pStr, const font::XTextDrawConect& ctx) const X_ABSTRACT;

	// ~Font

	// used by font's mainly.
//	virtual void DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
//		PrimitiveTypePublic::Enum type) X_ABSTRACT;

	// Shader Stuff

	virtual shader::XShaderItem LoadShaderItem(shader::XInputShaderResources& res) X_ABSTRACT;

	virtual bool DefferedBegin(void) X_ABSTRACT;
	virtual bool DefferedEnd(void) X_ABSTRACT;
	virtual bool SetWorldShader(void) X_ABSTRACT;
	virtual bool setGUIShader(bool textured = false) X_ABSTRACT;
	// ~Shader Stuff

	// Model
//	virtual model::IRenderMesh* createRenderMesh(void) X_ABSTRACT;
//	virtual model::IRenderMesh* createRenderMesh(const model::MeshHeader* pMesh,
//		shader::VertexFormat::Enum fmt, const char* name) X_ABSTRACT;
//	virtual void freeRenderMesh(model::IRenderMesh* pMesh) X_ABSTRACT;

	virtual void SetModelMatrix(const Matrix44f& mat) X_ABSTRACT;
	// ~Model

};

#endif
X_NAMESPACE_END

#endif // !_X_RENDER_I_H_
