#include "stdafx.h"
#include "DescriptorAllocator.h"

X_NAMESPACE_BEGIN(render)

DescriptorTypeAllocator::DescriptorTypeAllocator(DescriptorAllocator& allocator, D3D12_DESCRIPTOR_HEAP_TYPE type) :
    type_(type),
    allocator_(allocator),
    pCurrentHeap_(nullptr),
    descriptorSize_(0),
    remainingFreeHandles_(0)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorTypeAllocator::allocate(uint32_t num)
{
    if (!pCurrentHeap_ || remainingFreeHandles_ < num) {
        pCurrentHeap_ = allocator_.requestNewHeap(type_, NUM_DESCRIPTORS_PER_HEAP);
        currentHandle_ = pCurrentHeap_->GetCPUDescriptorHandleForHeapStart();
        remainingFreeHandles_ = NUM_DESCRIPTORS_PER_HEAP;

        if (descriptorSize_ == 0) {
            descriptorSize_ = allocator_.getDescriptorHandleIncrementSize(type_);
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE ret = currentHandle_;
    currentHandle_.ptr += num * descriptorSize_;
    remainingFreeHandles_ -= num;
    return ret;
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorTypeAllocator::getType(void) const
{
    return type_;
}

// ---------------------------------------------

X_DISABLE_WARNING(4355) // 'this': used in base member initializer list

DescriptorAllocator::DescriptorAllocator(core::MemoryArenaBase* arena, ID3D12Device* pDevice) :
    allocators_{{
        {*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV},
        {*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER},
        {*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV},
        {*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV},
    }},
    pDevice_(pDevice),
    descriptorHeaps_(arena)
{
    X_ASSERT_NOT_NULL(pDevice);
}

X_ENABLE_WARNING(4355)

DescriptorAllocator::~DescriptorAllocator()
{
    destoryAllHeaps();
}

void DescriptorAllocator::destoryAllHeaps(void)
{
    core::CriticalSection::ScopedLock lock(cs_);

    for (auto dh : descriptorHeaps_) {
        core::SafeReleaseDX(dh);
    }

    descriptorHeaps_.clear();
}

uint32_t DescriptorAllocator::getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return pDevice_->GetDescriptorHandleIncrementSize(type);
}

ID3D12DescriptorHeap* DescriptorAllocator::requestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t num)
{
    D3D12_DESCRIPTOR_HEAP_DESC Desc;
    Desc.Type = type;
    Desc.NumDescriptors = num;
    Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Desc.NodeMask = 1;

    core::CriticalSection::ScopedLock lock(cs_);

    ID3D12DescriptorHeap* pHeap;
    HRESULT hr = pDevice_->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap));
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to create descriptor heap: %" PRIu32, hr);
    }

    descriptorHeaps_.emplace_back(pHeap);

    return pHeap;
}

X_NAMESPACE_END
