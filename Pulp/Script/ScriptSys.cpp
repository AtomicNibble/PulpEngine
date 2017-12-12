#include "stdafx.h"
#include "ScriptSys.h"
#include "ScriptTable.h"

#include <ICore.h>
#include <IFileSys.h>
#include <IRender.h>

#include <Memory\VirtualMem.h>

X_NAMESPACE_BEGIN(script)

using namespace lua;

namespace
{

	static const size_t POOL_ALLOC_MAX = 1024 * 4; // script tables
	static const size_t POOL_ALLOCATION_SIZE = sizeof(XScriptTable);
	static const size_t POOL_ALLOCATION_ALIGN = X_ALIGN_OF(XScriptTable);

} // !namespace


XScriptSys::XScriptSys(core::MemoryArenaBase* arena) :
	L(nullptr),
	initialised_(false),
	arena_(arena),
	poolAllocator_(PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE) * POOL_ALLOC_MAX,
		core::VirtualMem::GetPageSize() * 4,
		0,
		PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE),
		PoolArena::getMemoryAlignmentRequirement(POOL_ALLOCATION_ALIGN),
		PoolArena::getMemoryOffsetRequirement()
	),
	poolArena_(&poolAllocator_, "TablePool")
{
	arena->addChildArena(&poolArena_);

	errrorHandler_ = Ref::Nil;
}

XScriptSys::~XScriptSys()
{
}

void XScriptSys::registerVars(void)
{
	vars_.registerVars();
}

void XScriptSys::registerCmds(void)
{
	// add the shieeeeeeet.
//	ADD_COMMAND("listScipts", ListScriptCmd, core::VarFlag::SYSTEM, "List loaded script files");
//
//	ADD_COMMAND("scriptLoad", LoadScriptCmd, core::VarFlag::SYSTEM, "Load and run a script file");
//	ADD_COMMAND("scriptList", ListScriptCmd, core::VarFlag::SYSTEM, "List loaded script files");
//	ADD_COMMAND("scriptReload", ReloadScriptCmd, core::VarFlag::SYSTEM, "Reload a given script <filename>");
//	ADD_COMMAND("scriptDumpState", LuaDumpState, core::VarFlag::SYSTEM, "Dump the lua state to a file <filename>");

}


bool XScriptSys::init(void)
{
	X_LOG0("Script", "Starting script system");
	X_ASSERT(initialised_ == false, "Already init")(initialised_);

	L = luaL_newstate();

	lua::StateView view(L);
	view.setPanic(myLuaPanic);
	view.openLibs(
		lua::libs(
			lua::lib::Base |
			lua::lib::Package |
			lua::lib::Os |
			lua::lib::Io |
			lua::lib::Ffi |
			lua::lib::Jit
		)	
	);

	stack::push(L, myErrorHandler);
	errrorHandler_ = stack::pop_to_ref(L);

	XScriptTable::L = L;
	XScriptTable::pScriptSystem_ = this;


	binds_.Init(this, gEnv->pCore);

	// hotreload
	gEnv->pHotReload->addfileType(this, X_SCRIPT_FILE_EXTENSION);

	initialised_ = true;

#if 0

	binds_.Init(this, gEnv->pCore);


#endif

	// new stuff
	{
		lua::State state(g_ScriptArena);

		state.openLibs(lua::libs(
				lua::lib::Base |
				lua::lib::Package |
				lua::lib::Os |
				lua::lib::Io |
				lua::lib::Ffi |
				lua::lib::Jit
			)
		);


	}


	return true;
}

void XScriptSys::shutDown(void)
{
	if (!initialised_) {
		return;
	}

	X_LOG0("ScriptSys", "Shutting Down");

	gEnv->pHotReload->addfileType(nullptr, X_SCRIPT_FILE_EXTENSION);

	// must be done before lua closes.
	binds_.Shutdown();

	for (std::set<XScriptTable*>::iterator it = XScriptTable::s_allTables_.begin();
		it != XScriptTable::s_allTables_.end(); )
	{
		XScriptTable* pTable = *it;

		++it;

		if (pTable) {
			pTable->release();
		}
	}

	XScriptTable::s_allTables_.clear();

	if (L != nullptr) {
		lua_close(L);
		L = nullptr;
	}


	initialised_ = false;
}

void XScriptSys::release(void)
{
	X_DELETE(this, g_ScriptArena);
}


void XScriptSys::Update(void)
{
	X_PROFILE_BEGIN("ScriptUpdate", core::profiler::SubSys::SCRIPT);

//	float time = 0.f; //  gEnv->pTimer->GetCurrTime();

//	setGlobalValue("_time", time);

	{
		X_PROFILE_BEGIN("Lua GC", core::profiler::SubSys::SCRIPT);

		// LUA_GCSTEP: performs an incremental step of garbage collection.
		// The step "size" is controlled by data(larger values mean more steps) 
		// in a non - specified way.If you want to control the step size you must 
		// experimentally tune the value of data.The function returns 1 if the step 
		// finished a garbage - collection cycle.

		state::gc_step(L, vars_.gcStepSize());
	}

}


bool XScriptSys::runScriptInSandbox(const char* pBegin, const char* pEnd)
{
//	lua::State state(g_ScriptArena);
	lua::StateView state(L);

#if 0
	state.openLibs(
		lua::libs(
			lua::lib::Base |
			lua::lib::Package |
			lua::lib::Os |
			lua::lib::Io |
			lua::lib::Ffi |
			lua::lib::Jit
		)
	);
#endif

	// can we just push the function?
	stack::push(state, myErrorHandler);
	int32_t errorHandlerRef = stack::pop_to_ref(state);

	if (!state.loadScript(pBegin, pEnd, "Sandbox")) {
		return false;
	}

	// the compiled code is on stack as a function.
	// we can call it.

	int base = stack::top(state);
	stack::push_ref(state, errorHandlerRef);
	stack::move_top_to(state, base); // move the error hander before the script.

	auto status = stack::pcall(state, 0, LUA_MULTRET, base);

	stack::remove(state, base);

	if (status != CallResult::Ok)
	{

	}

	return true;
}


ScriptFunctionHandle XScriptSys::getFunctionPtr(const char* pFuncName)
{
	X_LUA_CHECK_STACK(L);

	stack::push_global(L, pFuncName);
	if (!stack::isfunction(L))
	{
		stack::pop(L);
		return INVALID_HANLDE;
	}

	return refToScriptHandle(stack::pop_to_ref(L));
}

ScriptFunctionHandle XScriptSys::getFunctionPtr(const char* pTableName, const char* pFuncName)
{
	X_LUA_CHECK_STACK(L);

	if (!stack::push_global_table_value(L, pTableName, pFuncName)) {
		return INVALID_HANLDE;
	}

	if (!stack::isfunction(L)) {
		stack::pop(L);
		return INVALID_HANLDE;
	}

	return refToScriptHandle(stack::pop_to_ref(L));
}

bool XScriptSys::compareFuncRef(ScriptFunctionHandle f1, ScriptFunctionHandle f2)
{
	X_LUA_CHECK_STACK(L);
	// same ref?
	if (f1 == f2) {
		return true;
	}

	// load the pointer values and compare.
	stack::push_ref(L, f1);
	X_ASSERT(stack::get_type(L) == LUA_TFUNCTION, "type should be function")(stack::get_type(L));
	const void* f1p = stack::as_pointer(L);
	stack::pop(L);

	stack::push_ref(L, f1);
	X_ASSERT(stack::get_type(L) == LUA_TFUNCTION, "type should be function")(stack::get_type(L));
	const void* f2p = stack::as_pointer(L);
	stack::pop(L);

	return f1p == f2p;
}


void XScriptSys::releaseFunc(ScriptFunctionHandle f)
{
	X_LUA_CHECK_STACK(L);

	if (f != INVALID_HANLDE)
	{
#ifdef _DEBUG
		stack::push_ref(L, f);
		X_ASSERT(stack::get_type(L) == LUA_TFUNCTION, "type should be function")(stack::get_type(L));
		stack::pop(L);
#endif

		state::remove_ref(L, f);
	}
}

IScriptTable* XScriptSys::createTable(bool bEmpty)
{
	XScriptTable* pObj = allocTable();
	if (!bEmpty) {
		pObj->createNew();
	}
	return pObj;
}


void XScriptSys::setGlobalValue(const char* pKey, const ScriptValue& any)
{
	X_LUA_CHECK_STACK(L);

	pushAny(any);
	
	stack::pop_to_global(L, pKey);
}


bool XScriptSys::getGlobalValue(const char* pKey, ScriptValue& any)
{
	X_LUA_CHECK_STACK(L);

	const char* pSep = core::strUtil::Find(pKey, '.');
	if (pSep)
	{
		ScriptValue globalAny;
		core::StackString<256> key1(pKey, pSep);

		getGlobalValue(key1.c_str(), globalAny);
		if (globalAny.getType() == Type::Table)
		{
			return getRecursiveAny(globalAny.pTable_, key1, any);
		}

		return false;
	}

	stack::push_global(L, pKey);
	if (!popAny(any)) {
		return false;
	}

	return true;
}

IScriptTable* XScriptSys::createUserData(void* pPtr, size_t size)
{
	X_LUA_CHECK_STACK(L);

	state::newuserdata(L, pPtr, size);

	XScriptTable* pNewTbl = allocTable(); 
	pNewTbl->attach();

	return pNewTbl;
}

void XScriptSys::onScriptError(const char* pFmt, ...)
{
	core::StackString<2048> error;

	X_VALIST_START(pFmt);
	error.appendFmt(pFmt, args);
	X_VALIST_END;

	X_WARNING("Script", error.c_str());
}

XScriptTable* XScriptSys::allocTable(void)
{
	return X_NEW(XScriptTable, &poolArena_, "ScriptTable");
}

void XScriptSys::freeTable(XScriptTable* pTable)
{
	X_DELETE(pTable, &poolArena_);
}

bool XScriptSys::getRecursiveAny(IScriptTable* pTable, const core::StackString<256>& key, ScriptValue& any)
{
	core::StackString<256> key1;
	core::StackString<256> key2;

	const char* pSep = key.find('.');
	if (pSep)
	{
		key1.set(key.begin(), pSep);
		key2.set(pSep + 1, key.end());
	}
	else
	{
		key1 = key;
	}

	ScriptValue localAny;
	if (!pTable->getValueAny(key1.c_str(), localAny)) {
		return false;
	}

	if (localAny.getType() == Type::Function && nullptr == pSep)
	{
		any = localAny;
		return true;
	}
	else if (localAny.getType() == Type::Table && nullptr != pSep)
	{
		return getRecursiveAny(localAny.pTable_, key2, any);
	}

	return false;
}


bool XScriptSys::popAny(ScriptValue &var)
{
	bool res = toAny(var, -1);
	stack::pop(L);
	return res;
}

void XScriptSys::pushAny(const ScriptValue& var)
{
	switch (var.getType())
	{
		case Type::Nil:
			stack::pushnil(L);
			break;
		case Type::Boolean:
			stack::push(L, var.bool_);
			break;
		case Type::Handle:
			lua_pushlightuserdata(L, const_cast<void*>(var.pPtr_));
			break;
		case Type::Number:
			stack::push(L, var.number_);
			break;
		case Type::String:
			stack::push(L, var.str_.pStr, var.str_.len);
			break;
		case Type::Table:
			if (var.pTable_) {
				pushTable(var.pTable_);
			}
			else {
				stack::pushnil(L);
			}
			break;
		case Type::Function:
			stack::push_ref(L, var.pFunction_);
			X_ASSERT(stack::get_type(L) == LUA_TFUNCTION, "type should be function")(stack::get_type(L));
			break;
		case Type::Vector:
			pushVec3(Vec3f(var.vec3_.x, var.vec3_.y, var.vec3_.z));
			break;

		default:
			X_NO_SWITCH_DEFAULT_ASSERT;
	}
}

void XScriptSys::pushVec3(const Vec3f& vec)
{
	state::new_table(L);

	stack::pushliteral(L, "x");
	stack::push(L, vec.x);
	state::set_table_value(L);

	stack::pushliteral(L, "y");
	stack::push(L, vec.y);
	state::set_table_value(L);

	stack::pushliteral(L, "z");
	stack::push(L, vec.z);
	state::set_table_value(L);
}

X_INLINE void XScriptSys::pushTable(IScriptTable *pTable)
{
	static_cast<XScriptTable*>(pTable)->pushRef();
}

bool XScriptSys::toVec3(Vec3f& vec, int tableIndex)
{
	X_LUA_CHECK_STACK(L);

	if (tableIndex < 0) {
		tableIndex = stack::num(L) + tableIndex + 1;
	}

	if (stack::get_type(L, tableIndex) != LUA_TTABLE) {
		return false;
	}

	lua_Number x, y, z;
	stack::push_table_value(L, tableIndex, "x");

	if (stack::isnumber(L))
	{
		x = stack::as_number(L);
		stack::pop(L);

		stack::push_table_value(L, tableIndex, "y");
		if (!stack::isnumber(L)) {
			stack::pop(L);
			return false;
		}

		y = stack::as_number(L);
		stack::pop(L);

		stack::push_table_value(L, tableIndex, "z");
		if (!stack::isnumber(L)) {
			stack::pop(L);
			return false;
		}

		z = stack::as_number(L);
		stack::pop(L);

		vec.x = safe_static_cast<float>(x);
		vec.y = safe_static_cast<float>(y);
		vec.z = safe_static_cast<float>(z);
		return true;
	}


	stack::pop(L);

	// Try an indexed table.

	stack::push_table_value(L, tableIndex, 1);
	if (!stack::isnumber(L)) {
		stack::pop(L);
		return false;
	}
	x = stack::as_number(L);

	stack::push_table_value(L, tableIndex, 2);
	if (!stack::isnumber(L)) {
		stack::pop(L);
		return false;
	}
	y = stack::as_number(L);

	stack::push_table_value(L, tableIndex, 3);
	if (!stack::isnumber(L)) {
		stack::pop(L);
		return false;
	}
	z = stack::as_number(L);
	stack::pop(L);

	vec.x = safe_static_cast<float>(x);
	vec.y = safe_static_cast<float>(y);
	vec.z = safe_static_cast<float>(z);
	return true;
}

bool XScriptSys::toAny(ScriptValue& var, int index)
{
	return toAny(L, var, index);
}

bool XScriptSys::toAny(lua_State* L, ScriptValue& var, int index)
{
	if (stack::is_empty(L)) {
		return false;
	}

	X_LUA_CHECK_STACK(L);

	int luaType = stack::get_type(L, index);

	if (var.getType() != Type::None && !isTypeCompatible(var.getType(), luaType)) {
		return false;
	}

	switch (luaType)
	{
		case LUA_TNIL:
			var.type_ = Type::Nil;
			break;
		case LUA_TBOOLEAN:
			var.bool_ = stack::as_bool(L, index) != 0;
			var.type_ = Type::Boolean;
			break;
		case LUA_TLIGHTUSERDATA:
			var.pPtr_ = stack::as_pointer(L, index);
			var.type_ = Type::Handle;
			break;
		case LUA_TNUMBER:
			var.number_ = static_cast<float>(stack::as_number(L, index));
			var.type_ = Type::Number;
			break;
		case LUA_TSTRING:
		{
			size_t len = 0;
			var.str_.pStr = stack::as_string(L, index, &len);
			var.str_.len = safe_static_cast<int32_t>(len);
			var.type_ = Type::String;
		}
		break;
		case LUA_TTABLE:
		case LUA_TUSERDATA:
			if (!var.pTable_)
			{
				var.pTable_ = static_cast<XScriptSys*>(gEnv->pScriptSys)->allocTable();
				var.pTable_->addRef();
			}
			stack::push_copy(L, index);
			static_cast<XScriptTable*>(var.pTable_)->attach();
			var.type_ = Type::Table;
			break;
		case LUA_TFUNCTION:
		{
			var.type_ = Type::Function;
			// Make reference to function.
			lua_pushvalue(L, index);
			var.pFunction_ = refToScriptHandle(luaL_ref(L, 1));
		}
		break;
		case LUA_TTHREAD:
		default:
			return false;
	}

	return true;
}

// ~IScriptSys


#if 0

bool XScriptSys::ExecuteBuffer(const char* sBuffer, size_t nSize, const char* Description)
{
	int status;

	status = luaL_loadbuffer(L, sBuffer, nSize, Description);

	if (status == 0)
	{
		int base = lua_gettop(L);  // function index.

		lua_getref(L, g_pErrorHandlerFunc);
		lua_insert(L, base);

		status = lua_pcall(L, 0, LUA_MULTRET, base);  // call main

		lua_remove(L, base);
	}

	if (status != 0)
	{
		const char* Err = lua_tostring(L, -1);
		if (Description && strlen(Description) != 0)
		{
			X_ERROR("Script", "Failed to execute file %s: %s", Description, Err);
		}
		else
		{
			X_WARNING("Script", "Error executing lua %s", Err);
		}

		lua_pop(L, 1);
		return false;
	}
	return true;
}
#endif 

// IXHotReload

void XScriptSys::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys);
#if 0
	ScriptFileList::iterator it = fileList_.find(X_CONST_STRING(name));

	if (it != fileList_.end())
	{
		ReloadScript(name, true);
	}

	return true;
#else 
	X_UNUSED(name);
#endif
}

// ~IXHotReload


X_NAMESPACE_END