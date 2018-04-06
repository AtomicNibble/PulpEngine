#include "stdafx.h"
#include "GpuBuffer.h"

#include "Allocators\DescriptorAllocator.h"
#include "CommandContex.h"

X_NAMESPACE_BEGIN(render)

GpuBuffer::GpuBuffer() :
    bufferSize_(0),
    elementCount_(0),
    elementSize_(0)
{
    resourceFlags_ = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    UAV_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    SRV_.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

GpuBuffer::~GpuBuffer()
{
}

void GpuBuffer::destroy(void)
{
    GpuResource::destroy();
}

void GpuBuffer::create(ID3D12Device* pDevice, ContextManager& contexMan,
    DescriptorAllocator& allocator, uint32_t numElements, uint32_t elementSize,
    const void* pInitialData)
{
    bufferSize_ = numElements * elementSize;
    elementCount_ = numElements;
    elementSize_ = elementSize;
    usageState_ = D3D12_RESOURCE_STATE_COMMON;

    D3D12_RESOURCE_DESC resourceDesc = describeBuffer();

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    HRESULT hr = pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, usageState_, nullptr, IID_PPV_ARGS(&pResource_));
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to create commited resource: %" PRIu32, hr);
    }

    gpuVirtualAddress_ = pResource_->GetGPUVirtualAddress();

    if (pInitialData) {
        initializeBuffer(pDevice, contexMan, pInitialData, bufferSize_);
    }

    createDerivedViews(pDevice, contexMan, allocator);
}

void GpuBuffer::createPlaced(ID3D12Device* pDevice, ContextManager& contexMan,
    DescriptorAllocator& allocator, ID3D12Heap* pBackingHeap,
    uint32_t heapOffset, uint32_t numElements, uint32_t elementSize, const void* pInitialData)
{
    bufferSize_ = numElements * elementSize;
    elementCount_ = numElements;
    elementSize_ = elementSize;
    usageState_ = D3D12_RESOURCE_STATE_COMMON;

    D3D12_RESOURCE_DESC resourceDesc = describeBuffer();

    HRESULT hr = pDevice->CreatePlacedResource(pBackingHeap, heapOffset, &resourceDesc,
        usageState_, nullptr, IID_PPV_ARGS(&pResource_));
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to create placed resource: %" PRIu32, hr);
    }

    gpuVirtualAddress_ = pResource_->GetGPUVirtualAddress();

    if (pInitialData) {
        initializeBuffer(pDevice, contexMan, pInitialData, bufferSize_);
    }

    createDerivedViews(pDevice, contexMan, allocator);
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuBuffer::createConstantBufferView(ID3D12Device* pDevice, DescriptorAllocator& allocator,
    uint32_t offset, uint32_t size) const
{
    X_ASSERT(offset + size <= bufferSize_, "Range out of range")
    (offset, size, offset + size, bufferSize_);

    size = core::bitUtil::RoundUpToMultiple(size, 16u);

    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    CBVDesc.BufferLocation = gpuVirtualAddress_ + static_cast<size_t>(offset);
    CBVDesc.SizeInBytes = size;

    D3D12_CPU_DESCRIPTOR_HANDLE hCBV = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    pDevice->CreateConstantBufferView(&CBVDesc, hCBV);
    return hCBV;
}

D3D12_VERTEX_BUFFER_VIEW GpuBuffer::vertexBufferView(size_t offset, uint32_t size, uint32_t stride) const
{
    D3D12_VERTEX_BUFFER_VIEW VBView;
    VBView.BufferLocation = gpuVirtualAddress_ + offset;
    VBView.SizeInBytes = size;
    VBView.StrideInBytes = stride;
    return VBView;
}

D3D12_INDEX_BUFFER_VIEW GpuBuffer::indexBufferView(size_t offset, uint32_t size, bool b32Bit) const
{
    D3D12_INDEX_BUFFER_VIEW IBView;
    IBView.BufferLocation = gpuVirtualAddress_ + offset;
    IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    IBView.SizeInBytes = size;
    return IBView;
}

D3D12_RESOURCE_DESC GpuBuffer::describeBuffer(void)
{
    X_ASSERT(bufferSize_ > 0, "buffer size not set")
    (bufferSize_);

    D3D12_RESOURCE_DESC desc;
    core::zero_object(desc);
    desc.Alignment = 0;
    desc.DepthOrArraySize = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Flags = resourceFlags_;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Height = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Width = static_cast<uint64_t>(bufferSize_);
    return desc;
}

void GpuBuffer::initializeBuffer(ID3D12Device* pDevice, ContextManager& contexMan,
    const void* pData, size_t numBytes, bool useOffset, size_t offset)
{
    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC bufferDesc;
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = numBytes;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* pUploadBuffer;

    HRESULT hr = pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pUploadBuffer));
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to create commited resource: %" PRIu32, hr);
    }

    void* pDestAddress;
    pUploadBuffer->Map(0, nullptr, &pDestAddress);
    core::Mem::SIMDMemCopy(pDestAddress, pData, divideByMultiple(numBytes, 16));
    pUploadBuffer->Unmap(0, nullptr);

    render::CommandContext* pContext = contexMan.allocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);

    // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
    pContext->transitionResource(*this, D3D12_RESOURCE_STATE_COPY_DEST, true);

    if (useOffset) {
        pContext->copyBufferRegionRaw(*this, offset, pUploadBuffer, 0, numBytes);
    }
    else {
        pContext->copyResourceRaw(*this, pUploadBuffer);
    }

    pContext->transitionResource(*this, D3D12_RESOURCE_STATE_GENERIC_READ, true);

    // Execute the command list and wait for it to finish so we can release the upload buffer
    pContext->finishAndFree(true);

    pUploadBuffer->Release();
}

// -------------------------------------------------

void ByteAddressBuffer::createDerivedViews(ID3D12Device* pDevice, ContextManager& contexMan,
    DescriptorAllocator& allocator)
{
    X_UNUSED(contexMan);

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    core::zero_object(SRVDesc);
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Buffer.NumElements = static_cast<uint32_t>(bufferSize_ / 4);
    SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

    if (SRV_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
        SRV_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    pDevice->CreateShaderResourceView(pResource_, &SRVDesc, SRV_);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
    core::zero_object(UAVDesc);
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    UAVDesc.Buffer.NumElements = static_cast<uint32_t>(bufferSize_ / 4);
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

    if (UAV_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
        UAV_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    pDevice->CreateUnorderedAccessView(pResource_, nullptr, &UAVDesc, UAV_);
}

// -------------------------------------------------

void StructuredBuffer::destroy(void)
{
    counterBuffer_.destroy();
    GpuBuffer::destroy();
}

ByteAddressBuffer& StructuredBuffer::getCounterBuffer(void)
{
    return counterBuffer_;
}

const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::getCounterSRV(CommandContext& context)
{
    context.transitionResource(counterBuffer_, D3D12_RESOURCE_STATE_GENERIC_READ);
    return counterBuffer_.getSRV();
}

const D3D12_CPU_DESCRIPTOR_HANDLE& StructuredBuffer::getCounterUAV(CommandContext& context)
{
    context.transitionResource(counterBuffer_, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    return counterBuffer_.getUAV();
}

void StructuredBuffer::createDerivedViews(ID3D12Device* pDevice, ContextManager& contexMan,
    DescriptorAllocator& allocator)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    core::zero_object(SRVDesc);
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Buffer.NumElements = elementCount_;
    SRVDesc.Buffer.StructureByteStride = elementSize_;
    SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    if (SRV_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
        SRV_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    pDevice->CreateShaderResourceView(pResource_, &SRVDesc, SRV_);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
    core::zero_object(UAVDesc);
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    UAVDesc.Buffer.CounterOffsetInBytes = 0;
    UAVDesc.Buffer.NumElements = elementCount_;
    UAVDesc.Buffer.StructureByteStride = elementSize_;
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    counterBuffer_.create(pDevice, contexMan, allocator, 1, 4);

    if (UAV_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
        UAV_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    pDevice->CreateUnorderedAccessView(pResource_, counterBuffer_.getResource(), &UAVDesc, UAV_);
}

// -------------------------------------------------

TypedBuffer::TypedBuffer(DXGI_FORMAT format) :
    dataFormat_(format)
{
}

void TypedBuffer::createDerivedViews(ID3D12Device* pDevice, ContextManager& contexMan,
    DescriptorAllocator& allocator)
{
    X_UNUSED(contexMan);

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    core::zero_object(SRVDesc);
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = dataFormat_;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Buffer.NumElements = elementCount_;
    SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    if (SRV_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
        SRV_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    pDevice->CreateShaderResourceView(pResource_, &SRVDesc, SRV_);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
    core::zero_object(UAVDesc);
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = dataFormat_;
    UAVDesc.Buffer.NumElements = elementCount_;
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    if (UAV_.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
        UAV_ = allocator.allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    pDevice->CreateUnorderedAccessView(pResource_, nullptr, &UAVDesc, UAV_);
}

X_NAMESPACE_END