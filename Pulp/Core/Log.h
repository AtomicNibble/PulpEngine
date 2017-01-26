#pragma once


#ifndef _X_LOG_H_
#define _X_LOG_H_

#include <ILog.h>

X_NAMESPACE_BEGIN(core)

class LoggerBase;

class XLog : public ILog
{
public:
	XLog();
	~XLog();

	virtual void Init() X_OVERRIDE;
	virtual void ShutDown() X_OVERRIDE;

	virtual void Log(const SourceInfo& sourceInfo, const char* channel, int verbosity, const char* foramt, ...) X_OVERRIDE;

	virtual void Warning(const SourceInfo& sourceInfo, const char* channel, const char* foramt, ...) X_OVERRIDE;
	virtual void Error(const SourceInfo& sourceInfo, const char* channel, const char* foramt, ...) X_OVERRIDE;
	virtual void Fatal(const SourceInfo& sourceInfo, const char* channel, const char* foramt, ...) X_OVERRIDE;
	virtual void Assert(const SourceInfo& sourceInfo, const char* format, va_list args) X_OVERRIDE;
	virtual void AssertVariable(const SourceInfo& sourceInfo, const char* format, ...) X_OVERRIDE;
	 
	virtual void AddLogger(LoggerBase* logger) X_OVERRIDE;
	virtual void RemoveLogger(LoggerBase* logger) X_OVERRIDE;

	virtual const char* GetIndentation(void) X_OVERRIDE;

	virtual void Indent(void) X_OVERRIDE;
	virtual void UnIndent(void) X_OVERRIDE;


private:
	LoggerBase* listHead_;
	LoggerBase* listTail_;

	int logVerbosity_; 
};

X_NAMESPACE_END


#endif // !_X_LOG_H_

