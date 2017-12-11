#pragma once

#ifndef _X_SCRIPT_SYS_H_
#define _X_SCRIPT_SYS_H_

#include "wrapper\types.h"
#include "binds\ScriptBinds.h"

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingPoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\HeapArea.h>

// TODO: temp
X_DISABLE_WARNING(4702)
#include <set>
X_ENABLE_WARNING(4702)

X_NAMESPACE_BEGIN(script)

class XScriptTable;

class XScriptSys : public IScriptSys, public core::IXHotReload
{
	typedef std::set<core::string> ScriptFileList;

	typedef core::MemoryArena<
		core::GrowingPoolAllocator,
		core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
		core::SimpleBoundsChecking,
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> PoolArena;

public:
	XScriptSys(core::MemoryArenaBase* arena);
	// IScriptSys
	~XScriptSys() X_FINAL;

	virtual void registerVars(void) X_FINAL;
	virtual void registerCmds(void) X_FINAL;

	virtual bool init(void) X_FINAL;
	virtual void shutDown(void) X_FINAL;
	virtual void release(void) X_FINAL;

	virtual void Update(void) X_FINAL;

	virtual bool runScriptInSandbox(const char* pBegin, const char* pEnd) X_FINAL;

	virtual ScriptFunctionHandle getFunctionPtr(const char* pFuncName) X_FINAL;
	virtual	ScriptFunctionHandle getFunctionPtr(const char* pTableName, const char* pFuncName) X_FINAL;
	virtual bool compareFuncRef(ScriptFunctionHandle f1, ScriptFunctionHandle f2) X_FINAL;
	virtual void releaseFunc(ScriptFunctionHandle f) X_FINAL;

	virtual IScriptTable* createTable(bool bEmpty = false) X_FINAL;


	virtual void setGlobalValue(const char* pKey, const ScriptValue& val) X_FINAL;
	virtual bool getGlobalValue(const char* pKey, ScriptValue& any) X_FINAL;

	virtual IScriptTable* createUserData(void* ptr, size_t size) X_FINAL;

	virtual void onScriptError(const char* fmt, ...) X_FINAL;

	// ~IScriptSys
public:
	X_INLINE lua_State* getLuaState(void);

	XScriptTable* allocTable(void);
	void freeTable(XScriptTable* pTable);

	bool getRecursiveAny(IScriptTable* pTable, const core::StackString<256>& key, ScriptValue& any);

	bool popAny(ScriptValue& var);
	void pushAny(const ScriptValue &var);
	void pushVec3(const Vec3f& vec);
	void pushTable(IScriptTable* pTable);
	bool toVec3(Vec3f& vec, int index);
	bool toAny(ScriptValue& var, int index);

	static bool toAny(lua_State* L, ScriptValue& var, int index);

private:

//	bool ExecuteBuffer(const char* sBuffer, size_t nSize, const char* Description);

	// IXHotReload
	virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_FINAL;
	// ~IXHotReload

private:
	lua_State* L;

	core::MemoryArenaBase*		arena_;
	PoolArena::AllocationPolicy poolAllocator_;
	PoolArena					poolArena_;

	lua::RefId errrorHandler_;

	XScriptBinds binds_;
	ScriptFileList fileList_;
	bool initialised_;
};

X_INLINE lua_State* XScriptSys::getLuaState(void)
{
	return L;
}

X_NAMESPACE_END

#endif // !_X_SCRIPT_SYS_H_
