#pragma once

#ifndef X_HARDWAREBREAKPOINT_H_
#define X_HARDWAREBREAKPOINT_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
/// \brief Allows setting hardware breakpoints programmatically.
/// \details Hardware breakpoints are very useful for catching notorious hard-to-find bugs, such as memory stomps.
/// By setting a hardware breakpoint, program execution can halt whenever a certain amount of bytes (1, 2, 4 or 8)
/// in memory is either read from or written to by the CPU.
///
/// Even though hardware breakpoints are supported by most debuggers today, having the ability to set them in a
/// programmatic way enables programmers to e.g. track down memory stomps at ever-changing random addresses without
/// having to set them anew in the debugger each time.
///
/// Hardware breakpoints are implemented by making use of x86/x64 debug registers, named DR0, DR1, DR2, DR3, DR6, and DR7.
/// See http://en.wikipedia.org/wiki/X86_debug_register for more information.
/// \remark At most 4 hardware breakpoints can be set at any time.
namespace hardwareBP
{
    /// \struct Condition
    /// \brief Determines on which condition the hardware breakpoint should trigger.
    struct Condition
    {
        enum Enum
        {
            EXECUTE = 0, ///< Triggers an exception when code is executed.
            WRITE = 1,   ///< Triggers an exception when data is written.
            // 2 is reserved
            READ_OR_WRITE = 3 ///< Triggers an exception when data is either read or written.
        };
    };

    /// \struct Size
    /// \brief Determines how many bytes should be monitored for access by the hardware breakpoint.
    struct Size
    {
        enum Enum
        {
            BYTE_1 = 0, ///< Triggers an exception when 1 byte is touched.
            BYTE_2 = 1, ///< Triggers an exception when 2 bytes are touched.
            BYTE_4 = 3, ///< Triggers an exception when 4 bytes are touched.
            BYTE_8 = 2  ///< Triggers an exception when 8 bytes are touched.
        };
    };

    /// Installs a hardware breakpoint at the given address.
    void Install(void* address, Condition::Enum condition, Size::Enum size);

    /// Uninstalls a hardware breakpoint at the given address.
    void Uninstall(void* address);
} // namespace hardwareBP

X_NAMESPACE_END

#endif
