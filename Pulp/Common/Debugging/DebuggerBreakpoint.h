
#pragma once
#ifndef X_DEBUGGERBREAKPOINT_H_
#define X_DEBUGGERBREAKPOINT_H_

#include "Debugging/DebuggerConnection.h"

X_NAMESPACE_BEGIN(core)

namespace debugging
{
    /// Enables/disables debugger breakpoints.
    void EnableBreakpoints(bool enable);

    /// Returns whether breakpoints are enabled in the runtime settings.
    bool AreBreakpointsEnabled(void);
} // namespace debugging

X_NAMESPACE_END

#endif // X_DEBUGGERBREAKPOINT_H_
