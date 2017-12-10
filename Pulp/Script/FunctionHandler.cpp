#include "stdafx.h"
#include "FunctionHandler.h"


X_NAMESPACE_BEGIN(script)



XFunctionHandler::~XFunctionHandler()
{

}

IScriptSys* XFunctionHandler::GetIScriptSystem()
{
	return pSS_;
}

void* XFunctionHandler::GetThis()
{
	void *ptr = NULL;
	// Get implicit self table.
	if (paramIdOffset_ > 0 && lua_type(L, 1) == LUA_TTABLE)
	{
		// index "__this" member.
		lua_pushstring(L, "__this");
		lua_rawget(L, 1);
		if (lua_type(L, -1) == LUA_TLIGHTUSERDATA)
			ptr = const_cast<void*>(lua_topointer(L, -1));
		lua_pop(L, 1); // pop result.
	}
	return ptr;
}

bool XFunctionHandler::GetSelfAny(ScriptValue &any)
{
	bool bRes = false;
	if (paramIdOffset_ > 0)
	{
		bRes = pSS_->ToAny(any, 1);
	}
	return bRes;
}

const char* XFunctionHandler::GetFuncName()
{
	return sFuncName_;
}

int XFunctionHandler::GetParamCount()
{
	return core::Max(lua_gettop(L) - paramIdOffset_, 0);
}

Type::Enum XFunctionHandler::GetParamType(int nIdx)
{
	int nRealIdx = nIdx + paramIdOffset_;
	Type::Enum type = Type::NONE;
	int luatype = lua_type(L, nRealIdx);
	switch (luatype)
	{
		case LUA_TNIL: type = Type::NIL; break;
		case LUA_TBOOLEAN: type = Type::BOOLEAN; break;
		case LUA_TNUMBER: type = Type::NUMBER; break;
		case LUA_TSTRING: type = Type::STRING; break;
		case LUA_TFUNCTION: type = Type::FUNCTION; break;
		case LUA_TLIGHTUSERDATA: type = Type::POINTER; break;
		case LUA_TTABLE: type = Type::TABLE; break;
	}
	return type;
}


bool XFunctionHandler::GetParamAny(int nIdx, ScriptValue &any)
{
	int nRealIdx = nIdx + paramIdOffset_;
	bool bRes = pSS_->ToAny(any, nRealIdx);
	if (!bRes)
	{

		Type::Enum paramType = GetParamType(nIdx);
		const char* sParamType = Type::ToString(paramType);
		const char* sType = Type::ToString(any.getType());
		// Report wrong param.
		X_WARNING("Script","Wrong parameter type. Function %s expect parameter %d of type %s (Provided type %s)", 
			sFuncName_, nIdx, sType, sParamType);
	
//		pSS_->LogStackTrace();
	}
	return bRes;
}

int XFunctionHandler::EndFunctionAny(const ScriptValue& any)
{
	pSS_->PushAny(any);
	return (any.getType() == Type::NIL) ? 0 : 1;
}

int XFunctionHandler::EndFunctionAny(const ScriptValue& any1, const ScriptValue& any2)
{
	pSS_->PushAny(any1);
	pSS_->PushAny(any2);
	return 2;
}

int XFunctionHandler::EndFunctionAny(const ScriptValue& any1, const ScriptValue& any2,
	const ScriptValue& any3)
{
	pSS_->PushAny(any1);
	pSS_->PushAny(any2);
	pSS_->PushAny(any3);
	return 3;
}

int XFunctionHandler::EndFunction()
{
	return 0;
}


X_NAMESPACE_END
