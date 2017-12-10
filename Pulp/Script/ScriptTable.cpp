#include "stdafx.h"
#include "ScriptSys.h"
#include "ScriptTable.h"
#include "FunctionHandler.h"

#include <Util\ToggleChecker.h>


X_NAMESPACE_BEGIN(script)


namespace
{
	core::ToggleChecker g_setChainActive(false);

	struct ScriptTableRed
	{
		enum Enum
		{
			REF_DELETED = LUA_NOREF,
			REF_NULL = LUA_REFNIL,
		};
	};

}

lua_State* XScriptTable::L = nullptr;
XScriptSys* XScriptTable::pScriptSystem_ = nullptr;

std::set<class XScriptTable*> XScriptTable::s_allTables_;



XScriptTable::XScriptTable() :
	refCount_(0),
	luaRef_(ScriptTableRed::REF_NULL)
{

}

XScriptTable::~XScriptTable()
{
}



IScriptSys* XScriptTable::GetScriptSystem() const
{
	return pScriptSystem_;
}

void XScriptTable::Delegate(IScriptTable* pMetatable)
{
	if (!pMetatable)
		return;

	X_LUA_CHECK_STACK(L);

	PushRef(pMetatable);
	lua_pushstring(L, "__index"); // push key.	
	PushRef(pMetatable);
	lua_rawset(L, -3); // sets metatable.__index = metatable
	lua_pop(L, 1); // pop metatable from stack.

	SetMetatable(pMetatable);
}

void* XScriptTable::GetUserDataValue()
{
	PushRef();
	void * ptr = lua_touserdata(L, -1);
	lua_pop(L, 1);
	return ptr;
}


bool XScriptTable::BeginSetGetChain()
{
	g_setChainActive = true;

	PushRef();
	return true;
}

void XScriptTable::EndSetGetChain()
{
	g_setChainActive = false;

	if (lua_istable(L, -1))
		lua_pop(L, 1);
	else
	{
		X_ASSERT(false, "Mismatch in Set/Get Chain")();
	}
}

void XScriptTable::SetValueAny(const char* sKey, const ScriptValue& any, bool bChain)
{
	X_LUA_CHECK_STACK(L);
	X_ASSERT_NOT_NULL(sKey);

	int top = lua_gettop(L);

	ScriptValue oldValue;
	if (lua_getmetatable(L, -1))	// if there is no metatable nothing is pushed
	{
		lua_pop(L, 1);	// pop the metatable - we only care that it exists, not about the value
		if (GetValueAny(sKey, oldValue, bChain) && oldValue == any)
			return;
	}

	if (!bChain)
		PushRef();

	size_t len = strlen(sKey);

	if (any.getType() == ScriptValueType::VECTOR)
	{
		// Check if we can reuse Vec3 value already in the table.
		lua_pushlstring(L, sKey, len);
		lua_gettable(L, -2);
		int luatype = lua_type(L, -1);
		if (luatype == LUA_TTABLE)
		{
			lua_pushlstring(L, "x", 1);
			lua_gettable(L, -2);
			bool bXIsNumber = lua_isnumber(L, -1) != 0;
			lua_pop(L, 1); // pop x value.
			if (bXIsNumber)
			{
				// Assume its a vector, just fill it with new vector values.
				lua_pushlstring(L, "x", 1);
				lua_pushnumber(L, any.vec3.x);
				lua_settable(L, -3);
				lua_pushlstring(L, "y", 1);
				lua_pushnumber(L, any.vec3.y);
				lua_settable(L, -3);
				lua_pushlstring(L, "z", 1);
				lua_pushnumber(L, any.vec3.z);
				lua_settable(L, -3);

				lua_settop(L, top);
				return;
			}
		}
		lua_pop(L, 1); // pop key value.
	}
	lua_pushlstring(L, sKey, len);
	pScriptSystem_->PushAny(any);
	lua_settable(L, -3);
	lua_settop(L, top);
}

bool XScriptTable::GetValueAny(const char* sKey, ScriptValue& any, bool bChain)
{
	X_LUA_CHECK_STACK(L);
	int top = lua_gettop(L);
	if (!bChain)
		PushRef();
	bool res = false;
	lua_pushstring(L, sKey);
	lua_gettable(L, -2);
	res = pScriptSystem_->PopAny(any);
	lua_settop(L, top);
	return res;
}

//////////////////////////////////////////////////////////////////////////
void XScriptTable::SetAtAny(int nIndex, const ScriptValue &any)
{
	X_LUA_CHECK_STACK(L);
	PushRef();
	pScriptSystem_->PushAny(any);
	lua_rawseti(L, -2, nIndex);
	lua_pop(L, 1); // Pop table.
}

bool XScriptTable::GetAtAny(int nIndex, ScriptValue &any)
{
	X_LUA_CHECK_STACK(L);
	bool res = false;
	PushRef();
	lua_rawgeti(L, -1, nIndex);
	res = pScriptSystem_->PopAny(any);
	lua_pop(L, 1); // Pop table.

	return res;
}

ScriptValueType::Enum XScriptTable::GetValueType(const char* sKey)
{
	X_LUA_CHECK_STACK(L);
	ScriptValueType::Enum type = ScriptValueType::NONE;

	PushRef();
	lua_pushstring(L, sKey);
	lua_gettable(L, -2);
	int luatype = lua_type(L, -1);

	switch (luatype)
	{
		case LUA_TNIL: type = ScriptValueType::NONE; break;
		case LUA_TBOOLEAN: type = ScriptValueType::BOOLEAN; break;
		case LUA_TNUMBER: type = ScriptValueType::NUMBER; break;
		case LUA_TSTRING: type = ScriptValueType::STRING; break;
		case LUA_TFUNCTION: type = ScriptValueType::FUNCTION; break;
		case LUA_TLIGHTUSERDATA: type = ScriptValueType::POINTER; break;
		case LUA_TTABLE: type = ScriptValueType::TABLE; break;
	}

	lua_pop(L, 2); // Pop value and table.

	return type;
}

ScriptValueType::Enum XScriptTable::GetAtType(int nIdx)
{
	X_LUA_CHECK_STACK(L);
	ScriptValueType::Enum svtRetVal = ScriptValueType::NONE;
	PushRef();

//	if (luaL_getn(L, -1) < nIdx)
//	{
//		lua_pop(L, 1);
//		return svtNull;
//	}

	lua_rawgeti(L, -1, nIdx);

	if (lua_isnil(L, -1))
	{
		svtRetVal = ScriptValueType::TNIL;
	}
	else if (lua_isnone(L, -1))
	{
		svtRetVal = ScriptValueType::NONE;
	}
	else if (lua_isfunction(L, -1))
	{
		svtRetVal = ScriptValueType::FUNCTION;
	}
	else if (lua_isnumber(L, -1))
	{
		svtRetVal = ScriptValueType::NUMBER;
	}
	else if (lua_isstring(L, -1))
	{
		svtRetVal = ScriptValueType::STRING;
	}
	else if (lua_istable(L, -1))
	{
		svtRetVal = ScriptValueType::TABLE;
	}
	else if (lua_isboolean(L, -1))
	{
		svtRetVal = ScriptValueType::BOOLEAN;
	}

	lua_pop(L, 2);
	return svtRetVal;
}


// Iteration.
IScriptTable::Iterator XScriptTable::BeginIteration()
{
	Iterator iter;
	iter.internal = lua_gettop(L) + 1;

	PushRef();
	lua_pushnil(L);
	return iter;
}

bool XScriptTable::MoveNext(Iterator& iter)
{
	if (!iter.internal)
		return false;

	int nTop = iter.internal - 1;

	//leave only the index into the stack
	while ((lua_gettop(L) - (nTop + 1))>1)
	{
		lua_pop(L, 1);
	}

	bool res = lua_next(L, nTop + 1) != 0;
	if (res)
	{
		iter.value.Clear();
		res = pScriptSystem_->PopAny(iter.value);
		// Get current key.
		pScriptSystem_->ToAny(iter.key, -1);
		if (lua_type(L, -1) == LUA_TSTRING)
		{
			// String key.
	//		iter.sKey = (const char*)lua_tostring(L, -1);
	//		iter.nKey = -1;
		}
		else if (lua_type(L, -1) == LUA_TNUMBER)
		{
			// Number key.
	//		iter.sKey = NULL;
	//		iter.nKey = (int)lua_tonumber(L, -1);
		}
		else
		{
	//		iter.sKey = 0;
	//		iter.nKey = -1;
		}
	}

	if (!res)
	{
		EndIteration(iter);
		iter.internal = 0;
	}
	return res;
}

void XScriptTable::EndIteration(const Iterator& iter)
{
	if (iter.internal)
	{
		lua_settop(L, iter.internal - 1);
	}
}


void XScriptTable::Clear()
{
	X_LUA_CHECK_STACK(L);

	PushRef();
	int trgTable = lua_gettop(L);

	lua_pushnil(L);  // first key
	while (lua_next(L, trgTable) != 0)
	{
		lua_pop(L, 1); // pop value, leave index.
		lua_pushvalue(L, -1); // Push again index.
		lua_pushnil(L);
		lua_rawset(L, trgTable);
	}
	X_ASSERT(lua_istable(L, -1),"should be a table")();
	lua_pop(L, 1);
}

int XScriptTable::Count()
{
	X_LUA_CHECK_STACK(L);

	PushRef();
	int count = luaL_getn(L, -1);
	lua_pop(L, 1);

	return count;
}

bool XScriptTable::Clone(IScriptTable* pSrcTable, bool bDeepCopy, bool bCopyByReference)
{
	X_LUA_CHECK_STACK(L);

	int top = lua_gettop(L);

	PushRef(pSrcTable);
	PushRef();

	int srcTable = top + 1;
	int trgTable = top + 2;

	if (bDeepCopy)
	{
		if (bCopyByReference)
		{
			ReferenceTable_r(srcTable, trgTable);
		}
		else
		{
			CloneTable_r(srcTable, trgTable);
		}
	}
	else
	{
		CloneTable(srcTable, trgTable);
	}
	lua_settop(L, top); // Restore stack.

	return true;
}

void XScriptTable::Dump(IScriptTableDumpSink* p)
{
	if (!p)
		return;

	X_LUA_CHECK_STACK(L);
	int top = lua_gettop(L);

	PushRef();

	int trgTable = top + 1;

	lua_pushnil(L);  // first key
	int reftop = lua_gettop(L);
	while (lua_next(L, trgTable) != 0)
	{
		// `key' is at index -2 and `value' at index -1
		if (lua_type(L, -2) == LUA_TSTRING)
		{
			const char *sName = lua_tostring(L, -2); // again index
			switch (lua_type(L, -1))
			{
				case LUA_TNIL: p->OnElementFound(sName, ScriptValueType::TNIL); break;
				case LUA_TBOOLEAN: p->OnElementFound(sName, ScriptValueType::BOOLEAN); break;
				case LUA_TLIGHTUSERDATA: p->OnElementFound(sName, ScriptValueType::POINTER); break;
				case LUA_TNUMBER: p->OnElementFound(sName, ScriptValueType::NUMBER); break;
				case LUA_TSTRING: p->OnElementFound(sName, ScriptValueType::STRING); break;
				case LUA_TTABLE: p->OnElementFound(sName, ScriptValueType::TABLE); break;
				case LUA_TFUNCTION: p->OnElementFound(sName, ScriptValueType::FUNCTION); break;
		//		case LUA_TUSERDATA: p->OnElementFound(sName, svtUserData); break;
			};
		}
		else
		{
			int nIdx = (int)lua_tonumber(L, -2); // again index
			switch (lua_type(L, -1))
			{
				case LUA_TNIL: p->OnElementFound(nIdx, ScriptValueType::TNIL); break;
				case LUA_TBOOLEAN: p->OnElementFound(nIdx, ScriptValueType::BOOLEAN); break;
				case LUA_TLIGHTUSERDATA: p->OnElementFound(nIdx, ScriptValueType::POINTER); break;
				case LUA_TNUMBER: p->OnElementFound(nIdx, ScriptValueType::NUMBER); break;
				case LUA_TSTRING: p->OnElementFound(nIdx, ScriptValueType::STRING); break;
				case LUA_TTABLE: p->OnElementFound(nIdx, ScriptValueType::TABLE); break;
				case LUA_TFUNCTION: p->OnElementFound(nIdx, ScriptValueType::FUNCTION); break;
		//		case LUA_TUSERDATA: p->OnElementFound(nIdx, svtUserData); break;
			};
		}
		lua_settop(L, reftop); // pop value, leave index.
	}
	lua_pop(L, 1); // pop table ref
}

bool XScriptTable::AddFunction(const XUserFunctionDesc& fd)
{
	X_LUA_CHECK_STACK(L);
//	X_ASSERT_NOT_N(fd.pFunction);

	// Make function signature.
	core::StackString<256> funcSig;
	funcSig.appendFmt( "%s.%s(%s)", fd.sGlobalName, fd.sFunctionName, fd.sFunctionParams);

	PushRef();
	lua_pushstring(L, fd.sFunctionName);

	int8 nParamIdOffset = safe_static_cast<int8,int>(fd.nParamIdOffset);
	if (fd.function)
	{
		size_t nDataSize = sizeof(fd.function) + funcSig.length() + 1 + 1;

		// Store functor in first upvalue.
		unsigned char* pBuffer = (unsigned char*)lua_newuserdata(L, nDataSize);

		memcpy(pBuffer, &fd.function, sizeof(fd.function));
		memcpy(pBuffer + sizeof(fd.function), &nParamIdOffset, 1);
		memcpy(pBuffer + sizeof(fd.function) + 1, funcSig.c_str(), funcSig.length() + 1);
	
		lua_pushcclosure(L, StdCFunction, 1);
	}
	else
	{
		// user data function.
		UserDataFunction::Pointer function = fd.pUserDataFunc;
		size_t nSize = fd.userDataSize;
		size_t nTotalSize = sizeof(function)+sizeof(int)+nSize + funcSig.length() + 1 + 1;

		// Store functor in first upvalue.
		unsigned char *pBuffer = (unsigned char*)lua_newuserdata(L, nTotalSize);
		memcpy(pBuffer, &function, sizeof(function));
		memcpy(pBuffer + sizeof(function), &nSize, sizeof(nSize));
		memcpy(pBuffer + sizeof(function)+sizeof(nSize), fd.pDataBuffer, nSize);
		memcpy(pBuffer + sizeof(function)+sizeof(nSize)+nSize, &nParamIdOffset, 1);
		memcpy(pBuffer + sizeof(function)+sizeof(nSize)+nSize + 1, funcSig.c_str(), 
			funcSig.length() + 1);
		
		lua_pushcclosure(L, StdCUserDataFunction, 1);
	}


	lua_rawset(L, -3);
	lua_pop(L, 1); // pop table.
	return true;
}


// --------------------------------------------------------------------------


void XScriptTable::CreateNew()
{
	lua_newtable(L);
	Attach();
}

int XScriptTable::GetRef()
{
	return luaRef_;
}

void XScriptTable::Attach()
{
	if (luaRef_ != ScriptTableRed::REF_NULL)
		lua_unref(L, luaRef_);
	luaRef_ = lua_ref(L, 1);

	s_allTables_.insert(this);
}

void XScriptTable::AttachToObject(IScriptTable* so)
{
	PushRef(so);
	Attach();
}

void XScriptTable::DeleteThis()
{
	if (luaRef_ == ScriptTableRed::REF_DELETED)
	{
		X_FATAL("Script", "attempt to Release already released script table.");
	}

	s_allTables_.erase(this);

	if (luaRef_ != ScriptTableRed::REF_NULL)
		lua_unref(L, luaRef_);

	luaRef_ = ScriptTableRed::REF_DELETED;

	X_DELETE(this,g_ScriptArena);
}

// Create object from pool.
// Assign a metatable to a table.
void XScriptTable::SetMetatable(IScriptTable* pMetatable)
{
	X_LUA_CHECK_STACK(L);

	// Set metatable for this script object.
	PushRef(); // -2
	PushRef(pMetatable); // -1
	lua_setmetatable(L, -2);
	lua_pop(L, 1); // pop table
}

// Push reference of this object to the stack.
void XScriptTable::PushRef()
{
	if (luaRef_ != ScriptTableRed::REF_DELETED && luaRef_ != ScriptTableRed::REF_NULL)
		lua_getref(L, luaRef_);
	else
	{
		lua_pushnil(L);
		if (luaRef_ == ScriptTableRed::REF_DELETED)
		{

			X_FATAL("Script", "Access to deleted script object: %p", this);
		}
		else
		{
			X_ERROR("Script", "Pushing Nil table reference");
		}
	}
}

// Push reference to specified script table to the stack.
void XScriptTable::PushRef(IScriptTable* pObj)
{
	int nRef = ((XScriptTable*)pObj)->luaRef_;
	if (nRef != ScriptTableRed::REF_DELETED)
		lua_getref(L, nRef);
	else
	{
		X_FATAL("Script", "Access to deleted script object");
	}
}


X_DISABLE_WARNING(4458) // declaration of 'L' hides class member
int XScriptTable::StdCFunction(lua_State* L)
{
	unsigned char* pBuffer = (unsigned char*)lua_touserdata(L, lua_upvalueindex(1));


	ScriptFunction* function = (ScriptFunction*)pBuffer;
	int8 nParamIdOffset = *(int8*)(pBuffer + sizeof(ScriptFunction));
	const char* sFuncName = (const char*)(pBuffer + sizeof(ScriptFunction)+1);
	XFunctionHandler fh(pScriptSystem_, L, sFuncName, nParamIdOffset);
	
	
	int ret = function->Invoke(&fh);
	return ret;
}

int XScriptTable::StdCUserDataFunction(lua_State* L)
{
	unsigned char *pBuffer = (unsigned char*)lua_touserdata(L, lua_upvalueindex(1));

	UserDataFunction::Pointer* pFunction = (UserDataFunction::Pointer*)pBuffer;

	pBuffer += sizeof(UserDataFunction::Pointer);
	int nSize = *((int*)pBuffer); // first 4 bytes are size of user data.
	pBuffer += sizeof(int);
	int8 nParamIdOffset = *(int8*)(pBuffer + nSize);
	const char* FuncName = (const char*)(pBuffer + nSize + 1);

	XFunctionHandler fh(pScriptSystem_, L, FuncName, nParamIdOffset);
	// Call functor.
	int ret = (*pFunction)(&fh, pBuffer, nSize); 
	return ret;
}

X_ENABLE_WARNING(4458)

// --------------------------------------------------------------------------


void XScriptTable::CloneTable_r(int srcTable, int trgTable)
{
	X_LUA_CHECK_STACK(L);

	int top = lua_gettop(L);
	lua_pushnil(L);  // first key
	while (lua_next(L, srcTable) != 0)
	{
		if (lua_type(L, -1) == LUA_TTABLE)
		{
			int srct = lua_gettop(L);

			lua_pushvalue(L, -2); // Push again index.
			lua_newtable(L);      // Make value.
			int trgt = lua_gettop(L);
			CloneTable_r(srct, trgt);
			lua_rawset(L, trgTable); // Set new table to trgtable.
		}
		else
		{
			// `key' is at index -2 and `value' at index -1
			lua_pushvalue(L, -2); // Push again index.
			lua_pushvalue(L, -2); // Push value.
			lua_rawset(L, trgTable);
		}
		lua_pop(L, 1); // pop value, leave index.
	}
	lua_settop(L, top); // Restore stack.
}

void XScriptTable::ReferenceTable_r(int srcTable, int trgTable)
{
	X_LUA_CHECK_STACK(L);

	int top = lua_gettop(L);

	lua_newtable(L);									// push new meta table
	lua_pushlstring(L, "__index", strlen("__index"));	// push __index
	lua_pushvalue(L, srcTable);							// push src table
	lua_rawset(L, -3);									// meta.__index==src table
	lua_setmetatable(L, trgTable);						// set meta table

	lua_pushnil(L);  // first key
	while (lua_next(L, srcTable) != 0)
	{
		if (lua_type(L, -1) == LUA_TTABLE)
		{
			int srct = lua_gettop(L);
			lua_pushvalue(L, -2); // Push again index.
			lua_newtable(L);      // Make value.
			int trgt = lua_gettop(L);
			ReferenceTable_r(srct, trgt);
			lua_rawset(L, trgTable); // Set new table to trgtable.
		}
		lua_pop(L, 1); // pop value, leave index.
	}
	lua_settop(L, top); // Restore stack.
}

void XScriptTable::CloneTable(int srcTable, int trgTable)
{
	X_LUA_CHECK_STACK(L);

	int top = lua_gettop(L);
	lua_pushnil(L);  // first key
	while (lua_next(L, srcTable) != 0)
	{
		// `key' is at index -2 and `value' at index -1
		lua_pushvalue(L, -2); // Push again index.
		lua_pushvalue(L, -2); // Push value.
		lua_rawset(L, trgTable);
		lua_pop(L, 1); // pop value, leave index.
	}
	lua_settop(L, top); // Restore stack.
}


X_NAMESPACE_END

