#pragma once


#ifndef X_RENDER_DX12_H_
#define X_RENDER_DX12_H_

#include <IRender.h>
#include <IRenderCommands.h>

#include "CommandListManger.h"
#include "SamplerDesc.h"
#include "Allocators\DescriptorAllocator.h"
#include "Allocators\DynamicDescriptorHeap.h"
#include "Buffers\ColorBuffer.h"
#include "Features.h"

#include "Vars\RenderVars.h"
#include "Auxiliary\AuxRender.h"
#include "RootSignature.h"

X_NAMESPACE_DECLARE(texture,
	class TextureManager;
)
X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(render)

namespace shader
{
	class XShaderManager;
} // namespace shader

class RenderAuxImp;
class ContextManager;
class LinearAllocatorManager;
class BufferManager;
class RootSignatureDeviceCache;
class PSODeviceCache;

class GraphicsContext;

template<typename>
class CommandBucket;


class XRender : public IRender
{
	static const uint32_t SWAP_CHAIN_BUFFER_COUNT = 3;

	static const DXGI_FORMAT SWAP_CHAIN_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM; //  DXGI_FORMAT_R10G10B10A2_UNORM;

	typedef core::FixedArray<D3D12_INPUT_ELEMENT_DESC, 12> VertexLayoutDescArr;
	typedef std::array<VertexLayoutDescArr, shader::VertexFormat::ENUM_COUNT> VertexFormatILArr;
	typedef std::array<ConstantBufferHandle, MAX_CONST_BUFFERS_BOUND> ConstBuffersArr;
	typedef VertexBufferHandleArr VertexHandleArr;


	struct PassState
	{
		RenderTargetFmtsArr rtfs;

	};

	struct DeviceState
	{
		typedef core::Array<TextureState> TextureStateArray;

		DeviceState(core::MemoryArenaBase* arena) :
			rootSig(arena),
			texStates(arena)
		{}


		RootSignature rootSig;
		ID3D12PipelineState* pPso;

		D3D12_PRIMITIVE_TOPOLOGY topo;

		// we want to store also the textures and samplers you want to use slut.
		// this won't stay as vector, just no point doing it fast way as might refactor before done.
		TextureStateArray texStates;
	};

	struct State
	{
		State() {
			reset();
		}

		void reset() {
			handle = INVALID_BUF_HANLDE;
			indexBuffer = INVALID_BUF_HANLDE;
			vertexBuffers.fill(INVALID_BUF_HANLDE);
			constBuffers.fill(INVALID_BUF_HANLDE);

			pRootSig = nullptr;
			pPso = nullptr;
			topo = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}

		StateHandle handle;
		IndexBufferHandle indexBuffer;
		VertexHandleArr vertexBuffers;
		ConstBuffersArr constBuffers; // rootSig indexed

		ID3D12RootSignature* pRootSig;
		ID3D12PipelineState* pPso;
		D3D12_PRIMITIVE_TOPOLOGY topo;
	};


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

	void submitCommandPackets(CommandBucket<uint32_t>& cmdBucket) X_OVERRIDE;

	IRenderAux* getAuxRender(AuxRenderer::Enum user) X_OVERRIDE;

	Vec2<uint32_t> getDisplayRes(void) const X_OVERRIDE;

	IRenderTarget* createRenderTarget(void) X_OVERRIDE;
	void destoryRenderTarget(IRenderTarget* pRT) X_OVERRIDE;
	IRenderTarget* getCurBackBuffer(uint32_t* pIdx = nullptr) X_OVERRIDE;


	VertexBufferHandle createVertexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_OVERRIDE;
	VertexBufferHandle createVertexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_OVERRIDE;
	IndexBufferHandle createIndexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_OVERRIDE;
	IndexBufferHandle createIndexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_OVERRIDE;

	void destoryVertexBuffer(VertexBufferHandle handle) X_OVERRIDE;
	void destoryIndexBuffer(IndexBufferHandle handle) X_OVERRIDE;

	void getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_OVERRIDE;
	void getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_OVERRIDE;

	// cb's
	ConstantBufferHandle createConstBuffer(uint32_t size, BufUsage::Enum usage) X_OVERRIDE;
	void destoryConstBuffer(ConstantBufferHandle handle) X_OVERRIDE;


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

	virtual IRenderAux* GetIRenderAuxGeo(void) X_OVERRIDE;



private:
	void CreateVBView(GraphicsContext& context, const VertexHandleArr& vertexBuffers,
		D3D12_VERTEX_BUFFER_VIEW viewsOut[VertexStream::ENUM_COUNT], uint32_t& numVertexStreams);

	void ApplyState(GraphicsContext& context, State& curState, const StateHandle handle,
		const VertexHandleArr& vertexBuffers);

private:
	bool freeSwapChainResources(void);

	void initILDescriptions(void);
	bool initRenderBuffers(Vec2<uint32_t> res);
	bool resize(uint32_t width, uint32_t height);
	void handleResolutionChange(void);
	void populateFeatureInfo(void);
	bool deviceIsSupported(void) const;

private:

//	static void createDescFromState(StateFlag state, D3D12_BLEND_DESC& blendDesc);
//	static void createDescFromState(StateFlag state, D3D12_RASTERIZER_DESC& rasterizerDesc);
//	static void createDescFromState(StateFlag state, StencilState stencilState, D3D12_DEPTH_STENCIL_DESC& depthStencilDesc);

private:
	void Cmd_ListDeviceFeatures(core::IConsoleCmdArgs* pCmd);


private:
	core::MemoryArenaBase* arena_;
	ID3D12Device* pDevice_;
	IDXGIAdapter3* pAdapter_;
	IDXGISwapChain3* pSwapChain_;

	RenderVars vars_;

	shader::XShaderManager* pShaderMan_; // might make this allocoated, just so can forward declare like tex man.
	texture::TextureManager* pTextureMan_;
	RenderAuxImp* pAuxRender_;

	LinearAllocatorManager* pLinearAllocatorMan_;
	ContextManager* pContextMan_;
	CommandListManger cmdListManager_;
	BufferManager* pBuffMan_;
	DescriptorAllocator* pDescriptorAllocator_;
	DescriptorAllocatorPool* pDescriptorAllocatorPool_;

	RootSignatureDeviceCache* pRootSigCache_;
	PSODeviceCache* pPSOCache_;



	RootSignature presentRS_;

	Vec2<uint32_t> currentNativeRes_;	// the resolution we render to.
	Vec2<uint32_t> targetNativeRes_;	// if diffrent, the render buffers we be resized to this next frame.
	Vec2<uint32_t> displayRes_;			// the resolution of the final screen

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

	// pre created IL descriptinos for each supported vertex format.
	VertexFormatILArr ilDescriptions_;
	VertexFormatILArr ilStreamedDescriptions_;

	D3D_FEATURE_LEVEL featureLvl_;
	GpuFeatures features_;

	core::StackString<128, wchar_t> deviceName_;
	size_t dedicatedvideoMemory_;

	RenderAux auxQues_[AuxRenderer::ENUM_COUNT];
};


X_NAMESPACE_END

#endif // !X_RENDER_DX12_H_