
X_NAMESPACE_BEGIN(render)

X_INLINE uint32_t DynamicDescriptorHeap::DescriptorHandleCache::rootDescriptorTablesBitMap(void) const
{
    return rootDescriptorTablesBitMap_;
}

X_INLINE uint32_t DynamicDescriptorHeap::DescriptorHandleCache::staleRootParamsBitMap(void) const
{
    return staleRootParamsBitMap_;
}

X_INLINE uint32_t DynamicDescriptorHeap::DescriptorHandleCache::maxCachedDescriptors(void) const
{
    return maxCachedDescriptors_;
}

// ----------------------------------------------

X_INLINE void DynamicDescriptorHeap::commitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* pCmdList)
{
    if (graphicsHandleCache_.staleRootParamsBitMap() != 0) {
        copyAndBindStagedTables(graphicsHandleCache_, pCmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
    }
}

X_INLINE void DynamicDescriptorHeap::commitComputeRootDescriptorTables(ID3D12GraphicsCommandList* pCmdList)
{
    if (computeHandleCache_.staleRootParamsBitMap() != 0) {
        copyAndBindStagedTables(computeHandleCache_, pCmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
    }
}

X_NAMESPACE_END