#include "stdafx.h"
#include "CommandContex.h"

#include "CommandList.h"

X_NAMESPACE_BEGIN(render)


const uint32_t CommandContext::VALID_COMPUTE_QUEUE_RESOURCE_STATES = (D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
	D3D12_RESOURCE_STATE_COPY_DEST |
	D3D12_RESOURCE_STATE_COPY_SOURCE);

CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE type) :
	type_(type),
	pCommandList_(nullptr),
	pCurrentAllocator_(nullptr),
	pCurGraphicsRootSignature_(nullptr),
	pCurGraphicsPipelineState_(nullptr),
	pCurComputeRootSignature_(nullptr),
	pCurComputePipelineState_(nullptr),
	numBarriersToFlush_(0)
{
	core::zero_object(pCurrentDescriptorHeaps_);
}

CommandContext::~CommandContext(void)
{
	core::SafeReleaseDX(pCommandList_);
}


uint64_t CommandContext::flush(CommandListManger& cmdMng, bool waitForCompletion)
{
	flushResourceBarriers();

	X_ASSERT_NOT_NULL(pCurrentAllocator_);

	uint64_t fenceValue = cmdMng.getQueue(type_).executeCommandList(pCommandList_);

	if (waitForCompletion) {
		cmdMng.waitForFence(fenceValue);
	}

	// Reset the command list and restore previous state

	pCommandList_->Reset(pCurrentAllocator_, nullptr);

	if (pCurGraphicsRootSignature_)
	{
		pCommandList_->SetGraphicsRootSignature(pCurGraphicsRootSignature_);
		pCommandList_->SetPipelineState(pCurGraphicsPipelineState_);
	}
	if (pCurComputeRootSignature_)
	{
		pCommandList_->SetComputeRootSignature(pCurComputeRootSignature_);
		pCommandList_->SetPipelineState(pCurComputePipelineState_);
	}

	bindDescriptorHeaps();

	return fenceValue;
}

uint64_t CommandContext::finish(CommandListManger& cmdMng, bool waitForCompletion)
{
	X_ASSERT(type_ == D3D12_COMMAND_LIST_TYPE_DIRECT || type_ == D3D12_COMMAND_LIST_TYPE_COMPUTE, "Invalid type")(type_);
	X_ASSERT_NOT_NULL(pCurrentAllocator_);

	flushResourceBarriers();

	CommandQue& queue = cmdMng.getQueue(type_);
	uint64_t fenceValue = queue.executeCommandList(pCommandList_);
	

	queue.discardAllocator(fenceValue, pCurrentAllocator_);
	pCurrentAllocator_ = nullptr;


//	m_CpuLinearAllocator.CleanupUsedPages(fenceValue);
//	m_GpuLinearAllocator.CleanupUsedPages(fenceValue);
//	m_DynamicDescriptorHeap.CleanupUsedHeaps(fenceValue);

	if (waitForCompletion) {
		cmdMng.waitForFence(fenceValue);
	}

//	g_ContextManager.FreeContext(this);

	return fenceValue;
}


void CommandContext::initialize(CommandListManger& cmdMng)
{
	X_ASSERT(pCommandList_ == nullptr, "Command list already set")();
	X_ASSERT(pCurrentAllocator_ == nullptr, "Command allocator already set")();

	cmdMng.createNewCommandList(type_, &pCommandList_, &pCurrentAllocator_);
}


void CommandContext::reset(CommandListManger& cmdMng)
{
	// We only call Reset() on previously freed contexts. The command list persists, but we must
	// request a new allocator.
	X_ASSERT_NOT_NULL(pCommandList_);
	X_ASSERT_NOT_NULL(pCurrentAllocator_);

	pCurrentAllocator_ = cmdMng.getQueue(type_).requestAllocator();
	pCommandList_->Reset(pCurrentAllocator_, nullptr);

	pCurGraphicsRootSignature_ = nullptr;
	pCurGraphicsPipelineState_ = nullptr;
	pCurComputeRootSignature_ = nullptr;
	pCurComputePipelineState_ = nullptr;
	numBarriersToFlush_ = 0;

	bindDescriptorHeaps();
}


void CommandContext::copyBuffer(GpuResource& dest, GpuResource& src)
{
	transitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
	transitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
	flushResourceBarriers();

	pCommandList_->CopyResource(dest.getResource(), src.getResource());
}

void CommandContext::copyBufferRegion(GpuResource& dest, size_t destOffset,
	GpuResource& src, size_t srcOffset, size_t numBytes)
{
	transitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
	flushResourceBarriers();

	pCommandList_->CopyBufferRegion(dest.getResource(), destOffset, src.getResource(), srcOffset, numBytes);
}

void CommandContext::copySubresource(GpuResource& dest, uint32_t destSubIndex, GpuResource& src, uint32_t srcSubIndex)
{
	// TODO:  Add a TransitionSubresource()?
	transitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
	transitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
	flushResourceBarriers();

	D3D12_TEXTURE_COPY_LOCATION destLocation =
	{
		dest.getResource(),
		D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		destSubIndex
	};

	D3D12_TEXTURE_COPY_LOCATION srcLocation =
	{
		src.getResource(),
		D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
		srcSubIndex
	};

	pCommandList_->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
}

void CommandContext::writeBuffer(GpuResource& dest, size_t destOffset, const void* pData, size_t numBytes)
{
	X_ASSERT_NOT_NULL(pData);
	X_ASSERT_ALIGNMENT(pData, 16, 0);

//	DynAlloc tempSpace = m_CpuLinearAllocator.Allocate(NumBytes, 512);
//	SIMDMemCopy(tempSpace.DataPtr, BufferData, Math::DivideByMultiple(NumBytes, 16));
//	copyBufferRegion(dest, destOffset, tempSpace.Buffer, tempSpace.Offset, numBytes);
}

void CommandContext::fillBuffer(GpuResource& dest, size_t destOffset, Param val, size_t numBytes)
{
//	DynAlloc tempSpace = m_CpuLinearAllocator.Allocate(NumBytes, 512);
//	__m128 VectorValue = _mm_set1_ps(val.fval);
//	SIMDMemFill(tempSpace.DataPtr, VectorValue, Math::DivideByMultiple(NumBytes, 16));
	
//	copyBufferRegion(dest, destOffset, tempSpace.Buffer, tempSpace.Offset, numBytes);
}


void CommandContext::transitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
	const D3D12_RESOURCE_STATES oldState = resource.getUsageState();

	if (type_ == D3D12_COMMAND_LIST_TYPE_COMPUTE)
	{
		X_ASSERT((oldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == oldState, "No valid compute states set")(oldState);
		X_ASSERT((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState, "No valid compute states set")(newState);
	}

	if (oldState != newState)
	{
		X_ASSERT(numBarriersToFlush_ < 16, "Exceeded arbitrary limit on buffered barriers")(numBarriersToFlush_);
		D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarrierBuffer[numBarriersToFlush_++];

		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = resource.getResource();
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = oldState;
		barrierDesc.Transition.StateAfter = newState;

		// Check to see if we already started the transition
		if (newState == resource.getTransitioningStateState())
		{
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
			resource.setTransitioningStateState(static_cast<D3D12_RESOURCE_STATES>(-1));
		}
		else {
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		}

		resource.setUsageState(newState);
	}
	else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
		insertUAVBarrier(resource, flushImmediate);
	}

	if (flushImmediate || numBarriersToFlush_ == RESOURCE_BARRIER_BUF) {
		flushResourceBarriers();
	}
}

void CommandContext::beginResourceTransition(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
	// If it's already transitioning, finish that transition
	if (resource.getTransitioningStateState() != (D3D12_RESOURCE_STATES)-1) {
		transitionResource(resource, resource.getTransitioningStateState());
	}

	const D3D12_RESOURCE_STATES oldState = resource.getUsageState();

	if (oldState != newState)
	{
		X_ASSERT(numBarriersToFlush_ < 16, "Exceeded arbitrary limit on buffered barriers")(numBarriersToFlush_);
		D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarrierBuffer[numBarriersToFlush_++];

		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = resource.getResource();
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = oldState;
		barrierDesc.Transition.StateAfter = newState;

		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

		resource.setTransitioningStateState(newState);
	}

	if (flushImmediate || numBarriersToFlush_ == RESOURCE_BARRIER_BUF) {
		flushResourceBarriers();
	}
}


void CommandContext::insertUAVBarrier(GpuResource& resource, bool flushImmediate)
{
	X_ASSERT(numBarriersToFlush_ < 16, "Exceeded arbitrary limit on buffered barriers")(numBarriersToFlush_);

	D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarrierBuffer[numBarriersToFlush_++];
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.UAV.pResource = resource.getResource();

	if (flushImmediate) {
		flushResourceBarriers();
	}
}

void CommandContext::insertAliasBarrier(GpuResource& before, GpuResource& after, bool flushImmediate)
{
	X_ASSERT(numBarriersToFlush_ < 16, "Exceeded arbitrary limit on buffered barriers")(numBarriersToFlush_);

	D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarrierBuffer[numBarriersToFlush_++];
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.Aliasing.pResourceBefore = before.getResource();
	barrierDesc.Aliasing.pResourceAfter = after.getResource();

	if (flushImmediate) {
		flushResourceBarriers();
	}
}


void CommandContext::insertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t queryIdx)
{
	pCommandList_->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, queryIdx);
}

void CommandContext::resolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, 
	uint32_t numQueries)
{
	pCommandList_->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, numQueries, pReadbackHeap, 0);
}

void CommandContext::setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* pHeapPtr)
{
	if (pCurrentDescriptorHeaps_[type] != pHeapPtr)
	{
		pCurrentDescriptorHeaps_[type] = pHeapPtr;
		bindDescriptorHeaps();
	}
}

void CommandContext::setDescriptorHeaps(uint32_t heapCount, D3D12_DESCRIPTOR_HEAP_TYPE* pTypes, 
	ID3D12DescriptorHeap** pHeapPtrs)
{
	bool anyChanged = false;

	for (UINT i = 0; i < heapCount; ++i)
	{
		if (pCurrentDescriptorHeaps_[pTypes[i]] != pHeapPtrs[i])
		{
			pCurrentDescriptorHeaps_[pTypes[i]] = pHeapPtrs[i];
			anyChanged = true;
		}
	}

	if (anyChanged) {
		bindDescriptorHeaps();
	}
}

void CommandContext::setPredication(ID3D12Resource* pBuffer, uint64_t bufferOffset, 
	D3D12_PREDICATION_OP op)
{
	pCommandList_->SetPredication(pBuffer, bufferOffset, op);
}


void CommandContext::flushResourceBarriers(void)
{
	if (numBarriersToFlush_ > 0) {
		X_ASSERT_NOT_NULL(pCommandList_);

		pCommandList_->ResourceBarrier(numBarriersToFlush_, resourceBarrierBuffer);
		numBarriersToFlush_ = 0;
	}
}

void CommandContext::bindDescriptorHeaps(void)
{

	uint32_t nonNullHeaps = 0;
	ID3D12DescriptorHeap* pHeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		ID3D12DescriptorHeap* HeapIter = pCurrentDescriptorHeaps_[i];
		if (HeapIter != nullptr) {
			pHeapsToBind[nonNullHeaps++] = HeapIter;
		}
	}

	if (nonNullHeaps > 0) {
		X_ASSERT_NOT_NULL(pCommandList_);

		pCommandList_->SetDescriptorHeaps(nonNullHeaps, pHeapsToBind);
	}
}

// --------------------------------------------------------



GraphicsContext::~GraphicsContext()
{

}



void GraphicsContext::beginQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t heapIndex)
{

}

void GraphicsContext::endQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t heapIndex)
{

}

void GraphicsContext::resolveQueryData(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, uint32_t startIndex,
		uint32_t numQueries, ID3D12Resource* pDestinationBuffer, uint64_t destinationBufferOffset)
{

}

	 
void GraphicsContext::setRootSignature(const RootSignature& rootSig)
{

}

	 
void GraphicsContext::setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs)
{

}

void GraphicsContext::setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs, 
	D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{

}

void GraphicsContext::setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV)
{

}

void GraphicsContext::setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{

}

void GraphicsContext::setDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{

}

	 
void GraphicsContext::setViewport(const D3D12_VIEWPORT& vp)
{

}

void GraphicsContext::setViewport(float32_t x, float32_t y, float32_t w, float32_t h, 
	float32_t minDepth, float32_t maxDepth)
{

}

void GraphicsContext::setScissor(const D3D12_RECT& rect)
{

}

void GraphicsContext::setScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{

}

void GraphicsContext::setViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{

}

void GraphicsContext::setViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{

}

void GraphicsContext::setStencilRef(uint32_t stencilRef)
{

}

void GraphicsContext::setBlendFactor(Color8u blendFactor)
{

}

void GraphicsContext::setBlendFactor(const Colorf& blendFactor)
{

}

void GraphicsContext::setPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{

}


void GraphicsContext::setConstants(uint32_t rootIndex, uint32_t numConstants, const void* pConstants)
{

}

void GraphicsContext::setConstants(uint32_t rootIndex, Param X)
{

}

void GraphicsContext::setConstants(uint32_t rootIndex, Param X, Param Y)
{

}

void GraphicsContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z)
{

}

void GraphicsContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z, Param W)
{

}

void GraphicsContext::setConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{

}

void GraphicsContext::setDynamicConstantBufferView(uint32_t RootIndex, size_t BufferSize, const void* pBufferData)
{

}

//void GraphicsContext::setBufferSRV(uint32_t rootIndex, const GpuBuffer& SRV, uint64_t offset = 0)
//{
//
//}
//
//void GraphicsContext::setBufferUAV(uint32_t rootIndex, const GpuBuffer& UAV, uint64_t offset = 0)
//{
//
//}

void GraphicsContext::setDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
{

}

	 
void GraphicsContext::setDynamicDescriptor(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{

}

void GraphicsContext::setDynamicDescriptors(uint32_t rootIndex, uint32_t offset, uint32_t count, 
	const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles)
{

}

	 
void GraphicsContext::setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
{

}

void GraphicsContext::setVertexBuffer(uint32_t slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
{

}

void GraphicsContext::setVertexBuffers(uint32_t startSlot, uint32_t count, const D3D12_VERTEX_BUFFER_VIEW* pVBViews)
{

}

void GraphicsContext::setDynamicVB(uint32_t slot, size_t numVertices, size_t vertexStride, const void* pVBData)
{

}

void GraphicsContext::setDynamicIB(size_t indexCount, const uint16_t* pIBData)
{

}

void GraphicsContext::setDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* pBufferData)
{

}

	 
void GraphicsContext::draw(uint32_t vertexCount, uint32_t vertexStartOffset)
{

}

void GraphicsContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, 
	int32_t baseVertexLocation)
{

}

void GraphicsContext::drawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation)
{

}

void GraphicsContext::drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation)
{

}

//void GraphicsContext::drawIndirect(GpuBuffer& argumentBuffer, size_t argumentBufferOffset = 0)
//{
//
//}


X_NAMESPACE_END