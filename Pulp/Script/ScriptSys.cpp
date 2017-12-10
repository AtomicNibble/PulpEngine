#include "stdafx.h"
#include "ScriptSys.h"
#include "ScriptTable.h"

#include <ICore.h>
#include <ITimer.h>
#include <IConsole.h>
#include <IFileSys.h>
#include <IRender.h>

#include "wrapper\state_view.h"



core::MallocFreeAllocator g_LuaAllocator;

extern "C"
{
	void x_openlibs(lua_State* L);

	static const char* g_StackLevel[] = 
	{
		"",
		"  ",
		"    ",
		"      ",
		"        ",
		"          ",
		"            ",
		"              ",
	};

	static const int max_stack_lvl = sizeof(g_StackLevel) / sizeof(g_StackLevel[0]);

#if 0
	void DumpCallStack(lua_State *L)
	{
		lua_Debug ar;
		core::zero_object(ar);

		X_LOG_BULLET;

		int level = 0;
		while (lua_getstack(L, level++, &ar))
		{
			if (level >= max_stack_lvl) {
				level = max_stack_lvl - 1;
			}

			int res = lua_getinfo(L, "lnS", &ar);
			X_UNUSED(res); // TODO
			if (ar.name)
			{
				X_LOG0("ScriptError", "%s> %s, (%s: %i)",
					g_StackLevel[level], ar.name, ar.short_src, ar.currentline);
			}
			else
			{
				X_LOG0("ScriptError", "%s> (null) (%s: %i)", 
					g_StackLevel[level], ar.short_src, ar.currentline);
			}
		}
	}

	static int custom_lua_panic(lua_State *L)
	{
		DumpCallStack(L);
		return 0;
	}

	// behaves like realloc.
	void* lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
	{
		X_UNUSED(ud);
		X_UNUSED(osize);

		if (nsize == 0) {
			if (ptr)
				g_LuaAllocator.free(ptr);
			return NULL;
		}

		if (ptr) 
		{
			size_t size = g_LuaAllocator.getSize(ptr);

			if (size >= nsize) // do nothing, cba to shrink :D
				return ptr;

			void* pNew = g_LuaAllocator.allocate(nsize, 4, 0);

			// copy it
			memcpy(pNew, ptr, size);

			// free old
			g_LuaAllocator.free(ptr);

			return pNew;
		}

		return g_LuaAllocator.allocate(nsize, 4,0);
	}
#endif

} // Extern C !END!

X_NAMESPACE_BEGIN(script)

using namespace lua;


namespace
{

#if 0
	void LoadScriptCmd(core::IConsoleCmdArgs* pArgs)
	{
		size_t num = pArgs->GetArgCount();
		if (num < 2)
		{
			X_WARNING("Script", "script_load <filename>");
			return;
		}

		for (size_t i = 1; i<num; i++)
		{
			const char* Filename = pArgs->GetArg(i);

			if (gEnv->pScriptSys->ExecuteFile(Filename, true, true))
			{
				X_LOG0("Script", "Loaded: %s", Filename);
			}
		}
	}

	void ListScriptCmd(core::IConsoleCmdArgs* pArgs)
	{
		X_UNUSED(pArgs);
		gEnv->pScriptSys->ListLoadedScripts();
	}

	void ReloadScriptCmd(core::IConsoleCmdArgs* pArgs)
	{
		size_t num = pArgs->GetArgCount();
		if (num < 2)
		{
			X_WARNING("Script", "script_reload <filename>");
			return;
		}

		for (size_t i = 1; i<num; i++)
		{
			const char* Filename = pArgs->GetArg(i);

			if (gEnv->pScriptSys->ReloadScript(Filename, true))
			{
				X_LOG0("Script", "ReLoaded: %s", Filename);
			}
		}
	}

	void LuaDumpState(core::IConsoleCmdArgs* pArgs)
	{
		const char* pfileName = "LuaState.txt";
		if (pArgs->GetArgCount() > 0)
		{
			pfileName =	pArgs->GetArg(0);
		}

		static_cast<XScriptSys*>(gEnv->pScriptSys)->DumpStateToFile(pfileName);
	}
#endif

	int g_pErrorHandlerFunc;

	inline XScriptTable* AllocTable()
	{
		return X_NEW(XScriptTable,g_ScriptArena,"ScriptTable");
	}


} // !namespace


XScriptSys::XScriptSys() :
	initialised_(false)
{

}

XScriptSys::~XScriptSys()
{
}

void XScriptSys::registerVars(void)
{


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

	pFileSys_ = gEnv->pFileSys;

#if 0
	// lj_state_newstate(mem_alloc, mem_create());
	// L = lua_newstate(custom_lua_alloc, NULL);
	L = luaL_newstate();

	X_ASSERT_NOT_NULL(pFileSys_);
	X_ASSERT_NOT_NULL(L);

	lua_atpanic(L, &custom_lua_panic);

//	luaL_openlibs(L);
	x_openlibs(L);


	XScriptTable::L = L;
	XScriptTable::pScriptSystem_ = this;

	binds_.Init(this, gEnv->pCore);

	setGlobalValue("_time", 0);

	lua_pushcfunction(L, XScriptSys::ErrorHandler);
	g_pErrorHandlerFunc = lua_ref(L, 1);

	// hotreload
	gEnv->pHotReload->addfileType(this, X_SCRIPT_FILE_EXTENSION);

	initialised_ = true;

#endif

	// new stuff
	{
		lua::State state(g_ScriptArena);

		state.openLibs(lua::libs(
				lua::lib::base |
				lua::lib::package |
				lua::lib::os |
				lua::lib::io |
				lua::lib::ffi |
				lua::lib::jit
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

//		lua_gc(L, LUA_GCSTEP, 2);
	}

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

	return static_cast<ScriptFunctionHandle>(stack::pop_to_ref(L));
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

	return static_cast<ScriptFunctionHandle>(stack::pop_to_ref(L));
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
	XScriptTable* pObj = AllocTable();
	if (!bEmpty)
	{
		pObj->CreateNew();
	}
	return pObj;
}


void XScriptSys::setGlobalValue(const char* pKey, const ScriptValue& any)
{
	X_LUA_CHECK_STACK(L);

	PushAny(any);
	
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
		if (globalAny.getType() == Type::TABLE)
		{
			return GetRecursiveAny(globalAny.pTable_, key1, any);
		}

		return false;
	}

	stack::push_global(L, pKey);
	if (!PopAny(any)) {
		return false;
	}

	return true;
}

IScriptTable* XScriptSys::createUserData(void* pPtr, size_t size)
{
	X_LUA_CHECK_STACK(L);

	void* pUserPtr = lua_newuserdata(L, size);
	std::memcpy(pUserPtr, pPtr, size);

	XScriptTable* pNewTbl = X_NEW(XScriptTable, g_ScriptArena, "ScriptTable");
	pNewTbl->Attach();

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



bool XScriptSys::GetRecursiveAny(IScriptTable* pTable, const core::StackString<256>& key, ScriptValue& any)
{
	core::StackString<256> key1;
	core::StackString<256> key2;

	const char* sep = key.find('.');
	if (sep)
	{
		key1.set(key.begin(), sep);
		key2.set(sep + 1, key.end());
	}
	else
	{
		key1 = key;
	}

	ScriptValue localAny;
	if (!pTable->GetValueAny(key1.c_str(), localAny)) {
		return false;
	}

	if (localAny.getType() == Type::FUNCTION && nullptr == sep)
	{
		any = localAny;
		return true;
	}
	else if (localAny.getType() == Type::TABLE && nullptr != sep)
	{
		return GetRecursiveAny(localAny.pTable_, key2, any);
	}

	return false;
}



void XScriptSys::PushAny(const ScriptValue& var)
{
	switch (var.getType())
	{
		case Type::NIL:
			stack::pushnil(L);
			break;
		case Type::BOOLEAN:
			stack::push(L, var.bool_);
			break;
		case Type::HANDLE:
			lua_pushlightuserdata(L, const_cast<void*>(var.pPtr_));
			break;
		case Type::NUMBER:
			stack::push(L, var.number_);
			break;
		case Type::STRING:
			stack::push(L, var.str_.pStr, var.str_.len);
			break;
		case Type::TABLE:
			if (var.pTable_) {
				PushTable(var.pTable_);
			}
			else {
				stack::pushnil(L);
			}
			break;
		case Type::FUNCTION:
			stack::push_ref(L, var.pFunction_);
			X_ASSERT(stack::get_type(L) == LUA_TFUNCTION, "type should be function")(stack::get_type(L));
			break;
		case Type::VECTOR:
			PushVec3(Vec3f(var.vec3_.x, var.vec3_.y, var.vec3_.z));
			break;

		default:
			X_NO_SWITCH_DEFAULT_ASSERT;
	}
}

bool XScriptSys::PopAny(ScriptValue &var)
{
	bool res = ToAny(var, -1);
	stack::pop(L);
	return res;
}


bool isTypeCompatible(Type::Enum type, int32_t luaType)
{
	switch (luaType)
	{
		case LUA_TNIL:
			if (type != Type::NIL) {
				return false;
			}
			break;
		case LUA_TBOOLEAN:
			if (type != Type::BOOLEAN) {
				return false;
			}
			break;
		case LUA_TLIGHTUSERDATA:
			if (type != Type::HANDLE) {
				return false;
			}
			break;
		case LUA_TNUMBER:
			if (type != Type::NUMBER && type != Type::BOOLEAN) {
				return false;
			}
			break;
		case LUA_TSTRING:
			if (type != Type::STRING) {
				return false;
			}
			break;
		case LUA_TTABLE:
			if (type != Type::TABLE && type != Type::VECTOR) {
				return false;
			}
			break;
		case LUA_TUSERDATA:
			if (type == Type::TABLE) {
				return false;
			}
			break;
		case LUA_TFUNCTION:
			if (type == Type::FUNCTION) {
				return false;
			}
			break;
		case LUA_TTHREAD:
			break;
	}

	return true;
}

bool XScriptSys::ToAny(ScriptValue& var, int index)
{
	if (!lua_gettop(L)) {
		return false;
	}

	X_LUA_CHECK_STACK(L);

	int luaType = stack::get_type(L, index);

	if (var.getType() != Type::NONE && !isTypeCompatible(var.getType(), luaType))
	{
		return false;
	}

	switch (luaType)
	{
		case LUA_TNIL:
			var.type_ = Type::NIL;
			break;
		case LUA_TBOOLEAN:
			var.bool_ = lua_toboolean(L, index) != 0;
			var.type_ = Type::BOOLEAN;
			break;
		case LUA_TLIGHTUSERDATA:
			var.pPtr_ = lua_topointer(L, index);
			var.type_ = Type::HANDLE;
			break;
		case LUA_TNUMBER:
			var.number_ = (float)lua_tonumber(L, index);
			var.type_ = Type::NUMBER;
			break;
		case LUA_TSTRING:
		{
			size_t len = 0;
			var.str_.pStr = lua_tolstring(L, index, &len);
			var.str_.len = safe_static_cast<int32_t>(len);
			var.type_ = Type::STRING;
		}
		break;
		case LUA_TTABLE:
		case LUA_TUSERDATA:
			if (!var.pTable_)
			{
				var.pTable_ = AllocTable();
				var.pTable_->addRef();
			}
			lua_pushvalue(L, index);
			AttachTable(var.pTable_);
			var.type_ = Type::TABLE;
			break;
		case LUA_TFUNCTION:
		{
			var.type_ = Type::FUNCTION;
			// Make reference to function.
			lua_pushvalue(L, index);
			X_ASSERT_NOT_IMPLEMENTED();
			//	var.pFunction_ = reinterpret_cast<ScriptFunctionHandle>(luaL_ref(L, 1));
		}
		break;
		case LUA_TTHREAD:
		default:
			return false;
	}

	return true;
}

#if 0

bool XScriptSys::ExecuteFile(const char* FileName, bool silent, bool forceReload)
{
	core::Path<char> path(FileName);


	if (path.length() > 0)
	{
		path.setExtension(X_SCRIPT_FILE_EXTENSION);
		path.replaceSeprators();

		ScriptFileList::iterator it = fileList_.find(X_CONST_STRING(path.c_str()));

		// already loaded and not requesting a reload?
		if (it != fileList_.end() && !forceReload)
			return true;

		// Always add the name so it will be reloaded.
		addFileName(path.c_str());

		if (ExecuteFile_Internal(path, silent))
		{
			X_LOG0_IF(!silent, "ScriptSys", "Loaded: \"%s\"", path.c_str());

			return true;
		}
		else if (it != fileList_.end())
		{
			// reload failed, remove it from the loaded list.
			// i use this list for files that have been loaded
			// at any point in time.
			// for reload checks.
			// removeFileName(path.c_str());
		}

	}

	return false;
}

bool XScriptSys::ExecuteFile_Internal(const core::Path<char>& path, bool silent)
{
	X_UNUSED(silent);
	bool res = false;

	core::XFileMem* file = pFileSys_->openFileMem(path.c_str(), core::fileMode::READ);

	if (file)
	{
		res = ExecuteBuffer(file->getBufferStart(), 
			safe_static_cast<size_t, uint64_t>(file->getSize()), path.c_str());

		pFileSys_->closeFileMem(file);
	}

	return res;
}

bool XScriptSys::UnLoadScript(const char* FileName)
{
	X_ASSERT_NOT_NULL(FileName);
	removeFileName(FileName);
	return true;
}

void XScriptSys::UnloadScripts()
{
	fileList_.clear();
}

bool XScriptSys::ReloadScript(const char* FileName, bool raiseError)
{
	return ExecuteFile(FileName, raiseError, true);
}

bool XScriptSys::ReloadScripts()
{
	ScriptFileList::iterator it = fileList_.begin();
	for (; it != fileList_.end(); ++it)
	{
		ReloadScript(it->c_str(), true);
	}

	return true;
}

void XScriptSys::ListLoadedScripts(void)
{
	ScriptFileList::iterator it = fileList_.begin();
	for (; it != fileList_.end(); ++it)
	{
		X_LOG0("Script", "\"%s\"", it->c_str());
	}
}

void XScriptSys::addFileName(const char* name)
{
	fileList_.insert(core::string(name));
}

void XScriptSys::removeFileName(const char* name)
{
	fileList_.erase(X_CONST_STRING(name));
}

void XScriptSys::SetGlobalAny(const char* Key, const ScriptValue& any)
{
	X_LUA_CHECK_STACK(L);
	PushAny(any);
	lua_setglobal(L, Key);
}


ScriptFunctionHandle XScriptSys::AddFuncRef(ScriptFunctionHandle f)
{
	// type cast': conversion from 'int' to 'Potato::script::ScriptFunctionHandle' of greater size
	X_DISABLE_WARNING(4312) 
	X_ASSERT_NOT_IMPLEMENTED();


	X_LUA_CHECK_STACK(L);

	if (f)
	{
		int ret;
		lua_getref(L, (int)(INT_PTR)f);
		X_ASSERT(lua_type(L, -1) == LUA_TFUNCTION, "type should be function")(lua_type(L, -1));
		ret = lua_ref(L, 1);
		if (ret != LUA_REFNIL)
		{
			return (ScriptFunctionHandle)ret;
		}
		
		X_ASSERT_UNREACHABLE();
	}
	return 0;

	X_ENABLE_WARNING(4312)
}

bool XScriptSys::CompareFuncRef(ScriptFunctionHandle f1, ScriptFunctionHandle f2)
{
	X_LUA_CHECK_STACK(L);
	if (f1 == f2)
		return true;

	lua_getref(L, (int)(INT_PTR)f1);
	X_ASSERT(lua_type(L, -1) == LUA_TFUNCTION, "type should be function")(lua_type(L, -1));
	const void *f1p = lua_topointer(L, -1);
	lua_pop(L, 1);
	lua_getref(L, (int)(INT_PTR)f2);
	X_ASSERT(lua_type(L, -1) == LUA_TFUNCTION, "type should be function")(lua_type(L, -1));
	const void *f2p = lua_topointer(L, -1);
	lua_pop(L, 1);
	if (f1p == f2p)
		return true;
	return false;
}

void XScriptSys::ReleaseFunc(ScriptFunctionHandle f)
{
	X_LUA_CHECK_STACK(L);

	if (f)
	{
#ifdef _DEBUG
		lua_getref(L, (int)(INT_PTR)f);
		X_ASSERT(lua_type(L, -1) == LUA_TFUNCTION, "type should be function")(lua_type(L, -1));
		lua_pop(L, 1);
#endif
		lua_unref(L, (int)(INT_PTR)f);
	}
}




#endif



void XScriptSys::PushTable(IScriptTable *pTable)
{
	((XScriptTable*)pTable)->PushRef();
};


void XScriptSys::AttachTable(IScriptTable *pTable)
{
	((XScriptTable*)pTable)->Attach();
}

void XScriptSys::PushVec3(const Vec3f& vec)
{
	state::new_table(L);

	stack::push(L, "x", 1);
	stack::push(L, vec.x);
	state::set_table_value(L);

	stack::push(L, "y", 1);
	stack::push(L, vec.y);
	state::set_table_value(L);

	stack::push(L, "z", 1);
	stack::push(L, vec.z);
	state::set_table_value(L);
}

bool XScriptSys::ToVec3(Vec3f& vec, int tableIndex)
{
	X_LUA_CHECK_STACK(L);

	if (tableIndex < 0)
	{
		tableIndex = lua_gettop(L) + tableIndex + 1;
	}

	if (lua_type(L, tableIndex) != LUA_TTABLE)
	{
		return false;
	}


	lua_Number x, y, z;
	lua_pushlstring(L, "x", 1);
	lua_gettable(L, tableIndex);
	if (!lua_isnumber(L, -1))
	{
		lua_pop(L, 1); // pop x value.

		// Try an indexed table.
		lua_pushnumber(L, 1);
		lua_gettable(L, tableIndex);
		if (!lua_isnumber(L, -1))
		{
			lua_pop(L, 1); // pop value.
			return false;
		}
		x = lua_tonumber(L, -1);
		lua_pushnumber(L, 2);
		lua_gettable(L, tableIndex);
		if (!lua_isnumber(L, -1))
		{
			lua_pop(L, 2); // pop value.
			return false;
		}
		y = lua_tonumber(L, -1);
		lua_pushnumber(L, 3);
		lua_gettable(L, tableIndex);
		if (!lua_isnumber(L, -1))
		{
			lua_pop(L, 3); // pop value.
			return false;
		}
		z = lua_tonumber(L, -1);
		lua_pop(L, 3); // pop value.

		vec.x = safe_static_cast<float>(x);
		vec.y = safe_static_cast<float>(y);
		vec.z = safe_static_cast<float>(z);
		return true;
	}

	x = lua_tonumber(L, -1);
	lua_pop(L, 1); // pop value.

	lua_pushlstring(L, "y", 1);
	lua_gettable(L, tableIndex);
	if (!lua_isnumber(L, -1))
	{
		lua_pop(L, 1); // pop table.
		return false;
	}
	y = lua_tonumber(L, -1);
	lua_pop(L, 1); // pop value.

	lua_pushlstring(L, "z", 1);
	lua_gettable(L, tableIndex);
	if (!lua_isnumber(L, -1))
	{
		lua_pop(L, 1); // pop table.
		return false;
	}
	z = lua_tonumber(L, -1);
	lua_pop(L, 1); // pop value.

	vec.x = safe_static_cast<float>(x);
	vec.y = safe_static_cast<float>(y);
	vec.z = safe_static_cast<float>(z);
	return true;
}




// ~IScriptSys


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

#if 0

void XScriptSys::LogStackTrace()
{
//	DumpCallStack(L);
}

void XScriptSys::TraceScriptError()
{
	lua_Debug ar;
	core::zero_object(ar);

	// TODO
	X_DISABLE_WARNING(4127)
	if (0)
	X_ENABLE_WARNING(4127)
	{
		if (lua_getstack(L, 1, &ar))
		{
			if (lua_getinfo(L, "lnS", &ar))
			{
				//	ShowDebugger(file, line, errorStr);
			}
		}
	}
	else
	{
		LogStackTrace();
	}
}
#endif

int XScriptSys::ErrorHandler(lua_State *L)
{
	XScriptSys* pThis = static_cast<XScriptSys*>(gEnv->pScriptSys);

//	lua_Debug ar;
//	core::zero_object(ar);

	const char* Err = lua_tostring(L, 1);

	X_ERROR("ScriptError", "------------------------------------------");
	{
		X_LOG0("ScriptError", "%s", Err);
	//	X_LOG_BULLET;

	//	pThis->TraceScriptError();
	}

	pThis->DumpStateToFile("lua_error_state_dump");

	X_ERROR("ScriptError", "------------------------------------------");
	return 0;
}



struct XRecursiveLuaDumpToFile : public IRecursiveLuaDump
{
	XRecursiveLuaDumpToFile(const char* filename)
	{
		file_.openFile(filename, core::fileMode::WRITE | core::fileMode::RECREATE);
		nSize = 0;
	}
	~XRecursiveLuaDumpToFile() X_FINAL {

	}

	core::XFileScoped file_;

	char sLevelOffset[1024];
	char sKeyStr[32];
	size_t nSize;

	const char* GetOffsetStr(int nLevel)
	{
		if (nLevel > sizeof(sLevelOffset)-1)
			nLevel = sizeof(sLevelOffset)-1;
		memset(sLevelOffset, '\t', nLevel);
		sLevelOffset[nLevel] = 0;
		return sLevelOffset;
	}
	const char* GetKeyStr(const char *sKey, int nKey)
	{
		if (sKey)
			return sKey;
		sprintf(sKeyStr, "[%02d]", nKey);
		return sKeyStr;
	}

	virtual void OnElement(int nLevel, const char* sKey, int nKey, ScriptValue& value) X_FINAL
	{
	//	nSize += sizeof(Node);
		if (sKey)
			nSize += core::strUtil::strlen(sKey) + 1;
		else
			nSize += sizeof(int);
		switch (value.getType())
		{
			case Type::BOOLEAN:
			if (value.bool_)
				file_.printf( "[%6d] %s %s=true\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey));
			else
				file_.printf("[%6d] %s %s=false\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey));
			break;
			case Type::HANDLE:
				file_.printf("[%6d] %s %s=%p\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey), value.pPtr_);
			break;
			case Type::NUMBER:
				file_.printf( "[%6d] %s %s=%g\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey), value.number_);
			break;
			case Type::STRING:
			file_.printf("[%6d] %s %s=%s\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey), value.str_.pStr);
			nSize += value.str_.len + 1;
			break;
			//case ANY_TTABLE:
			case Type::FUNCTION:
			file_.printf("[%6d] %s %s()\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey));
			break;
		//	case Type::USERDATA:
		//	fprintf(file, "[%6d] %s [userdata] %s\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey));
		//	break;
			case Type::VECTOR:
			file_.printf("[%6d] %s %s=%g,%g,%g\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey), value.vec3_.x, value.vec3_.y, value.vec3_.z);
			nSize += sizeof(Vec3f);
			break;
		}
	}
	virtual void OnBeginTable(int nLevel, const char* sKey, int nKey) X_FINAL
	{
	//	nSize += sizeof(Node);
	//	nSize += sizeof(Table);
		file_.printf("[%6d] %s %s = {\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey));
	}
	virtual void OnEndTable(int nLevel) X_FINAL
	{
		file_.printf("[%6d] %s }\n", nSize, GetOffsetStr(nLevel));
	}
};

//////////////////////////////////////////////////////////////////////////
static void RecursiveTableDump(XScriptSys* pSS, lua_State* L, int idx, int nLevel, 
	IRecursiveLuaDump *sink, std::set<void*>& tables)
{
	const char *sKey = 0;
	int nKey = 0;

	X_LUA_CHECK_STACK(L);

	void* pTable = (void*)lua_topointer(L, idx);
	if (tables.find(pTable) != tables.end())
	{
		// This table was already dumped.
		return;
	}
	tables.insert(pTable);

	lua_pushnil(L);
	while (lua_next(L, idx) != 0)
	{
		// `key' is at index -2 and `value' at index -1
		if (lua_type(L, -2) == LUA_TSTRING)
			sKey = lua_tostring(L, -2);
		else
		{
			sKey = 0;
			nKey = (int)lua_tonumber(L, -2); // key index.
		}
		int type = lua_type(L, -1);
		switch (type)
		{
			case LUA_TNIL:
			break;
			case LUA_TTABLE:
			{
				if (!(sKey != 0 && nLevel == 0 && strcmp(sKey, "_G") == 0))
				{
					sink->OnBeginTable(nLevel, sKey, nKey);
					RecursiveTableDump(pSS, L, lua_gettop(L), nLevel + 1, sink, tables);
					sink->OnEndTable(nLevel);
				}
			}
			break;
			default:
			{
				ScriptValue any;
				pSS->ToAny(any, -1);
				sink->OnElement(nLevel, sKey, nKey, any);
			}
			break;
		}
		lua_pop(L, 1);
	}
}



bool XScriptSys::DumpStateToFile(const char* name)
{
	X_LUA_CHECK_STACK(L);

	core::Path<char> path(name);
	path.setExtension(".txt");

	XRecursiveLuaDumpToFile sink(path.c_str());

	if (sink.file_)
	{
		std::set<void*> tables;
		RecursiveTableDump(this, L, LUA_GLOBALSINDEX, 0, &sink, tables);

		X_LUA_CHECK_STACK(L);

		for (std::set<XScriptTable*>::iterator it = XScriptTable::s_allTables_.begin();
			it != XScriptTable::s_allTables_.end(); ++it)
		{
			XScriptTable* pTable = *it;

			pTable->PushRef();
			RecursiveTableDump(this, L, lua_gettop(L), 1, &sink, tables);
			lua_pop(L, 1);
			sink.OnEndTable(0);
		}
	}

	return false;
}


X_NAMESPACE_END