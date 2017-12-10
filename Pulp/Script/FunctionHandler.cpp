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
	if (paramIdOffset_ > 0 && stack::get_type(L_, 1) == LUA_TTABLE)
	{
		// index "__this" member.
		stack::pushliteral(L_, "__this");

		lua_rawget(L_, 1);
		if (stack::get_type(L_) == LUA_TLIGHTUSERDATA) {
			pPtr = const_cast<void*>(stack::as_pointer(L_));
		}

		stack::pop(L_);
	}

	return pPtr;
}

bool XFunctionHandler::getSelfAny(ScriptValue &any)
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
	return core::Max(lua_gettop(L_) - paramIdOffset_, 0);
}

Type::Enum XFunctionHandler::getParamType(int idx)
{
	const int realIdx = idx + paramIdOffset_;
	const int luatype = stack::get_type(L_, realIdx);

	return typeFormLua(luatype);
}


bool XFunctionHandler::getParamAny(int nIdx, ScriptValue &any)
{
	int realIdx = nIdx + paramIdOffset_;

	if (pSS_->toAny(any, realIdx)) {
		return true;
	}


	Type::Enum paramType = getParamType(nIdx);
	const char* sParamType = Type::ToString(paramType);
	const char* sType = Type::ToString(any.getType());
	// Report wrong param.
	X_WARNING("Script", "Wrong parameter type. Function %s expect parameter %d of type %s (Provided type %s)",
		pFuncName_, nIdx, sType, sParamType);

	//		pSS_->LogStackTrace();
	return false;
}

int XFunctionHandler::endFunctionAny(const ScriptValue& any)
{
	pSS_->pushAny(any);
	return (any.getType() == Type::NIL) ? 0 : 1;
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
