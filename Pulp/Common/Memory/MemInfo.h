#pragma once

#ifndef _X_MEMINFO_H_
#define _X_MEMINFO_H_

X_NAMESPACE_BEGIN(core)

struct XProcessMemInfo
{
    uint64 PageFaultCount;
    uint64 PeakWorkingSetSize;
    uint64 WorkingSetSize;
    uint64 QuotaPeakPagedPoolUsage;
    uint64 QuotaPagedPoolUsage;
    uint64 QuotaPeakNonPagedPoolUsage;
    uint64 QuotaNonPagedPoolUsage;
    uint64 PagefileUsage;
    uint64 PeakPagefileUsage;

    uint64 TotalPhysicalMemory;
    int64 FreePhysicalMemory;

    uint64 TotalVideoMemory;
    int64 FreeVideoMemory;
};

bool GetProcessMemInfo(XProcessMemInfo& minfo);

X_NAMESPACE_END

#endif // !_X_MEMINFO_H_
