#include "EngineCommon.h"
#include "VirtualMem.h"

#include "Util\LastError.h"

#include <ICore.h>

X_NAMESPACE_BEGIN(core)

namespace VirtualMem
{
    namespace
    {
        uint32_t g_pageSize = 0;

        void LoadPageSize(void)
        {
            _SYSTEM_INFO systemInfo;
            ::GetSystemInfo(&systemInfo);
            g_pageSize = systemInfo.dwPageSize;
        }
    } // namespace

    void* ReserveAddressSpace(size_t size)
    {
        LPVOID pMem = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);

        if (!pMem) {
            lastError::Description Dsc;
            X_ERROR("VirtualMem", "Cannot reserve %d bytes of virtual address space. Error: %s", size, lastError::ToString(Dsc));
        }

        return pMem;
    }

    void ReleaseAddressSpace(void* reservedMemory)
    {
        if (!VirtualFree(reservedMemory, 0, MEM_RELEASE)) {
            lastError::Description Dsc;
            X_ERROR("VirtualMem", "Cannot release virtual address space at address 0x%08p. Error: %s", reservedMemory, lastError::ToString(Dsc));
        }
    }

    void* AllocatePhysicalMemory(void* reservedMemory, size_t size)
    {
        LPVOID pMem = VirtualAlloc(reservedMemory, size, MEM_COMMIT, PAGE_READWRITE);

        if (!pMem) {
            lastError::Description Dsc;
            X_ERROR("VirtualMem", "Cannot allocate %d bytes of physical memory at virtual address 0x%08p. Error: %s",
                size, reservedMemory, lastError::ToString(Dsc));
        }

        return pMem;
    }

    void FreePhysicalMemory(void* allocatedMemory, size_t size)
    {
        if (!VirtualFree(allocatedMemory, size, MEM_DECOMMIT)) {
            lastError::Description Dsc;
            X_ERROR("VirtualMem", "Cannot free %d bytes of physical memory at virtual address 0x%08p. Error: %s",
                size, allocatedMemory, lastError::ToString(Dsc));
        }
    }

    void* ReserveAndAllocatePhysicalMemory(size_t size)
    {
        LPVOID pMem = VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);

        if (!pMem) {
            lastError::Description Dsc;
            X_ERROR("VirtualMem", "Cannot allocate %d bytes of physical memory. Error: %s", size, lastError::ToString(Dsc));
        }

        return pMem;
    }

    void ReleaseAndFreePhysicalMemory(void* allocatedMemory)
    {
        if (!VirtualFree(allocatedMemory, 0, MEM_RELEASE)) {
            lastError::Description Dsc;
            X_ERROR("VirtualMem", "Cannot free physical memory at virtual address 0x%08p. Error: %s", allocatedMemory, lastError::ToString(Dsc));
        }
    }

    uint32_t GetPageSize(void)
    {
        if (g_pageSize) {
            return g_pageSize;
        }
        
        LoadPageSize();
        return g_pageSize;
    }

} // namespace VirtualMem

X_NAMESPACE_END