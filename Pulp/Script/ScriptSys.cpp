#include "stdafx.h"
#include "ScriptSys.h"
#include "ScriptTable.h"

#include <ICore.h>
#include <ITimer.h>
#include <IConsole.h>
#include <IFileSys.h>
#include <IRender.h>


#if X_DEBUG == 1
// X_LINK_LIB("lua51d")

// X_LINK_LIB("luajit")

// X_LINK_LIB("engine_LuaJit")

#else
X_LINK_LIB("lua51")
#endif

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

namespace
{
	static const char* X_SCRIPT_FILE_EXTENSION = "lua";

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

	ADD_CVAR_REF("script_draw_memory_stats", c_script_draw_memory_stats_, 0, 0, 1, core::VarFlag::SYSTEM,
		"Draw lua memory stats on screen");
}

void XScriptSys::registerCmds(void)
{
	// add the shieeeeeeet.
	ADD_COMMAND("listScipts", ListScriptCmd, core::VarFlag::SYSTEM, "List loaded script files");

	ADD_COMMAND("scriptLoad", LoadScriptCmd, core::VarFlag::SYSTEM, "Load and run a script file");
	ADD_COMMAND("scriptList", ListScriptCmd, core::VarFlag::SYSTEM, "List loaded script files");
	ADD_COMMAND("scriptReload", ReloadScriptCmd, core::VarFlag::SYSTEM, "Reload a given script <filename>");
	ADD_COMMAND("scriptDumpState", LuaDumpState, core::VarFlag::SYSTEM, "Dump the lua state to a file <filename>");

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

	SetGlobalValue("_time", 0);

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

//	SetGlobalValue("_time", time);

	{
		X_PROFILE_BEGIN("Lua GC", core::profiler::SubSys::SCRIPT);

		// LUA_GCSTEP: performs an incremental step of garbage collection.
		// The step "size" is controlled by data(larger values mean more steps) 
		// in a non - specified way.If you want to control the step size you must 
		// experimentally tune the value of data.The function returns 1 if the step 
		// finished a garbage - collection cycle.

//		lua_gc(L, LUA_GCSTEP, 2);
	}


	if (c_script_draw_memory_stats_)
	{
		X_ASSERT_NOT_IMPLEMENTED();

	//	core::MemoryAllocatorStatistics allocStats = g_ScriptArena->getAllocatorStatistics(true);
	//	core::MemoryAllocatorStatistics luaAllocStats = g_LuaAllocator.getStatistics();


	//	Vec3f pos(10,30, 1);
	//	render::DrawTextInfo ti;
	//	ti.col = Col_Whitesmoke;
	//	ti.flags = render::DrawTextFlags::POS_2D | render::DrawTextFlags::MONOSPACE;
	

	//	gEnv->pRender->DrawAllocStats(Vec3f(10, 35, 1), ti, allocStats, "ScriptSys");
	//	gEnv->pRender->DrawAllocStats(Vec3f(250, 35, 1), ti, luaAllocStats, "Lua");
	}
}


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

HSCRIPTFUNCTION XScriptSys::GetFunctionPtr(const char* sFuncName)
{
	X_LUA_CHECK_STACK(L);
	HSCRIPTFUNCTION func;
	lua_getglobal(L, sFuncName);
	if (lua_isnil(L, -1) || (!lua_isfunction(L, -1)))
	{
		lua_pop(L, 1);
		return nullptr;
	}
	func = (HSCRIPTFUNCTION)(INT_PTR)lua_ref(L, 1);

	return func;
}

HSCRIPTFUNCTION XScriptSys::GetFunctionPtr(const char* sTableName, const char* sFuncName)
{
	X_LUA_CHECK_STACK(L);
	HSCRIPTFUNCTION func;
	lua_getglobal(L, sTableName);
	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return 0;
	}
	lua_pushstring(L, sFuncName);
	lua_gettable(L, -2);
	lua_remove(L, -2); // Remove table global.
	if (lua_isnil(L, -1) || (!lua_isfunction(L, -1)))
	{
		lua_pop(L, 1);
		return FALSE;
	}
	func = (HSCRIPTFUNCTION)(INT_PTR)lua_ref(L, 1);
	return func;
}

HSCRIPTFUNCTION XScriptSys::AddFuncRef(HSCRIPTFUNCTION f)
{
	// type cast': conversion from 'int' to 'Potato::script::HSCRIPTFUNCTION' of greater size
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
			return (HSCRIPTFUNCTION)ret;
		}
		
		X_ASSERT_UNREACHABLE();
	}
	return 0;

	X_ENABLE_WARNING(4312)
}

bool XScriptSys::CompareFuncRef(HSCRIPTFUNCTION f1, HSCRIPTFUNCTION f2)
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

void XScriptSys::ReleaseFunc(HSCRIPTFUNCTION f)
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



void XScriptSys::PushAny(const ScriptValue& var)
{
	switch (var.getType())
	{
	//	case ANY_ANY:
		case ScriptValueType::TNIL:
		lua_pushnil(L);
		break;
		case ScriptValueType::BOOLEAN:
		lua_pushboolean(L, var.b);
		break;
		case ScriptValueType::HANDLE:
		lua_pushlightuserdata(L, const_cast<void*>(var.ptr));
		break;
		case ScriptValueType::NUMBER:
		lua_pushnumber(L, var.number);
		break;
		case ScriptValueType::STRING:
		lua_pushstring(L, var.str);
		break;
		case ScriptValueType::TABLE:
		if (var.pTable)
			PushTable(var.pTable);
		else
			lua_pushnil(L);
		break;
		case ScriptValueType::FUNCTION:
		lua_getref(L, (int)(INT_PTR)var.pFunction);
		X_ASSERT(lua_type(L, -1) == LUA_TFUNCTION, "invalid type")(lua_type(L, -1));
		break;
//		case ScriptValueType::USERDATA:
//		lua_getref(L, var.ud.nRef);
//		break;
		case ScriptValueType::VECTOR:
		PushVec3(Vec3f(var.vec3.x, var.vec3.y, var.vec3.z));
		break;
		default:
			X_ASSERT_UNREACHABLE();
	}
}

bool XScriptSys::ToAny(ScriptValue& var, int index)
{
	if (!lua_gettop(L))
		return false;

	X_LUA_CHECK_STACK(L);

	if (var.getType() == ScriptValueType::NONE)
	{
		switch (lua_type(L, index))
		{
			case LUA_TNIL:
			var.type_ = ScriptValueType::TNIL;
			break;
			case LUA_TBOOLEAN:
			var.b = lua_toboolean(L, index) != 0;
			var.type_ = ScriptValueType::BOOLEAN;
			break;
			case LUA_TLIGHTUSERDATA:
			var.ptr = lua_topointer(L, index);
			var.type_ = ScriptValueType::HANDLE;
			break;
			case LUA_TNUMBER:
			var.number = (float)lua_tonumber(L, index);
			var.type_ = ScriptValueType::NUMBER;
			break;
			case LUA_TSTRING:
			var.str = lua_tostring(L, index);
			var.type_ = ScriptValueType::STRING;
			break;
			case LUA_TTABLE:
			case LUA_TUSERDATA:
			if (!var.pTable)
			{
				var.pTable = AllocTable();
				var.pTable->addRef();
			}
			lua_pushvalue(L, index);
			AttachTable(var.pTable);
			var.type_ = ScriptValueType::TABLE;
			break;
			case LUA_TFUNCTION:
			{
								  var.type_ = ScriptValueType::FUNCTION;
								  // Make reference to function.
								  lua_pushvalue(L, index);
								  var.pFunction = (HSCRIPTFUNCTION)(INT_PTR)lua_ref(L, 1);
			}
			break;
			case LUA_TTHREAD:
			default:
				return false;
		}
		return true;
	}
	else
	{
		bool res = false;
		switch (lua_type(L, index))
		{
			case LUA_TNIL:
			if (var.type_ == ScriptValueType::TNIL)
				res = true;
			break;
			case LUA_TBOOLEAN:
			if (var.type_ == ScriptValueType::BOOLEAN)
			{
				var.b = lua_toboolean(L, index) != 0;
				res = true;
			}
			break;
			case LUA_TLIGHTUSERDATA:
			if (var.type_ == ScriptValueType::HANDLE)
			{
				var.ptr = lua_topointer(L, index);
				res = true;
			}
			break;
			case LUA_TNUMBER:
			if (var.type_ == ScriptValueType::NUMBER)
			{
				var.number = (float)lua_tonumber(L, index);
				res = true;
			}
			else if (var.type_ == ScriptValueType::BOOLEAN)
			{
				var.b = lua_tonumber(L, index) != 0;
				res = true;
			}
			break;
			case LUA_TSTRING:
			if (var.type_ == ScriptValueType::STRING)
			{
				var.str = lua_tostring(L, index);
				res = true;
			}
			break;
			case LUA_TTABLE:
			if (var.type_ == ScriptValueType::TABLE)
			{
				if (!var.pTable)
				{
					var.pTable = AllocTable();
					var.pTable->addRef();
				}
				lua_pushvalue(L, index);
				AttachTable(var.pTable);
				res = true;
			}
			else if (var.type_ == ScriptValueType::VECTOR)
			{
				Vec3f v(0, 0, 0);
				res = ToVec3(v, index);

				if (res)
				{
					var.vec3.x = v.x;
					var.vec3.y = v.y;
					var.vec3.z = v.z;
				}
			}
			break;
			case LUA_TUSERDATA:
			if (var.type_ == ScriptValueType::TABLE)
			{
				if (!var.pTable)
				{
					var.pTable = AllocTable();
					var.pTable->addRef();
				}
				lua_pushvalue(L, index);
				AttachTable(var.pTable);
				res = true;
			}
			break;
			case LUA_TFUNCTION:
			if (var.type_ == ScriptValueType::FUNCTION)
			{
				// Make reference to function.
				lua_pushvalue(L, index);
				var.pFunction = (HSCRIPTFUNCTION)(INT_PTR)lua_ref(L, 1);
				res = true;
			}
			break;
			case LUA_TTHREAD:
			break;
		}
		return res;
	}
	return false;
}


bool XScriptSys::PopAny(ScriptValue &var)
{
	bool res = ToAny(var, -1);
	lua_pop(L, 1);
	return res;
}

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
	lua_newtable(L);
	lua_pushlstring(L, "x", 1);
	lua_pushnumber(L, vec.x);
	lua_settable(L, -3);
	lua_pushlstring(L, "y", 1);
	lua_pushnumber(L, vec.y);
	lua_settable(L, -3);
	lua_pushlstring(L, "z", 1);
	lua_pushnumber(L, vec.z);
	lua_settable(L, -3);
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



IScriptTable* XScriptSys::CreateTable(bool bEmpty)
{
	XScriptTable* pObj = AllocTable();
	if (!bEmpty)
	{
		pObj->CreateNew();
	}
	return pObj;
}


bool XScriptSys::GetGlobalAny(const char* Key, ScriptValue& any)
{
	X_LUA_CHECK_STACK(L);

	const char* sep = core::strUtil::Find(Key,Key+core::strUtil::strlen(Key),'.'); // strchr(Key, '.');
	if (sep)
	{
		ScriptValue globalAny;
		core::StackString<256> key1(Key, sep);

		GetGlobalAny(key1.c_str(), globalAny);
		if (globalAny.getType() == ScriptValueType::TABLE)
		{
			return GetRecursiveAny(globalAny.pTable, key1, any);
		}
		return false;
	}

	lua_getglobal(L, Key);
	if (!PopAny(any))
	{
		return false;
	}
	return true;
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
	if (!pTable->GetValueAny(key1.c_str(), localAny))
		return false;

	if (localAny.getType() == ScriptValueType::FUNCTION && nullptr == sep)
	{
		any = localAny;
		return true;
	}
	else if (localAny.getType() == ScriptValueType::TABLE && nullptr != sep)
	{
		return GetRecursiveAny(localAny.pTable, key2, any);
	}
	return false;
}

IScriptTable* XScriptSys::CreateUserData(void* ptr, size_t size)
{
	X_LUA_CHECK_STACK(L);

	void* nptr = lua_newuserdata(L, size);
	memcpy(nptr, ptr, size);
	XScriptTable* pNewTbl = X_NEW( XScriptTable, g_ScriptArena, "ScriptTable");
	pNewTbl->Attach();

	return pNewTbl;
}

void XScriptSys::OnScriptError(const char* fmt, ...)
{
	core::StackString<1024> error;

	X_VALIST_START(fmt);

	error.appendFmt(fmt, args);

	X_WARNING("LuaError", error.c_str());

	X_VALIST_END;
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

		pThis->TraceScriptError();
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
			case ScriptValueType::BOOLEAN:
			if (value.b)
				file_.printf( "[%6d] %s %s=true\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey));
			else
				file_.printf("[%6d] %s %s=false\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey));
			break;
			case ScriptValueType::HANDLE:
				file_.printf("[%6d] %s %s=%p\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey), value.ptr);
			break;
			case ScriptValueType::NUMBER:
				file_.printf( "[%6d] %s %s=%g\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey), value.number);
			break;
			case ScriptValueType::STRING:
			file_.printf("[%6d] %s %s=%s\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey), value.str);
			nSize += core::strUtil::strlen(value.str) + 1;
			break;
			//case ANY_TTABLE:
			case ScriptValueType::FUNCTION:
			file_.printf("[%6d] %s %s()\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey));
			break;
		//	case ScriptValueType::USERDATA:
		//	fprintf(file, "[%6d] %s [userdata] %s\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey));
		//	break;
			case ScriptValueType::VECTOR:
			file_.printf("[%6d] %s %s=%g,%g,%g\n", nSize, GetOffsetStr(nLevel), GetKeyStr(sKey, nKey), value.vec3.x, value.vec3.y, value.vec3.z);
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