#pragma once


X_NAMESPACE_BEGIN(render)

class DescriptorAllocator;

class DescriptorTypeAllocator
{
	static const uint32_t NUM_DESCRIPTORS_PER_HEAP = 256;

public:
	DescriptorTypeAllocator(DescriptorAllocator& allocator, D3D12_DESCRIPTOR_HEAP_TYPE type);

	D3D12_CPU_DESCRIPTOR_HANDLE allocate(uint32_t num);
	D3D12_DESCRIPTOR_HEAP_TYPE getType(void) const;

private:
	D3D12_DESCRIPTOR_HEAP_TYPE type_;
	ID3D12DescriptorHeap* pCurrentHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE currentHandle_;
	uint32_t descriptorSize_;
	uint32_t remainingFreeHandles_;
	
	DescriptorAllocator& allocator_;
};


class DescriptorAllocator
{
	friend class DescriptorTypeAllocator;

	typedef std::array<DescriptorTypeAllocator, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DescriptorAllocatorsArr;


public:
	DescriptorAllocator(core::MemoryArenaBase* arena, ID3D12Device* pDevice);
	~DescriptorAllocator();

	X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE allocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t num = 1);
	void destoryAllHeaps(void);

protected:
	uint32_t getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type);
	ID3D12DescriptorHeap* requestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t num);

private:
	DescriptorAllocatorsArr allocators_;
	ID3D12Device* pDevice_;

	core::CriticalSection cs_;
	core::Array<ID3D12DescriptorHeap*> descriptorHeaps_;
};


class DescriptorHandle
{
public:
	X_INLINE DescriptorHandle();
	X_INLINE explicit DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle);
	X_INLINE DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle);

	X_INLINE DescriptorHandle operator+ (int32_t offsetScaledByDescriptorSize) const;

	X_INLINE void operator += (int32_t offsetScaledByDescriptorSize);

	X_INLINE D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(void) const;
	X_INLINE D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(void) const;

	X_INLINE bool isNull(void) const;
	X_INLINE bool isShaderVisible(void) const;

private:
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle_;
};

X_NAMESPACE_END

#include "DescriptorAllocator.inl"