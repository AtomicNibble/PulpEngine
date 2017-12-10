#pragma once

#ifndef X_SCRIPT_CLASS_BASE_H_
#define X_SCRIPT_CLASS_BASE_H_


#include "String\StackString.h"
#include "Traits\MemberFunctionTraits.h"


X_NAMESPACE_BEGIN(script)

#define SCRIPT_CHECK_PARAMETERS(_n) \
if (pH->GetParamCount() != _n) \
{  \
	pH->GetIScriptSystem()->onScriptError("[%s] %d arguments passed, " #_n " expected)", __FUNCTION__, pH->GetParamCount()); \
	return pH->EndFunction();  \
}

#define SCRIPT_CHECK_PARAMETERS_MIN(_n) \
if (pH->GetParamCount() < _n) \
{  \
	pH->GetIScriptSystem()->onScriptError("[%s] %d arguments passed, at least " #_n " expected)", __FUNCTION__, pH->GetParamCount()); \
	return pH->EndFunction(); \
}


class XScriptableBase
{
public:
	typedef IScriptTable::ScriptFunction ScriptFunction;

public:
	XScriptableBase();
	virtual ~XScriptableBase();

	virtual void Init(IScriptSys* pSS, ICore* pCore, int paramIdOffset = 0) X_ABSTRACT;
	void SetGlobalName(const char* GlobalName);
	void Delegate(XScriptableBase* pScriptableBase);

	IScriptTable* GetMethodsTable(void);

protected:
	void RegisterGlobal(const char* Name, float value);
	void RegisterGlobal(const char* Name, int value);

	void RegisterFunction(const char* funcName,
		const IScriptTable::ScriptFunction& function);

protected:
	core::StackString<60> name_;
	ICore* pCore_;
	IScriptSys* pScriptSys_;
	IScriptTable* pMethodsTable_;
	int paramIdOffset_;
};



X_NAMESPACE_END

#endif // !X_SCRIPT_CLASS_BASE_H_