#include <EngineCommon.h>

#include "MemInfo.h"

#include <ICore.h>

#include <Psapi.h>
#include <Platform\Module.h>

X_NAMESPACE_BEGIN(core)

namespace
{
    typedef BOOL(WINAPI* GetProcessMemoryInfoProc)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);

} // namespace

bool GetProcessMemInfo(XProcessMemInfo& info)
{
    core::zero_object(info);

#if _WIN32

    PROCESS_MEMORY_COUNTERS pc;
    core::zero_object(pc);
    pc.cb = sizeof(pc);

    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);

    info.TotalPhysicalMemory = mem.ullTotalPhys;
    info.FreePhysicalMemory = mem.ullAvailPhys;

    // why do i lock here?
    static core::CriticalSection cs;
    core::CriticalSection::ScopedLock lock(cs);
    
    static core::Module::Handle hPSAPI = core::Module::Load(L"psapi.dll");

    if (!hPSAPI) {
        X_WARNING("Mem", "Failed to load 'psapi.dll'");
        return false;
    }

        
    static auto pGetProcessMemoryInfo = reinterpret_cast<GetProcessMemoryInfoProc>(core::Module::GetProc(hPSAPI, "GetProcessMemoryInfo"));
    if (!pGetProcessMemoryInfo) {
        X_WARNING("Mem", "Failed to resolve 'GetProcessMemoryInfo'");
        return false;
    }

    if (!pGetProcessMemoryInfo(GetCurrentProcess(), &pc, sizeof(pc))) {
        X_WARNING("Mem", "Failed to get process memory info");
        return false;
    }

    info.PageFaultCount = pc.PageFaultCount;
    info.PeakWorkingSetSize = pc.PeakWorkingSetSize;
    info.WorkingSetSize = pc.WorkingSetSize;
    info.QuotaPeakPagedPoolUsage = pc.QuotaPeakPagedPoolUsage;
    info.QuotaPagedPoolUsage = pc.QuotaPagedPoolUsage;
    info.QuotaPeakNonPagedPoolUsage = pc.QuotaPeakNonPagedPoolUsage;
    info.QuotaNonPagedPoolUsage = pc.QuotaNonPagedPoolUsage;
    info.PagefileUsage = pc.PagefileUsage;
    info.PeakPagefileUsage = pc.PeakPagefileUsage;
    return true;
    
#endif
    return false;
}

X_NAMESPACE_END