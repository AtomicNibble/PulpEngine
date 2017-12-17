#pragma once

#include "ScriptBinds.h"

#include "Vars\ScriptVars.h"

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingPoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\HeapArea.h>

#include <Assets\AssertContainer.h>
#include <Assets\AssetBase.h>
#include <Assets\AssetManagerBase.h>


X_NAMESPACE_DECLARE(core,
	namespace V2 {
		struct Job;
		class JobSystem;
	}
	struct XFileAsync;
	struct IoRequestBase;

	struct IConsoleCmdArgs
);

X_NAMESPACE_BEGIN(script)

class Script;

class XScriptSys : 
	public IScriptSys, 
	public core::IXHotReload,
	public core::AssetManagerBase
{
	
	typedef core::AssetContainer<Script, SCRIPT_MAX_LOADED, core::SingleThreadPolicy> ScriptContainer;
	typedef ScriptContainer::Resource ScriptResource;


	typedef core::Fifo<Script*> ScriptQueue;

	typedef core::Array<XScriptTable*> ScriptTableArr;
	typedef core::Array<XScriptBinds*> ScriptBindsArr;
	typedef core::Array<XScriptBindsBase*> ScriptBindsBaseArr;
	

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

	virtual IScript* findScript(const char* pFileName);
	virtual IScript* loadScript(const char* pFileName) X_FINAL;
	
	virtual bool waitForLoad(core::AssetBase* pScript) X_FINAL;
	virtual bool waitForLoad(IScript* pScript) X_FINAL; // returns true if load succeed.


	bool processLoadedScript(Script* pScript);

	bool onInclude(const char* pFileName);
	bool executeBuffer(const char* pBegin, const char* pEnd, const char* pDesc);

	virtual bool runScriptInSandbox(const char* pBegin, const char* pEnd) X_FINAL;

	virtual ScriptFunctionHandle getFunctionPtr(const char* pFuncName) X_FINAL;
	virtual	ScriptFunctionHandle getFunctionPtr(const char* pTableName, const char* pFuncName) X_FINAL;
	virtual bool compareFuncRef(ScriptFunctionHandle f1, ScriptFunctionHandle f2) X_FINAL;
	virtual void releaseFunc(ScriptFunctionHandle f) X_FINAL;

	virtual IScriptTable* createTable(bool empty = false) X_FINAL;
	virtual IScriptBinds* createScriptBind(void) X_FINAL;

	virtual void setGlobalValue(const char* pKey, const ScriptValue& val) X_FINAL;
	virtual bool getGlobalValue(const char* pKey, ScriptValue& any) X_FINAL;

	virtual bool call(ScriptFunctionHandle f) X_FINAL;

	virtual bool beginCall(ScriptFunctionHandle f) X_FINAL;
	virtual bool beginCall(const char* pFunName) X_FINAL;
	virtual bool beginCall(const char* pTableName, const char* pFunName) X_FINAL;
	virtual bool beginCall(IScriptTable* pTable, const char* pFunName) X_FINAL;

	virtual void pushCallArg(const ScriptValue& any) X_FINAL;

	bool endCall(int32_t numReturnValues);
	virtual bool endCall(void) X_FINAL;
	virtual bool endCall(ScriptValue& value) X_FINAL;

	virtual IScriptTable* createUserData(void* ptr, size_t size) X_FINAL;

	virtual void onScriptError(const char* fmt, ...) X_FINAL;
	virtual void logCallStack(void) X_FINAL;

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
	void releaseScript(Script* pScript);

	void addLoadRequest(ScriptResource* pScript);
	void dispatchLoad(Script* pScript, core::CriticalSection::ScopedLock&);
	void dispatchLoadRequest(AssetLoadRequest* pLoadReq);

	// load / processing
	void onLoadRequestFail(AssetLoadRequest* pLoadReq);
	void loadRequestCleanup(AssetLoadRequest* pLoadReq);

	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void processData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

	bool processData(Script* pScript, core::UniquePointer<char[]> data, uint32_t dataSize);

private:

	// IXHotReload
	virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_FINAL;
	// ~IXHotReload

private:
	void listBinds(void) const;
	void listScripts(const char* pSearchPatten = nullptr) const;


private:
	void listBinds(core::IConsoleCmdArgs* pArgs);
	void listScripts(core::IConsoleCmdArgs* pArgs);
	void dumpState(core::IConsoleCmdArgs* pArgs);

private:
	lua_State* L; // hot

	PoolArena::AllocationPolicy poolAllocator_;
	PoolArena					poolArena_;

	lua::RefId errrorHandler_;
	int32_t numCallParams_;

	ScriptVars vars_;

	ScriptBindsArr scriptBinds_;
	ScriptBindsBaseArr baseBinds_;

	ScriptContainer	scripts_;

	// loading
	ScriptQueue completedLoads_;
	

	bool initialised_;
};

X_INLINE lua_State* XScriptSys::getLuaState(void)
{
	return L;
}

X_NAMESPACE_END

