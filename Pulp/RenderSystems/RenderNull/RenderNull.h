#pragma once

#ifndef X_RENDER_NULL_H_
#define X_RENDER_NULL_H_

X_NAMESPACE_BEGIN(render)


class RenderNull : public IRender
{
public:

	virtual bool init(HWND hWnd, uint32_t width, uint32_t hieght, texture::Texturefmt::Enum depthFmt, bool reverseZ) X_FINAL;
	virtual void shutDown(void) X_FINAL;
	virtual void freeResources(void) X_FINAL;
	
	virtual void release(void) X_FINAL;

	virtual void registerVars(void) X_FINAL;
	virtual void registerCmds(void) X_FINAL;

	virtual void renderBegin(void) X_FINAL;
	virtual void renderEnd(void) X_FINAL;

	virtual void submitCommandPackets(CommandBucket<uint32_t>& cmdBucket) X_FINAL;

	virtual Vec2<uint32_t> getDisplayRes(void) const X_FINAL;

	virtual IPixelBuffer* createDepthBuffer(const char* pNickName, Vec2i dim) X_FINAL;
	virtual IPixelBuffer* createColorBuffer(const char* pNickName, Vec2i dim, uint32_t numMips,
		texture::Texturefmt::Enum fmt) X_FINAL;
	virtual void releasePixelBuffer(render::IPixelBuffer* pPixelBuf) X_FINAL;
	
	virtual IRenderTarget* getCurBackBuffer(uint32_t* pIdx = nullptr) X_FINAL;

	virtual VertexBufferHandle createVertexBuffer(uint32_t numElements, uint32_t elementSize, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;
	virtual VertexBufferHandle createVertexBuffer(uint32_t numElements, uint32_t elementSize, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;
	virtual IndexBufferHandle createIndexBuffer(uint32_t numElements, uint32_t elementSize, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;
	virtual IndexBufferHandle createIndexBuffer(uint32_t numElements, uint32_t elementSize, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;

	virtual void destoryVertexBuffer(VertexBufferHandle handle) X_FINAL;
	virtual void destoryIndexBuffer(IndexBufferHandle handle) X_FINAL;

	virtual void getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_FINAL;
	virtual void getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_FINAL;

	virtual ConstantBufferHandle createConstBuffer(shader::XCBuffer& cbuffer, BufUsage::Enum usage) X_FINAL;
	virtual void destoryConstBuffer(ConstantBufferHandle handle) X_FINAL;

	texture::ITexture* getTexture(const char* pName, texture::TextureFlags flags) X_FINAL;
	texture::ITexture* getDefaultTexture(void) const X_FINAL;
	texture::ITexture* createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, const uint8_t* pInitialData = nullptr) X_FINAL;

	shader::IShaderSource* getShaderSource(const char* pName) X_FINAL;
	shader::IHWShader* createHWShader(shader::ShaderType::Enum type, const core::string& entry, shader::IShaderSource* pSourceFile) X_FINAL;
	shader::IShaderPermatation* createPermatation(const shader::ShaderStagesArr& stages) X_FINAL;

	void releaseShaderPermatation(shader::IShaderPermatation* pPerm) X_FINAL;
	void releaseTexture(texture::ITexture* pTex) X_FINAL;

	PassStateHandle createPassState(const RenderTargetFmtsArr& rtfs) X_FINAL;
	void destoryPassState(PassStateHandle handle) X_FINAL;

	StateHandle createState(PassStateHandle passHandle, const shader::IShaderPermatation* pPerm, const StateDesc& state, const TextureState* pTextStates, size_t numStates) X_FINAL;
	void destoryState(StateHandle handle) X_FINAL;

};


extern RenderNull g_NullRender;

X_NAMESPACE_END

#endif //  X_RENDER_NULL_H_