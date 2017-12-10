#include "stdafx.h"
#include "ScriptableBase.h"


X_NAMESPACE_BEGIN(script)

XScriptableBase::XScriptableBase() :
	pScriptSys_(nullptr),
	pMethodsTable_(nullptr),
	paramIdOffset_(0)
{

}

XScriptableBase::~XScriptableBase()
{
	if (!name_.isEmpty() && pScriptSys_) {
		pScriptSys_->setGlobalToNull(name_.c_str());
	}
	core::SafeRelease(pMethodsTable_);
}

void XScriptableBase::Init(IScriptSys* pSS, ICore* pCore, int paramIdOffset)
{
	X_ASSERT_NOT_NULL(pCore);
	X_ASSERT_NOT_NULL(pSS);

	pCore_ = pCore;
	pScriptSys_ = pSS;
	pMethodsTable_ = pSS->createTable();
	pMethodsTable_->addRef();
	paramIdOffset_ = paramIdOffset;
}


void XScriptableBase::SetGlobalName(const char* pGlobalName)
{
	X_ASSERT_NOT_NULL(pGlobalName);

	name_.set(pGlobalName);

	if (pMethodsTable_) {
		pScriptSys_->setGlobalValue(name_.c_str(), pMethodsTable_);
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

void XScriptableBase::RegisterGlobal(const char* pName, float value)
{
	pScriptSys_->setGlobalValue(pName, value);
}

void XScriptableBase::RegisterGlobal(const char* pName, int value)
{
	pScriptSys_->setGlobalValue(pName, value);
}


void XScriptableBase::RegisterFunction(const char* pFuncName,
	const IScriptTable::ScriptFunction& function)
{
	if (pMethodsTable_)
	{
		IScriptTable::XUserFunctionDesc fd;
		fd.sGlobalName = name_.c_str();
		fd.sFunctionName = pFuncName;
		fd.function = function;
		fd.nParamIdOffset = paramIdOffset_;

		pMethodsTable_->AddFunction(fd);
	}
}




X_NAMESPACE_END