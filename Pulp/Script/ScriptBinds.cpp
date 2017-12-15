#include "stdafx.h"
#include "ScriptBinds.h"

#include "ScriptTable.h"

X_NAMESPACE_BEGIN(script)

XScriptBindsBase::XScriptBindsBase(XScriptSys* pScriptSys) :
	pScriptSys_(pScriptSys),
	pBindTable_(nullptr)
{

}


void XScriptBindsBase::createBindTable(void)
{
	pBindTable_ = static_cast<XScriptBinds*>(pScriptSys_->createScriptBind());
}


void XScriptBindsBase::setName(const char* pName)
{
	pBindTable_->setName(pName);
}


void XScriptBindsBase::setGlobalName(const char* pGlobalName)
{
	pBindTable_->setGlobalName(pGlobalName);
}

void XScriptBindsBase::setParamOffset(int paramIdOffset)
{
	pBindTable_->setParamOffset(paramIdOffset);
}

XScriptTable* XScriptBindsBase::getMethodsTable(void)
{
	return static_cast<XScriptTable*>(pBindTable_->getMethodsTable());
}

// ----------------------------

XScriptBinds::XScriptBinds(XScriptSys* pScriptSys) :
	pScriptSys_(pScriptSys),
	pMethodsTable_(nullptr),
	paramIdOffset_(0)
{
	pMethodsTable_ = static_cast<XScriptTable*>(pScriptSys->createTable());
}

XScriptBinds::~XScriptBinds()
{
	if (name_.isNotEmpty() && pScriptSys_) {
		pScriptSys_->setGlobalToNull(name_.c_str());
	}

	core::SafeRelease(pMethodsTable_);
}

void XScriptBinds::setName(const char* pName)
{
	name_.set(pName);
}


void XScriptBinds::setGlobalName(const char* pGlobalName)
{
	X_ASSERT_NOT_NULL(pMethodsTable_);

	name_.set(pGlobalName);

	pScriptSys_->setGlobalValue(name_.c_str(), pMethodsTable_);
}

void XScriptBinds::setParamOffset(int paramIdOffset)
{
	paramIdOffset_ = paramIdOffset;
}

IScriptTable* XScriptBinds::getMethodsTable(void)
{
	return pMethodsTable_;
}

const char* XScriptBinds::getGlobalName(void) const
{
	return name_.c_str();
}

void XScriptBinds::registerFunction(const char* pFuncName, const IScriptTable::ScriptFunction& function)
{
	X_ASSERT_NOT_NULL(pMethodsTable_);

	ScriptFunctionDesc fd;
	fd.pGlobalName = name_.c_str();
	fd.pFunctionName = pFuncName;
	fd.function = function;
	fd.paramIdOffset = paramIdOffset_;

	pMethodsTable_->addFunction(fd);
}



X_NAMESPACE_END