#pragma once

#ifndef _X_RENDER_BASE_H_
#define _X_RENDER_BASE_H_

#include "DeviceManager\DeviceManager.h"
#include "DeviceManager\VidMemManager.h"
#include "TextDrawList.h"

#include <Containers\Array.h>

#include <Math\XRect.h>
#include <Math\XMatrix44.h>
#include <Math\XViewPort.h>
#include <Math\VertexFormats.h>

#include "Shader\XShader.h"

X_NAMESPACE_BEGIN(render)

struct RenderResource
{
    core::StackString<28> name;
    texture::Texturefmt::Enum fmt;
    Vec2f scale;
};

class XRender : public IRender
{
public:
    XRender();
    virtual ~XRender();

    virtual bool Init(HWND hWnd, uint32_t width, uint32_t hieght) X_OVERRIDE;
    virtual void ShutDown(void) X_OVERRIDE;
    virtual void freeResources(void) X_OVERRIDE;

    virtual void RenderBegin(void) X_OVERRIDE;
    virtual void RenderEnd(void) X_OVERRIDE;

    virtual void SetState(StateFlag state) X_ABSTRACT;
    virtual void SetStencilState(StencilState::Value ss) X_ABSTRACT;

    virtual void SetCullMode(CullMode::Enum mode) X_ABSTRACT;
    virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_ABSTRACT;

    // ViewPort
    virtual void GetViewport(int* left, int* top, int* right, int* bottom) X_OVERRIDE;
    virtual void SetViewport(int left, int top, int right, int bottom) X_OVERRIDE;
    virtual void GetViewport(Recti& rect) X_OVERRIDE;
    virtual void SetViewport(const Recti& rect) X_OVERRIDE;

    X_INLINE virtual int getWidth(void) const X_OVERRIDE;
    X_INLINE virtual int getHeight(void) const X_OVERRIDE;
    X_INLINE virtual float getWidthf(void) const X_OVERRIDE;
    X_INLINE virtual float getHeightf(void) const X_OVERRIDE;
    // ~ViewPort

    // scales from 800x600
    X_INLINE virtual float ScaleCoordX(float value) const X_OVERRIDE;
    X_INLINE virtual float ScaleCoordY(float value) const X_OVERRIDE;
    X_INLINE virtual void ScaleCoord(float& x, float& y) const X_OVERRIDE;
    X_INLINE virtual void ScaleCoord(Vec2f& xy) const X_OVERRIDE;

    // none virtual versions for this lib.
    X_INLINE float ScaleCoordXInternal(float value) const;
    X_INLINE float ScaleCoordYInternal(float value) const;
    X_INLINE void ScaleCoordInternal(float& x, float& y) const;
    X_INLINE void ScaleCoordInternal(Vec2f& xy) const;

    // AuxGeo
    virtual IRenderAux* GetIRenderAuxGeo(void) X_ABSTRACT;
    // ~AuxGeo

    virtual void GetModelViewMatrix(Matrix44f* pMat) X_ABSTRACT;
    virtual void GetProjectionMatrix(Matrix44f* pMat) X_ABSTRACT;

    virtual void FX_PipelineShutdown(void) X_ABSTRACT;

    // Textures
    virtual texture::ITexture* LoadTexture(const char* path, texture::TextureFlags flags) X_OVERRIDE;

    virtual void ReleaseTexture(texture::TexID id) X_ABSTRACT;
    // ~Textures

    // Camera
    virtual void SetCamera(const XCamera& cam) X_ABSTRACT;
    X_INLINE virtual const XCamera& GetCamera(void) X_OVERRIDE;
    // ~Camera

    // font
    virtual int FontCreateTexture(const Vec2i& size, BYTE* pData,
        texture::Texturefmt::Enum eTF = texture::Texturefmt::R8G8B8A8, bool genMips = false) X_OVERRIDE;

    virtual bool FontUpdateTexture(int texId, int X, int Y, int USize,
        int VSize, uint8_t* pData) X_ABSTRACT;

    virtual bool FontSetTexture(int texId) X_ABSTRACT;
    virtual bool FontSetRenderingState(void) X_ABSTRACT;
    virtual void FontRestoreRenderingState(void) X_ABSTRACT;
    virtual void FontSetBlending(void) X_ABSTRACT;

    //	virtual void DrawStringW(font::IXFont_RenderProxy* pFont, const Vec3f& pos,
    //		const wchar_t* pStr, const font::TextDrawContext& ctx) const X_OVERRIDE;

    void Draw2dText(float posX, float posY, const char* pStr, const XDrawTextInfo& ti);
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

    X_INLINE VidMemManager* VidMemMng(void);

    virtual void DrawLines(Vec3f* points, uint32_t num, const Colorf& col) X_ABSTRACT;
    virtual void DrawString(const Vec3f& pos, const char* pStr) X_ABSTRACT;

    X_INLINE Matrix44f* pViewMatrix(void);
    X_INLINE Matrix44f* pProjMatrix(void);
    X_INLINE Matrix44f* pViewProjMatrix(void);
    X_INLINE Matrix44f* pViewProjInvMatrix(void);

    static uint32_t vertexFormatStride[shader::VertexFormat::Num];
    static uint32_t vertexSteamStride[VertexStream::ENUM_COUNT][shader::VertexFormat::Num];

protected:
    void RegisterVars(void);

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
    font::IFont* pDefaultFont_;

    // vars
    Colorf r_clear_color;
};

extern XRender* gRenDev;

X_NAMESPACE_END

#include "XRender.inl"

#endif // !_X_RENDER_BASE_H_
