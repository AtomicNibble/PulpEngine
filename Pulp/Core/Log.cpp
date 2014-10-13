#include "stdafx.h"
#include "Log.h"
#include "Logging\LoggerBase.h"

#include "IConsole.h"

X_NAMESPACE_BEGIN(core)

namespace {

	const char *const INDENTATION_STRINGS[7] = {

		"| ",
		"|    ",
		"|      ",
		"|        ",
		"|          ",
		"|            ",
		"|              "

	};
}

XLog::XLog() :
listHead_(nullptr),
listTail_(nullptr),
logVerbosity_(0)
{
	
}

XLog::~XLog()
{

}


void XLog::Init()
{
	// no point printing a 'starting msg' since  log system 
	// not quite ready yet xD

	ADD_CVAR_REF("log_verbosity", logVerbosity_, 0, 0, 2, 0, "Logging verbosity");

}

void XLog::ShutDown()
{
	X_LOG0("LogSys", "Shutting Down");

	// nothing todo here yet.
	// but function is here so engine can call it in shutdown code.
	// to be uniform with shutdown of moudles.

}


void XLog::AddLogger(LoggerBase* logger)
{
	X_ASSERT_NOT_NULL(logger);

	logger->SetParent(this);
	if (listHead_)
	{
		logger->SetPrevious(listTail_);
		listTail_->SetNext(logger);
		listTail_ = logger;
	}
	else
	{
		listHead_ = logger;
		listTail_ = logger;
	}
}

void XLog::RemoveLogger(LoggerBase* logger)
{
	X_ASSERT_NOT_NULL(logger);

	LoggerBase *prev = logger->GetPrevious();
	LoggerBase *next = logger->GetNext();

	if (prev)
		prev->SetNext(next);
	else
		listHead_ = next;

	if (next)
		next->SetPrevious(prev);
	else
		listTail_ = prev;
}

const char* XLog::GetIndentation(void)
{
	const char *result;

	if (logVerbosity_ < 7)
		result = INDENTATION_STRINGS[logVerbosity_];
	else
		result = "|              ";
	return result;
}

void XLog::Indent()
{
	logVerbosity_++;
}

void XLog::UnIndent()
{
	--logVerbosity_;
}

#define X_CALL_LOGGERS( pfnc ) \
for (LoggerBase* logger = listHead_; logger; logger = logger->GetNext()) \
	logger->pfnc;


void XLog::Log(const SourceInfo& sourceInfo, const char* channel, size_t verbosity, const char* format, ...)
{
	X_VALIST_START(format)
		X_CALL_LOGGERS(Log(sourceInfo, channel, verbosity, format, args))
	X_VALIST_END
}

void XLog::Warning(const SourceInfo& sourceInfo, const char* channel, const char* format, ...)
{
	X_VALIST_START(format)
		X_CALL_LOGGERS(Warning(sourceInfo, channel, format, args))
	X_VALIST_END
}

void XLog::Error(const SourceInfo& sourceInfo, const char* channel, const char* format, ...)
{
	X_VALIST_START(format)
		X_CALL_LOGGERS(Error(sourceInfo, channel, format, args))
	X_VALIST_END
}

void XLog::Fatal(const SourceInfo& sourceInfo, const char* channel, const char* format, ...)
{
	X_VALIST_START(format)
		X_CALL_LOGGERS(Fatal(sourceInfo, channel, format, args))

		// do shit.
	if (gEnv && gEnv->pCore)
		gEnv->pCore->OnFatalError(format, args);
	else
	{
		// fail.
		X_ASSERT_UNREACHABLE();
		exit(1);
	}

	X_VALIST_END

}


void XLog::Assert(const SourceInfo& sourceInfo, const char* format, va_list args)
{
	X_CALL_LOGGERS(Assert(sourceInfo, format, args))
}

void XLog::AssertVariable(const SourceInfo& sourceInfo, const char* format, ...)
{
	X_VALIST_START(format)
		X_CALL_LOGGERS(AssertVariable(sourceInfo, format, args))
	X_VALIST_END
}


X_NAMESPACE_END
