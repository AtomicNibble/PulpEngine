#include "stdafx.h"
#include "ReadbackBuffer.h"

X_NAMESPACE_BEGIN(render)

ReadbackBuffer::~ReadbackBuffer() 
{
    destroy(); 
}

void ReadbackBuffer::create(ID3D12Device* pDevice, uint32_t numElements, uint32_t elementSize)
{
    elementCount_ = numElements;
    elementSize_ = elementSize;
    bufferSize_ = numElements * elementSize;
    usageState_ = D3D12_RESOURCE_STATE_COPY_DEST;

    // Create a readback buffer large enough to hold all texel data
    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    // Readback buffers must be 1-dimensional, i.e. "buffer" not "texture2d"
    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Width = bufferSize_;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = pDevice->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pResource_));
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to create commited resource. err: %" PRIu32, hr);
    }

    gpuVirtualAddress_ = pResource_->GetGPUVirtualAddress();
}

void* ReadbackBuffer::map(void)
{
    void* Memory;
    pResource_->Map(0, &CD3DX12_RANGE(0, bufferSize_), &Memory);
    return Memory;
}

void ReadbackBuffer::unmap(void)
{
    pResource_->Unmap(0, &CD3DX12_RANGE(0, 0));
}


X_NAMESPACE_END
