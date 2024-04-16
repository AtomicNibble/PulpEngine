#pragma once

#ifndef _X_LOG_I_H_
#define _X_LOG_I_H_

//
// This is the log interface, so that all modules can sends logs.
// it will be part of the global environment
//
// What info do we want?
//
// The module it came from
// The type of log: log, warning, error, fatal
// Verbosity level: 1(always), 2, 3
//
// We want to be able to add loggers
// so we can add in a file logger on the fly.
//
//

X_NAMESPACE_BEGIN(core)

X_DECLARE_ENUM(LogType)
(
    INFO,
    WARNING,
    ERROR,
    FATAL,
    ASSERT);

class LoggerBase;

struct ILog
{
    virtual ~ILog() = default;

    virtual void Init(void) X_ABSTRACT;
    virtual void ShutDown(void) X_ABSTRACT;

    virtual void Log(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, int verbosity, const char* foramt, ...) X_ABSTRACT;

    virtual void Warning(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, const char* foramt, ...) X_ABSTRACT;
    virtual void Error(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, const char* foramt, ...) X_ABSTRACT;
    virtual void Fatal(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* channel, const char* foramt, ...) X_ABSTRACT;
    virtual void Assert(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* format, va_list args) X_ABSTRACT;
    virtual void AssertVariable(X_SOURCE_INFO_LOG_CA(const SourceInfo& sourceInfo) const char* format, ...) X_ABSTRACT;

    virtual void AddLogger(LoggerBase* logger) X_ABSTRACT;
    virtual void RemoveLogger(LoggerBase* logger) X_ABSTRACT;

    virtual const char* GetIndentation(void) X_ABSTRACT;

    struct Bullet
    {
        Bullet(ILog* pLog) :
            pLog_(pLog)
        {
            if (pLog) {
                pLog_->Indent();
            }
        }
        ~Bullet(void)
        {
            if (pLog_) {
                pLog_->UnIndent();
            }
        }

    private:
        ILog* pLog_;
    };

    friend struct Bullet;

private:
    virtual void Indent(void) X_ABSTRACT;
    virtual void UnIndent(void) X_ABSTRACT;
};

X_NAMESPACE_END

#endif // !_X_LOG_I_H_
