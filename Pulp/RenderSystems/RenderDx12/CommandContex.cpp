#include "stdafx.h"
#include "CommandContex.h"
#include "CommandListManger.h"
#include "CommandSignature.h"
#include "RootSignature.h"
#include "Texture\Texture.h"
#include "Buffers\GpuBuffer.h"
#include "Buffers\ColorBuffer.h"
#include "Buffers\DepthBuffer.h"
#include "PipelineState.h"

#include "Allocators\DynamicDescriptorHeap.h"

#if X_DEBUG && _MSC_VER >= 1800
#include <pix.h>
#endif

X_NAMESPACE_BEGIN(render)

namespace
{
    X_INLINE void memcpySubresource(const D3D12_MEMCPY_DEST* pDest, const D3D12_SUBRESOURCE_DATA* pSrc,
        size_t rowSizeInBytes, uint32_t numRows, uint32_t numSlices)
    {
        for (uint32_t z = 0; z < numSlices; ++z) {
            BYTE* pDestSlice = reinterpret_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
            const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * z;

            for (uint32_t y = 0; y < numRows; ++y) {
                memcpy(
                    pDestSlice + pDest->RowPitch * y,
                    pSrcSlice + pSrc->RowPitch * y,
                    rowSizeInBytes);
            }
        }
    }
} // namespace

const uint32_t CommandContext::VALID_COMPUTE_QUEUE_RESOURCE_STATES = (D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE);

X_DISABLE_WARNING(4355) // 'this': used in base member initializer list

ContextManager::ContextManager(core::MemoryArenaBase* arena, ID3D12Device* pDevice,
    CommandListManger& cmdListMan, DescriptorAllocatorPool& pool, LinearAllocatorManager& linAllocMan) :
    arena_(arena),
    pDevice_(pDevice),
    cmdListMan_(cmdListMan),
    pool_(pool),
    linAllocMan_(linAllocMan),
    contextPool_{arena, arena, arena, arena},
    availableContexts_{arena, arena, arena, arena}
{
}

ContextManager::~ContextManager()
{
}

GraphicsContext* ContextManager::allocateGraphicsContext(void)
{
    return reinterpret_cast<GraphicsContext*>(allocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT));
}

CommandContext* ContextManager::allocateContext(D3D12_COMMAND_LIST_TYPE type)
{
    core::CriticalSection::ScopedLock lock(cs_);

    auto& availableContexts = availableContexts_[type];

    CommandContext* pRet = nullptr;
    if (availableContexts.isEmpty()) {
        pRet = X_NEW(CommandContext, arena_, "CmdContex")(*this, arena_, pDevice_, pool_, linAllocMan_, type);
        contextPool_[type].emplace_back(pRet);
        pRet->initialize();
    }
    else {
        pRet = availableContexts.front();
        availableContexts.pop();
        pRet->reset();
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
    dynamicDescriptorHeap_(arena, pDevice, pool, *this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
    dynamicSamplerDescriptorHeap_(arena, pDevice, pool, *this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
    cpuLinearAllocator_(arena, linAllocMan, LinearAllocatorType::CPU_WRITABLE),
    gpuLinearAllocator_(arena, linAllocMan, LinearAllocatorType::GPU_EXCLUSIVE),
    numBarriersToFlush_(0)
{
    core::zero_object(pCurrentDescriptorHeaps_);

#if X_DEBUG
    contexFreed_ = 0;
#endif // !X_DEBUG
}

X_ENABLE_WARNING(4355)

CommandContext::~CommandContext(void)
{
    core::SafeReleaseDX(pCommandList_);

#if X_DEBUG
    // can't just go out of scope, must manually call finishAndFree()
    // not sure i Like this..
    // might make a commandContext scope helper.
    X_ASSERT(contexFreed_ == 1, "COmmandContext was not freed correct.")(contexFreed_);
#endif // !X_DEBUG
}

uint64_t CommandContext::flush(bool waitForCompletion)
{
    flushResourceBarriers();

    X_ASSERT_NOT_NULL(pCurrentAllocator_);

    auto& cmdListMan = contextManager_.getCmdListMan();
    uint64_t fenceValue = cmdListMan.getQueue(type_).executeCommandList(pCommandList_);

    if (waitForCompletion) {
        cmdListMan.waitForFence(fenceValue);
    }

    // Reset the command list and restore previous state

    pCommandList_->Reset(pCurrentAllocator_, nullptr);

    if (pCurGraphicsRootSignature_) {
        X_ASSERT(pCurComputeRootSignature_ == nullptr, "Both graphics and compute root sig set")();
        X_ASSERT(pCurComputePipelineState_ == nullptr, "Both graphics and compute PSO set")();

        pCommandList_->SetGraphicsRootSignature(pCurGraphicsRootSignature_);
        pCommandList_->SetPipelineState(pCurGraphicsPipelineState_);
    }
    if (pCurComputeRootSignature_) {
        X_ASSERT(pCurGraphicsRootSignature_ == nullptr, "Both graphics and compute root sig set")();
        X_ASSERT(pCurGraphicsPipelineState_ == nullptr, "Both graphics and compute PSO set")();

        pCommandList_->SetComputeRootSignature(pCurComputeRootSignature_);
        pCommandList_->SetPipelineState(pCurComputePipelineState_);
    }

    bindDescriptorHeaps();

    return fenceValue;
}

uint64_t CommandContext::finishAndFree(bool waitForCompletion)
{
    X_ASSERT(type_ == D3D12_COMMAND_LIST_TYPE_DIRECT || type_ == D3D12_COMMAND_LIST_TYPE_COMPUTE, "Invalid type")(type_);
    X_ASSERT_NOT_NULL(pCurrentAllocator_);

    flushResourceBarriers();

    auto& cmdListMan = contextManager_.getCmdListMan();
    CommandQue& queue = cmdListMan.getQueue(type_);
    uint64_t fenceValue = queue.executeCommandList(pCommandList_);

    queue.discardAllocator(fenceValue, pCurrentAllocator_);
    pCurrentAllocator_ = nullptr;

    cpuLinearAllocator_.cleanupUsedPages(fenceValue);
    gpuLinearAllocator_.cleanupUsedPages(fenceValue);
    dynamicDescriptorHeap_.cleanupUsedHeaps(fenceValue);
    dynamicSamplerDescriptorHeap_.cleanupUsedHeaps(fenceValue);

    if (waitForCompletion) {
        cmdListMan.waitForFence(fenceValue);
    }

#if X_DEBUG
    contexFreed_ = 1;
#endif // !X_DEBUG

    contextManager_.freeContext(this);

    return fenceValue;
}

void CommandContext::initialize(void)
{
    X_ASSERT(pCommandList_ == nullptr, "Command list already set")();
    X_ASSERT(pCurrentAllocator_ == nullptr, "Command allocator already set")();

    auto& cmdListMan = contextManager_.getCmdListMan();

    cmdListMan.createNewCommandList(type_, &pCommandList_, &pCurrentAllocator_);

#if X_DEBUG
    contexFreed_ = 0;
#endif // !X_DEBUG
}

void CommandContext::reset(void)
{
    // We only call Reset() on previously freed contexts. The command list persists, but we must
    // request a new allocator.
    X_ASSERT_NOT_NULL(pCommandList_);
    X_ASSERT(pCurrentAllocator_ == nullptr, "Command allocator should be null")(pCurrentAllocator_);

    auto& cmdListMan = contextManager_.getCmdListMan();

    pCurrentAllocator_ = cmdListMan.getQueue(type_).requestAllocator();
    pCommandList_->Reset(pCurrentAllocator_, nullptr);

    pCurGraphicsRootSignature_ = nullptr;
    pCurGraphicsPipelineState_ = nullptr;
    pCurComputeRootSignature_ = nullptr;
    pCurComputePipelineState_ = nullptr;
    numBarriersToFlush_ = 0;

    bindDescriptorHeaps();

#if X_DEBUG
    contexFreed_ = 0;
#endif // !X_DEBUG
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

    D3D12_TEXTURE_COPY_LOCATION destLocation = {
        dest.getResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        destSubIndex};

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {
        src.getResource(),
        D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        srcSubIndex};

    pCommandList_->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
}

// used for compute shaders
void CommandContext::copyCounter(GpuResource& dest, size_t destOffset, StructuredBuffer& src)
{
    transitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
    transitionResource(src.getCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    flushResourceBarriers();
    pCommandList_->CopyBufferRegion(dest.getResource(), destOffset, src.getCounterBuffer().getResource(), 0, 4);
}

void CommandContext::resetCounter(StructuredBuffer& buf, uint32_t value)
{
    fillBuffer(buf.getCounterBuffer(), 0, Param(value), sizeof(value));
    transitionResource(buf.getCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void CommandContext::writeBuffer(GpuResource& dest, size_t destOffset, const void* pData, size_t numBytes)
{
    X_ASSERT_NOT_NULL(pData);
    X_ASSERT_ALIGNMENT(pData, 16, 0);

    DynAlloc tempSpace = cpuLinearAllocator_.allocate(numBytes, 512);
    core::Mem::SIMDMemCopy(tempSpace.getCpuData(), pData, divideByMultiple(numBytes, 16));
    copyBufferRegion(dest, destOffset, tempSpace.getBuffer(), tempSpace.getOffset(), numBytes);
}

void CommandContext::writeBufferUnAligned(GpuResource& dest, size_t destOffset, const void* pData, size_t numBytes)
{
    X_ASSERT_NOT_NULL(pData);

    DynAlloc tempSpace = cpuLinearAllocator_.allocate(numBytes, 512);
    std::memcpy(tempSpace.getCpuData(), pData, numBytes);
    copyBufferRegion(dest, destOffset, tempSpace.getBuffer(), tempSpace.getOffset(), numBytes);
}

void CommandContext::fillBuffer(GpuResource& dest, size_t destOffset, Param val, size_t numBytes)
{
    DynAlloc tempSpace = cpuLinearAllocator_.allocate(numBytes, 512);
    __m128 VectorValue = _mm_set1_ps(val.fval);

    core::Mem::SIMDMemFill(tempSpace.getCpuData(), VectorValue, divideByMultiple(numBytes, 16));

    copyBufferRegion(dest, destOffset, tempSpace.getBuffer(), tempSpace.getOffset(), numBytes);
}

void CommandContext::transitionResource(GpuResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
    const D3D12_RESOURCE_STATES oldState = resource.getUsageState();

    if (type_ == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        X_ASSERT((oldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == oldState, "No valid compute states set")(oldState);
        X_ASSERT((newState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == newState, "No valid compute states set")(newState);
    }

    if (oldState != newState) {
        X_ASSERT(numBarriersToFlush_ < 16, "Exceeded arbitrary limit on buffered barriers")(numBarriersToFlush_);
        D3D12_RESOURCE_BARRIER& barrierDesc = resourceBarrierBuffer[numBarriersToFlush_++];

        barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierDesc.Transition.pResource = resource.getResource();
        barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrierDesc.Transition.StateBefore = oldState;
        barrierDesc.Transition.StateAfter = newState;

        // Check to see if we already started the transition
        if (newState == resource.getTransitioningStateState()) {
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
    if (resource.getTransitioningStateState() != static_cast<D3D12_RESOURCE_STATES>(-1)) {
        transitionResource(resource, resource.getTransitioningStateState());
    }

    const D3D12_RESOURCE_STATES oldState = resource.getUsageState();

    if (oldState != newState) {
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

void CommandContext::pIXBeginEvent(const wchar_t* pLabel)
{
#if !X_DEBUG || _MSC_VER < 1800
    X_UNUSED(pLabel);
#else
    ::PIXBeginEvent(pCommandList_, 0, pLabel);
#endif
}

void CommandContext::pIXEndEvent(void)
{
#if X_DEBUG && _MSC_VER >= 1800
    ::PIXEndEvent(pCommandList_);
#endif
}

void CommandContext::pIXSetMarker(const wchar_t* pLabel)
{
#if !X_DEBUG || _MSC_VER < 1800
    X_UNUSED(pLabel);
#else
    ::PIXSetMarker(pCommandList_, 0, pLabel);
#endif
}

void CommandContext::setDescriptorHeaps(uint32_t heapCount, D3D12_DESCRIPTOR_HEAP_TYPE* pTypes,
    ID3D12DescriptorHeap** pHeapPtrs)
{
    bool anyChanged = false;

    X_ASSERT(heapCount <= 2, "Can't set more than 2 descriptor heaps")(heapCount);

    for (uint32_t i = 0; i < heapCount; ++i) {
        X_ASSERT(pTypes[i] != D3D12_DESCRIPTOR_HEAP_TYPE_RTV, "Heap type RTV not allowed on command list")(pTypes[i]);
        X_ASSERT(pTypes[i] != D3D12_DESCRIPTOR_HEAP_TYPE_DSV, "Heap type DSV not allowed on command list")(pTypes[i]);

        if (pCurrentDescriptorHeaps_[pTypes[i]] != pHeapPtrs[i]) {
            pCurrentDescriptorHeaps_[pTypes[i]] = pHeapPtrs[i];
            anyChanged = true;
        }
    }

    if (anyChanged) {
        bindDescriptorHeaps();
    }
}

void CommandContext::bindDescriptorHeaps(void)
{
    uint32_t nonNullHeaps = 0;
    ID3D12DescriptorHeap* pHeapsToBind[MAX_DISTRIPTOR_HEAP_TYPES];

    // make sure the enum's we allow are as expected.
    // if not we need to change how we cache / store them in pCurrentDescriptorHeaps_
    static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == 0, "Enum changed");
    static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER == 1, "Enum changed");

    for (uint32_t i = 0; i < MAX_DISTRIPTOR_HEAP_TYPES; ++i) {
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

uint64_t CommandContext::updateSubresources(
    GpuResource& dest,
    ID3D12Resource* pIntermediate,
    uint32_t firstSubresource,
    uint32_t numSubresources,
    uint64_t requiredSize,
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
    const uint32_t* pNumRows,
    const uint64_t* pRowSizesInBytes,
    const D3D12_SUBRESOURCE_DATA* pSrcData)
{
    ID3D12Resource* pDestinationResource = dest.getResource();

    // Minor validation
    const D3D12_RESOURCE_DESC intermediateDesc = pIntermediate->GetDesc();
    const D3D12_RESOURCE_DESC destinationDesc = pDestinationResource->GetDesc();

    {
        const uint64_t requiredWidth = requiredSize + pLayouts[0].Offset;

        if (intermediateDesc.Width < requiredWidth) {
            X_ERROR("CmdContex", "intermidiate width is less than required width: %" PRIu64 " required: %" PRIu64,
                intermediateDesc.Width, requiredWidth);
            return 0;
        }

        if (intermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
            X_ERROR("CmdContex", "intermidiate dimension is not of type buffer");
            return 0;
        }

        // if D3D12_RESOURCE_DIMENSION_BUFFER
        // numSubRes should be 1
        // firstSub should not be zero
        if (destinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && (firstSubresource != 0 || numSubresources != 1)) {
            X_ERROR("CmdContex", "you done fucked up!");
            return 0;
        }
    }

    {
        BYTE* pData;
        HRESULT hr = pIntermediate->Map(0, NULL, reinterpret_cast<void**>(&pData));
        if (FAILED(hr)) {
            X_ERROR("CmdContex", "failed to map intermediate buffer. err: %" PRIu32, hr);
            return 0;
        }

        for (uint32_t i = 0; i < numSubresources; ++i) {
            if (pRowSizesInBytes[i] > std::numeric_limits<size_t>::max()) {
                return 0;
            }

            D3D12_MEMCPY_DEST DestData = {
                pData + pLayouts[i].Offset,
                pLayouts[i].Footprint.RowPitch,
                pLayouts[i].Footprint.RowPitch * pNumRows[i]};

            memcpySubresource(&DestData, &pSrcData[i], safe_static_cast<size_t, uint64_t>(pRowSizesInBytes[i]),
                pNumRows[i], pLayouts[i].Footprint.Depth);
        }

        pIntermediate->Unmap(0, NULL);
    }

    if (destinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        CD3DX12_BOX SrcBox(
            uint32_t(pLayouts[0].Offset),
            uint32_t(pLayouts[0].Offset + pLayouts[0].Footprint.Width));

        pCommandList_->CopyBufferRegion(pDestinationResource, 0, pIntermediate,
            pLayouts[0].Offset, pLayouts[0].Footprint.Width);
    }
    else {
        for (uint32_t i = 0; i < numSubresources; ++i) {
            CD3DX12_TEXTURE_COPY_LOCATION dst(pDestinationResource, i + firstSubresource);
            CD3DX12_TEXTURE_COPY_LOCATION src(pIntermediate, pLayouts[i]);

            pCommandList_->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        }
    }
    return requiredSize;
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
    CD3DX12_RECT ClearRect(0, 0, target.getTex().getWidth(), target.getTex().getHeight());

    pCommandList_->ClearUnorderedAccessViewFloat(gpuVisibleHandle, target.getUAV(), target.getGpuResource().getResource(),
        target.getClearColor(), 1, &ClearRect);
}

// not inline so can forward declare.
void GraphicsContext::clearColor(const ColorBuffer& target)
{
    pCommandList_->ClearRenderTargetView(target.getRTV(), target.getClearColor(), 0, nullptr);
}

void GraphicsContext::clearDepth(const DepthBuffer& target)
{
    pCommandList_->ClearDepthStencilView(target.getDSV(), D3D12_CLEAR_FLAG_DEPTH,
        target.getClearDepth(), safe_static_cast<uint8_t>(target.getClearStencil()), 0, nullptr);
}

void GraphicsContext::clearStencil(const DepthBuffer& target)
{
    pCommandList_->ClearDepthStencilView(target.getDSV(), D3D12_CLEAR_FLAG_STENCIL,
        target.getClearDepth(), target.getClearStencil(), 0, nullptr);
}

void GraphicsContext::clearDepthAndStencil(const DepthBuffer& target)
{
    pCommandList_->ClearDepthStencilView(target.getDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        target.getClearDepth(), target.getClearStencil(), 0, nullptr);
}

void GraphicsContext::setRootSignature(const RootSignature& rootSig)
{
    if (rootSig.getSignature() == pCurGraphicsRootSignature_) {
        return;
    }

    pCommandList_->SetGraphicsRootSignature(pCurGraphicsRootSignature_ = rootSig.getSignature());

    dynamicDescriptorHeap_.parseGraphicsRootSignature(rootSig);
    dynamicSamplerDescriptorHeap_.parseGraphicsRootSignature(rootSig);
}

void GraphicsContext::setPipelineState(const GraphicsPSO& PSO)
{
    setPipelineState(PSO.getPipelineStateObject());
}

void GraphicsContext::setPipelineState(ID3D12PipelineState* pPso)
{
    if (pPso == pCurGraphicsPipelineState_) {
        return;
    }

    pCommandList_->SetPipelineState(pPso);
    pCurGraphicsPipelineState_ = pPso;
}

void GraphicsContext::setDynamicCBV(uint32_t rootIndex, size_t bufferSize,
    const void* pBufferData)
{
    X_ASSERT_NOT_NULL(pBufferData);
    X_ASSERT_ALIGNMENT(pBufferData, 16, 0);

    DynAlloc cb = cpuLinearAllocator_.allocate(bufferSize);
    memcpy(cb.getCpuData(), pBufferData, bufferSize);
    pCommandList_->SetGraphicsRootConstantBufferView(rootIndex, cb.getGpuAddress());
}

void GraphicsContext::setBufferSRV(uint32_t rootIndex, const GpuBuffer& SRV, uint64_t offset)
{
    X_ASSERT(((SRV.getUsageState() & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0), "Missing one of required usage states.")(SRV.getUsageState());

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

void GraphicsContext::setDynamicVB(uint32_t slot, size_t numVertices, size_t vertexStride,
    const void* pVBData)
{
    X_ASSERT_NOT_NULL(pVBData);
    X_ASSERT_ALIGNMENT(pVBData, 16, 0);

    size_t bufferSize = core::bitUtil::RoundUpToMultiple<size_t>(numVertices * vertexStride, 16u);
    DynAlloc vb = cpuLinearAllocator_.allocate(bufferSize);

    core::Mem::SIMDMemCopy(vb.getCpuData(), pVBData, bufferSize >> 4);

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

    core::Mem::SIMDMemCopy(ib.getCpuData(), pIBData, bufferSize >> 4);

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
    core::Mem::SIMDMemCopy(cb.getCpuData(), pBufferData, bufferSize >> 4);

    pCommandList_->SetGraphicsRootShaderResourceView(rootIndex, cb.getGpuAddress());
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
    const uint32_t clearColor[4] = {};
    pCommandList_->ClearUnorderedAccessViewUint(GpuVisibleHandle, target.getUAV(), target.getResource(), clearColor, 0, nullptr);
}

void ComputeContext::clearUAV(ColorBuffer& target)
{
    // After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
    // a shader to set all of the values).
    D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = dynamicDescriptorHeap_.uploadDirect(target.getUAV());
    CD3DX12_RECT ClearRect(0, 0, target.getTex().getWidth(), target.getTex().getHeight());

    pCommandList_->ClearUnorderedAccessViewFloat(GpuVisibleHandle, target.getUAV(), target.getGpuResource().getResource(),
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

void ComputeContext::setDynamicCBV(uint32_t rootIndex, size_t bufferSize, const void* pBufferData)
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
    core::Mem::SIMDMemCopy(cb.getCpuData(), pBufferData, core::bitUtil::RoundUpToMultiple<size_t>(bufferSize, 16u) >> 4);
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

void ComputeContext::dispatchIndirect(CommandSignature& dispatchCmdSig, GpuBuffer& argumentBuffer, size_t argumentBufferOffset)
{
    flushResourceBarriers();

    dynamicDescriptorHeap_.commitComputeRootDescriptorTables(pCommandList_);

    pCommandList_->ExecuteIndirect(dispatchCmdSig.getSignature(), 1, argumentBuffer.getResource(),
        static_cast<uint64_t>(argumentBufferOffset), nullptr, 0);
}

X_NAMESPACE_END