#pragma once

#include "GpuResource.h"

X_NAMESPACE_BEGIN(render)

class CommandContext;
class DescriptorAllocator;
class ContextManager;
class CommandListManger;

class GpuBuffer : public GpuResource
{
public:
    GpuBuffer();
    virtual ~GpuBuffer() X_OVERRIDE;

    virtual void destroy(void);

    // Create a buffer. If initial data is provided, it will be copied into the buffer using the default command context.
    void create(ID3D12Device* pDevice, ContextManager& contexMan,
        DescriptorAllocator& allocator, uint32_t numElements, uint32_t elementSize,
        const void* pInitialData = nullptr);

    // Sub-Allocate a buffer out of a pre-allocated heap. If initial data is provided, it will be copied into the buffer using the default command context.
    void createPlaced(ID3D12Device* pDevice, ContextManager& contexMan,
        DescriptorAllocator& allocator, ID3D12Heap* pBackingHeap, uint32_t heapOffset,
        uint32_t numElements, uint32_t elementSize, const void* pInitialData = nullptr);

    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getUAV(void) const;
    X_INLINE const D3D12_CPU_DESCRIPTOR_HANDLE& getSRV(void) const;

    X_INLINE D3D12_GPU_VIRTUAL_ADDRESS rootConstantBufferView(void) const;

    D3D12_CPU_DESCRIPTOR_HANDLE createConstantBufferView(ID3D12Device* pDevice, DescriptorAllocator& allocator, uint32_t offset, uint32_t size) const;

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView(size_t offset, uint32_t size, uint32_t stride) const;
    X_INLINE D3D12_VERTEX_BUFFER_VIEW vertexBufferView(size_t baseVertexIndex = 0) const;

    D3D12_INDEX_BUFFER_VIEW indexBufferView(size_t offset, uint32_t size, bool b32Bit = false) const;
    X_INLINE D3D12_INDEX_BUFFER_VIEW indexBufferView(size_t startIndex = 0) const;

private:
    D3D12_RESOURCE_DESC describeBuffer(void);
    void initializeBuffer(ID3D12Device* pDevice, ContextManager& contexMan,
        const void* pData, size_t numBytes, bool useOffset = false, size_t offset = 0);

private:
    virtual void createDerivedViews(ID3D12Device* pDevice, ContextManager& contexMan,
        DescriptorAllocator& allocator) X_ABSTRACT;

protected:
    D3D12_CPU_DESCRIPTOR_HANDLE UAV_;
    D3D12_CPU_DESCRIPTOR_HANDLE SRV_;

    size_t bufferSize_;
    uint32_t elementCount_;
    uint32_t elementSize_;
    D3D12_RESOURCE_FLAGS resourceFlags_;
};

class ByteAddressBuffer : public GpuBuffer
{
public:
    ByteAddressBuffer() = default;
    virtual ~ByteAddressBuffer() X_OVERRIDE = default;

private:
    void createDerivedViews(ID3D12Device* pDevice, ContextManager& contexMan,
        DescriptorAllocator& allocator) X_OVERRIDE;
};

class IndirectArgsBuffer : public ByteAddressBuffer
{
public:
    IndirectArgsBuffer(void) = default;
    virtual ~IndirectArgsBuffer() X_OVERRIDE = default;
};

class StructuredBuffer : public GpuBuffer
{
public:
    StructuredBuffer() = default;
    virtual ~StructuredBuffer() X_OVERRIDE = default;

    void destroy(void) X_OVERRIDE;

    ByteAddressBuffer& getCounterBuffer(void);
    const D3D12_CPU_DESCRIPTOR_HANDLE& getCounterSRV(CommandContext& context);
    const D3D12_CPU_DESCRIPTOR_HANDLE& getCounterUAV(CommandContext& context);

private:
    void createDerivedViews(ID3D12Device* pDevice, ContextManager& contexMan,
        DescriptorAllocator& allocator) X_OVERRIDE;

private:
    // the RWStructuredBuffer hidden counter.
    ByteAddressBuffer counterBuffer_;
};

class TypedBuffer : public GpuBuffer
{
public:
    TypedBuffer(DXGI_FORMAT format);

private:
    void createDerivedViews(ID3D12Device* pDevice, ContextManager& contexMan,
        DescriptorAllocator& allocator) X_OVERRIDE;

protected:
    DXGI_FORMAT dataFormat_;
};

X_NAMESPACE_END

#include "GpuBuffer.inl"