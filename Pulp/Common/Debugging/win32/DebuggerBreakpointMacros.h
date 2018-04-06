#pragma once
#ifndef X_DEBUGGERBREAKPOINTMACROS_WIN32_H
#define X_DEBUGGERBREAKPOINTMACROS_WIN32_H

/// \def X_TOGGLE_BREAKPOINT
/// \ingroup Debugging
/// \brief Halts execution by using compiler/platform-specific instructions such as e.g. __debugbreak(), irrespective of
/// preprocessor and runtime options, and irrespective of whether a debugger is actually attached or not.
/// \remark Most of the time, you will want to use \ref X_BREAKPOINT or \ref X_BREAKPOINT_IF instead.
/// \sa X_ENABLE_DEBUGGER_BREAKPOINTS X_BREAKPOINT X_BREAKPOINT_IF debugging::EnableBreakpoints
#define X_TOGGLE_BREAKPOINT __debugbreak()

#endif
