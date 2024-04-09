#pragma once

#ifndef X_HARDWAREBREAKPOINT_H_
#define X_HARDWAREBREAKPOINT_H_

X_NAMESPACE_BEGIN(core)


// Allows setting hardware breakpoints programmatically.
//
// Hardware breakpoints are implemented by making use of x86/x64 debug registers, named DR0, DR1, DR2, DR3, DR6, and DR7.
// See http://en.wikipedia.org/wiki/X86_debug_register for more information.
// At most 4 hardware breakpoints can be set at any time.
namespace hardwareBP
{
    // Determines on which condition the hardware breakpoint should trigger.
    struct Condition
    {
        enum Enum
        {
            EXECUTE = 0, // Triggers an exception when code is executed.
            WRITE = 1,   // Triggers an exception when data is written.
            // 2 is reserved
            READ_OR_WRITE = 3 // Triggers an exception when data is either read or written.
        };
    };

    // Determines how many bytes should be monitored for access by the hardware breakpoint.
    struct Size
    {
        enum Enum
        {
            BYTE_1 = 0, // Triggers an exception when 1 byte is touched.
            BYTE_2 = 1, // Triggers an exception when 2 bytes are touched.
            BYTE_4 = 3, // Triggers an exception when 4 bytes are touched.
            BYTE_8 = 2  // Triggers an exception when 8 bytes are touched.
        };
    };

    // Installs a hardware breakpoint at the given address.
    void Install(void* address, Condition::Enum condition, Size::Enum size);

    // Uninstalls a hardware breakpoint at the given address.
    void Uninstall(void* address);

} // namespace hardwareBP

X_NAMESPACE_END

#endif
