#pragma once

#ifndef X_SYSINFO_H_
#define X_SYSINFO_H_

#ifdef GetUserName
#undef GetUserName
#endif

#include <Containers\Array.h>

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

    X_DECLARE_ENUM(FixedOutputMode)(
        DEFAULT,
        STRETCH,
        CENTER
    );

    struct DeviceMode
    {
        Vec2i position;
        uint32_t pelsWidth; // Specifies the dim, in pixels, of the visible device surface
        uint32_t pelsHeight;
        uint32_t bitsPerPel;
        uint32_t dispalyFrequency;
        FixedOutputMode::Enum fixedOutputMode;
    };

    typedef core::ArrayGrowMultiply<DeviceMode> DeviceModeArr;

    struct DeviceMonitor
    {
        DeviceMonitor(core::MemoryArenaBase* arena) :
            modes(arena)
        {}

        core::StackString<32, wchar_t> deviceName;
        core::StackString<128, wchar_t> deviceString;
        core::StackString<128, wchar_t> deviceID;
        core::StackString<128, wchar_t> deviceKey;

        DeviceMode currentMode;
        DeviceMode registryMode;
        DeviceModeArr modes;
    };

    typedef core::ArrayGrowMultiply<DeviceMonitor> DeviceMonitorArr;

    struct DisplayDevice
    {
        DisplayDevice(core::MemoryArenaBase* arena) :
            monitors(arena)
        {}

        core::StackString<32, wchar_t> deviceName;
        core::StackString<128, wchar_t> deviceString;
        core::StackString<128, wchar_t> deviceID;
        core::StackString<128, wchar_t> deviceKey;

        DeviceMonitorArr monitors;
    };

    typedef core::ArrayGrowMultiply<DisplayDevice> DisplayDeviceArr;


    const wchar_t* GetUserName(UserNameStr& strBuf);
    const wchar_t* GetLanguage(LanguageStr& strBuf);

    void GetSystemMemInfo(MemInfo& info);
    void GetCurrentDisplayMode(DeviceMode& mode);
    void GetDisplayDevices(DisplayDeviceArr& devices);

} // namespace SysInfo

X_NAMESPACE_END

#endif // !X_SYSINFO_H_