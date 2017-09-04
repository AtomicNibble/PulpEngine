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

	virtual void Init(void) X_FINAL;
	virtual void ShutDown(void) X_FINAL;

	virtual void Log(const SourceInfo& sourceInfo, const char* channel, int verbosity, const char* foramt, ...) X_FINAL;

	virtual void Warning(const SourceInfo& sourceInfo, const char* channel, const char* foramt, ...) X_FINAL;
	virtual void Error(const SourceInfo& sourceInfo, const char* channel, const char* foramt, ...) X_FINAL;
	virtual void Fatal(const SourceInfo& sourceInfo, const char* channel, const char* foramt, ...) X_FINAL;
	virtual void Assert(const SourceInfo& sourceInfo, const char* format, va_list args) X_FINAL;
	virtual void AssertVariable(const SourceInfo& sourceInfo, const char* format, ...) X_FINAL;
	 
	virtual void AddLogger(LoggerBase* logger) X_FINAL;
	virtual void RemoveLogger(LoggerBase* logger) X_FINAL;

	virtual const char* GetIndentation(void) X_FINAL;

	virtual void Indent(void) X_FINAL;
	virtual void UnIndent(void) X_FINAL;


private:
	LoggerBase* listHead_;
	LoggerBase* listTail_;

	int logVerbosity_; 
};

X_NAMESPACE_END


#endif // !_X_LOG_H_

