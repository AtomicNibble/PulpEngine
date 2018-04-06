#include "EngineCommon.h"

X_NAMESPACE_BEGIN(core)

namespace debugging
{
    /// Returns whether a debugger is connected.
    bool IsDebuggerConnected(void)
    {
        return IsDebuggerPresent() == TRUE;
    }
} // namespace debugging

X_NAMESPACE_END;