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

	virtual ConstantBufferHandle createConstBuffer(shader::XCBuffer& cbuffer, BufUsage::Enum usage) X_OVERRIDE;
	virtual void destoryConstBuffer(ConstantBufferHandle handle) X_OVERRIDE;

	texture::ITexture* getTexture(const char* pName, texture::TextureFlags flags) X_OVERRIDE;
	texture::ITexture* createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, const uint8_t* pInitialData = nullptr) X_OVERRIDE;

	shader::IShaderSource* getShaderSource(const char* pName) X_OVERRIDE;
	shader::IHWShader* createHWShader(shader::ShaderType::Enum type, const core::string& entry, shader::IShaderSource* pSourceFile) X_OVERRIDE;
	shader::IShaderPermatation* createPermatation(shader::IHWShader* pVertex, shader::IHWShader* pPixel) X_OVERRIDE;
	shader::IShaderPermatation* createPermatation(const shader::ShaderStagesArr& stages) X_OVERRIDE;

	void releaseShaderPermatation(shader::IShaderPermatation* pPerm) X_OVERRIDE;
	void releaseTexture(texture::ITexture* pTex) X_OVERRIDE;

	PassStateHandle createPassState(const RenderTargetFmtsArr& rtfs) X_OVERRIDE;
	void destoryPassState(PassStateHandle handle) X_OVERRIDE;

	StateHandle createState(PassStateHandle passHandle, const shader::IShaderPermatation* pPerm, const StateDesc& state, const TextureState* pTextStates, size_t numStates) X_OVERRIDE;
	void destoryState(StateHandle handle) X_OVERRIDE;

private:
	XCamera cam_;
};


extern RenderNull g_NullRender;

X_NAMESPACE_END

#endif //  X_RENDER_NULL_H_