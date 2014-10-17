#pragma once


#ifndef _X_RENDER_DX10_H_
#define _X_RENDER_DX10_H_

#include "../Common/XRender.h"
#include "../Common/VertexFormat.h"
#include "../Common/MatrixStack.h"

#include "DeviceManager\DeviceManager.h"
#include <Hashing\Fnva1Hash.h>

#include "DynamicVB.h"

#include <array>

X_NAMESPACE_BEGIN(render)


class XRenderAuxImp;

struct PrimitiveType
{
	enum Enum
	{
		TriangleList = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		TriangleStrip = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		LineList = D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
		LineStrip = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
		PointList = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST
	};
};


struct VertexPool
{
	enum Enum
	{
		P3F_C4B_T2F,
		P3F_C4B_T2S,
		PoolMax
	};
};

// diffrent states baby.
// we want to be able to check if the states are diffrent
// saves us having to change state on GPU.
// we only create the state on the gpu once.
// dx returns same pointer to identical states but we are using the
// hash pre-state creation, saving a api call.
// so we should make a static hash.


struct StateHash
{
	bool operator == (const StateHash& oth) const
	{
		return this->hash == oth.hash;
	}

	virtual void createHash() X_ABSTRACT;

	uint64_t hash;
};

struct BlendState : public StateHash
{
	BlendState() : pState(nullptr) {
		core::zero_object(Desc);
	}

	ID3D11BlendState* pState;
	D3D11_BLEND_DESC Desc;

	void createHash() X_OVERRIDE
	{
		hash = core::Hash::Int64::Fnv1aHash(&Desc, sizeof(Desc));
	}
};

struct RasterState : public StateHash
{
	RasterState() : pState(nullptr) {
		core::zero_object(Desc);
	}

	ID3D11RasterizerState* pState;
	D3D11_RASTERIZER_DESC Desc;

	void createHash() X_OVERRIDE
	{
		hash = core::Hash::Int64::Fnv1aHash(&Desc, sizeof(Desc));
	}
};

struct DepthState : public StateHash
{
	DepthState() : pState(nullptr) {
		core::zero_object(Desc);
	}

	ID3D11DepthStencilState* pState;
	D3D11_DEPTH_STENCIL_DESC Desc;

	void createHash() X_OVERRIDE
	{
		hash = core::Hash::Int64::Fnv1aHash(&Desc, sizeof(Desc));
	}
};




class RenderState
{
public:
	static const int MAX_VERTEX_STREAMS = 8;

	RenderState() :
		vertexLayoutDescriptions({ g_rendererArena, 
		g_rendererArena,
		g_rendererArena,
		g_rendererArena,
		g_rendererArena })
	{
		pIndexStream = nullptr;

		core::zero_object(vertexLayoutCache);
		pCurrentVertexFmt = nullptr;

		pCurShader = nullptr;
		pCurShaderTech = nullptr;
		CurShaderTechIdx = -1;
	}

	struct XStreamInfo
	{
		XStreamInfo() : pBuf(nullptr), offset(0) {}
		ID3D11Buffer* pBuf;
		uint32 offset;
	};


	

	XStreamInfo VertexStreams[MAX_VERTEX_STREAMS];
	ID3D11Buffer* pIndexStream;

	// vertex format cache.
	typedef core::Array<D3D11_INPUT_ELEMENT_DESC> XVertexLayout;

	// Layouts used for creating device layouts.
	// XVertexLayout vertexLayoutDescriptions[shader::VertexFormat::Num];
	std::array<XVertexLayout, shader::VertexFormat::Num> vertexLayoutDescriptions;

	// GPU layouts.
	ID3D11InputLayout* vertexLayoutCache[shader::VertexFormat::Num];
	ID3D11InputLayout* pCurrentVertexFmt;


	StateFlag currentState;
	CullMode::Enum cullMode;

	// shader
	shader::XShader*			pCurShader;
	shader::XShaderTechnique*	pCurShaderTech;
	int32						CurShaderTechIdx;
};






class DX11XRender : public XRender
{
	friend class shader::XShader;

public:
	DX11XRender();
	~DX11XRender();

	virtual bool Init(HWND hWnd, uint32_t width, uint32_t hieght) X_OVERRIDE;
	virtual void ShutDown() X_OVERRIDE;

	virtual void RenderBegin() X_OVERRIDE;
	virtual void RenderEnd() X_OVERRIDE;

	bool Create2DTexture(texture::XTextureFile* image_data,
					texture::XDeviceTexture& dev_tex);


	virtual void SetState(StateFlag state) X_OVERRIDE;
	virtual void SetStencilState(StencilState::Value ss) X_OVERRIDE;
	virtual void SetCullMode(CullMode::Enum mode) X_OVERRIDE;
	virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_OVERRIDE;

	virtual void GetModelViewMatrix(Matrix44f* pMat) X_OVERRIDE {
		*pMat = *m_ViewMat.GetTop();
	}
	virtual void GetProjectionMatrix(Matrix44f* pMat) X_OVERRIDE {
		*pMat = *m_ProMat.GetTop();
	}

	// States
	bool SetBlendState(BlendState& state);
	bool SetRasterState(RasterState& state);
	bool SetDepthState(DepthState& state);
	// ~States


	// ViewPort
	virtual void GetViewport(int* x, int* y, int* width, int* height) X_OVERRIDE;
	virtual void SetViewport(int x, int y, int width, int height) X_OVERRIDE;
	virtual void GetViewport(Vec4<int>& viewport) X_OVERRIDE;
	virtual void SetViewport(const Vec4<int>& viewport) X_OVERRIDE;
	// ~ViewPort

	// Camera
	virtual void  SetCamera(const XCamera& cam) X_OVERRIDE;
	void SetCameraInfo(void);
	// ~Camera

	// AuxGeo
	virtual IRenderAux* GetIRenderAuxGeo(void) X_OVERRIDE;
	// ~AuxGeo

	// Textures 
	virtual void Draw2dImage(float xpos, float ypos,
		float w, float h, texture::TexID texture_id, ColorT<float>& col) X_OVERRIDE;

	void DrawImage(float xpos, float ypos, float z, float w, float h,
		int texture_id, float s0, float t0, float s1, float t1, const Colorf& col, bool filtered = true);

	virtual void ReleaseTexture(texture::TexID id) X_OVERRIDE;

	// ~Textures 

	// Shaders 
	virtual void FX_PipelineShutdown() X_OVERRIDE;

	virtual bool SetWorldShader(void);
	virtual bool SetFFE(bool textured = false);
	virtual bool SetFontShader();
	virtual bool SetZPass();

	// ~Shaders 

	// font
	virtual bool FontUpdateTexture(int texId, int X, int Y, int USize,
		int VSize, uint8_t* pData) X_OVERRIDE;

	virtual bool FontSetTexture(int texId) X_OVERRIDE;
	virtual void FontSetRenderingState() X_OVERRIDE;
	virtual void FontRestoreRenderingState() X_OVERRIDE;
	virtual void FontSetBlending() X_OVERRIDE;

	// ~font


	virtual void DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
		int texture_id, float* s, float* t, const Colorf& col, bool filtered = true) X_OVERRIDE;

	virtual void DrawVB(Vertex_P3F_C4B_T2F* pVertBuffer, uint32_t size,
		PrimitiveTypePublic::Enum type) X_OVERRIDE;

	void DrawQuad(float x, float y, float z, float width, float height, const Color& col) X_OVERRIDE;
	void DrawQuad(float x, float y, float z, float width, float height, const Color& col, const Color& borderCol) X_OVERRIDE;
	void DrawQuad(float x, float y, float width, float height, const Color& col) X_OVERRIDE;
	void DrawQuad(float x, float y, float width, float height, const Color& col, const Color& borderCol) X_OVERRIDE;
	void DrawQuad(Vec2<float> pos, float width, float height, const Color& col) X_OVERRIDE;
	void DrawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2, const Vec3f& pos3, const Color& col) X_OVERRIDE;

	void DrawLines(Vec3f* points, uint32_t num, const Color& col) X_OVERRIDE;
	void DrawLine(const Vec3f& pos1, const Vec3f& pos2) X_OVERRIDE;
	void DrawLineColor(const Vec3f& vPos1, const Color& color1,
		const Vec3f& vPos2, const Color& vColor2) X_OVERRIDE;

	void DrawRect(float x, float y, float width, float height, Color col) X_OVERRIDE;
	
	void DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
		float padding, uint32_t max) X_OVERRIDE;
	// RT

	virtual void RT_DrawLines(Vec3f* points, uint32_t num, const Colorf& col) X_OVERRIDE;

	virtual void RT_DrawString(const Vec3f& pos, const char* pStr) X_OVERRIDE;

	virtual void RT_SetState(StateFlag state) X_OVERRIDE;
	virtual void RT_SetCullMode(CullMode::Enum mode) X_OVERRIDE;

	virtual void RT_DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
		int texture_id, float *s, float *t, const Colorf& col, bool filtered = true) X_OVERRIDE;
	
	virtual void RT_SetCameraInfo(void) X_OVERRIDE;

	// ~RT
	void FX_ComitParams(void);
	void FX_Init(void);

	HRESULT FX_SetVertexDeclaration(shader::VertexFormat::Enum vertexFmt);
//	HRESULT FX_SetTextureAsVStream(int nID, texture::XTexture* pVBTexture, uint32 nStride);
	void FX_SetVStream(ID3D11Buffer* pVertexBuffer, uint32 startslot,
				uint32 stride, uint32 offset);
	void FX_SetVStream(uint32 VertexBuffer, uint32 startslot,
		uint32 stride, uint32 offset);
	void FX_SetIStream(ID3D11Buffer* pIndexBuffer);
	void FX_SetIStream(uint32 IndexBuffer);

	void FX_DrawPrimitive(const PrimitiveType::Enum type, const int StartVertex, const int VerticesCount);
	void FX_DrawIndexPrimitive(const PrimitiveType::Enum type, const int IndexCount, 
		const int StartIndex,	 
		const int BaseVertexLocation); // A value added to each index before reading a vertex from the vertex buffer.


	const D3D11_PRIMITIVE_TOPOLOGY FX_ConvertPrimitiveType(const PrimitiveType::Enum type) {
		return (D3D11_PRIMITIVE_TOPOLOGY)type;
	}

	PrimitiveType::Enum PrimitiveTypeToInternal(PrimitiveTypePublic::Enum type) {
		if (type == PrimitiveTypePublic::LineList)
			return PrimitiveType::LineList;
		if (type == PrimitiveTypePublic::LineStrip)
			return PrimitiveType::LineStrip;
		if (type == PrimitiveTypePublic::TriangleList)
			return PrimitiveType::TriangleList;
		if (type == PrimitiveTypePublic::TriangleStrip)
			return PrimitiveType::TriangleStrip;
		X_ASSERT_UNREACHABLE();
		return PrimitiveType::TriangleStrip;
	}

	void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topType)
	{
		if (m_CurTopology != topType)
		{
			m_CurTopology = topType;
			m_deviceContext->IASetPrimitiveTopology(m_CurTopology);
		}
	}



	X_INLINE ID3D11Device* DxDevice() const {
		return m_device;
	}
	X_INLINE ID3D11DeviceContext* DxDeviceContext() const {
		return m_deviceContext;
	}

	  
	X_INLINE void PushMatrix()
	{
		m_ViewMat.Push();
	}
	X_INLINE void PopMatrix()
	{
		m_ViewMat.Pop();
		DirtyMatrix();
	}
	X_INLINE void DirtyMatrix()
	{

	}

	X_INLINE Matrix44f* pCurViewMat(void) {
		return m_ViewMat.GetTop();
	}
	X_INLINE Matrix44f* pCurProjMat(void) {
		return m_ProMat.GetTop();
	}

	X_INLINE bool IsDeviceLost(void) const {
		return false;
	}


private:
	void DefferedTest();

	virtual void SetArenas(core::MemoryArenaBase* arena) X_OVERRIDE;
	void OnPostCreateDevice(void);
	void InitResources(void);
	void InitDynamicBuffers(void);
	void InitVertexLayoutDescriptions(void);

	void FreeDynamicBuffers(void);
protected:


	XMatrixStack m_ViewMat;
	XMatrixStack m_ProMat;

	XDynamicVB<byte> m_DynVB[VertexPool::PoolMax];

	// States
	core::Array<BlendState> m_BlendStates;
	core::Array<RasterState> m_RasterStates;
	core::Array<DepthState> m_DepthStates;

	uint32_t m_CurBlendState;
	uint32_t m_CurRasterState;
	uint32_t m_CurDepthState;

	BlendState& curBlendState() { return m_BlendStates[m_CurBlendState]; }
	RasterState& curRasterState() { return m_RasterStates[m_CurRasterState]; }
	DepthState& curDepthState() { return m_DepthStates[m_CurDepthState]; }


protected:
#if X_DEBUG
	ID3D11Debug* m_d3dDebug;
#endif

	ID3D11Device* m_device;
	ID3D11DeviceContext *m_deviceContext;

	IDXGISwapChain* m_swapChain;
	ID3D11RenderTargetView* m_renderTargetView;
	ID3D11Texture2D* m_depthStencilBuffer;
	ID3D11DepthStencilView* m_depthStencilViewReadOnly;
	ID3D11DepthStencilView* m_depthStencilView;

	D3D11_PRIMITIVE_TOPOLOGY m_CurTopology;

	RenderState m_State;

	XRenderAuxImp* m_AuxGeo_;
};


extern DX11XRender g_Dx11D3D;

X_NAMESPACE_END

#endif // !_X_RENDER_DX10_H_
