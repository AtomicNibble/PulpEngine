
#pragma once
#ifndef X_DEBUGGERBREAKPOINTMACROS_H_
#define X_DEBUGGERBREAKPOINTMACROS_H_

#include "Debugging/DebuggerBreakpoint.h"
#include X_INCLUDE(Debugging/X_PLATFORM/DebuggerBreakpointMacros.h)

#if X_ENABLE_DEBUGGER_BREAKPOINTS
#define X_BREAKPOINT X_BREAKPOINT_IF(true)
#define X_BREAKPOINT_IF(condition) ((condition) && (X_NAMESPACE(core)::debugging::AreBreakpointsEnabled()) && (X_NAMESPACE(core)::debugging::IsDebuggerConnected()) ? X_TOGGLE_BREAKPOINT : X_UNUSED(true))
#else
#define X_BREAKPOINT X_UNUSED(true)
#define X_BREAKPOINT_IF(condition) X_UNUSED(condition)
#endif

#endif
