#include "stdafx.h"
#include "MayaLogger.h"

#include "MayaUtil.h"

X_NAMESPACE_BEGIN(maya)

void LoggerMayaWritePolicy::Init(void)
{
}

/// Empty implementation.
void LoggerMayaWritePolicy::Exit(void)
{
}

/// Writes a log message to the debugger.
void LoggerMayaWritePolicy::WriteLog(const core::LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);

    MayaUtil::MayaPrintMsg(line);
}

/// Writes a warning message to the debugger.
void LoggerMayaWritePolicy::WriteWarning(const core::LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    MayaUtil::MayaPrintWarning(line);
}

/// Writes an error message to the debugger.
void LoggerMayaWritePolicy::WriteError(const core::LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    MayaUtil::MayaPrintError(line);
}

/// Writes a fatal error message to the debugger.
void LoggerMayaWritePolicy::WriteFatal(const core::LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    MayaUtil::MayaPrintError(line);
}

/// Writes an assert message to the debugger.
void LoggerMayaWritePolicy::WriteAssert(const core::LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    MayaUtil::MayaPrintError(line);
}

/// Writes an assert variable message to the debugger.
void LoggerMayaWritePolicy::WriteAssertVariable(const core::LoggerBase::Line& line, uint32_t length)
{
    X_UNUSED(length);
    MayaUtil::MayaPrintError(line);
}

X_NAMESPACE_END