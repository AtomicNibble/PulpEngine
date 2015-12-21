#pragma once

#ifndef _X_RENDER_BASE_H_
#define _X_RENDER_BASE_H_

#include "DeviceManager\DeviceManager.h"
#include "DeviceManager\VidMemManager.h"
#include "ReaderThread.h"
#include "TextDrawList.h"
#include "Math\XMatrix44.h"

#include <Containers\Array.h>

#include <Math\XRect.h>

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
	X_INLINE bool operator==(const XViewPort& oth) const {
		return z == oth.z; // && view == oth.view;
	}

	void setZ(float32_t near_, float32_t far_) {
		z.x = near_;
		z.y = far_;
	}

	void set(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) {
		view.set(left, top, right, bottom);
		viewf.set(
			static_cast<float>(left),
			static_cast<float>(top),
			static_cast<float>(right),
			static_cast<float>(bottom));

	}
	void set(uint32_t width, uint32_t height) {
		set(0, 0, width, height);
	}
	void set(const Vec2<uint32_t>& wh) {
		set(wh.x,wh.y);
	}

	X_INLINE int getWidth(void) const {
		return view.getWidth();
	}
	X_INLINE int getHeight(void) const {
		return view.getHeight();
	}
	X_INLINE float getWidthf(void) const {
		return viewf.getWidth();
	}
	X_INLINE float getHeightf(void) const {
		return viewf.getHeight();
	}

	X_INLINE Recti getRect(void) {
		return view; 
	}
	X_INLINE const Recti& getRect(void) const {
		return view;
	} 

	X_INLINE float getZNear(void) const {
		return z.x;
	}
	X_INLINE float getZFar(void) const {
		return z.y;
	}
private:
	Recti view;
	Rectf viewf;
	Vec2f z;
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

	virtual void SetState(StateFlag state) X_ABSTRACT;
	virtual void SetStencilState(StencilState::Value ss) X_ABSTRACT;

	virtual void SetCullMode(CullMode::Enum mode) X_ABSTRACT;
	virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_ABSTRACT;


	// ViewPort
	virtual void GetViewport(int* left, int* top, int* right, int* bottom) X_OVERRIDE;
	virtual void SetViewport(int left, int top, int right, int bottom) X_OVERRIDE;
	virtual void GetViewport(Recti& rect) X_OVERRIDE;
	virtual void SetViewport(const Recti& rect) X_OVERRIDE;

	virtual int getWidth(void) const X_OVERRIDE{ return ViewPort_.getWidth(); }
	virtual int getHeight(void) const X_OVERRIDE{ return ViewPort_.getHeight(); }
	virtual float getWidthf(void) const X_OVERRIDE{ return ViewPort_.getWidthf(); }
	virtual float getHeightf(void) const X_OVERRIDE{ return ViewPort_.getHeightf(); }
	// ~ViewPort

	// scales from 800x600
	virtual float ScaleCoordX(float value) const X_OVERRIDE{ return ScaleCoordXInternal(value); }
	virtual float ScaleCoordY(float value) const X_OVERRIDE{ return ScaleCoordYInternal(value); }
	virtual void ScaleCoord(float& x, float& y) const X_OVERRIDE{ ScaleCoordInternal(x, y); }
	virtual void ScaleCoord(Vec2f& xy) const X_OVERRIDE{ ScaleCoordInternal(xy); }

	// none virtual versions for this lib.
	X_INLINE float ScaleCoordXInternal(float value) const
	{
		value *= ViewPort_.getWidthf() / 800.0f;
		return (value);
	}
	X_INLINE float ScaleCoordYInternal(float value) const
	{
		value *= ViewPort_.getHeightf() / 600.0f;
		return (value);
	}
	X_INLINE void ScaleCoordInternal(float& x, float& y) const
	{
		x = ScaleCoordXInternal(x);
		y = ScaleCoordYInternal(y);
	}
	X_INLINE void ScaleCoordInternal(Vec2f& xy) const
	{
		xy.x = ScaleCoordXInternal(xy.x);
		xy.y = ScaleCoordYInternal(xy.y);
	}

	// AuxGeo
	virtual IRenderAux* GetIRenderAuxGeo(void) X_ABSTRACT;
	// ~AuxGeo

	virtual void GetModelViewMatrix(Matrix44f* pMat) X_ABSTRACT;
	virtual void GetProjectionMatrix(Matrix44f* pMat) X_ABSTRACT;

	virtual void FX_PipelineShutdown() X_ABSTRACT;

	// Textures 
	virtual texture::ITexture* LoadTexture(const char* path, texture::TextureFlags flags) X_OVERRIDE;
	

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
	virtual model::IRenderMesh* createRenderMesh(const model::MeshHeader* pMesh,
		shader::VertexFormat::Enum fmt, const char* name) X_OVERRIDE;
	virtual void freeRenderMesh(model::IRenderMesh* pMesh) X_OVERRIDE;

	// ~Model

	// Drawing Util

	virtual void DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
		texture::TexID texture_id, const float* s, const float* t, const Colorf& col, bool filtered = true) X_ABSTRACT;

	virtual void DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
		PrimitiveTypePublic::Enum type) X_ABSTRACT;

	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* format, va_list args) X_OVERRIDE;
	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* text) X_OVERRIDE;
	
	virtual void DrawAllocStats(Vec3f pos, const XDrawTextInfo& ti,
		const core::MemoryAllocatorStatistics& allocStats, const char* title) X_OVERRIDE;

	virtual void FlushTextBuffer(void) X_OVERRIDE;

	// ~Drawing


	VidMemManager* VidMemMng(void) {
		return &vidMemMng_;
	}


	// RT

	virtual void RT_DrawLines(Vec3f* points, uint32_t num, const Colorf& col) X_ABSTRACT;

	virtual void RT_DrawString(const Vec3f& pos, const char* pStr) X_ABSTRACT;

	virtual void RT_SetState(StateFlag state) X_ABSTRACT;
	virtual void RT_SetCullMode(CullMode::Enum mode) X_ABSTRACT;

	virtual void RT_DrawImageWithUV(float xpos, float ypos, float z, float w, float h,
		texture::TexID texture_id, const float *s, const float *t, const Colorf& col, bool filtered = true) X_ABSTRACT;

	virtual void RT_SetCameraInfo(void) X_ABSTRACT;

	virtual void RT_FlushTextBuffer(void); // not ab

	// ~RT

	X_INLINE Matrix44f* pViewMatrix() { return &ViewMatrix_; }
	X_INLINE Matrix44f* pProjMatrix() { return &ProjMatrix_; }
	X_INLINE Matrix44f* pViewProjMatrix() { return &ViewProjMatrix_; }
	X_INLINE Matrix44f* pViewProjInvMatrix() { return &ViewProjInvMatrix_; }



	static uint32_t vertexFormatStride[shader::VertexFormat::Num];
	static uint32_t vertexSteamStride[VertexStream::ENUM_COUNT][shader::VertexFormat::Num];
protected:

	virtual void SetArenas(core::MemoryArenaBase* arena);

	bool LoadResourceDeffintion(void);

public:
	shader::XShaderManager ShaderMan_;

	core::Array<RenderResource> RenderResources_;
protected:

	// Warning Render object was padded due to alignments.
	X_DISABLE_WARNING(4324)

	X_ALIGN16_MATRIX44F(ViewMatrix_);
	X_ALIGN16_MATRIX44F(ProjMatrix_);
	X_ALIGN16_MATRIX44F(ViewProjMatrix_);
	X_ALIGN16_MATRIX44F(ViewProjInvMatrix_);

	X_ENABLE_WARNING(4324)

	VidMemManager vidMemMng_;

	// stores the with + height and the znear + zfar
	XViewPort ViewPort_;
	XCamera cam_;

	XTextDrawList* pTextDrawList_;

	// font texture index.
	int fontIdx_;

	// default font baby.
	font::IFFont* pDefaultFont_;
};

extern XRender* gRenDev;


X_NAMESPACE_END

#endif // !_X_RENDER_BASE_H_


