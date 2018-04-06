#include "EngineCommon.h"

X_NAMESPACE_BEGIN(core)

namespace debugging
{
    namespace
    {
        bool g_enableBreakpoints = true;
    }

    /// Enables/disables debugger breakpoints.
    void EnableBreakpoints(bool enable)
    {
        g_enableBreakpoints = enable;
    }

    /// Returns whether breakpoints are enabled in the runtime settings.
    bool AreBreakpointsEnabled(void)
    {
        return g_enableBreakpoints;
    }
} // namespace debugging

X_NAMESPACE_END