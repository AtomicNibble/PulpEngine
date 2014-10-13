#pragma once

#ifndef X_RENDER_NULL_H_
#define X_RENDER_NULL_H_

X_NAMESPACE_BEGIN(render)


class RenderNull : public IRender
{
public:

	virtual bool Init(HWND hWnd, uint32_t width, uint32_t hieght) X_OVERRIDE;
	virtual void ShutDown() X_OVERRIDE;
	virtual void freeResources() X_OVERRIDE;


	virtual void RenderBegin() X_OVERRIDE;
	virtual void RenderEnd() X_OVERRIDE;

	virtual bool FlushRenderThreadCommands(bool wait) X_OVERRIDE;


	virtual void SetState(StateFlag state) X_OVERRIDE;
	virtual void SetStencilState(StencilState::Value ss) X_OVERRIDE;
	virtual void SetCullMode(CullMode::Enum mode) X_OVERRIDE;
	virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_OVERRIDE;

	virtual void GetViewport(int* x, int* y, int* width, int* height) X_OVERRIDE;
	virtual void SetViewport(int x, int y, int width, int height) X_OVERRIDE;
	virtual void GetViewport(Vec4<int>& viewport) X_OVERRIDE;
	virtual void SetViewport(const Vec4<int>& viewport) X_OVERRIDE;


	virtual int getWidth(void) const X_FINAL;
	virtual int getHeight(void) const X_FINAL;

	virtual void  SetCamera(const XCamera& cam) X_OVERRIDE;
	virtual const XCamera& GetCamera() X_OVERRIDE;
	
	// AuxGeo
	virtual IRenderAux* GetIRenderAuxGeo(void) X_FINAL;
	// ~AuxGeo

	// Textures 
	virtual texture::ITexture* LoadTexture(const char* path, texture::TextureFlags flags) X_OVERRIDE;

	virtual void Draw2dImage(float xpos, float ypos,
		float w, float h, texture::TexID texture_id, ColorT<float>& col) X_OVERRIDE;

	virtual void ReleaseTexture(texture::TexID id) X_OVERRIDE;
	// ~Textures

	// Drawing
	virtual void DrawQuad(float x, float y, float z, float width, float height, const Color& col) X_OVERRIDE;
	virtual void DrawQuad(float x, float y, float z, float width, float height, const Color& col, const Color& borderCol) X_OVERRIDE;
	virtual void DrawQuad(float x, float y, float width, float height, const Color& col) X_OVERRIDE;
	virtual void DrawQuad(float x, float y, float width, float height, const Color& col, const Color& borderCol) X_OVERRIDE;
	virtual void DrawQuad(Vec2<float> pos, float width, float height, const Color& col) X_OVERRIDE;
	virtual void DrawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2, const Vec3f& pos3, const Color& col) X_OVERRIDE;

	virtual void DrawLines(Vec3f* points, uint32_t num, const Color& col) X_OVERRIDE;
	virtual void DrawLine(const Vec3f& pos1, const Vec3f& pos2) X_OVERRIDE;
	virtual void DrawLineColor(const Vec3f& vPos1, const Color& color1,
		const Vec3f& vPos2, const Color& vColor2) X_OVERRIDE;

	virtual void DrawRect(float x, float y, float width, float height, Color col) X_OVERRIDE;

	virtual void DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
		float padding, uint32_t max) X_OVERRIDE;

	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* format, va_list args) X_OVERRIDE;
	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* text) X_OVERRIDE;

	virtual void DrawAllocStats(Vec3f pos, const XDrawTextInfo& ti,
		const core::MemoryAllocatorStatistics& allocStats, const char* title) X_FINAL;

	virtual void FlushTextBuffer(void) X_FINAL;
	// ~Drawing

	// Font
	virtual int FontCreateTexture(const Vec2i& size, BYTE *pData,
		texture::Texturefmt::Enum eTF = texture::Texturefmt::R8G8B8A8, bool genMips = false) X_OVERRIDE;

	virtual bool FontUpdateTexture(int texId, int X, int Y, int USize, int VSize, uint8_t* pData) X_OVERRIDE;
	virtual bool FontSetTexture(int texId) X_OVERRIDE;
	virtual void FontSetRenderingState() X_OVERRIDE;
	virtual void FontRestoreRenderingState() X_OVERRIDE;
	virtual void FontSetBlending() X_OVERRIDE;
	virtual void DrawStringW(font::IXFont_RenderProxy* pFont, const Vec3f& pos,
		const wchar_t* pStr, const font::XTextDrawConect& ctx) const X_OVERRIDE;
	// ~Font


	// Shader Stuff

	virtual shader::XShaderItem LoadShaderItem(shader::XInputShaderResources& res) X_OVERRIDE;

	// ~Shader Stuff

	// Model
	virtual model::IRenderMesh* createRenderMesh(void) X_OVERRIDE;
	virtual void freeRenderMesh(model::IRenderMesh* pMesh) X_OVERRIDE;

	// ~Model

	virtual void DrawVB(Vertex_P3F_C4B_T2F* pVertBuffer, uint32_t size,
		PrimitiveTypePublic::Enum type) X_OVERRIDE;

private:
	XCamera cam_;
};


extern RenderNull g_NullRender;

X_NAMESPACE_END

#endif //  X_RENDER_NULL_H_