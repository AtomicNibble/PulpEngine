#pragma once


#include "GpuResource.h"

X_NAMESPACE_BEGIN(render)

class CommandListManger;
class CommandContext;

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


X_INLINE Param::Param(float32_t f) :
	fval(f)
{}
X_INLINE Param::Param(uint32_t u) :
	uint(u)
{}
X_INLINE Param::Param(int32_t i) :
	sint(i)
{}

X_INLINE void Param::operator= (float32_t f)
{
	fval = f; 
}
X_INLINE void Param::operator= (uint32_t u)
{ 
	uint = u; 
}
X_INLINE void Param::operator= (int32_t i)
{ 
	sint = i;
}


class ContextManager
{
public:
	ContextManager(void) {}

	CommandContext* AllocateContext(D3D12_COMMAND_LIST_TYPE Type);
	void FreeContext(CommandContext*);
	void DestroyAllContexts();

private:
//	std::vector<std::unique_ptr<CommandContext> > sm_ContextPool[4];
//	std::queue<CommandContext*> sm_AvailableContexts[4];
//	std::mutex sm_ContextAllocationMutex;
};

class CommandContext
{
	X_NO_COPY(CommandContext);
	X_NO_ASSIGN(CommandContext);

	static const size_t RESOURCE_BARRIER_BUF = 16;

	static const uint32_t VALID_COMPUTE_QUEUE_RESOURCE_STATES;


public:
	CommandContext(D3D12_COMMAND_LIST_TYPE type);
	~CommandContext(void);


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
	ID3D12GraphicsCommandList* pCommandList_;
	ID3D12CommandAllocator* pCurrentAllocator_;

	// graphics
	ID3D12RootSignature* pCurGraphicsRootSignature_;
	ID3D12PipelineState* pCurGraphicsPipelineState_;
	// compute
	ID3D12RootSignature* pCurComputeRootSignature_;
	ID3D12PipelineState* pCurComputePipelineState_;

	uint32_t numBarriersToFlush_;
	D3D12_RESOURCE_BARRIER resourceBarrierBuffer[RESOURCE_BARRIER_BUF];

	ID3D12DescriptorHeap* pCurrentDescriptorHeaps_[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	D3D12_COMMAND_LIST_TYPE type_;
};


X_NAMESPACE_END

#include "CommandContex.inl"