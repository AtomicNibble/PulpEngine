#include <EngineCommon.h>

#include "MemInfo.h"

#include <ICore.h>

#include <Psapi.h>

X_NAMESPACE_BEGIN(core)


bool GetProcessMemInfo(XProcessMemInfo &info)
{
	typedef BOOL(WINAPI *GetProcessMemoryInfoProc)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);

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

	static HMODULE hPSAPI = GoatLoadLibary("psapi.dll");

	if (hPSAPI)
	{
		static GetProcessMemoryInfoProc pGetProcessMemoryInfo = (GetProcessMemoryInfoProc)GetProcAddress(hPSAPI, "GetProcessMemoryInfo");
		if (pGetProcessMemoryInfo)
		{
			if (pGetProcessMemoryInfo(GetCurrentProcess(), &pc, sizeof(pc)))
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