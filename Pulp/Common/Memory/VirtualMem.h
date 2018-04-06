#pragma once

#ifndef _H_VIRTUALMEM_H_
#define _H_VIRTUALMEM_H_

X_NAMESPACE_BEGIN(core)

namespace VirtualMem
{
    void* ReserveAddressSpace(size_t size);

    void ReleaseAddressSpace(void* reservedMemory);

    void* AllocatePhysicalMemory(void* reservedMemory, size_t size);

    void FreePhysicalMemory(void* allocatedMemory, size_t size);

    void* ReserveAndAllocatePhysicalMemory(size_t size);

    void ReleaseAndFreePhysicalMemory(void* allocatedMemory);

    unsigned int GetPageSize(void);
} // namespace VirtualMem

X_NAMESPACE_END

#endif // _H_VIRTUALMEM_H_