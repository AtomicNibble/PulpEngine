

X_NAMESPACE_BEGIN(render)

X_INLINE Param::Param(float32_t f) :
    fval(f)
{
}
X_INLINE Param::Param(uint32_t u) :
    uint(u)
{
}
X_INLINE Param::Param(int32_t i) :
    sint(i)
{
}

X_INLINE void Param::operator=(float32_t f)
{
    fval = f;
}
X_INLINE void Param::operator=(uint32_t u)
{
    uint = u;
}
X_INLINE void Param::operator=(int32_t i)
{
    sint = i;
}

// ----------------------------------

X_INLINE const CommandListManger& ContextManager::getCmdListMan(void) const
{
    return cmdListMan_;
}

X_INLINE CommandListManger& ContextManager::getCmdListMan(void)
{
    return cmdListMan_;
}

// ----------------------------------

X_INLINE void CommandContext::copyBufferRegionRaw(GpuResource& dest, size_t destOffset, ID3D12Resource* pSrc,
    size_t srcOffset, size_t numBytes)
{
    pCommandList_->CopyBufferRegion(dest.getResource(), destOffset, pSrc, srcOffset, numBytes);
}

X_INLINE void CommandContext::copyResourceRaw(GpuResource& dest, ID3D12Resource* pSrc)
{
    pCommandList_->CopyResource(dest.getResource(), pSrc);
}

X_INLINE DynAlloc CommandContext::AllocUploadBuffer(size_t sizeInBytes)
{
    return cpuLinearAllocator_.allocate(sizeInBytes);
}

X_INLINE void CommandContext::insertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t queryIdx)
{
    pCommandList_->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, queryIdx);
}

X_INLINE void CommandContext::resolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap,
    uint32_t numQueries)
{
    pCommandList_->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, numQueries, pReadbackHeap, 0);
}

X_INLINE void CommandContext::setPredication(ID3D12Resource* pBuffer, uint64_t bufferOffset,
    D3D12_PREDICATION_OP op)
{
    pCommandList_->SetPredication(pBuffer, bufferOffset, op);
}

X_INLINE void CommandContext::setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* pHeapPtr)
{
    X_ASSERT(type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV, "Heap type RTV not allowed on command list")
    (type);
    X_ASSERT(type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV, "Heap type DSV not allowed on command list")
    (type);

    if (pCurrentDescriptorHeaps_[type] != pHeapPtr) {
        pCurrentDescriptorHeaps_[type] = pHeapPtr;
        bindDescriptorHeaps();
    }
}

X_INLINE void CommandContext::flushResourceBarriers(void)
{
    if (numBarriersToFlush_ > 0) {
        X_ASSERT_NOT_NULL(pCommandList_);

        pCommandList_->ResourceBarrier(numBarriersToFlush_, resourceBarrierBuffer);
        numBarriersToFlush_ = 0;
    }
}

// ----------------------------------

X_INLINE D3D12_COMMAND_LIST_TYPE CommandContext::getType(void) const
{
    return type_;
}

X_INLINE GraphicsContext& CommandContext::getGraphicsContext(void)
{
    X_ASSERT(type_ != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics")
    (type_);
    return reinterpret_cast<GraphicsContext&>(*this);
}

X_INLINE ComputeContext& CommandContext::getComputeContext(void)
{
    return reinterpret_cast<ComputeContext&>(*this);
}

X_INLINE void GraphicsContext::beginQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type,
    uint32_t heapIndex)
{
    pCommandList_->BeginQuery(pQueryHeap, type, heapIndex);
}

X_INLINE void GraphicsContext::endQuery(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type,
    uint32_t heapIndex)
{
    pCommandList_->EndQuery(pQueryHeap, type, heapIndex);
}

X_INLINE void GraphicsContext::resolveQueryData(ID3D12QueryHeap* pQueryHeap, D3D12_QUERY_TYPE type,
    uint32_t startIndex, uint32_t numQueries, ID3D12Resource* pDestinationBuffer,
    uint64_t destinationBufferOffset)
{
    pCommandList_->ResolveQueryData(pQueryHeap, type, startIndex, numQueries, pDestinationBuffer,
        destinationBufferOffset);
}

X_INLINE void GraphicsContext::setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs)
{
    pCommandList_->OMSetRenderTargets(numRTVs, pRTVs, FALSE, nullptr);
}

X_INLINE void GraphicsContext::setRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVs,
    D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
    pCommandList_->OMSetRenderTargets(numRTVs, pRTVs, FALSE, &DSV);
}

X_INLINE void GraphicsContext::setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV)
{
    setRenderTargets(1, &RTV);
}

X_INLINE void GraphicsContext::setRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV,
    D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
    setRenderTargets(1, &RTV, DSV);
}

X_INLINE void GraphicsContext::setDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
    setRenderTargets(0, nullptr, DSV);
}

X_INLINE void GraphicsContext::setViewport(const XViewPort& vp)
{
    D3D12_VIEWPORT dvp;
    dvp.Width = vp.getWidthf();
    dvp.Height = vp.getHeightf();
    dvp.TopLeftX = vp.getXf();
    dvp.TopLeftY = vp.getYf();
    dvp.MinDepth = vp.getZNear();
    dvp.MaxDepth = vp.getZFar();

    setViewport(dvp);
}

X_INLINE void GraphicsContext::setViewport(const D3D12_VIEWPORT& vp)
{
#if X_DEBUG
    if (vp.MinDepth == 0.f && vp.MaxDepth == 0.f) {
        X_WARNING_EVERY_N(60, "Dx12", "Viewport min and max depth are zero is this intended?");
    }
#endif // !X_DEBUG

    pCommandList_->RSSetViewports(1, &vp);
}

X_INLINE void GraphicsContext::setViewport(float32_t x, float32_t y, float32_t w, float32_t h,
    float32_t minDepth, float32_t maxDepth)
{
    D3D12_VIEWPORT vp;
    vp.Width = w;
    vp.Height = h;
    vp.MinDepth = minDepth;
    vp.MaxDepth = maxDepth;
    vp.TopLeftX = x;
    vp.TopLeftY = y;
    setViewport(vp);
}

X_INLINE void GraphicsContext::setScissor(const D3D12_RECT& rect)
{
    X_ASSERT(rect.left < rect.right && rect.top < rect.bottom, "Invalid rect")
    ();
    pCommandList_->RSSetScissorRects(1, &rect);
}

X_INLINE void GraphicsContext::setScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    D3D12_RECT rect;
    rect.left = left;
    rect.right = right;
    rect.top = top;
    rect.bottom = bottom;
    setScissor(rect);
}

X_INLINE void GraphicsContext::setViewportAndScissor(const XViewPort& vp)
{
    setViewport(vp);
    const auto r = vp.getRect();

    D3D12_RECT rect;
    rect.left = r.getX1();
    rect.right = r.getX2();
    rect.top = r.getY1();
    rect.bottom = r.getY2();
    setScissor(rect);
}

X_INLINE void GraphicsContext::setViewportAndScissor(const XViewPort& vp, const D3D12_RECT& rect)
{
    X_ASSERT(rect.left < rect.right && rect.top < rect.bottom, "Invalid rect")
    ();
    setViewport(vp);
    setScissor(rect);
}

X_INLINE void GraphicsContext::setViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
    X_ASSERT(rect.left < rect.right && rect.top < rect.bottom, "Invalid rect")
    ();
    setViewport(vp);
    setScissor(rect);
}

X_INLINE void GraphicsContext::setViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    setViewport(static_cast<float32_t>(x), static_cast<float32_t>(y),
        static_cast<float32_t>(w), static_cast<float32_t>(h));
    setScissor(x, y, x + w, y + h);
}

X_INLINE void GraphicsContext::setStencilRef(uint32_t stencilRef)
{
    pCommandList_->OMSetStencilRef(stencilRef);
}

X_INLINE void GraphicsContext::setBlendFactor(Color8u blendFactor)
{
    Colorf col(blendFactor);
    pCommandList_->OMSetBlendFactor(col);
}

X_INLINE void GraphicsContext::setBlendFactor(const Colorf& blendFactor)
{
    pCommandList_->OMSetBlendFactor(blendFactor);
}

X_INLINE void GraphicsContext::setPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
    pCommandList_->IASetPrimitiveTopology(topology);
}

X_INLINE void GraphicsContext::setConstants(uint32_t rootIndex, uint32_t numConstants,
    const void* pConstants)
{
    pCommandList_->SetGraphicsRoot32BitConstants(rootIndex, numConstants, pConstants, 0);
}

X_INLINE void GraphicsContext::setConstants(uint32_t rootIndex, Param X)
{
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, X.uint, 0);
}

X_INLINE void GraphicsContext::setConstants(uint32_t rootIndex, Param X, Param Y)
{
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, X.uint, 0);
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, Y.uint, 1);
}

X_INLINE void GraphicsContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z)
{
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, X.uint, 0);
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, Y.uint, 1);
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, Z.uint, 2);
}

X_INLINE void GraphicsContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z, Param W)
{
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, X.uint, 0);
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, Y.uint, 1);
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, Z.uint, 2);
    pCommandList_->SetGraphicsRoot32BitConstant(rootIndex, W.uint, 3);
}

X_INLINE void GraphicsContext::setConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
    pCommandList_->SetGraphicsRootConstantBufferView(rootIndex, CBV);
}

X_INLINE void GraphicsContext::setDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
{
    pCommandList_->SetGraphicsRootDescriptorTable(rootIndex, firstHandle);
}

X_INLINE void GraphicsContext::setDynamicDescriptor(uint32_t rootIndex, uint32_t offset,
    D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    setDynamicDescriptors(rootIndex, offset, 1, &handle);
}

X_INLINE void GraphicsContext::setDynamicDescriptors(uint32_t rootIndex, uint32_t offset, uint32_t count,
    const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles)
{
#if X_ENABLE_ASSERTIONS
    for (uint32_t i = 0; i < count; i++) {
        X_ASSERT(pHandles[i].ptr != 0 && pHandles[i].ptr != std::numeric_limits<size_t>::max(), "Invalid handle")
        ();
    }
#endif // !X_ENABLE_ASSERTIONS

    dynamicDescriptorHeap_.setGraphicsDescriptorHandles(rootIndex, offset, count, pHandles);
}

X_INLINE void GraphicsContext::setDynamicSamplerDescriptor(uint32_t rootIndex, uint32_t offset,
    D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    setDynamicSamplerDescriptors(rootIndex, offset, 1, &handle);
}

X_INLINE void GraphicsContext::setDynamicSamplerDescriptors(uint32_t rootIndex, uint32_t offset, uint32_t count,
    const D3D12_CPU_DESCRIPTOR_HANDLE* pHandles)
{
#if X_ENABLE_ASSERTIONS
    for (uint32_t i = 0; i < count; i++) {
        X_ASSERT(pHandles[i].ptr != 0 && pHandles[i].ptr != std::numeric_limits<size_t>::max(), "Invalid handle")
        ();
    }
#endif // !X_ENABLE_ASSERTIONS

    dynamicSamplerDescriptorHeap_.setGraphicsDescriptorHandles(rootIndex, offset, count, pHandles);
}

X_INLINE void GraphicsContext::setIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
{
    pCommandList_->IASetIndexBuffer(&IBView);
}

X_INLINE void GraphicsContext::setVertexBuffer(uint32_t slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
{
    pCommandList_->IASetVertexBuffers(slot, 1, &VBView);
}

X_INLINE void GraphicsContext::setVertexBuffers(uint32_t startSlot, uint32_t count,
    const D3D12_VERTEX_BUFFER_VIEW* pVBViews)
{
    pCommandList_->IASetVertexBuffers(startSlot, count, pVBViews);
}

X_INLINE void GraphicsContext::draw(uint32_t vertexCount, uint32_t vertexStartOffset)
{
    drawInstanced(vertexCount, 1, vertexStartOffset, 0);
}

X_INLINE void GraphicsContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation,
    int32_t baseVertexLocation)
{
    drawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}

X_INLINE void GraphicsContext::drawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
    uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
    flushResourceBarriers();
    dynamicDescriptorHeap_.commitGraphicsRootDescriptorTables(pCommandList_);
    dynamicSamplerDescriptorHeap_.commitGraphicsRootDescriptorTables(pCommandList_);
    pCommandList_->DrawInstanced(vertexCountPerInstance, instanceCount,
        startVertexLocation, startInstanceLocation);
}

X_INLINE void GraphicsContext::drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount,
    uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
    flushResourceBarriers();
    dynamicDescriptorHeap_.commitGraphicsRootDescriptorTables(pCommandList_);
    dynamicSamplerDescriptorHeap_.commitGraphicsRootDescriptorTables(pCommandList_);
    pCommandList_->DrawIndexedInstanced(indexCountPerInstance, instanceCount,
        startIndexLocation, baseVertexLocation, startInstanceLocation);
}

template<uint32_t maxSubresources>
X_INLINE uint64_t CommandContext::updateSubresources(
    ID3D12Device* pDevice,
    GpuResource& dest,
    ID3D12Resource* pIntermediate,
    uint64_t intermediateOffset,
    uint32_t firstSubresource,
    uint32_t numSubresources,
    D3D12_SUBRESOURCE_DATA* pSrcData)
{
    uint64_t requiredSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[maxSubresources];
    uint32_t numRows[maxSubresources];
    uint64_t rowSizesInBytes[maxSubresources];

    ID3D12Resource* pDestinationResource = dest.getResource();

    D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
    pDevice->GetCopyableFootprints(&Desc, firstSubresource, numSubresources, intermediateOffset,
        layouts, numRows, rowSizesInBytes, &requiredSize);

    return updateSubresources(dest, pIntermediate, firstSubresource, numSubresources,
        requiredSize, layouts, numRows, rowSizesInBytes, pSrcData);
}

// --------------------------------------------------------------

X_INLINE void ComputeContext::setConstants(uint32_t rootIndex, uint32_t numConstants, const void* pConstants)
{
    pCommandList_->SetComputeRoot32BitConstants(rootIndex, numConstants, pConstants, 0);
}

X_INLINE void ComputeContext::setConstants(uint32_t rootIndex, Param X)
{
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
}

X_INLINE void ComputeContext::setConstants(uint32_t rootIndex, Param X, Param Y)
{
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, Y.uint, 1);
}

X_INLINE void ComputeContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z)
{
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, Y.uint, 1);
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, Z.uint, 2);
}

X_INLINE void ComputeContext::setConstants(uint32_t rootIndex, Param X, Param Y, Param Z, Param W)
{
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, X.uint, 0);
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, Y.uint, 1);
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, Z.uint, 2);
    pCommandList_->SetComputeRoot32BitConstant(rootIndex, W.uint, 3);
}

X_INLINE void ComputeContext::setConstantBuffer(uint32_t rootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
    pCommandList_->SetComputeRootConstantBufferView(rootIndex, CBV);
}

X_INLINE void ComputeContext::setDescriptorTable(uint32_t rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
{
    pCommandList_->SetComputeRootDescriptorTable(rootIndex, firstHandle);
}

X_INLINE void ComputeContext::dispatch(size_t groupCountX, size_t groupCountY, size_t groupCountZ)
{
    flushResourceBarriers();

    dynamicDescriptorHeap_.commitComputeRootDescriptorTables(pCommandList_);
    dynamicSamplerDescriptorHeap_.commitComputeRootDescriptorTables(pCommandList_);

    pCommandList_->Dispatch(
        safe_static_cast<uint32_t, size_t>(groupCountX),
        safe_static_cast<uint32_t, size_t>(groupCountY),
        safe_static_cast<uint32_t, size_t>(groupCountZ));
}

X_INLINE void ComputeContext::dispatch1D(size_t threadCountX, size_t groupSizeX)
{
    dispatch(divideByMultiple(threadCountX, groupSizeX), 1, 1);
}

X_INLINE void ComputeContext::dispatch2D(size_t threadCountX, size_t threadCountY, size_t groupSizeX, size_t groupSizeY)
{
    dispatch(
        divideByMultiple(threadCountX, groupSizeX),
        divideByMultiple(threadCountY, groupSizeY), 1);
}

X_INLINE void ComputeContext::dispatch3D(size_t threadCountX, size_t threadCountY, size_t threadCountZ,
    size_t groupSizeX, size_t groupSizeY, size_t groupSizeZ)
{
    dispatch(
        divideByMultiple(threadCountX, groupSizeX),
        divideByMultiple(threadCountY, groupSizeY),
        divideByMultiple(threadCountZ, groupSizeZ));
}

X_NAMESPACE_END