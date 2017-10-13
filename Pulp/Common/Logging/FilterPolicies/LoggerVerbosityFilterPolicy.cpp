#include "EngineCommon.h"
#include "LoggerVerbosityFilterPolicy.h"


#include <IConsole.h>

X_NAMESPACE_BEGIN(core)

LoggerVerbosityFilterPolicy::LoggerVerbosityFilterPolicy(int initialVerbosityLvl, 
	const char* nickName) :
	logVerbosity_(initialVerbosityLvl),
	pVar_(nullptr),
	nickName_(nickName)
{
	
}

void LoggerVerbosityFilterPolicy::Init(void)
{

}


void LoggerVerbosityFilterPolicy::Exit(void)
{

}

void LoggerVerbosityFilterPolicy::RegisterVars(void)
{
	// check if 'log_verbosity' has already been registered
	// that way all verbosity instances are controlled by same var.

	core::ICVar* pVar = gEnv->pConsole->GetCVar("log_verbosity");
	if (pVar) 
	{
		pVar_ = pVar;
	}
	else
	{
		// ref
		ADD_CVAR_REF("log_verbosity", logVerbosity_, logVerbosity_, 0, 2,
			VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
			"Logging verbosity level");
	}
}

bool LoggerVerbosityFilterPolicy::Filter(const char* type, X_SOURCE_INFO_LOG_CA(const SourceInfo&)
	const char* channel, int verbosity, const char* format, va_list args)
{
	X_UNUSED(type, channel, format, args);

	// if we where not the first logger to register var
	// we must update out value.
	// var sticks around in memory even if 1st instances dies.
	if (pVar_) {
		logVerbosity_ = pVar_->GetInteger();
	}

	return logVerbosity_ >= verbosity;
}



X_NAMESPACE_END