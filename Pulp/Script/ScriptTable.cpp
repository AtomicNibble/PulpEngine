#include "stdafx.h"
#include "ScriptSys.h"
#include "ScriptTable.h"
#include "FunctionHandler.h"


#include "wrapper\types.h"

X_NAMESPACE_BEGIN(script)

using namespace lua;

namespace
{


}

lua_State* XScriptTable::L = nullptr;
XScriptSys* XScriptTable::pScriptSystem_ = nullptr;

// std::set<class XScriptTable*> XScriptTable::s_allTables_;



XScriptTable::XScriptTable() :
	refCount_(1),
	luaRef_(lua::Ref::Nil)
#if X_DEBUG
	, setChainActive_(false)
#endif // !X_DEBUG
{

}

XScriptTable::~XScriptTable()
{

}

void XScriptTable::addRef(void) 
{ 
	refCount_++; 
}

void XScriptTable::release(void) 
{ 
	if (--refCount_ <= 0) {
		deleteThis();
	}
}

IScriptSys* XScriptTable::getIScriptSystem(void) const
{
	return pScriptSystem_;
}

void* XScriptTable::getUserData(void)
{
	pushRef();

	void* pPtr = stack::as_userdata(L);
	
	stack::pop(L, 1);
	return pPtr;
}

void XScriptTable::setValueAny(const char* pKey, const ScriptValue& any, bool bChain)
{
	X_LUA_CHECK_STACK(L);
	X_ASSERT_NOT_NULL(pKey);

	int top = stack::top(L);

	ScriptValue oldValue;
	if (lua_getmetatable(L, -1))	// if there is no metatable nothing is pushed
	{
		stack::pop(L);	// pop the metatable - we only care that it exists, not about the value
		if (getValueAny(pKey, oldValue, bChain) && oldValue == any) {
			return;
		}
	}

	if (!bChain) {
		pushRef();
	}
	else {
#if X_DEBUG
		X_ASSERT(setChainActive_, "begin chain not called")();
#endif // !X_DEBUG
	}

	size_t len = strlen(pKey);

	if (any.getType() == Type::Vector)
	{
		// Check if we can reuse Vec3 value already in the table.
		lua_pushlstring(L, pKey, len);
		lua_gettable(L, -2);
		int luatype = lua_type(L, -1);
		if (luatype == LUA_TTABLE)
		{
			lua_pushlstring(L, "x", 1);
			lua_gettable(L, -2);
			bool bXIsNumber = lua_isnumber(L, -1) != 0;
			stack::pop(L); // pop x value.
			if (bXIsNumber)
			{
				// Assume its a vector, just fill it with new vector values.
				lua_pushlstring(L, "x", 1);
				lua_pushnumber(L, any.vec3_.x);
				lua_settable(L, -3);
				lua_pushlstring(L, "y", 1);
				lua_pushnumber(L, any.vec3_.y);
				lua_settable(L, -3);
				lua_pushlstring(L, "z", 1);
				lua_pushnumber(L, any.vec3_.z);
				lua_settable(L, -3);

				lua_settop(L, top);
				return;
			}
		}
		stack::pop(L); // pop key value.
	}
	lua_pushlstring(L, pKey, len);
	pScriptSystem_->pushAny(any);
	lua_settable(L, -3);
	lua_settop(L, top);
}

bool XScriptTable::getValueAny(const char* pKey, ScriptValue& any, bool bChain)
{
	X_LUA_CHECK_STACK(L);

	int top = stack::top(L);

	if (!bChain) {
		pushRef();
	}
	else {
#if X_DEBUG
		X_ASSERT(setChainActive_, "begin chain not called")();
#endif // !X_DEBUG
	}
	
	stack::push_table_value(L, -2, pKey);

	bool res = pScriptSystem_->popAny(any);

	stack::settop(L, top);
	return res;
}

void XScriptTable::setValueAny(int idx, const ScriptValue& any)
{
	X_LUA_CHECK_STACK(L);
	pushRef();

	pScriptSystem_->pushAny(any);
	stack::pop_value_to_table(L, -2, idx);

	stack::pop(L); 
}

bool XScriptTable::getValueAny(int idx, ScriptValue& any)
{
	X_LUA_CHECK_STACK(L);
	pushRef();

	stack::push_table_value(L, -1, idx);
	bool res = pScriptSystem_->popAny(any);
	stack::pop(L); 

	return res;
}

Type::Enum XScriptTable::getValueType(const char* pKey)
{
	X_LUA_CHECK_STACK(L);

	pushRef();

	stack::push_table_value(L, -2, pKey);
	int luatype = stack::get_type(L);

	stack::pop(L, 2); // value and table.

	return lua::typeFormLua(luatype);
}

Type::Enum XScriptTable::getValueType(int idx)
{
	X_LUA_CHECK_STACK(L);

	pushRef();

	stack::push_table_value(L, -1, idx);
	int luatype = stack::get_type(L);

	stack::pop(L, 2); // value and table.

	return lua::typeFormLua(luatype);
}


bool XScriptTable::beginChain(void)
{
#if X_DEBUG
	setChainActive_ = true;
#endif // !X_DEBUG

	pushRef();
	return true;
}

void XScriptTable::endChain(void)
{
#if X_DEBUG
	setChainActive_ = false;
#endif // !X_DEBUG

	if (stack::istable(L)) {
		stack::pop(L);
	}
	else
	{
		X_ASSERT(false, "Mismatch in Set/Get Chain")();
	}
}

void XScriptTable::clear(void)
{
	X_LUA_CHECK_STACK(L);

	pushRef();

	int trgTable = stack::top(L);

	stack::pushnil(L); // first key
	while (lua_next(L, trgTable) != 0)
	{
		stack::pop(L); // pop value, leave index.
		lua_pushvalue(L, -1); // Push again index.
		stack::pushnil(L);
		lua_rawset(L, trgTable);
	}

	X_ASSERT(stack::istable(L), "should be a table")(stack::get_type(L));
	stack::pop(L);
}

size_t XScriptTable::count(void)
{
	X_LUA_CHECK_STACK(L);

	pushRef();
	size_t count = stack::rawlen(L);
	stack::pop(L);
	return count;
}

void XScriptTable::setMetatable(IScriptTable* pMetatable)
{
	X_LUA_CHECK_STACK(L);

	pushRef(pMetatable);
	stack::pushliteral(L, "__index"); // push key.	
	pushRef(pMetatable);
	stack::pop_value_to_table(L, -3); // sets metatable.__index = metatable
	stack::pop(L); // pop metatable from stack.



	// Set metatable for this script object.
	pushRef(); // -2
	pushRef(pMetatable); // -1
	stack::setmetatable(L, -2);
	stack::pop(L); // pop table
}

void* XScriptTable::getThis(void)
{
	void* pPtr = nullptr;

	if (stack::get_type(L, 1) == Type::Table)
	{
		// index "__this" member.
		stack::pushliteral(L, "__this");
		stack::push_table_value_raw(L, 1);
		if (stack::get_type(L) == Type::Pointer) {
			pPtr = const_cast<void*>(stack::as_pointer(L));
		}

		stack::pop(L);
	}

	return pPtr;
}

// Iteration.
IScriptTable::Iterator XScriptTable::begin(void)
{
	Iterator iter;
	iter.internal = stack::top(L) + 1;

	pushRef();
	lua_pushnil(L);
	return iter;
}

bool XScriptTable::next(Iterator& iter)
{
	if (!iter.internal) {
		return false;
	}

	int nTop = iter.internal - 1;

	//leave only the index into the stack
	while ((stack::top(L) - (nTop + 1))>1)
	{
		stack::pop(L);
	}

	bool res = lua_next(L, nTop + 1) != 0;
	if (res)
	{
		iter.value.clear();
		res = pScriptSystem_->popAny(iter.value);
		// Get current key.
		pScriptSystem_->toAny(iter.key, -1);
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
		end(iter);
		iter.internal = 0;
	}

	return res;
}

void XScriptTable::end(const Iterator& iter)
{
	if (iter.internal)
	{
		stack::settop(L, iter.internal - 1);
	}
}

bool XScriptTable::clone(IScriptTable* pSrcTable, bool bDeepCopy, bool bCopyByReference)
{
	X_LUA_CHECK_STACK(L);

	int top = stack::top(L);

	pushRef(pSrcTable);
	pushRef();

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

	stack::settop(L, top); // Restore stack.

	return true;
}

void XScriptTable::dump(IScriptTableDumpSink* p)
{
	if (!p) {
		return;
	}

	X_LUA_CHECK_STACK(L);
	int top = stack::top(L);

	pushRef();

	int trgTable = top + 1;

	stack::pushnil(L);  // first key
	int reftop = stack::top(L);
	while (lua_next(L, trgTable) != 0)
	{
		// `key' is at index -2 and `value' at index -1
		if (stack::get_type(L, -2) == Type::String)
		{
			const char* pName = stack::as_string(L, -2); // again index
			p->onElementFound(pName, stack::get_type(L));
		}
		else
		{
			int idx = stack::as_int(L, -2); // again index
			p->onElementFound(idx, stack::get_type(L)); 
		}

		stack::settop(L, reftop); // pop value, leave index.
	}
	stack::pop(L); // pop table ref
}

bool XScriptTable::addFunction(const ScriptFunctionDesc& fd)
{
	X_LUA_CHECK_STACK(L);

	// Make function signature.
	core::StackString<256> funcSig;
	funcSig.appendFmt("%s.%s(%s)", fd.pGlobalName, fd.pFunctionName, fd.pFunctionParams);

	pushRef();
	stack::push(L, fd.pFunctionName);

	int8_t paramIdOffset = safe_static_cast<int8, int>(fd.paramIdOffset);
	if (fd.function)
	{
		const size_t dataSize = CFunctionData::requiredSize(funcSig.length()); 

		CFunctionData* pFuncData = reinterpret_cast<CFunctionData*>(state::newuserdata(L, dataSize));
		pFuncData->pFunction = fd.function;
		pFuncData->paramIdOffset = paramIdOffset;
		std::memcpy(pFuncData->funcName, funcSig.c_str(), funcSig.length() + 1);

		stack::push(L, s_CFunction, 1);
	}
	else
	{
		const size_t dataSize = UserDataFunctionData::requiredSize(funcSig.length(), fd.userDataSize);

		UserDataFunctionData* pFuncData = reinterpret_cast<UserDataFunctionData*>(state::newuserdata(L, dataSize));
		pFuncData->pFunction = fd.pUserDataFunc;
		pFuncData->nameSize = safe_static_cast<uint8_t>(funcSig.length());
		pFuncData->paramIdOffset = paramIdOffset;
		pFuncData->dataSize = safe_static_cast<int16_t>(fd.userDataSize);
		std::memcpy(pFuncData->funcName, funcSig.c_str(), funcSig.length() + 1);

		stack::push(L, s_CUserDataFunction, 1);
	}

	// pop the function pointer into the table with key of the function name.
	stack::pop_value_to_table(L, -3);
	stack::pop(L); // pop table.
	return true;
}


// --------------------------------------------------------------------------


void XScriptTable::createNew(void)
{
	state::new_table(L);
	attach();
}


void XScriptTable::deleteThis(void)
{
	if (luaRef_ == lua::Ref::Deleted)
	{
		X_FATAL("Script", "attempt to Release already released script table.");
	}

//	s_allTables_.erase(this);

	if (luaRef_ != lua::Ref::Nil) {
		state::remove_ref(L, luaRef_);
	}

	luaRef_ = lua::Ref::Deleted;

	pScriptSystem_->freeTable(this);
}

int32_t XScriptTable::getRef(void) const
{
	return luaRef_;
}

void XScriptTable::attach(void)
{
	if (luaRef_ != lua::Ref::Nil) {
		state::remove_ref(L, luaRef_);
	}

	luaRef_ = stack::pop_to_ref(L);

//	s_allTables_.insert(this);
}

void XScriptTable::attach(IScriptTable* pSO)
{
	pushRef(X_ASSERT_NOT_NULL(pSO));
	attach();
}

X_INLINE void XScriptTable::pushRef(void)
{
	X_ASSERT(luaRef_ != lua::Ref::Deleted && luaRef_ != lua::Ref::Nil, "Invalid table ref")(luaRef_);

#if X_DEBUG
	// extra logic in debug only, otherwise bloat all the functions.
	if (luaRef_ != lua::Ref::Deleted && luaRef_ != lua::Ref::Nil) {
		stack::push_ref(L, luaRef_);
	}
	else
	{
		stack::pushnil(L);
		if (luaRef_ == lua::Ref::Deleted)
		{
			X_FATAL("Script", "Access to deleted script object: %p", this);
		}
		else
		{
			X_ERROR("Script", "Pushing Nil table reference");
		}
	}
#else
	stack::push_ref(L, luaRef_);
#endif // !X_DEBUG
}

// Push reference to specified script table to the stack.
void XScriptTable::pushRef(IScriptTable* pObj)
{
	int32_t ref = static_cast<XScriptTable*>(pObj)->luaRef_;
	X_ASSERT(ref != lua::Ref::Deleted, "Access to deleted script object")(ref);
	
	stack::push_ref(L, ref);
}


X_DISABLE_WARNING(4458) // declaration of 'L' hides class member


int XScriptTable::s_CFunction(lua_State* L)
{
	auto* pFuncData = reinterpret_cast<const CFunctionData*>(stack::as_userdata(L, lua_upvalueindex(1)));

	XFunctionHandler fh(pScriptSystem_, L, pFuncData->funcName, pFuncData->paramIdOffset);
	
	int32_t ret = pFuncData->pFunction.Invoke(&fh);
	return ret;
}

int XScriptTable::s_CUserDataFunction(lua_State* L)
{
	auto* pFuncData = reinterpret_cast<const UserDataFunctionData*>(stack::as_userdata(L, lua_upvalueindex(1)));

	XFunctionHandler fh(pScriptSystem_, L, pFuncData->funcName, pFuncData->paramIdOffset);
	auto* pBuffer = pFuncData->getBuffer();
	
	int32_t ret = (*pFuncData->pFunction)(&fh, pBuffer, pFuncData->dataSize);
	return ret;
}

X_ENABLE_WARNING(4458)

// --------------------------------------------------------------------------


void XScriptTable::CloneTable_r(int srcTable, int trgTable)
{
	X_LUA_CHECK_STACK(L);

	int top = stack::top(L);

	stack::pushnil(L);  // first key
	while (lua_next(L, srcTable) != 0)
	{
		if (lua_type(L, -1) == LUA_TTABLE)
		{
			int srct = stack::top(L);

			stack::push_copy(L, -2); // Push again index.
			state::new_table(L);      // Make value.
			int trgt = stack::top(L);
			CloneTable_r(srct, trgt);
			lua_rawset(L, trgTable); // Set new table to trgtable.
		}
		else
		{
			// `key' is at index -2 and `value' at index -1
			stack::push_copy(L, -2); // Push again index.
			stack::push_copy(L, -2); // Push value.
			lua_rawset(L, trgTable);
		}
		stack::pop(L); // pop value, leave index.
	}
	lua_settop(L, top); // Restore stack.
}

void XScriptTable::ReferenceTable_r(int srcTable, int trgTable)
{
	X_LUA_CHECK_STACK(L);

	int top = stack::top(L);

	state::new_table(L);									// push new meta table
	stack::pushliteral(L, "__index");					// push __index
	lua_pushvalue(L, srcTable);							// push src table
	lua_rawset(L, -3);									// meta.__index==src table
	lua_setmetatable(L, trgTable);						// set meta table

	stack::pushnil(L);  // first key
	while (lua_next(L, srcTable) != 0)
	{
		if (lua_type(L, -1) == LUA_TTABLE)
		{
			int srct = stack::top(L);
			lua_pushvalue(L, -2); // Push again index.
			state::new_table(L);      // Make value.
			int trgt = stack::top(L);
			ReferenceTable_r(srct, trgt);
			lua_rawset(L, trgTable); // Set new table to trgtable.
		}
		stack::pop(L); // pop value, leave index.
	}
	lua_settop(L, top); // Restore stack.
}

void XScriptTable::CloneTable(int srcTable, int trgTable)
{
	X_LUA_CHECK_STACK(L);

	int top = stack::top(L);
	stack::pushnil(L);  // first key
	while (lua_next(L, srcTable) != 0)
	{
		// `key' is at index -2 and `value' at index -1
		lua_pushvalue(L, -2); // Push again index.
		lua_pushvalue(L, -2); // Push value.
		lua_rawset(L, trgTable);
		stack::pop(L); // pop value, leave index.
	}
	lua_settop(L, top); // Restore stack.
}


X_NAMESPACE_END

