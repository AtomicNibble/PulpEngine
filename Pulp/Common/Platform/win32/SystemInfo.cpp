#include <EngineCommon.h>
#include "SystemInfo.h"

X_NAMESPACE_BEGIN(core)

namespace SysInfo
{
    namespace
    {
        void devModeToDeviceMode(const DEVMODE& dm, DeviceMode& mode)
        {
            mode.position.x = dm.dmPosition.x;
            mode.position.y = dm.dmPosition.y;
            mode.pelsHeight = dm.dmPelsHeight;
            mode.pelsHeight = dm.dmPelsHeight;
            mode.pelsWidth = dm.dmPelsWidth;
            mode.bitsPerPel = dm.dmBitsPerPel;
            mode.dispalyFrequency = dm.dmDisplayFrequency;

            static_assert(FixedOutputMode::DEFAULT == DMDFO_DEFAULT, "Enum value mismatch");
            static_assert(FixedOutputMode::STRETCH == DMDFO_STRETCH, "Enum value mismatch");
            static_assert(FixedOutputMode::CENTER == DMDFO_CENTER, "Enum value mismatch");

            mode.fixedOutputMode = static_cast<FixedOutputMode::Enum>(dm.dmDisplayFixedOutput);
        }


    } // namespace

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
        if (::GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SENGLANGUAGE, strBuf, sizeof(strBuf) / sizeof(wchar_t)) == 0) {
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

    void GetCurrentDisplayMode(DeviceMode& mode)
    {
        core::zero_object(mode);

        DEVMODEW win32Mode;
        if (!EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &win32Mode)) {
            X_ERROR("SysInfo", "Failed to get display info");
            return;
        }

        devModeToDeviceMode(win32Mode, mode);
    }


    void GetDisplayDevices(DisplayDeviceArr& devices)
    {
        for (int32_t deviceNum = 0; ; deviceNum++)
        {
            DISPLAY_DEVICEW win32Device;
            core::zero_object(win32Device);
            win32Device.cb = sizeof(win32Device);
            if (!EnumDisplayDevicesW(nullptr, deviceNum, &win32Device, 0)) {
                break;
            }

            DisplayDevice& device = devices.AddOne(devices.getArena());
            device.deviceName.set(win32Device.DeviceName);
            device.deviceString.set(win32Device.DeviceString);
            device.deviceID.set(win32Device.DeviceID);
            device.deviceKey.set(win32Device.DeviceKey);

            for (int32_t monitorNum = 0; ; monitorNum++)
            {
                DISPLAY_DEVICEW win32Monitor;
                core::zero_object(win32Monitor);
                win32Monitor.cb = sizeof(win32Monitor);
                if (!EnumDisplayDevicesW(win32Device.DeviceName, monitorNum, &win32Monitor, 0)) {
                    break;
                }

                DeviceMonitor& monitor = device.monitors.AddOne(devices.getArena());
                monitor.deviceName.set(win32Monitor.DeviceName);
                monitor.deviceString.set(win32Monitor.DeviceString);
                monitor.deviceID.set(win32Monitor.DeviceID);
                monitor.deviceKey.set(win32Monitor.DeviceKey);

                DEVMODEW win32Mode;
                core::zero_object(win32Mode);
                if (!EnumDisplaySettingsW(win32Device.DeviceName, ENUM_CURRENT_SETTINGS, &win32Mode)) {
                    X_ERROR("SysInfo", "Failed to get devices current mode");
                }

                devModeToDeviceMode(win32Mode, monitor.currentMode);

                core::zero_object(win32Mode);
                if (!EnumDisplaySettingsW(win32Device.DeviceName, ENUM_CURRENT_SETTINGS, &win32Mode)) {
                    X_ERROR("SysInfo", "Failed to get devices registry mode");
                }
                
                devModeToDeviceMode(win32Mode, monitor.registryMode);

                auto& modes = monitor.modes;
                for (int32_t modeNum = 0; ; modeNum++)
                {
                    core::zero_object(win32Mode);
                    if (!EnumDisplaySettingsW(win32Device.DeviceName, modeNum, &win32Mode)) {
                        break;
                    }

                    if (win32Mode.dmBitsPerPel != 32) {
                        continue;
                    }
                    if (win32Mode.dmDisplayFrequency < 60) {
                        continue;
                    }

                    DeviceMode& mode = modes.AddOne();
                    devModeToDeviceMode(win32Mode, mode);
                }

                // sort them so lowest res first
                // order: height, width, freq
                std::sort(modes.begin(), modes.end(), [](const DeviceMode& lhs, const DeviceMode& rhs) {
                    if (lhs.pelsHeight != rhs.pelsHeight) {
                        return lhs.pelsHeight < rhs.pelsHeight;
                    }
                    if (lhs.pelsWidth != rhs.pelsWidth) {
                        return lhs.pelsWidth < rhs.pelsWidth;
                    }
                    if (lhs.dispalyFrequency != rhs.dispalyFrequency) {
                        return lhs.dispalyFrequency < rhs.dispalyFrequency;
                    }

                    return false;
                });
            }
        }
    }


} // namespace SysInfo

X_NAMESPACE_END
