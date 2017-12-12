#include "stdafx.h"
#include "FunctionHandler.h"


X_NAMESPACE_BEGIN(script)

using namespace lua;

XFunctionHandler::XFunctionHandler(XScriptSys* pSS, lua_State* lState, const char* pFuncName, int32_t paramIdOffset)
{
	pSS_ = pSS;
	L_ = lState;
	pFuncName_ = pFuncName;
	paramIdOffset_ = paramIdOffset;
}


XFunctionHandler::~XFunctionHandler()
{

}

IScriptSys* XFunctionHandler::getIScriptSystem()
{
	return pSS_;
}

void* XFunctionHandler::getThis(void)
{
	void* pPtr = nullptr;

	// Get implicit self table.
	if (paramIdOffset_ > 0 && stack::get_type(L_, 1) == Type::Table)
	{
		// index "__this" member.
		stack::pushliteral(L_, "__this");
		stack::push_table_value_raw(L_, 1);
		if (stack::get_type(L_) == Type::Pointer) {
			pPtr = const_cast<void*>(stack::as_pointer(L_));
		}

		stack::pop(L_);
		return pPtr;
	}

	X_WARNING("Script", "GetThis called without param offset");
	return pPtr;
}

bool XFunctionHandler::getSelfAny(ScriptValue& any)
{
	bool bRes = false;

	if (paramIdOffset_ > 0) {
		bRes = pSS_->toAny(any, 1);
	}

	return bRes;
}

const char* XFunctionHandler::getFuncName(void)
{
	return pFuncName_;
}

int XFunctionHandler::getParamCount(void)
{
	return core::Max(stack::top(L_) - paramIdOffset_, 0);
}

Type::Enum XFunctionHandler::getParamType(int idx)
{
	const int realIdx = idx + paramIdOffset_;
	const int luatype = stack::get_type(L_, realIdx);

	return typeFormLua(luatype);
}


bool XFunctionHandler::getParamAny(int idx, ScriptValue& any)
{
	int realIdx = idx + paramIdOffset_;

	if (pSS_->toAny(any, realIdx)) {
		return true;
	}

	Type::Enum paramType = getParamType(idx);
	const char* pParamType = Type::ToString(paramType);
	const char* pType = Type::ToString(any.getType());

	// Report wrong param.
	X_WARNING("Script", "Wrong parameter type. Function %s expect parameter %" PRIi32 " of type %s (Provided type %s)",
		pFuncName_, idx, pType, pParamType);

	pSS_->logCallStack();
	return false;
}

int XFunctionHandler::endFunctionAny(const ScriptValue& any)
{
	pSS_->pushAny(any);
	return (any.getType() == Type::Nil) ? 0 : 1;
}

int XFunctionHandler::endFunctionAny(const ScriptValue& any1, const ScriptValue& any2)
{
	pSS_->pushAny(any1);
	pSS_->pushAny(any2);
	return 2;
}

int XFunctionHandler::endFunctionAny(const ScriptValue& any1, const ScriptValue& any2,
	const ScriptValue& any3)
{
	pSS_->pushAny(any1);
	pSS_->pushAny(any2);
	pSS_->pushAny(any3);
	return 3;
}

X_NAMESPACE_END
