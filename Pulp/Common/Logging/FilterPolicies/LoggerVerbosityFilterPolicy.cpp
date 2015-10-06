#include "EngineCommon.h"


#include "LoggerVerbosityFilterPolicy.h"

X_NAMESPACE_BEGIN(core)

static int log_verbosity;

void LoggerVerbosityFilterPolicy::Init(void)
{
//	ADD_CVAR_REF_NO_NAME(log_verbosity, 0, 0, 3, VarFlag::SYSTEM, "filer logs");

}


void LoggerVerbosityFilterPolicy::Exit(void)
{
	
}

bool LoggerVerbosityFilterPolicy::Filter(const char* type, const SourceInfo& sourceInfo,
	const char* channel, int verbosity, const char* format, va_list args)
{
	X_UNUSED(type);
	X_UNUSED(sourceInfo);
	X_UNUSED(channel);
	X_UNUSED(format);
	X_UNUSED(args);
	return log_verbosity >= verbosity;
}



X_NAMESPACE_END