#pragma once

#ifndef X_RENDER_NULL_H_
#define X_RENDER_NULL_H_

X_NAMESPACE_BEGIN(render)


class RenderNull : public IRender
{
public:

	bool init(HWND hWnd, uint32_t width, uint32_t hieght, texture::Texturefmt::Enum depthFmt, bool reverseZ) X_FINAL;
	void shutDown(void) X_FINAL;
	void freeResources(void) X_FINAL;
	
	void release(void) X_FINAL;

	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	void renderBegin(void) X_FINAL;
	void renderEnd(void) X_FINAL;

	void submitCommandPackets(CommandBucket<uint32_t>& cmdBucket) X_FINAL;

	Vec2<uint32_t> getDisplayRes(void) const X_FINAL;

	IPixelBuffer* createDepthBuffer(const char* pNickName, Vec2i dim) X_FINAL;
	IPixelBuffer* createColorBuffer(const char* pNickName, Vec2i dim, uint32_t numMips,
		texture::Texturefmt::Enum fmt) X_FINAL;
	void releasePixelBuffer(render::IPixelBuffer* pPixelBuf) X_FINAL;
	
	IRenderTarget* getCurBackBuffer(uint32_t* pIdx = nullptr) X_FINAL;

	VertexBufferHandle createVertexBuffer(uint32_t numElements, uint32_t elementSize, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;
	VertexBufferHandle createVertexBuffer(uint32_t numElements, uint32_t elementSize, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;
	IndexBufferHandle createIndexBuffer(uint32_t numElements, uint32_t elementSize, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;
	IndexBufferHandle createIndexBuffer(uint32_t numElements, uint32_t elementSize, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;

	void destoryVertexBuffer(VertexBufferHandle handle) X_FINAL;
	void destoryIndexBuffer(IndexBufferHandle handle) X_FINAL;

	void getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_FINAL;
	void getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_FINAL;

	ConstantBufferHandle createConstBuffer(const shader::XCBuffer& cbuffer, BufUsage::Enum usage) X_FINAL;
	void destoryConstBuffer(ConstantBufferHandle handle) X_FINAL;

	IDeviceTexture* getDeviceTexture(int32_t id) X_FINAL;
	IDeviceTexture* createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, const uint8_t* pInitialData = nullptr) X_FINAL;

	bool initDeviceTexture(IDeviceTexture* pTex) X_FINAL;
	bool initDeviceTexture(IDeviceTexture* pTex, const texture::XTextureFile& imgFile) X_FINAL;

	shader::IShaderSource* getShaderSource(const core::string& name) X_FINAL;
	shader::IHWShader* createHWShader(shader::ShaderType::Enum type, const core::string& entry, shader::IShaderSource* pSourceFile, shader::PermatationFlags permFlags) X_FINAL;
	shader::IShaderPermatation* createPermatation(const shader::ShaderStagesArr& stages) X_FINAL;

	void releaseShaderPermatation(shader::IShaderPermatation* pPerm) X_FINAL;
	void releaseTexture(IDeviceTexture* pTex) X_FINAL;

	PassStateHandle createPassState(const RenderTargetFmtsArr& rtfs) X_FINAL;
	void destoryPassState(PassStateHandle handle) X_FINAL;

	StateHandle createState(PassStateHandle passHandle, const shader::IShaderPermatation* pPerm, const StateDesc& state, const TextureState* pTextStates, size_t numStates) X_FINAL;
	void destoryState(StateHandle handle) X_FINAL;

};


extern RenderNull g_NullRender;

X_NAMESPACE_END

#endif //  X_RENDER_NULL_H_