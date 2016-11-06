#pragma once

#ifndef X_RENDER_NULL_H_
#define X_RENDER_NULL_H_

X_NAMESPACE_BEGIN(render)


class RenderNull : public IRender
{
public:

	virtual bool init(HWND hWnd, uint32_t width, uint32_t hieght) X_OVERRIDE;
	virtual void shutDown(void) X_OVERRIDE;
	virtual void freeResources(void) X_OVERRIDE;
	
	virtual void release(void) X_OVERRIDE;

	virtual void registerVars(void) X_OVERRIDE;
	virtual void registerCmds(void) X_OVERRIDE;

	virtual void renderBegin(void) X_OVERRIDE;
	virtual void renderEnd(void) X_OVERRIDE;

	virtual void submitCommandPackets(CommandBucket<uint32_t>& cmdBucket) X_OVERRIDE;

	virtual IRenderAux* getAuxRender(AuxRenderer::Enum user) X_OVERRIDE;

	virtual Vec2<uint32_t> getDisplayRes(void) const X_OVERRIDE;

	virtual IRenderTarget* createRenderTarget() X_OVERRIDE;
	virtual void destoryRenderTarget(IRenderTarget* pRT) X_OVERRIDE;
	virtual IRenderTarget* getCurBackBuffer(uint32_t* pIdx = nullptr) X_OVERRIDE;

	virtual VertexBufferHandle createVertexBuffer(uint32_t numElements, uint32_t elementSize, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_OVERRIDE;
	virtual VertexBufferHandle createVertexBuffer(uint32_t numElements, uint32_t elementSize, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_OVERRIDE;
	virtual IndexBufferHandle createIndexBuffer(uint32_t numElements, uint32_t elementSize, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_OVERRIDE;
	virtual IndexBufferHandle createIndexBuffer(uint32_t numElements, uint32_t elementSize, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_OVERRIDE;

	virtual void destoryVertexBuffer(VertexBufferHandle handle) X_OVERRIDE;
	virtual void destoryIndexBuffer(IndexBufferHandle handle) X_OVERRIDE;

	virtual void getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_OVERRIDE;
	virtual void getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_OVERRIDE;


	texture::ITexture* getTexture(const char* pName, texture::TextureFlags flags) X_OVERRIDE;
	texture::ITexture* createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, const uint8_t* pInitialData = nullptr) X_OVERRIDE;
	shader::IShader* getShader(const char* pName) X_OVERRIDE;

	void releaseTexture(texture::ITexture* pTex) X_OVERRIDE;
	void releaseShader(shader::IShader* pShader) X_OVERRIDE;

	PassStateHandle createPassState(const RenderTargetFmtsArr& rtfs) X_OVERRIDE;
	void destoryPassState(PassStateHandle handle) X_OVERRIDE;

	StateHandle createState(PassStateHandle passHandle, const shader::IShaderTech* pTech, const StateDesc& state, const TextureState* pTextStates, size_t numStates) X_OVERRIDE;
	void destoryState(StateHandle handle) X_OVERRIDE;

	// =============================================
	// ============== OLD API ======================
	// =============================================

	virtual void Set2D(bool value, float znear = -1e10f, float zfar = 1e10f) X_OVERRIDE;

	virtual void GetViewport(int* x, int* y, int* width, int* height) X_OVERRIDE;
	virtual void SetViewport(int x, int y, int width, int height) X_OVERRIDE;
	virtual void GetViewport(Recti& rect) X_OVERRIDE;
	virtual void SetViewport(const Recti& rect) X_OVERRIDE;


	virtual int getWidth(void) const X_OVERRIDE;
	virtual int getHeight(void) const X_OVERRIDE;
	virtual float getWidthf(void) const X_OVERRIDE;
	virtual float getHeightf(void) const X_OVERRIDE;
	// ~ViewPort

	// scales from 800x600 range to what ever res.
	// 400(x) on 1650x1050 becomes 825
	virtual float ScaleCoordX(float value) const X_OVERRIDE;
	virtual float ScaleCoordY(float value) const X_OVERRIDE;
	virtual void ScaleCoord(float& x, float& y) const X_OVERRIDE;
	virtual void ScaleCoord(Vec2f& xy) const X_OVERRIDE;


	virtual void  SetCamera(const XCamera& cam) X_OVERRIDE;
	virtual const XCamera& GetCamera() X_OVERRIDE;
	
	// AuxGeo
	virtual IRenderAux* GetIRenderAuxGeo(void) X_OVERRIDE;
	// ~AuxGeo

	// Drawing


private:
	XCamera cam_;
};


extern RenderNull g_NullRender;

X_NAMESPACE_END

#endif //  X_RENDER_NULL_H_