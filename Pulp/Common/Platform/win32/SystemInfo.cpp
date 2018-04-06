#include <EngineCommon.h>
#include "SystemInfo.h"

X_NAMESPACE_BEGIN(core)

namespace SysInfo
{
    const wchar_t* GetUserName(UserNameStr& strBuf)
    {
        DWORD dwSize = sizeof(strBuf) / sizeof(wchar_t);

        if (::GetUserNameW(strBuf, &dwSize) == 0) {
            core::zero_object(strBuf);
            core::lastError::Description Dsc;
            X_ERROR("SysInfo", "Failed to get username. Err: %s", core::lastError::ToString(Dsc));
        }

        return strBuf;
    }

    const wchar_t* GetLanguage(LanguageStr& strBuf)
    {
        if (::GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SENGLANGUAGE,
                strBuf,
                sizeof(strBuf) / sizeof(wchar_t))
            == 0) {
            core::zero_object(strBuf);
            core::lastError::Description Dsc;
            X_ERROR("SysInfo", "Failed to get language. Err: %s", core::lastError::ToString(Dsc));
        }

        return strBuf;
    }

    void GetSystemMemInfo(MemInfo& info)
    {
        core::zero_object(info);

        MEMORYSTATUSEX MemoryStatus;
        MemoryStatus.dwLength = sizeof(MemoryStatus);
        if (!GlobalMemoryStatusEx(&MemoryStatus)) {
            core::lastError::Description Dsc;
            X_ERROR("SysInfo", "Failed to get system memory info. Err: %s", core::lastError::ToString(Dsc));
            return;
        }

        info.dwMemoryLoad = MemoryStatus.dwMemoryLoad;
        info.TotalPhys = MemoryStatus.ullTotalPhys;
        info.AvailPhys = MemoryStatus.ullAvailPhys;
        info.TotalPageFile = MemoryStatus.ullTotalPageFile;
        info.AvailPageFile = MemoryStatus.ullAvailPageFile;
        info.TotalVirtual = MemoryStatus.ullTotalVirtual;
        info.AvailVirtual = MemoryStatus.ullAvailVirtual;
        info.AvailExtendedVirtual = MemoryStatus.ullAvailExtendedVirtual;
    }

    void GetDisplayInfo(DisplayInfo& info)
    {
        core::zero_object(info);

        DEVMODEW DisplayConfig;
        if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &DisplayConfig)) {
            core::lastError::Description Dsc;
            X_ERROR("SysInfo", "Failed to get display info. Err: %s", core::lastError::ToString(Dsc));
            return;
        }

        info.pelsHeight = DisplayConfig.dmPelsHeight;
        info.pelsWidth = DisplayConfig.dmPelsWidth;
        info.bitsPerPel = DisplayConfig.dmBitsPerPel;
    }

} // namespace SysInfo

X_NAMESPACE_END
