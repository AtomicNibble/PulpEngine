#pragma once

X_NAMESPACE_BEGIN(core)

struct ILog;

class LoggerBase
{
public:
#if X_DEBUG
	typedef char Line[4096];
#else
	typedef char Line[1024];
#endif // !X_DEBUG

	/// Default constructor, initializing the linked-list pointers.
	LoggerBase(void);

	/// Destructor.
	virtual ~LoggerBase(void);

	void Log(const SourceInfo& sourceInfo, const char* channel, int verbosity, const char* foramt, va_list args);
	void Warning(const SourceInfo& sourceInfo, const char* channel, const char* foramt, va_list args);
	void Error(const SourceInfo& sourceInfo, const char* channel, const char* foramt, va_list args);
	void Fatal(const SourceInfo& sourceInfo, const char* channel, const char* foramt, va_list args);
	void Assert(const SourceInfo& sourceInfo, const char* format, va_list args);
	void AssertVariable(const SourceInfo& sourceInfo, const char* format, va_list args);


	/// \brief intrusive linked-list.
	inline void SetPrevious(LoggerBase* logger);
	inline void SetNext(LoggerBase* logger);

	inline LoggerBase* GetPrevious(void) const;
	inline LoggerBase* GetNext(void) const;

	void SetParent(ILog* pLog);

private:
	// derived implmentation.
	virtual void DoLog(const SourceInfo& sourceInfo, const char* channel, int verbosity, const char* foramt, va_list args) X_ABSTRACT;
	virtual void DoWarning(const SourceInfo& sourceInfo, const char* channel, const char* foramt, va_list args) X_ABSTRACT;
	virtual void DoError(const SourceInfo& sourceInfo, const char* channel, const char* foramt, va_list args) X_ABSTRACT;
	virtual void DoFatal(const SourceInfo& sourceInfo, const char* channel, const char* foramt, va_list args) X_ABSTRACT;
	virtual void DoAssert(const SourceInfo& sourceInfo, const char* format, va_list args) X_ABSTRACT;
	virtual void DoAssertVariable(const SourceInfo& sourceInfo, const char* format, va_list args) X_ABSTRACT;

	LoggerBase* previous_;
	LoggerBase* next_;

protected:
	ILog*		pLog_;
};

#include "LoggerBase.inl"


X_NAMESPACE_END
