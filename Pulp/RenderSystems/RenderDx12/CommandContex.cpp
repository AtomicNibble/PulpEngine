#include "stdafx.h"
#include "CommandContex.h"
#include "CommandList.h"
#include "CommandSignature.h"
#include "RootSignature.h"
#include "Buffers\GpuBuffer.h"
#include "Buffers\ColorBuffer.h"
#include "Buffers\DepthBuffer.h"
#include "PipelineState.h"

#include "Allocators\DynamicDescriptorHeap.h"


X_NAMESPACE_BEGIN(render)


const uint32_t CommandContext::VALID_COMPUTE_QUEUE_RESOURCE_STATES = (D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
	D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
	D3D12_RESOURCE_STATE_COPY_DEST |
	D3D12_RESOURCE_STATE_COPY_SOURCE);

X_DISABLE_WARNING(4355) // 'this': used in base member initializer list


ContextManager::ContextManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
		DescriptorAllocatorPool& pool, LinearAllocatorManager& linAllocMan) :
	arena_(arena),
	pDevice_(pDevice),
	pool_(pool),
	linAllocMan_(linAllocMan),
	contextPool_{ arena, arena, arena, arena }
{

}

ContextManager::~ContextManager()
{

}

CommandContext* ContextManager::allocateContext(CommandListManger& cmdListMan, D3D12_COMMAND_LIST_TYPE type)
{
	core::CriticalSection::ScopedLock lock(cs_);

	auto& availableContexts = availableContexts_[type];

	CommandContext* pRet = nullptr;
	if (availableContexts.empty())
	{
		pRet = X_NEW(CommandContext, arena_, "CmdContex")(*this, arena_, pDevice_, pool_, linAllocMan_, type);
		contextPool_[type].emplace_back(pRet);
		pRet->initialize(cmdListMan);
	}
	else
	{
		pRet = availableContexts.front();
		availableContexts.pop();
		pRet->reset(cmdListMan);
	}

	X_ASSERT_NOT_NULL(pRet);
	X_ASSERT(pRet->getType() == type, "contex tpye mismatch")(pRet->getType(), type);

	return pRet;
}

void ContextManager::freeContext(CommandContext* pContex)
{
	core::CriticalSection::ScopedLock lock(cs_);

	availableContexts_[pContex->getType()].push(pContex);
}

void ContextManager::destroyAllContexts(void)
{
	for (size_t i = 0; i < COMMAND_LIST_TYPE_NUM; ++i) {
		for (auto pCon : contextPool_[i]) {
			X_DELETE(pCon, arena_);
		}
		contextPool_[i].clear();
	}
}

// --------------------------------------------------------------------


CommandContext::CommandContext(ContextManager& contexMan, core::MemoryArenaBase* arena, ID3D12Device* pDevice,
		DescriptorAllocatorPool& pool, LinearAllocatorManager& linAllocMan, D3D12_COMMAND_LIST_TYPE type) :
	contextManager_(contexMan),
	type_(type),
	pCommandList_(nullptr),
	pCurrentAllocator_(nullptr),
	pCurGraphicsRootSignature_(nullptr),
	pCurGraphicsPipelineState_(nullptr),
	pCurComputeRootSignature_(nullptr),
	pCurComputePipelineState_(nullptr),
	dynamicDescriptorHeap_(arena, pDevice, pool, *this),
	cpuLinearAllocator_(arena, linAllocMan, LinearAllocatorType::CPU_WRITABLE),
	gpuLinearAllocator_(arena, linAllocMan, LinearAllocatorType::GPU_EXCLUSIVE),
	numBarriersToFlush_(0)
{
	core::zero_object(pCurrentDescriptorHeaps_);
}

X_ENABLE_WARNING(4355)

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


	cpuLinearAllocator_.cleanupUsedPages(fenceValue);
	gpuLinearAllocator_.cleanupUsedPages(fenceValue);
	dynamicDescriptorHeap_.cleanupUsedHeaps(fenceValue);

	if (waitForCompletion) {
		cmdMng.waitForFence(fenceValue);
	}

	contextManager_.freeContext(this);

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

	DynAlloc tempSpace = cpuLinearAllocator_.allocate(numBytes, 512);
	core::SIMDMemCopy(tempSpace.getCpuData(), pData, divideByMultiple(numBytes, 16));
	copyBufferRegion(dest, destOffset, tempSpace.getBuffer(), tempSpace.getOffset(), numBytes);
}

void CommandContext::fillBuffer(GpuResource& dest, size_t destOffset, Param val, size_t numBytes)
{
	X_ASSERT_NOT_IMPLEMENTED();

	DynAlloc tempSpace = cpuLinearAllocator_.allocate(numBytes, 512);
	__m128 VectorValue = _mm_set1_ps(val.fval);

	core::SIMDMemFill(tempSpace.getCpuData(), VectorValue, divideByMultiple(numBytes, 16));

	copyBufferRegion(dest, destOffset, tempSpace.getBuffer(), tempSpace.getOffset(), numBytes);
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


void GraphicsContext::clearUAV(GpuBuffer& target)
{
	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV 
	// (because it essentially runs a shader to set all of the values).

	D3D12_GPU_DESCRIPTOR_HANDLE gpuVisibleHandle = dynamicDescriptorHeap_.uploadDirect(target.getUAV());
	
	uint32_t clearColor[4];
	core::zero_object(clearColor);
	
	pCommandList_->ClearUnorderedAccessViewUint(gpuVisibleHandle, target.getUAV(), 
		target.getResource(), clearColor, 0, nullptr);
}


void GraphicsContext::clearUAV(ColorBuffer& target)
{
	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
	// a shader to set all of the values).

	D3D12_GPU_DESCRIPTOR_HANDLE gpuVisibleHandle = dynamicDescriptorHeap_.uploadDirect(target.getUAV());
	CD3DX12_RECT ClearRect(0, 0, target.getWidth(), target.getHeight());

	pCommandList_->ClearUnorderedAccessViewFloat(gpuVisibleHandle, target.getUAV(), target.getResource(),
		target.getClearColor(), 1, &ClearRect);
}

void GraphicsContext::clearColor(ColorBuffer& target)
{
	pCommandList_->ClearRenderTargetView(target.getRTV(), target.getClearColor(), 0, nullptr);
}

void GraphicsContext::clearDepth(DepthBuffer& Target)
{
	pCommandList_->ClearDepthStencilView(Target.getDSV(), D3D12_CLEAR_FLAG_DEPTH,
		Target.getClearDepth(), Target.getClearStencil(), 0, nullptr);
}

void GraphicsContext::clearStencil(DepthBuffer& Target)
{
	pCommandList_->ClearDepthStencilView(Target.getDSV(), D3D12_CLEAR_FLAG_STENCIL,
		Target.getClearDepth(), Target.getClearStencil(), 0, nullptr);
}

void GraphicsContext::clearDepthAndStencil(DepthBuffer& target)
{
	pCommandList_->ClearDepthStencilView(target.getDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		target.getClearDepth(), target.getClearStencil(), 0, nullptr);
}


void GraphicsContext::beginQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, 
	uint32_t heapIndex)
{
	pCommandList_->BeginQuery(pQueryHeap, type, heapIndex);
}

void GraphicsContext::endQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type,
	uint32_t heapIndex)
{
	pCommandList_->EndQuery(pQueryHeap, type, heapIndex);
}

void GraphicsContext::resolveQueryData(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type, 
	uint32_t startIndex, uint32_t numQueries, ID3D12Resource* pDestinationBuffer, 
	uint64_t destinationBufferOffset)
{
	pCommandList_->ResolveQueryData(pQueryHeap, type, startIndex, numQueries, pDestinationBuffer,
		destinationBufferOffset);
}

	 
void GraphicsContext::setRootSignature(const RootSignature& rootSig)
{
	if (rootSig.getSignature() == pCurGraphicsRootSignature_) {
		return;
	}

	pCommandList_->SetGraphicsRootSignature(pCurGraphicsRootSignature_ = rootSig.getSignature());

	dynamicDescriptorHeap_.parseGraphicsRootSignature(rootSig);
}

	 
void GraphicsContext::setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs)
{
	pCommandList_->OMSetRenderTargets(numRTVs, pRTVs, FALSE, nullptr);
}

void GraphicsContext::setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs, 
	D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
	pCommandList_->OMSetRenderTargets(numRTVs, pRTVs, FALSE, &DSV);
}

void GraphicsContext::setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV)
{
	setRenderTargets(1, &RTV);
}

void GraphicsContext::setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, 
	D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
	setRenderTargets(1, &RTV, DSV);
}

void GraphicsContext::setDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
	setRenderTargets(0, nullptr, DSV);
}


void GraphicsContext::setViewport(const XViewPort& vp)
{
	D3D12_VIEWPORT dvp;
	dvp.Width = vp.getWidthf();
	dvp.Height = vp.getHeightf();
	dvp.TopLeftX = 0;
	dvp.TopLeftY = 0;
	dvp.MinDepth = vp.getZNear();
	dvp.MaxDepth = vp.getZFar();

	pCommandList_->RSSetViewports(1, &dvp);
}
	 
void GraphicsContext::setViewport(const D3D12_VIEWPORT& vp)
{
	pCommandList_->RSSetViewports(1, &vp);
}

void GraphicsContext::setViewport(float32_t x, float32_t y, float32_t w, float32_t h, 
	float32_t minDepth, float32_t maxDepth)
{
	D3D12_VIEWPORT vp;
	vp.Width = w;
	vp.Height = h;
	vp.MinDepth = minDepth;
	vp.MaxDepth = maxDepth;
	vp.TopLeftX = x;
	vp.TopLeftY = y;
	pCommandList_->RSSetViewports(1, &vp);
}

void GraphicsContext::setScissor(const D3D12_RECT& rect)
{
	X_ASSERT(rect.left < rect.right && rect.top < rect.bottom, "Invalid rect")();
	pCommandList_->RSSetScissorRects(1, &rect);
}

void GraphicsContext::setScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	D3D12_RECT rect;
	rect.left = left;
	rect.right = right;
	rect.top = top;
	rect.bottom = bottom;
	setScissor(rect);
}

void GraphicsContext::setViewportAndScissor(const XViewPort& vp, const D3D12_RECT& rect)
{
	X_ASSERT(rect.left < rect.right && rect.top < rect.bottom, "Invalid rect")();
	setViewport(vp);
	pCommandList_->RSSetScissorRects(1, &rect);
}


void GraphicsContext::setViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	X_ASSERT(rect.left < rect.right && rect.top < rect.bottom, "Invalid rect")();
	pCommandList_->RSSetViewports(1, &vp);
	pCommandList_->RSSetScissorRects(1, &rect);
}

void GraphicsContext::setViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	setViewport(static_cast<float32_t>(x), static_cast<float32_t>(y),
		static_cast<float32_t>(w), static_cast<float32_t>(h));
	setScissor(x, y, x + w, y + h);
}

void GraphicsContext::setStencilRef(uint32_t stencilRef)
{
	pCommandList_->OMSetStencilRef(stencilRef);
}

void GraphicsContext::setBlendFactor(Color8u blendFactor)
{
	Colorf col(blendFactor);
	pCommandList_->OMSetBlendFactor(col);
}

void GraphicsContext::setBlendFactor(const Colorf& blendFactor)
{
	pCommandList_->OMSetBlendFactor(blendFactor);
}

void GraphicsContext::setPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
	pCommandList_->IASetPrimitiveTopology(topology);
}

void GraphicsContext::setPipelineState(const GraphicsPSO& PSO)
{
	ID3D12PipelineState* pPipelineState = PSO.getPipelineStateObject();
	if (pPipelineState == pCurComputePipelineState_) {
		return;
	}

	pCommandList_->SetPipelineState(pPipelineState);
	pCurComputePipelineState_ = pPipelineState;
}

void GraphicsContext::setConstants(uint32_t rootIndex, uint32_t numConstants, 
	const void* pConstants)
{
	pCommandList_->SetComputeRoot32BitConstants(rootIndex, numConstants, pConstants, 0);
}

void GraphicsContext::setConstants(uint32_t rootIndex, Param X)
{
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
}

void GraphicsContext::setConstants(uint32_t rootIndex, Param X, Param Y)
{
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Y.uint, 1);
}

void GraphicsContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z)
{
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Y.uint, 1);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Z.uint, 2);
}

void GraphicsContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z, Param W)
{
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Y.uint, 1);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Z.uint, 2);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, W.uint, 3);
}

void GraphicsContext::setConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
	pCommandList_->SetComputeRootConstantBufferView(rootIndex, CBV);
}

void GraphicsContext::setDynamicConstantBufferView(uint32_t RootIndex, size_t BufferSize, 
	const void* pBufferData)
{
	X_ASSERT_NOT_NULL(pBufferData);
	X_ASSERT_ALIGNMENT(pBufferData, 16, 0);

}

void GraphicsContext::setBufferSRV(uint32_t rootIndex, const GpuBuffer& SRV, uint64_t offset)
{
	X_ASSERT(core::bitUtil::IsBitFlagSet(SRV.getUsageState(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE), "UNORDERED_ACCESS flag missing")(SRV.getUsageState());
	X_ASSERT(core::bitUtil::IsBitFlagSet(SRV.getUsageState(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE), "NON_PIXEL_SHADER_RESOURCE flag missing")(SRV.getUsageState());

	pCommandList_->SetGraphicsRootShaderResourceView(rootIndex, SRV.getGpuVirtualAddress() + offset);
}

void GraphicsContext::setBufferUAV(uint32_t rootIndex, const GpuBuffer& UAV, uint64_t offset)
{
	X_ASSERT(core::bitUtil::IsBitFlagSet(UAV.getUsageState(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS), "UNORDERED_ACCESS flag missing")(UAV.getUsageState());

	if (offset != 0) {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	pCommandList_->SetGraphicsRootUnorderedAccessView(rootIndex, UAV.getGpuVirtualAddress());
}

void GraphicsContext::setDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
{
	pCommandList_->SetGraphicsRootDescriptorTable(rootIndex, firstHandle);
}
	 
void GraphicsContext::setDynamicDescriptor(uint32_t rootIndex, uint32_t offset, 
	D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	setDynamicDescriptors(rootIndex, offset, 1, &handle);
}


void GraphicsContext::setDynamicDescriptors(uint32_t rootIndex, uint32_t offset, uint32_t count, 
	const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles)
{
	dynamicDescriptorHeap_.setGraphicsDescriptorHandles(rootIndex, offset, count, pHandles);
}

	 
void GraphicsContext::setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
{
	pCommandList_->IASetIndexBuffer(&IBView);
}

void GraphicsContext::setVertexBuffer(uint32_t slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
{
	pCommandList_->IASetVertexBuffers(slot, 1, &VBView);
}

void GraphicsContext::setVertexBuffers(uint32_t startSlot, uint32_t count, 
	const D3D12_VERTEX_BUFFER_VIEW* pVBViews)
{
	pCommandList_->IASetVertexBuffers(startSlot, count, pVBViews);
}

void GraphicsContext::setDynamicVB(uint32_t slot, size_t numVertices, size_t vertexStride, 
	const void* pVBData)
{
	X_ASSERT_NOT_NULL(pVBData);
	X_ASSERT_ALIGNMENT(pVBData,16,0);

	size_t bufferSize = core::bitUtil::RoundUpToMultiple<size_t>(numVertices * vertexStride, 16u);
	DynAlloc vb = cpuLinearAllocator_.allocate(bufferSize);

	core::SIMDMemCopy(vb.getCpuData(), pVBData, bufferSize >> 4);

	D3D12_VERTEX_BUFFER_VIEW VBView;
	VBView.BufferLocation = vb.getGpuAddress();
	VBView.SizeInBytes = safe_static_cast<uint32_t, size_t>(bufferSize);
	VBView.StrideInBytes = safe_static_cast<uint32_t, size_t>(vertexStride);

	pCommandList_->IASetVertexBuffers(slot, 1, &VBView);
}

void GraphicsContext::setDynamicIB(size_t indexCount, const uint16_t* pIBData)
{
	X_ASSERT_NOT_NULL(pIBData);
	X_ASSERT_ALIGNMENT(pIBData, 16, 0);

	size_t bufferSize = core::bitUtil::RoundUpToMultiple<size_t>(indexCount * sizeof(uint16_t), 16u);
	DynAlloc ib = cpuLinearAllocator_.allocate(bufferSize);

	core::SIMDMemCopy(ib.getCpuData(), pIBData, bufferSize >> 4);

	D3D12_INDEX_BUFFER_VIEW IBView;
	IBView.BufferLocation = ib.getGpuAddress();
	IBView.SizeInBytes = safe_static_cast<uint32_t, size_t>(bufferSize);
	IBView.Format = DXGI_FORMAT_R16_UINT;

	pCommandList_->IASetIndexBuffer(&IBView);
}

void GraphicsContext::setDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* pBufferData)
{
	X_ASSERT_NOT_NULL(pBufferData);
	X_ASSERT_ALIGNMENT(pBufferData, 16, 0);

	DynAlloc cb = cpuLinearAllocator_.allocate(bufferSize);

	bufferSize = core::bitUtil::RoundUpToMultiple<size_t>(bufferSize, 16u);
	core::SIMDMemCopy(cb.getCpuData(), pBufferData, bufferSize >> 4);

	pCommandList_->SetGraphicsRootShaderResourceView(rootIndex, cb.getGpuAddress());
}

	 
void GraphicsContext::draw(uint32_t vertexCount, uint32_t vertexStartOffset)
{
	drawInstanced(vertexCount, 1, vertexStartOffset, 0);
}

void GraphicsContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, 
	int32_t baseVertexLocation)
{
	drawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}

void GraphicsContext::drawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	flushResourceBarriers();
	dynamicDescriptorHeap_.commitGraphicsRootDescriptorTables(pCommandList_);
	pCommandList_->DrawInstanced(vertexCountPerInstance, instanceCount, 
		startVertexLocation, startInstanceLocation);
}

void GraphicsContext::drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, 
	uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	flushResourceBarriers();
	dynamicDescriptorHeap_.commitGraphicsRootDescriptorTables(pCommandList_);
	pCommandList_->DrawIndexedInstanced(indexCountPerInstance, instanceCount,
		startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void GraphicsContext::drawIndirect(CommandSignature& drawIndirectCmdSig, GpuBuffer& argumentBuffer, size_t argumentBufferOffset)
{
	flushResourceBarriers();
	dynamicDescriptorHeap_.commitGraphicsRootDescriptorTables(pCommandList_);

	pCommandList_->ExecuteIndirect(drawIndirectCmdSig.getSignature(), 1, argumentBuffer.getResource(),
		static_cast<uint64_t>(argumentBufferOffset), nullptr, 0);

}


// ------------------------------------------------------



ComputeContext::~ComputeContext()
{

}

void ComputeContext::clearUAV(GpuBuffer& target)
{
	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
	// a shader to set all of the values).
	D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = dynamicDescriptorHeap_.uploadDirect(target.getUAV());
	const UINT clearColor[4] = {};
	pCommandList_->ClearUnorderedAccessViewUint(GpuVisibleHandle, target.getUAV(), target.getResource(), clearColor, 0, nullptr);
}

void ComputeContext::clearUAV(ColorBuffer& target)
{
	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
	// a shader to set all of the values).
	D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = dynamicDescriptorHeap_.uploadDirect(target.getUAV());
	CD3DX12_RECT ClearRect(0, 0, target.getWidth(), target.getHeight());

	pCommandList_->ClearUnorderedAccessViewFloat(GpuVisibleHandle, target.getUAV(), target.getResource(), 
		target.getClearColor(), 1, &ClearRect);
}

void ComputeContext::setRootSignature(const RootSignature& rootSig)
{
	if (rootSig.getSignature() == pCurComputeRootSignature_) {
		return;
	}

	pCommandList_->SetComputeRootSignature(pCurComputeRootSignature_ = rootSig.getSignature());

	dynamicDescriptorHeap_.parseComputeRootSignature(rootSig);
}

void ComputeContext::setPipelineState(const ComputePSO& PSO)
{
	ID3D12PipelineState* pPipelineState = PSO.getPipelineStateObject();
	if (pPipelineState == pCurComputePipelineState_) {
		return;
	}

	pCommandList_->SetPipelineState(pPipelineState);
	pCurComputePipelineState_ = pPipelineState;
}

void ComputeContext::setConstants(uint32_t rootIndex, uint32_t numConstants, const void* pConstants)
{
	pCommandList_->SetComputeRoot32BitConstants(rootIndex, numConstants, pConstants, 0);
}

void ComputeContext::setConstants(uint32_t rootIndex, Param X)
{
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
}

void ComputeContext::setConstants(uint32_t rootIndex, Param X, Param Y)
{
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Y.uint, 1);
}

void ComputeContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z)
{
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Y.uint, 1);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Z.uint, 2);
}

void ComputeContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z, Param W)
{
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Y.uint, 1);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, Z.uint, 2);
	pCommandList_->SetComputeRoot32BitConstant(rootIndex, W.uint, 3);
}

void ComputeContext::setConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
	pCommandList_->SetComputeRootConstantBufferView(rootIndex, CBV);
}

void ComputeContext::setDynamicConstantBufferView(uint32_t rootIndex, size_t bufferSize, const void* pBufferData)
{
	X_ASSERT_NOT_NULL(pBufferData);
	X_ASSERT_ALIGNMENT(pBufferData, 16, 0);

	DynAlloc cb = cpuLinearAllocator_.allocate(bufferSize);
	//core::SIMDMemCopy(cb.DataPtr, BufferData, core::bitUtil::RoundUpToMultiple(BufferSize, 16) >> 4);
	std::memcpy(cb.getCpuData(), pBufferData, bufferSize);
	pCommandList_->SetComputeRootConstantBufferView(rootIndex, cb.getGpuAddress());
}

void ComputeContext::setDynamicSRV(uint32_t rootIndex, size_t bufferSize, const void* pBufferData)
{
	X_ASSERT_NOT_NULL(pBufferData);
	X_ASSERT_ALIGNMENT(pBufferData, 16, 0);

	DynAlloc cb = cpuLinearAllocator_.allocate(bufferSize);
	core::SIMDMemCopy(cb.getCpuData(), pBufferData, core::bitUtil::RoundUpToMultiple<size_t>(bufferSize, 16u) >> 4);
	pCommandList_->SetComputeRootShaderResourceView(rootIndex, cb.getGpuAddress());
}

void ComputeContext::setBufferSRV(uint32_t rootIndex, const GpuBuffer& SRV, uint64_t offset)
{
	X_ASSERT(core::bitUtil::IsBitFlagSet(SRV.getUsageState(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE), "NON_PIXEL_SHADER_RESOURCE flag missing")(SRV.getUsageState());

	pCommandList_->SetComputeRootShaderResourceView(rootIndex, SRV.getGpuVirtualAddress() + offset);
}

void ComputeContext::setBufferUAV(uint32_t rootIndex, const GpuBuffer& UAV, uint64_t offset)
{
	X_ASSERT(core::bitUtil::IsBitFlagSet(UAV.getUsageState(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS), "UNORDERED_ACCESS flag missing")(UAV.getUsageState());

	pCommandList_->SetComputeRootUnorderedAccessView(rootIndex, UAV.getGpuVirtualAddress());
}

void ComputeContext::setDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
{
	pCommandList_->SetComputeRootDescriptorTable(rootIndex, firstHandle);
}

	 
void ComputeContext::setDynamicDescriptor(uint32_t rootIndex, uint32_t offset, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	setDynamicDescriptors(rootIndex, offset, 1, &handle);
}

void ComputeContext::setDynamicDescriptors(uint32_t rootIndex, uint32_t offset, uint32_t count, const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles)
{
	dynamicDescriptorHeap_.setGraphicsDescriptorHandles(rootIndex, offset, count, pHandles);
}

	 
void ComputeContext::dispatch(size_t groupCountX, size_t groupCountY, size_t groupCountZ)
{
	flushResourceBarriers();
	
	dynamicDescriptorHeap_.commitComputeRootDescriptorTables(pCommandList_);

	pCommandList_->Dispatch(
		safe_static_cast<uint32_t, size_t>(groupCountX),
		safe_static_cast<uint32_t, size_t>(groupCountY),
		safe_static_cast<uint32_t, size_t>(groupCountZ)
	);
}

void ComputeContext::dispatch1D(size_t threadCountX, size_t groupSizeX)
{
	dispatch(divideByMultiple(threadCountX, groupSizeX), 1, 1);
}

void ComputeContext::dispatch2D(size_t threadCountX, size_t threadCountY, size_t groupSizeX, size_t groupSizeY )
{
	dispatch(
		divideByMultiple(threadCountX, groupSizeX),
		divideByMultiple(threadCountY, groupSizeY), 1);
}

void ComputeContext::dispatch3D(size_t threadCountX, size_t threadCountY, size_t threadCountZ,
	size_t groupSizeX, size_t groupSizeY, size_t groupSizeZ)
{
	dispatch(
		divideByMultiple(threadCountX, groupSizeX),
		divideByMultiple(threadCountY, groupSizeY),
		divideByMultiple(threadCountZ, groupSizeZ));
}

void ComputeContext::dispatchIndirect(CommandSignature& dispatchCmdSig, GpuBuffer& argumentBuffer, size_t argumentBufferOffset)
{
	flushResourceBarriers();

	dynamicDescriptorHeap_.commitComputeRootDescriptorTables(pCommandList_);

	pCommandList_->ExecuteIndirect(dispatchCmdSig.getSignature(), 1, argumentBuffer.getResource(),
		static_cast<uint64_t>(argumentBufferOffset), nullptr, 0);
}



X_NAMESPACE_END