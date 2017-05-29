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
#include "RootSignature.h"

X_NAMESPACE_DECLARE(texture,
	class TextureManager;
	class TextureVars;
)
X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(render)

namespace shader
{
	class XShaderManager;
} // namespace shader

class ContextManager;
class LinearAllocatorManager;
class BufferManager;
class RootSignatureDeviceCache;
class PSODeviceCache;
class SamplerDescriptorCache;

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
		{
			cbRootIdxBase = std::numeric_limits<decltype(cbRootIdxBase)>::max();
		}

#if DEVICE_STATE_STORE_CPU_DESC
		StateDesc cpuDesc;
#endif // !DEVICE_STATE_STORE_CPU_DESC

		RootSignature rootSig; // dam rootsig is a chunky fucker.
		ID3D12PipelineState* pPso;

		D3D12_PRIMITIVE_TOPOLOGY topo;

		uint8_t texRootIdxBase; 
		uint8_t samplerRootIdxBase; 
		uint8_t cbRootIdxBase; // root index base for cbuffers
		uint8_t _pad[1];

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

			variableStateSize = 0;
			core::zero_object(variableState);
		}

		StateHandle handle;
		IndexBufferHandle indexBuffer;
		VertexHandleArr vertexBuffers;
		ConstBuffersArr constBuffers; // rootSig indexed

		// a buffer that can store any variable state.
		// and we can just memcmp to know if it's diffrent.
		int32_t variableStateSize;
		uint8_t variableState[render::Commands::ResourceStateBase::getMaxStateSize()];

		ID3D12RootSignature* pRootSig;
		ID3D12PipelineState* pPso;
		D3D12_PRIMITIVE_TOPOLOGY topo;
	};


	struct Stats
	{
		Stats();

		uint32_t numPassStates;
		uint32_t maxPassStates;
		
		uint32_t numStates;
		uint32_t maxStates;
	};

	static const size_t MAX_STATES = 1024 * 4;
	static const size_t MAX_STATE_ALOC_SIZE = core::Max(sizeof(PassState), sizeof(DeviceState));
	static const size_t MAX_STATE_ALOC_ALIGN = core::Max(X_ALIGN_OF(PassState), X_ALIGN_OF(DeviceState));

	typedef core::MemoryArena<
		core::PoolAllocator,
		core::MultiThreadPolicy<core::Spinlock>, // allow multi thread creation.

#if X_ENABLE_MEMORY_DEBUG_POLICIES
		core::SimpleBoundsChecking,
		core::NoMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> StatePoolArena;


public:
	XRender(core::MemoryArenaBase* arena);
	~XRender();

	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height, texture::Texturefmt::Enum depthFmt, bool reverseZ) X_FINAL;
	void shutDown(void) X_FINAL;
	void freeResources(void) X_FINAL;
	void release(void) X_FINAL;

	void renderBegin(void) X_FINAL;
	void renderEnd(void) X_FINAL;

	void submitCommandPackets(CommandBucket<uint32_t>& cmdBucket) X_FINAL;

	Vec2<uint32_t> getDisplayRes(void) const X_FINAL;

	IPixelBuffer* createDepthBuffer(const char* pNickName, Vec2i dim) X_FINAL;
	IPixelBuffer* createColorBuffer(const char* pNickName, Vec2i dim, uint32_t numMips, 
		texture::Texturefmt::Enum fmt) X_FINAL;

	IRenderTarget* getCurBackBuffer(uint32_t* pIdx = nullptr) X_FINAL;


	VertexBufferHandle createVertexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;
	VertexBufferHandle createVertexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;
	IndexBufferHandle createIndexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;
	IndexBufferHandle createIndexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag) X_FINAL;

	// cb's
	ConstantBufferHandle createConstBuffer(const shader::XCBuffer& cbuffer, BufUsage::Enum usage) X_FINAL;

	texture::ITexture* getDeviceTexture(int32_t id) X_FINAL;
	texture::ITexture* getTexture(const char* pName, texture::TextureFlags flags) X_FINAL;
	texture::ITexture* createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, const uint8_t* pInitialData = nullptr) X_FINAL;
	
	bool initDeviceTexture(texture::ITexture* pTex) X_FINAL;
	bool initDeviceTexture(texture::ITexture* pTex, const texture::XTextureFile& imgFile) X_FINAL;

	shader::IShaderSource* getShaderSource(const char* pName) X_FINAL;
	shader::IHWShader* createHWShader(shader::ShaderType::Enum type, const core::string& entry, shader::IShaderSource* pSourceFile, shader::PermatationFlags permFlags) X_FINAL;
	shader::IShaderPermatation* createPermatation(const shader::ShaderStagesArr& stages) X_FINAL;

	PassStateHandle createPassState(const RenderTargetFmtsArr& rtfs) X_FINAL;
	StateHandle createState(PassStateHandle passHandle, const shader::IShaderPermatation* pPerm, const StateDesc& state, const TextureState* pTextStates, size_t numStates) X_FINAL;

	// Release
	void releaseShaderPermatation(shader::IShaderPermatation* pPerm) X_FINAL;
	void releaseTexture(texture::ITexture* pTex) X_FINAL;
	void releasePixelBuffer(render::IPixelBuffer* pPixelBuf) X_FINAL;
	void destoryPassState(PassStateHandle handle) X_FINAL;
	void destoryState(StateHandle handle) X_FINAL;

	void destoryVertexBuffer(VertexBufferHandle handle) X_FINAL;
	void destoryIndexBuffer(IndexBufferHandle handle) X_FINAL;
	void destoryConstBuffer(ConstantBufferHandle handle) X_FINAL;

	void getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_FINAL;
	void getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize = nullptr) X_FINAL;

private:
	void CreateVBView(GraphicsContext& context, const VertexHandleArr& vertexBuffers,
		D3D12_VERTEX_BUFFER_VIEW viewsOut[VertexStream::ENUM_COUNT], uint32_t& numVertexStreams);

	void ApplyState(GraphicsContext& context, State& curState, const StateHandle handle,
		const VertexHandleArr& vertexBuffers,
		const Commands::ResourceStateBase& resourceState, const char* pStateData);

	void ApplyIndexBuffer(GraphicsContext& context, State& curState, IndexBufferHandle ib);

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
	texture::TextureVars* pTexVars_;

	shader::XShaderManager* pShaderMan_; // might make this allocoated, just so can forward declare like tex man.
	texture::TextureManager* pTextureMan_;

	LinearAllocatorManager* pLinearAllocatorMan_;
	ContextManager* pContextMan_;
	CommandListManger cmdListManager_;
	BufferManager* pBuffMan_;
	DescriptorAllocator* pDescriptorAllocator_;
	DescriptorAllocatorPool* pDescriptorAllocatorPool_;

	SamplerDescriptorCache* pSamplerCache_;
	RootSignatureDeviceCache* pRootSigCache_;
	PSODeviceCache* pPSOCache_;


	core::HeapArea      statePoolHeap_;
	core::PoolAllocator statePoolAllocator_;
	StatePoolArena		statePool_;

	Vec2<uint32_t> currentNativeRes_;	// the resolution we render to.
	Vec2<uint32_t> targetNativeRes_;	// if diffrent, the render buffers we be resized to this next frame.
	Vec2<uint32_t> displayRes_;			// the resolution of the final screen

	texture::Texture* pDisplayPlanes_[SWAP_CHAIN_BUFFER_COUNT];
	uint32_t currentBufferIdx_;

	// pre created IL descriptinos for each supported vertex format.
	VertexFormatILArr ilDescriptions_;
	VertexFormatILArr ilStreamedDescriptions_;
	VertexLayoutDescArr ilInstanced_;

	D3D_FEATURE_LEVEL featureLvl_;
	GpuFeatures features_;

	core::StackString<128, wchar_t> deviceName_;
	size_t dedicatedvideoMemory_;

#if RENDER_STATS
	Stats stats_;
#endif // !RENDER_STATS
};


X_NAMESPACE_END

#endif // !X_RENDER_DX12_H_