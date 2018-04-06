#pragma once

#ifndef X_DEBUGGERCONNECTION_H_
#define X_DEBUGGERCONNECTION_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Debugging
namespace debugging
{
    /// Returns whether a debugger is connected.
    bool IsDebuggerConnected(void);
} // namespace debugging

X_NAMESPACE_END

#endif // X_DEBUGGERCONNECTION_H_
