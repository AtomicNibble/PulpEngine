#pragma once

#ifndef X_SYSINFO_H_
#define X_SYSINFO_H_

#ifdef GetUserName
#undef GetUserName
#endif

X_NAMESPACE_BEGIN(core)

namespace SysInfo
{
    typedef wchar_t UserNameStr[128];
    typedef wchar_t LanguageStr[128];

    struct MemInfo
    {
        uint32_t dwMemoryLoad;
        uint64_t TotalPhys;
        uint64_t AvailPhys;
        uint64_t TotalPageFile;
        uint64_t AvailPageFile;
        uint64_t TotalVirtual;
        uint64_t AvailVirtual;
        uint64_t AvailExtendedVirtual;
    };

    struct DisplayInfo
    {
        uint32_t pelsWidth;
        uint32_t pelsHeight;
        uint32_t bitsPerPel;
    };

    const wchar_t* GetUserName(UserNameStr& strBuf);
    const wchar_t* GetLanguage(LanguageStr& strBuf);

    void GetSystemMemInfo(MemInfo& info);
    void GetDisplayInfo(DisplayInfo& info);

} // namespace SysInfo

X_NAMESPACE_END

#endif // !X_SYSINFO_H_