#include "EngineCommon.h"

#pragma hdrstop

X_NAMESPACE_BEGIN(core)


namespace debugging
{
	/// Returns whether a debugger is connected.
	bool IsDebuggerConnected(void)
	{
		return IsDebuggerPresent() == TRUE;
	}
}


X_NAMESPACE_END;