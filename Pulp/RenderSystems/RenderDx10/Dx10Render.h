#pragma once


#ifndef _X_RENDER_DX10_H_
#define _X_RENDER_DX10_H_

#include "../Common/XRender.h"

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
		P3F_T2F_C4B,
		P3F_T2S_C4B,
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

	RenderState()
	{
		pIndexStream = nullptr;

		core::zero_object(ILCache);
		core::zero_object(streamedILCache);

		pCurrentVertexFmt = nullptr;
		CurrentVertexFmt = shader::VertexFormat::P3F_T2S;
		streamedIL = false;

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


	

	XStreamInfo VertexStreams[VertexStream::ENUM_COUNT];
	ID3D11Buffer* pIndexStream;

	// vertex format cache.
	typedef core::FixedArray<D3D11_INPUT_ELEMENT_DESC, 12> XVertexLayout;

	// Layouts used for creating device layouts.
	// XVertexLayout vertexLayoutDescriptions[shader::VertexFormat::Num];
	std::array<XVertexLayout, shader::VertexFormat::Num> ILDescriptions;
	std::array<XVertexLayout, shader::VertexFormat::Num> streamedILDescriptions;

	// GPU layouts.
	ID3D11InputLayout* ILCache[shader::VertexFormat::Num];
	ID3D11InputLayout* streamedILCache[shader::VertexFormat::Num];
	ID3D11InputLayout* pCurrentVertexFmt;
	shader::VertexFormat::Enum CurrentVertexFmt;
	bool streamedIL;
	bool _pad[3];

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
	virtual void ShutDown(void) X_OVERRIDE;

	virtual void RenderBegin(void) X_OVERRIDE;
	virtual void RenderEnd(void) X_OVERRIDE;


	bool Create2DTexture(texture::XTextureFile* image_data,
					texture::XDeviceTexture& dev_tex);


	virtual void SetState(StateFlag state) X_OVERRIDE;
	virtual void SetStencilState(StencilState::Value ss) X_OVERRIDE;
	virtual void SetCullMode(CullMode::Enum mode) X_OVERRIDE;
	virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_OVERRIDE;

	X_INLINE virtual void GetModelViewMatrix(Matrix44f* pMat) X_OVERRIDE;
	X_INLINE virtual void GetProjectionMatrix(Matrix44f* pMat) X_OVERRIDE;

	// States
	bool SetBlendState(BlendState& state);
	bool SetRasterState(RasterState& state);
	bool SetDepthState(DepthState& state);
	// ~States


	// Camera
	virtual void SetCamera(const XCamera& cam) X_OVERRIDE;
	void SetCameraInfo(void);
	// ~Camera

	// AuxGeo
	virtual IRenderAux* GetIRenderAuxGeo(void) X_OVERRIDE;
	// ~AuxGeo

	// Textures 
//	virtual void Draw2dImage(float xpos, float ypos,
//		float w, float h, texture::TexID texture_id, ColorT<float>& col) X_OVERRIDE;

//	void DrawImage(float xpos, float ypos, float z, float w, float h,
//		int texture_id, float s0, float t0, float s1, float t1, const Colorf& col, bool filtered = true);

	virtual void ReleaseTexture(texture::TexID id) X_OVERRIDE;
	
	virtual bool SetTexture(int texId) X_OVERRIDE;
	// ~Textures 

	// Shaders 
	virtual void FX_PipelineShutdown(void) X_OVERRIDE;


	virtual bool DefferedBegin(void) X_FINAL;
	virtual bool DefferedEnd(void) X_FINAL;

	virtual bool SetWorldShader(void) X_FINAL;
	virtual bool SetSkyboxShader(void);
	virtual bool SetFFE(shader::VertexFormat::Enum vertFmt, bool textured = false);
	virtual bool SetFontShader(void);
	virtual bool SetZPass(void);
	virtual bool setGUIShader(bool textured = false) X_FINAL;
	bool SetModelShader(shader::VertexFormat::Enum vertFmt);

	// ~Shaders 

	// font
	virtual bool FontUpdateTexture(int texId, int X, int Y, int USize,
		int VSize, uint8_t* pData) X_OVERRIDE;

	virtual bool FontSetTexture(int texId) X_OVERRIDE;
	virtual bool FontSetRenderingState() X_OVERRIDE;
	virtual void FontRestoreRenderingState() X_OVERRIDE;
	virtual void FontSetBlending() X_OVERRIDE;

	// ~font

	// Drawing util

	// Screen Space Draw: range 0-2 width / h is also scrrenspace not pixels
	void DrawQuadSS(float x, float y, float width, float height, const Color& col) X_FINAL;
	void DrawQuadSS(const Rectf& rect, const Color& col) X_FINAL;
	void DrawQuadSS(float x, float y, float width, float height, const Color& col, const Color& borderCol) X_FINAL;
	void DrawQuadImageSS(float x, float y, float width, float height, texture::TexID texture_id, const Color& col) X_FINAL;
	void DrawQuadImageSS(const Rectf& rect, texture::TexID texture_id, const Color& col) X_FINAL;
	void DrawRectSS(float x, float y, float width, float height, const Color& col) X_FINAL;
	void DrawRectSS(const Rectf& rect, const Color& col) X_FINAL;
	void DrawLineColorSS(const Vec2f& vPos1, const Color& color1,
		const Vec2f& vPos2, const Color& vColor2) X_FINAL;


	void DrawQuadImage(float x, float y, float width, float height, texture::TexID texture_id, const Color& col) X_FINAL;
	void DrawQuadImage(float x, float y, float width, float height, texture::ITexture* pTexutre, const Color& col) X_FINAL;
	void DrawQuadImage(const Rectf& rect, texture::ITexture* pTexutre, const Color& col) X_OVERRIDE;

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

	void DrawRect(float x, float y, float width, float height, const Color& col) X_OVERRIDE;
	
	void DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
		float padding, uint32_t max) X_OVERRIDE;

	// not exposed
	void DrawImage(float xpos, float ypos, float z, float w, float h,
		int texture_id, float s0, float t0, float s1, float t1, const Colorf& col, bool filtered = true);

	void DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
		int texture_id, const float* s, const float* t, const Colorf& col, bool filtered);

	// ~Drawing util

	virtual void DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
		PrimitiveTypePublic::Enum type) X_OVERRIDE;


	virtual void DrawString(const Vec3f& pos, const char* pStr) X_OVERRIDE;

	// ~RT
	void FX_ComitParams(void);
	void FX_Init(void);


	bool FX_SetVertexDeclaration(shader::VertexFormat::Enum vertexFmt, bool streamed);
//	HRESULT FX_SetTextureAsVStream(int nID, texture::XTexture* pVBTexture, uint32 nStride);
	void FX_SetVStream(ID3D11Buffer* pVertexBuffer, VertexStream::Enum streamSlot,
				uint32 stride, uint32 offset);
	void FX_SetVStream(uint32 VertexBuffer, VertexStream::Enum streamSlot,
		uint32 stride, uint32 offset);
	void FX_SetIStream(ID3D11Buffer* pIndexBuffer);
	void FX_SetIStream(uint32 IndexBuffer);

	void FX_DrawPrimitive(const PrimitiveType::Enum type, const int StartVertex, const int VerticesCount);
	void FX_DrawIndexPrimitive(const PrimitiveType::Enum type, const int IndexCount, 
		const int StartIndex,	 
		const int BaseVertexLocation); // A value added to each index before reading a vertex from the vertex buffer.

	void FX_UnBindVStream(ID3D11Buffer* pVertexBuffer);
	void FX_UnBindIStream(ID3D11Buffer* pIndexBuffer);


	X_INLINE const D3D11_PRIMITIVE_TOPOLOGY FX_ConvertPrimitiveType(const PrimitiveType::Enum type) const;
	X_INLINE PrimitiveType::Enum PrimitiveTypeToInternal(PrimitiveTypePublic::Enum type) const;

	X_INLINE void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topType);

	X_INLINE ID3D11Device* DxDevice(void) const;
	X_INLINE ID3D11DeviceContext* DxDeviceContext(void) const;
	X_INLINE void PushViewMatrix(void);
	X_INLINE void PopViewMatrix(void);
	X_INLINE void DirtyMatrix(void);

	X_INLINE Matrix44f* pCurViewMat(void);
	X_INLINE Matrix44f* pCurProjMat(void);
	X_INLINE bool IsDeviceLost(void) const;

	X_INLINE void SetModelMatrix(const Matrix44f& mat) X_OVERRIDE;


private:
	ID3D11InputLayout* CreateILFromDesc(const shader::VertexFormat::Enum vertexFmt,
		const RenderState::XVertexLayout& layout);

	virtual void SetArenas(core::MemoryArenaBase* arena) X_OVERRIDE;
	bool OnPostCreateDevice(void);
	void InitResources(void);
	void InitDynamicBuffers(void);
	void InitILDescriptions(void);

	void FreeDynamicBuffers(void);

protected:
	XMatrixStack ViewMat_;
	XMatrixStack ProMat_;
	Matrix44f modelMat_;

	XDynamicVB<byte> DynVB_[VertexPool::PoolMax];

	// States
	core::Array<BlendState> BlendStates_;
	core::Array<RasterState> RasterStates_;
	core::Array<DepthState> DepthStates_;

	uint32_t CurBlendState_;
	uint32_t CurRasterState_;
	uint32_t CurDepthState_;

	X_INLINE BlendState& curBlendState(void);
	X_INLINE RasterState& curRasterState(void);
	X_INLINE DepthState& curDepthState(void);

protected:
#if X_DEBUG
	ID3D11Debug* d3dDebug_;
#endif

	ID3D11Device* device_;
	ID3D11DeviceContext* deviceContext_;

	IDXGISwapChain* swapChain_;
	ID3D11RenderTargetView* renderTargetView_;
	ID3D11Texture2D* depthStencilBuffer_;
	ID3D11DepthStencilView* depthStencilViewReadOnly_;
	ID3D11DepthStencilView* depthStencilView_;

	D3D11_PRIMITIVE_TOPOLOGY CurTopology_;

	RenderState State_;

	XRenderAuxImp* AuxGeo_;
};

extern DX11XRender g_Dx11D3D;

X_NAMESPACE_END

#include "Dx10Render.inl"

#endif // !_X_RENDER_DX10_H_
