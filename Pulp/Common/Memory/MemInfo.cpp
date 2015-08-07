#include <EngineCommon.h>

#include "MemInfo.h"

#include <ICore.h>

#include <Psapi.h>

X_NAMESPACE_BEGIN(core)

namespace
{
	typedef BOOL(WINAPI *GetProcessMemoryInfoProc)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);

	core::CriticalSection g_lock;

	HMODULE g_hPSAPI = 0;
	GetProcessMemoryInfoProc g_pGetProcessMemoryInfo = nullptr;
}

bool GetProcessMemInfo(XProcessMemInfo &info)
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

	core::CriticalSection::ScopedLock lock(g_lock);

	if (!g_hPSAPI) {
		g_hPSAPI = GoatLoadLibaryW(L"psapi.dll");
	}

	if (g_hPSAPI)
	{
		if (!g_pGetProcessMemoryInfo) {
			g_pGetProcessMemoryInfo = (GetProcessMemoryInfoProc)GoatGetProcAddress(g_hPSAPI,
				"GetProcessMemoryInfo");
		}

		if (g_pGetProcessMemoryInfo)
		{
			if (g_pGetProcessMemoryInfo(GetCurrentProcess(), &pc, sizeof(pc)))
			{
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
			}
		}
	}
	else
	{
		X_WARNING("Mem", "Failed to load 'psapi.dll'");
	}
#endif
	return false;
}


X_NAMESPACE_END