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

	X_INLINE XScriptableBase();
	X_INLINE ~XScriptableBase();

	X_INLINE void Init(IScriptSys* pSS, ICore* pCore, int paramIdOffset = 0);
	X_INLINE void SetGlobalName(const char* GlobalName);
	X_INLINE void Delegate(XScriptableBase* pScriptableBase);

	X_INLINE IScriptTable* GetMethodsTable(void);

protected:

	X_INLINE void RegisterGlobal(const char* Name, float value);
	X_INLINE void RegisterGlobal(const char* Name, int value);

	X_INLINE void RegisterFunction(const char* funcName,
		const IScriptTable::ScriptFunction& function);

protected:
	core::StackString<60> name_;
	IScriptSys* pScriptSys_;
	IScriptTable* pMethodsTable_;
	int paramIdOffset_;
};


XScriptableBase::XScriptableBase() :
	pScriptSys_(nullptr),
	pMethodsTable_(nullptr),
	paramIdOffset_(0)
{

}

XScriptableBase::~XScriptableBase()
{
	if (!name_.isEmpty() && pScriptSys_) {
		pScriptSys_->SetGlobalToNull(name_.c_str());
	}
	core::SafeRelease(pMethodsTable_);
}

void XScriptableBase::Init(IScriptSys* pSS, ICore* pCore, int paramIdOffset)
{
	X_UNUSED(pCore);
	pScriptSys_ = pSS;
	pMethodsTable_ = pSS->CreateTable();
	pMethodsTable_->addRef();
	paramIdOffset_ = paramIdOffset;
}


void XScriptableBase::SetGlobalName(const char* GlobalName)
{
	name_.set(GlobalName);

	if (pMethodsTable_) {
		pScriptSys_->SetGlobalValue(name_.c_str(), pMethodsTable_);
	}
}

void XScriptableBase::Delegate(XScriptableBase* pScriptableBase)
{
	if (pMethodsTable_ && pScriptableBase) {
		pMethodsTable_->Delegate(pScriptableBase->pMethodsTable_);
	}
}

IScriptTable* XScriptableBase::GetMethodsTable(void)
{ 
	return pMethodsTable_; 
}


// ----------------------------------------------------------------

void XScriptableBase::RegisterGlobal(const char* Name, float value)
{
	pScriptSys_->SetGlobalValue(Name, value);
}

void XScriptableBase::RegisterGlobal(const char* Name, int value)
{
	pScriptSys_->SetGlobalValue(Name, value);
}


void XScriptableBase::RegisterFunction(const char* funcName,
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


X_NAMESPACE_END

#endif // !X_SCRIPT_CLASS_BASE_H_