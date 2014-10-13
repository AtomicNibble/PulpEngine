#pragma once

#ifndef X_SCRIPT_CLASS_BASE_H_
#define X_SCRIPT_CLASS_BASE_H_


#include "String\StackString.h"
#include "Traits\MemberFunctionTraits.h"


X_NAMESPACE_BEGIN(script)

#define SCRIPT_CHECK_PARAMETERS(_n) \
if (pH->GetParamCount() != _n) \
{  \
	pH->GetIScriptSystem()->OnScriptError("[%s] %d arguments passed, " #_n " expected)", __FUNCTION__, pH->GetParamCount()); \
	return pH->EndFunction();  \
}

#define SCRIPT_CHECK_PARAMETERS_MIN(_n) \
if (pH->GetParamCount() < _n) \
{  \
	pH->GetIScriptSystem()->OnScriptError("[%s] %d arguments passed, at least " #_n " expected)", __FUNCTION__, pH->GetParamCount()); \
	return pH->EndFunction(); \
}




struct IScriptableBase
{
	virtual ~IScriptableBase(){}


};


class XScriptableBase
{
public:
	typedef IScriptTable::ScriptFunction ScriptFunction;

	XScriptableBase() :
		pScriptSys_(nullptr)
	{

	}

	~XScriptableBase()
	{
		if (!name_.isEmpty() && pScriptSys_)
			pScriptSys_->SetGlobalToNull(name_.c_str());
	
		core::SafeRelease(pMethodsTable_);
	}

	void Init(IScriptSys* pSS, ICore* pCore, int paramIdOffset = 0)
	{
		pScriptSys_ = pSS;
		pMethodsTable_ = pSS->CreateTable();
		pMethodsTable_->addRef();
		paramIdOffset_ = paramIdOffset;
	}


	void SetGlobalName(const char* GlobalName)
	{
		name_.set(GlobalName);

		if (pMethodsTable_)
			pScriptSys_->SetGlobalValue(name_.c_str(), pMethodsTable_);
	}

	void Delegate(XScriptableBase* pScriptableBase)
	{
		if (pMethodsTable_ && pScriptableBase)
			pMethodsTable_->Delegate(pScriptableBase->pMethodsTable_);
	}

	IScriptTable* GetMethodsTable() { return pMethodsTable_; };

protected:

	void RegisterGlobal(const char* Name, float value)
	{
		pScriptSys_->SetGlobalValue(Name, value);
	}

	void RegisterGlobal(const char* Name, int value)
	{
		pScriptSys_->SetGlobalValue(Name, value);
	}


	void RegisterFunction(const char* funcName,
		const IScriptTable::ScriptFunction& function)
	{
		if (pMethodsTable_)
		{
			IScriptTable::XUserFunctionDesc fd;
			fd.sGlobalName = name_.c_str();
			fd.sFunctionName = funcName;
			fd.function = function;
			fd.nParamIdOffset = paramIdOffset_;

			pMethodsTable_->AddFunction(fd);
		}
	}

protected:
	core::StackString<60> name_;
	IScriptSys* pScriptSys_;
	IScriptTable* pMethodsTable_;
	int paramIdOffset_;
};


X_NAMESPACE_END

#endif // !X_SCRIPT_CLASS_BASE_H_