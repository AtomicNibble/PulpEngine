#pragma once

#ifndef _X_RENDER_BASE_H_
#define _X_RENDER_BASE_H_

#include "DeviceManager\DeviceManager.h"
#include "DeviceManager\VidMemManager.h"
#include "ReaderThread.h"
#include "TextDrawList.h"
#include "Math\XMatrix44.h"

#include <Containers\Array.h>

#include "Shader\XShader.h"

X_NAMESPACE_BEGIN(render)

struct RenderResource
{
	core::StackString<28> name;
	texture::Texturefmt::Enum fmt;
	Vec2f scale;
};

struct XViewPort
{
	Vec4<int> view;
	Vec2f z;

	X_INLINE bool operator==(const XViewPort& oth) const {
		return z == oth.z && view == oth.view;
	}
};

class XRender : public IRender
{
public:
	XRender();
	virtual ~XRender();

	virtual bool Init(HWND hWnd, uint32_t width, uint32_t hieght) X_OVERRIDE;
	virtual void ShutDown() X_OVERRIDE;
	virtual void freeResources() X_OVERRIDE;

	virtual void RenderBegin() X_OVERRIDE;
	virtual void RenderEnd() X_OVERRIDE;

	virtual bool FlushRenderThreadCommands(bool wait) X_OVERRIDE;

	virtual void SetState(StateFlag state) X_ABSTRACT;
	virtual void SetStencilState(StencilState::Value ss) X_ABSTRACT;

	virtual void SetCullMode(CullMode::Enum mode) X_ABSTRACT;
	virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_ABSTRACT;


	virtual void GetViewport(int* x, int* y, int* width, int* height) X_ABSTRACT;
	virtual void SetViewport(int x, int y, int width, int height) X_ABSTRACT;
	virtual void GetViewport(Vec4<int>& viewport) X_ABSTRACT;
	virtual void SetViewport(const Vec4<int>& viewport) X_ABSTRACT;

	virtual int getWidth(void) const X_OVERRIDE{ return width_; }
	virtual int getHeight(void) const X_OVERRIDE{ return height_; }


	// AuxGeo
	virtual IRenderAux* GetIRenderAuxGeo(void) X_ABSTRACT;
	// ~AuxGeo

	virtual void GetModelViewMatrix(Matrix44f* pMat) X_ABSTRACT;
	virtual void GetProjectionMatrix(Matrix44f* pMat) X_ABSTRACT;

	virtual void FX_PipelineShutdown() X_ABSTRACT;

	// Textures 
	virtual texture::ITexture* LoadTexture(const char* path, texture::TextureFlags flags) X_OVERRIDE;
	
	virtual void Draw2dImage(float xpos, float ypos,
		float w, float h, texture::TexID texture_id, ColorT<float>& col) X_ABSTRACT;
	
	virtual void ReleaseTexture(texture::TexID id) X_ABSTRACT;
	// ~Textures 

	// Camera
	virtual void  SetCamera(const XCamera& cam) X_ABSTRACT;
	virtual const XCamera& GetCamera() X_OVERRIDE { return cam_; };
	// ~Camera


	// font
	virtual int FontCreateTexture(const Vec2i& size, BYTE *pData,
		texture::Texturefmt::Enum eTF = texture::Texturefmt::R8G8B8A8, bool genMips = false) X_OVERRIDE;

	virtual bool FontUpdateTexture(int texId, int X, int Y, int USize, 
		int VSize, uint8_t* pData) X_ABSTRACT;

	virtual bool FontSetTexture(int texId) X_ABSTRACT;
	virtual void FontSetRenderingState() X_ABSTRACT;
	virtual void FontRestoreRenderingState() X_ABSTRACT;
	virtual void FontSetBlending() X_ABSTRACT;


	virtual void DrawStringW(font::IXFont_RenderProxy* pFont, const Vec3f& pos,
		const wchar_t* pStr, const font::XTextDrawConect& ctx) const X_OVERRIDE;

	void Draw2dText(float posX, float posY,const char* pStr, const XDrawTextInfo& ti);
	// ~font


	// Shader Stuff

	virtual shader::XShaderItem LoadShaderItem(shader::XInputShaderResources& res) X_OVERRIDE;

	// ~Shader Stuff

	// Model
	virtual model::IRenderMesh* createRenderMesh(void) X_OVERRIDE;
	virtual void freeRenderMesh(model::IRenderMesh* pMesh) X_OVERRIDE;

	// ~Model

	// Drawing

	virtual void DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
		int texture_id, float* s, float* t, const Colorf& col, bool filtered = true) X_ABSTRACT;

	virtual void DrawVB(Vertex_P3F_C4B_T2F* pVertBuffer, uint32_t size,
		PrimitiveTypePublic::Enum type) X_ABSTRACT;

	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* format, va_list args) X_OVERRIDE;
	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* text) X_OVERRIDE;
	
	virtual void DrawAllocStats(Vec3f pos, const XDrawTextInfo& ti,
		const core::MemoryAllocatorStatistics& allocStats, const char* title) X_OVERRIDE;

	virtual void FlushTextBuffer(void) X_OVERRIDE;

	// ~Drawing


	X_INLINE XRenderThread* rThread() {
		return m_pRt;
	}


	VidMemManager* VidMemMng(void) {
		return &vidMemMng_;
	}


	// RT

	virtual void RT_DrawLines(Vec3f* points, uint32_t num, const Colorf& col) X_ABSTRACT;

	virtual void RT_DrawString(const Vec3f& pos, const char* pStr) X_ABSTRACT;

	virtual void RT_SetState(StateFlag state) X_ABSTRACT;
	virtual void RT_SetCullMode(CullMode::Enum mode) X_ABSTRACT;

	virtual void RT_DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
		int texture_id, float *s, float *t, const Colorf& col, bool filtered = true) X_ABSTRACT;

	virtual void RT_SetCameraInfo(void) X_ABSTRACT;

	virtual void RT_FlushTextBuffer(void); // not ab

	// ~RT

	X_INLINE Matrix44f* pViewMatrix() { return &ViewMatrix_; }
	X_INLINE Matrix44f* pProjMatrix() { return &ProjMatrix_; }
	X_INLINE Matrix44f* pViewProjMatrix() { return &ViewProjMatrix_; }
	X_INLINE Matrix44f* pViewProjInvMatrix() { return &ViewProjInvMatrix_; }



	static uint32_t vertexFormatStride[shader::VertexFormat::Num];
protected:

	virtual void SetArenas(core::MemoryArenaBase* arena);

	bool LoadResourceDeffintion(void);

public:
	shader::XShaderManager m_ShaderMan;

	core::Array<RenderResource> RenderResources_;
protected:

	X_ALIGN16_MATRIX44F(ViewMatrix_);
	X_ALIGN16_MATRIX44F(ProjMatrix_);
	X_ALIGN16_MATRIX44F(ViewProjMatrix_);
	X_ALIGN16_MATRIX44F(ViewProjInvMatrix_);

	int width_, height_;

	XRenderThread* m_pRt;
	VidMemManager vidMemMng_;

	XViewPort ViewPort_;
	XCamera cam_;

	XTextDrawList textDrawList_;

	// font texture index.
	int fontIdx_;

	// default font baby.
	font::IFFont* pDefaultFont_;
};

extern XRender* gRenDev;


X_NAMESPACE_END

#endif // !_X_RENDER_BASE_H_


