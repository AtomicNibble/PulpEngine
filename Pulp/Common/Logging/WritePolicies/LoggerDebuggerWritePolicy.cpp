#include "EngineCommon.h"

#include "LoggerDebuggerWritePolicy.h"


X_NAMESPACE_BEGIN(core)



void LoggerDebuggerWritePolicy::Init(void)
{

}

/// Empty implementation.
void LoggerDebuggerWritePolicy::Exit(void)
{

}

/// Writes a log message to the debugger.
void LoggerDebuggerWritePolicy::WriteLog(const LoggerBase::Line& line, uint32_t length)
{
	OutputDebugStringA( line );
}

/// Writes a warning message to the debugger.
void LoggerDebuggerWritePolicy::WriteWarning(const LoggerBase::Line& line, uint32_t length)
{
	OutputDebugStringA( line );
}

/// Writes an error message to the debugger.
void LoggerDebuggerWritePolicy::WriteError(const LoggerBase::Line& line, uint32_t length)
{
	OutputDebugStringA( line );
}

/// Writes a fatal error message to the debugger.
void LoggerDebuggerWritePolicy::WriteFatal(const LoggerBase::Line& line, uint32_t length)
{
	OutputDebugStringA( line );
}

/// Writes an assert message to the debugger.
void LoggerDebuggerWritePolicy::WriteAssert(const LoggerBase::Line& line, uint32_t length)
{
	OutputDebugStringA( line );
}

/// Writes an assert variable message to the debugger.
void LoggerDebuggerWritePolicy::WriteAssertVariable(const LoggerBase::Line& line, uint32_t length)
{
	OutputDebugStringA( line );
}





X_NAMESPACE_END