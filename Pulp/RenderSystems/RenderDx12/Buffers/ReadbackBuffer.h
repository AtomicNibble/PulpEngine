#pragma once


#include "GpuBuffer.h"

X_NAMESPACE_BEGIN(render)

class ReadbackBuffer : public GpuBuffer
{
public:
    virtual ~ReadbackBuffer() X_OVERRIDE;

    void create(ID3D12Device* pDevice, uint32_t numElements, uint32_t elementSize);

    void* map(void);
    void unmap(void);

private:
    void createDerivedViews(ID3D12Device* pDevice, ContextManager& contexMan,
        DescriptorAllocator& allocator) X_FINAL {};

};

X_NAMESPACE_END
