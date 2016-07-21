#pragma once


#include "GpuResource.h"
#include "DynamicDescriptorHeap.h"
#include "LinearAllocator.h"

#include <Math\XViewPort.h>

X_NAMESPACE_BEGIN(render)

class CommandListManger;
class CommandContext;
class CommandSignature;
class RootSignature;
class GpuBuffer;
class ColorBuffer;
class DepthBuffer;
class GraphicsPSO;
class ComputePSO;
class DescriptorAllocatorPool;

struct Param
{
	X_INLINE explicit Param(float32_t f);
	X_INLINE explicit Param(uint32_t u);
	X_INLINE explicit Param(int32_t i);

	X_INLINE void operator= (float32_t f);
	X_INLINE void operator= (uint32_t u);
	X_INLINE void operator= (int32_t i);

	union {
		float32_t fval;
		uint32_t uint;
		int32_t sint;
	};
};



class ContextManager
{
	static const size_t COMMAND_LIST_TYPE_NUM = 4; // num D3D12_COMMAND_LIST_TYPE

public:
	ContextManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
		DescriptorAllocatorPool& pool, LinearAllocatorManager& linAllocMan);
	~ContextManager();

	CommandContext* allocateContext(CommandListManger& cmdListMan, D3D12_COMMAND_LIST_TYPE type);
	void freeContext(CommandContext* pContex);
	void destroyAllContexts(void);

private:
	core::MemoryArenaBase* arena_;
	ID3D12Device* pDevice_;
	DescriptorAllocatorPool& pool_;
	LinearAllocatorManager& linAllocMan_;

	core::CriticalSection cs_;
	core::Array<CommandContext*> contextPool_[COMMAND_LIST_TYPE_NUM];
	std::queue<CommandContext*> availableContexts_[COMMAND_LIST_TYPE_NUM];
};

class CommandContext
{
	X_NO_COPY(CommandContext);
	X_NO_ASSIGN(CommandContext);

	static const size_t RESOURCE_BARRIER_BUF = 16;
	static const uint32_t VALID_COMPUTE_QUEUE_RESOURCE_STATES;

public:
	CommandContext(ContextManager& contexMan, core::MemoryArenaBase* arena, ID3D12Device* pDevice,
		 DescriptorAllocatorPool& pool, LinearAllocatorManager& linAllocMan, D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandContext(void);

	X_INLINE D3D12_COMMAND_LIST_TYPE getType(void) const;

	// Flush existing commands to the GPU but keep the context alive
	uint64_t flush(CommandListManger& cmdMng, bool waitForCompletion = false);
	// Flush existing commands and release the current context
	uint64_t finish(CommandListManger& cmdMng, bool waitForCompletion = false);

	// Prepare to render by reserving a command list and command allocator
	void initialize(CommandListManger& cmdMng);
	void reset(CommandListManger& cmdMng);


	void copyBuffer(GpuResource& dest, GpuResource& src);
	void copyBufferRegion(GpuResource& dest, size_t destOffset, GpuResource& src, size_t srcOffset, size_t numBytes);
	void copySubresource(GpuResource& dest, uint32_t destSubIndex, GpuResource& src, uint32_t srcSubIndex);
//	void copyCounter(GpuResource& dest, size_t destOffset, StructuredBuffer& Src);
//	void resetCounter(StructuredBuffer& Buf, uint32_t Value = 0);

	void writeBuffer(GpuResource& dest, size_t destOffset, const void* pData, size_t numBytes);
	void fillBuffer(GpuResource& dest, size_t destOffset, Param val, size_t numBytes);


	void transitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
	void beginResourceTransition(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
	void insertUAVBarrier(GpuResource& resource, bool flushImmediate = false);
	void insertAliasBarrier(GpuResource& before, GpuResource& after, bool flushImmediate = false);

	void insertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t queryIdx);
	void resolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t numQueries);

	void setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* pHeapPtr);
	void setDescriptorHeaps(uint32_t heapCount, D3D12_DESCRIPTOR_HEAP_TYPE* pType, ID3D12DescriptorHeap** pHeapPtrs);
	void setPredication(ID3D12Resource* pBuffer, uint64_t bufferOffset, D3D12_PREDICATION_OP op);

	void flushResourceBarriers(void);


protected:
	void bindDescriptorHeaps(void);

protected:
	ContextManager& contextManager_;

	ID3D12GraphicsCommandList* pCommandList_;
	ID3D12CommandAllocator* pCurrentAllocator_;

	// graphics
	ID3D12RootSignature* pCurGraphicsRootSignature_;
	ID3D12PipelineState* pCurGraphicsPipelineState_;
	// compute
	ID3D12RootSignature* pCurComputeRootSignature_;
	ID3D12PipelineState* pCurComputePipelineState_;

	DynamicDescriptorHeap dynamicDescriptorHeap_;

	LinearAllocator cpuLinearAllocator_;
	LinearAllocator gpuLinearAllocator_;

	uint32_t numBarriersToFlush_;
	D3D12_RESOURCE_BARRIER resourceBarrierBuffer[RESOURCE_BARRIER_BUF];

	ID3D12DescriptorHeap* pCurrentDescriptorHeaps_[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	D3D12_COMMAND_LIST_TYPE type_;
};


class GraphicsContext : public CommandContext
{
public:
//	GraphicsContext();
	~GraphicsContext() X_OVERRIDE;


	void clearUAV(GpuBuffer& target);
	void clearUAV(ColorBuffer& target);
	void clearColor(ColorBuffer& target);
	void clearDepth(DepthBuffer& target);
	void clearStencil(DepthBuffer& target);
	void clearDepthAndStencil(DepthBuffer& target);

	void beginQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t heapIndex);
	void endQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t heapIndex);
	void resolveQueryData(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t startIndex, 
		uint32_t numQueries, ID3D12Resource* pDestinationBuffer, uint64_t destinationBufferOffset);

	void setRootSignature(const RootSignature& rootSig);

	void setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs);
	void setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs, D3D12_CPU_DESCRIPTOR_HANDLE DSV);
	void setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV);
	void setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV);
	void setDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV);

	void setViewport(const XViewPort& vp);
	void setViewport(const D3D12_VIEWPORT& vp);
	void setViewport(float32_t x, float32_t y, float32_t w, float32_t h, float32_t minDepth = 0.0f, float32_t maxDepth = 1.0f);
	void setScissor(const D3D12_RECT& rect);
	void setScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
	void setViewportAndScissor(const XViewPort& vp, const D3D12_RECT& rect);
	void setViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	void setViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	void setStencilRef(uint32_t stencilRef);
	void setBlendFactor(Color8u blendFactor);
	void setBlendFactor(const Colorf& blendFactor);
	void setPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);
	
	void setPipelineState(const GraphicsPSO& PSO);
	void setConstants(uint32_t rootIndex, uint32_t numConstants, const void* pConstants);
	void setConstants(uint32_t rootIndex, Param X);
	void setConstants(uint32_t rootIndex, Param X, Param Y);
	void setConstants(uint32_t rootIndex, Param X, Param Y, Param Z);
	void setConstants(uint32_t rootIndex, Param X, Param Y, Param Z, Param W);
	void setConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
	void setDynamicConstantBufferView(uint32_t RootIndex, size_t BufferSize, const void* pBufferData);
	void setBufferSRV(uint32_t rootIndex, const GpuBuffer& SRV, uint64_t offset = 0);
	void setBufferUAV(uint32_t rootIndex, const GpuBuffer& UAV, uint64_t offset = 0);
	void setDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle);

	void setDynamicDescriptor(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void setDynamicDescriptors(uint32_t rootIndex, uint32_t offset, uint32_t count, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles);

	void setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView);
	void setVertexBuffer(uint32_t slot, const D3D12_VERTEX_BUFFER_VIEW& VBView);
	void setVertexBuffers(uint32_t startSlot, uint32_t count, const D3D12_VERTEX_BUFFER_VIEW* pVBViews);
	void setDynamicVB(uint32_t slot, size_t numVertices, size_t vertexStride, const void* pVBData);
	void setDynamicIB(size_t indexCount, const uint16_t* pIBData);
	void setDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* pBufferData);

	void draw(uint32_t vertexCount, uint32_t vertexStartOffset = 0);
	void drawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, int32_t baseVertexLocation = 0);
	void drawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0);
	void drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation);
	void drawIndirect(CommandSignature& drawIndirectCmdSig, GpuBuffer& argumentBuffer, size_t argumentBufferOffset = 0);

private:


};

class ComputeContext : public CommandContext
{
public:
//	ComputeContext();
	~ComputeContext() X_OVERRIDE;

	void clearUAV(GpuBuffer& target);
	void clearUAV(ColorBuffer& target);

	void setRootSignature(const RootSignature& rootSig);

	void setPipelineState(const ComputePSO& PSO);
	void setConstants(uint32_t rootIndex, uint32_t numConstants, const void* pConstants);
	void setConstants(uint32_t rootIndex, Param X);
	void setConstants(uint32_t rootIndex, Param X, Param Y);
	void setConstants(uint32_t rootIndex, Param X, Param Y, Param Z);
	void setConstants(uint32_t rootIndex, Param X, Param Y, Param Z, Param W);
	void setConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
	void setDynamicConstantBufferView(uint32_t rootIndex, size_t bufferSize, const void* pBufferData);
	void setDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* pBufferData);
	void setBufferSRV(uint32_t rootIndex, const GpuBuffer& SRV, uint64_t offset = 0);
	void setBufferUAV(uint32_t rootIndex, const GpuBuffer& UAV, uint64_t offset = 0);
	void setDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle);

	void setDynamicDescriptor(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle);
	void setDynamicDescriptors(uint32_t rootIndex, uint32_t offset, uint32_t count, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles);

	void dispatch(size_t groupCountX = 1, size_t groupCountY = 1, size_t groupCountZ = 1);
	void dispatch1D(size_t threadCountX, size_t groupSizeX = 64);
	void dispatch2D(size_t threadCountX, size_t threadCountY, size_t groupSizeX = 8, size_t groupSizeY = 8);
	void dispatch3D(size_t threadCountX, size_t threadCountY, size_t threadCountZ, size_t groupSizeX, size_t groupSizeY, size_t groupSizeZ);
	void dispatchIndirect(CommandSignature& dispatchCmdSig, GpuBuffer& argumentBuffer, size_t argumentBufferOffset = 0);

private:

};


	
	
X_NAMESPACE_END

#include "CommandContex.inl"