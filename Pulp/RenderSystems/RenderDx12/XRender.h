#pragma once


#ifndef X_RENDER_DX12_H_
#define X_RENDER_DX12_H_

#include <IRender.h>

#include "CommandList.h"
#include "SamplerDesc.h"
#include "Allocators\DescriptorAllocator.h"
#include "Allocators\DynamicDescriptorHeap.h"
#include "Buffers\ColorBuffer.h"
#include "Features.h"

#include "Shader\ShaderManager.h"
#include "Vars\RenderVars.h"
#include "Auxiliary\AuxRender.h"


X_NAMESPACE_DECLARE(texture,
	class TextureManager;
)
X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(render)

class RenderAuxImp;

class XRender : public IRender
{
	static const uint32_t SWAP_CHAIN_BUFFER_COUNT = 3;

	static const DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_R10G10B10A2_UNORM;

public:
	XRender(core::MemoryArenaBase* arena);
	~XRender();

	bool init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height) X_OVERRIDE;
	void shutDown(void) X_OVERRIDE;
	void freeResources(void) X_OVERRIDE;

	void release(void) X_OVERRIDE;

	void registerVars(void) X_OVERRIDE;
	void registerCmds(void) X_OVERRIDE;

	void renderBegin(void) X_OVERRIDE;
	void renderEnd(void) X_OVERRIDE;

	IRenderAux* getAuxRender(AuxRenderer::Enum user) X_OVERRIDE;


	// =============================================
	// ============== OLD API ======================
	// =============================================

	virtual void SetState(StateFlag state) X_OVERRIDE;
	virtual void SetStencilState(StencilState::Value ss) X_OVERRIDE;
	virtual void SetCullMode(CullMode::Enum mode) X_OVERRIDE;
	virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_OVERRIDE;
	virtual void GetViewport(int* x, int* y, int* width, int* height) X_OVERRIDE;
	virtual void SetViewport(int x, int y, int width, int height) X_OVERRIDE;
	virtual void GetViewport(Recti& rect) X_OVERRIDE;
	virtual void SetViewport(const Recti& rect) X_OVERRIDE;
	virtual int getWidth(void) const X_OVERRIDE;
	virtual int getHeight(void) const X_OVERRIDE;
	virtual float getWidthf(void) const X_OVERRIDE;
	virtual float getHeightf(void) const X_OVERRIDE;
	virtual float ScaleCoordX(float value) const X_OVERRIDE;
	virtual float ScaleCoordY(float value) const X_OVERRIDE;
	virtual void ScaleCoord(float& x, float& y) const X_OVERRIDE;
	virtual void ScaleCoord(Vec2f& xy) const X_OVERRIDE;
	virtual void  SetCamera(const XCamera& cam) X_OVERRIDE;
	virtual const XCamera& GetCamera() X_OVERRIDE;
	virtual IRenderAux* GetIRenderAuxGeo(void) X_OVERRIDE;
	virtual texture::ITexture* LoadTexture(const char* path, texture::TextureFlags flags) X_OVERRIDE;
	virtual void ReleaseTexture(texture::TexID id) X_OVERRIDE;
	virtual bool SetTexture(texture::TexID texId) X_OVERRIDE;
	virtual void DrawQuadSS(float x, float y, float width, float height, const Color& col) X_OVERRIDE;
	virtual void DrawQuadSS(const Rectf& rect, const Color& col) X_OVERRIDE;
	virtual void DrawQuadSS(float x, float y, float width, float height, const Color& col, const Color& borderCol) X_OVERRIDE;
	virtual void DrawQuadImageSS(float x, float y, float width, float height, texture::TexID texture_id, const Color& col) X_OVERRIDE;
	virtual void DrawQuadImageSS(const Rectf& rect, texture::TexID texture_id, const Color& col) X_OVERRIDE;
	virtual void DrawRectSS(float x, float y, float width, float height, const Color& col) X_OVERRIDE;
	virtual void DrawRectSS(const Rectf& rect, const Color& col) X_OVERRIDE;
	virtual void DrawLineColorSS(const Vec2f& vPos1, const Color& color1,
		const Vec2f& vPos2, const Color& vColor2) X_OVERRIDE;
	virtual void DrawQuadImage(float x, float y, float width, float height, texture::TexID texture_id, const Color& col) X_OVERRIDE;
	virtual void DrawQuadImage(float x, float y, float width, float height, texture::ITexture* pTexutre, const Color& col) X_OVERRIDE;
	virtual void DrawQuadImage(const Rectf& rect, texture::ITexture* pTexutre, const Color& col) X_OVERRIDE;
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
	virtual void DrawRect(float x, float y, float width, float height, const Color& col) X_OVERRIDE;
	virtual void DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
		float padding, uint32_t max) X_OVERRIDE;
	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* format, va_list args) X_OVERRIDE;
	virtual void DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* text) X_OVERRIDE;
	virtual void DrawAllocStats(Vec3f pos, const XDrawTextInfo& ti,
		const core::MemoryAllocatorStatistics& allocStats, const char* title) X_OVERRIDE;
	virtual void FlushTextBuffer(void) X_OVERRIDE;
	virtual int FontCreateTexture(const Vec2i& size, BYTE *pData,
		texture::Texturefmt::Enum eTF = texture::Texturefmt::R8G8B8A8, bool genMips = false) X_OVERRIDE;
	virtual bool FontUpdateTexture(int texId, int X, int Y, int USize, int VSize, uint8_t* pData) X_OVERRIDE;
	virtual bool FontSetTexture(int texId) X_OVERRIDE;
	virtual bool FontSetRenderingState() X_OVERRIDE;
	virtual void FontRestoreRenderingState() X_OVERRIDE;
	virtual void FontSetBlending() X_OVERRIDE;
	virtual void DrawStringW(font::IXFont_RenderProxy* pFont, const Vec3f& pos,
		const wchar_t* pStr, const font::XTextDrawConect& ctx) const X_OVERRIDE;
	virtual void DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
		PrimitiveTypePublic::Enum type) X_OVERRIDE;
	virtual shader::XShaderItem LoadShaderItem(shader::XInputShaderResources& res) X_OVERRIDE;
	virtual bool DefferedBegin(void) X_OVERRIDE;
	virtual bool DefferedEnd(void) X_OVERRIDE;
	virtual bool SetWorldShader(void) X_OVERRIDE;
	virtual bool setGUIShader(bool textured = false) X_OVERRIDE;
	virtual model::IRenderMesh* createRenderMesh(void) X_OVERRIDE;
	virtual model::IRenderMesh* createRenderMesh(const model::MeshHeader* pMesh,
		shader::VertexFormat::Enum fmt, const char* name) X_OVERRIDE;
	virtual void freeRenderMesh(model::IRenderMesh* pMesh) X_OVERRIDE;
	virtual void SetModelMatrix(const Matrix44f& mat) X_OVERRIDE;

	// =============================================
	// =============================================

private:
	bool freeSwapChainResources(void);

	bool initRenderBuffers(Vec2<uint32_t> res);
	bool resize(uint32_t width, uint32_t height);
	void handleResolutionChange(void);
	void populateFeatureInfo(void);
	bool deviceIsSupported(void) const;

private:
	void Cmd_ListDeviceFeatures(core::IConsoleCmdArgs* pCmd);


private:
	core::MemoryArenaBase* arena_;
	ID3D12Device* pDevice_;
	ID3D12Debug* pDebug_;
	IDXGISwapChain1* pSwapChain_;

	RenderVars vars_;

	shader::XShaderManager shaderMan_; // might make this allocoated, just so can forward declare like tex man.
	texture::TextureManager* pTextureMan_;
	RenderAuxImp* pAuxRender_;

	CommandListManger cmdListManager_;

	core::StackString<128, wchar_t> deviceName_;
	size_t dedicatedvideoMemory_;

	DescriptorAllocator* pDescriptorAllocator_;
	DescriptorAllocatorPool* pDescriptorAllocatorPool_;

	Vec2<uint32_t> currentNativeRes_;
	Vec2<uint32_t> targetNativeRes_;
	Vec2<uint32_t> displayRes_;
	ColorBuffer displayPlane_[SWAP_CHAIN_BUFFER_COUNT];
	uint32_t currentBufferIdx_;

	SamplerDesc samplerLinearWrapDesc_;
	SamplerDesc samplerAnisoWrapDesc_;
	SamplerDesc samplerShadowDesc_;
	SamplerDesc samplerLinearClampDesc_;
	SamplerDesc samplerVolumeWrapDesc_;
	SamplerDesc samplerPointClampDesc_;
	SamplerDesc samplerPointBorderDesc_;
	SamplerDesc samplerLinearBorderDesc_;

	SamplerDescriptor samplerLinearWrap_;
	SamplerDescriptor samplerAnisoWrap_;
	SamplerDescriptor samplerShadow_;
	SamplerDescriptor samplerLinearClamp_;
	SamplerDescriptor samplerVolumeWrap_;
	SamplerDescriptor samplerPointClamp_;
	SamplerDescriptor samplerPointBorder_;
	SamplerDescriptor samplerLinearBorder_;

	D3D_FEATURE_LEVEL featureLvl_;
	GpuFeatures features_;

	RenderAux auxQues_[AuxRenderer::ENUM_COUNT];
};


X_NAMESPACE_END

#endif // !X_RENDER_DX12_H_