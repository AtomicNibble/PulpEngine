#pragma once


X_NAMESPACE_BEGIN(core)

struct IEngineSysBase
{
	virtual ~IEngineSysBase() = default;

	virtual void registerVars(void) X_ABSTRACT;
	virtual void registerCmds(void) X_ABSTRACT;

	virtual bool init(void) X_ABSTRACT;
	virtual void shutDown(void) X_ABSTRACT;
	virtual void release(void) X_ABSTRACT;


};


X_NAMESPACE_END