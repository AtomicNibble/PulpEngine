#pragma once

#ifndef _X_SCRIPT_SYS_H_
#define _X_SCRIPT_SYS_H_

#include "binds\ScriptBinds.h"
#include <String\Path.h>

#include <IDirectoryWatcher.h>

// TODO: temp
X_DISABLE_WARNING(4702)
#include <set>
X_ENABLE_WARNING(4702)

X_NAMESPACE_BEGIN(script)


struct LuaStackGuard
{
	LuaStackGuard(lua_State *p)
	{
		m_pLS = p;
		m_nTop = lua_gettop(m_pLS);
	}
	~LuaStackGuard()
	{
		lua_settop(m_pLS, m_nTop);
	}
private:
	int m_nTop;
	lua_State *m_pLS;
};

#if defined(X_DEBUG) && 1

struct LuaStackValidator
{
	const char *text;
	lua_State *L;
	int top;
	LuaStackValidator(lua_State *pL, const char *sText)
	{
		text = sText;
		L = pL;
		top = lua_gettop(L);
	}
	~LuaStackValidator()
	{
		if (top != lua_gettop(L))
		{
			X_ASSERT(false, "Lua Stack Validation Failed")();
			lua_settop(L, top);
		}
	}
};

#define X_LUA_CHECK_STACK(L) LuaStackValidator __stackCheck__((L),__FUNCTION__);
#else //_DEBUG
#define X_LUA_CHECK_STACK(L) (void)0;
#endif //_DEBUG

struct IRecursiveLuaDump
{
	virtual ~IRecursiveLuaDump(){}
	virtual void OnElement(int nLevel, const char *sKey, int nKey, ScriptValue &value) X_ABSTRACT;
	virtual void OnBeginTable(int nLevel, const char *sKey, int nKey) X_ABSTRACT;
	virtual void OnEndTable(int nLevel) X_ABSTRACT;
};

class XScriptSys : public IScriptSys, public core::IXHotReload
{
	typedef std::set<core::string> ScriptFileList;

public:
	XScriptSys();
	// IScriptSys
	~XScriptSys() X_OVERRIDE;

	virtual void registerVars(void) X_OVERRIDE;
	virtual void registerCmds(void) X_OVERRIDE;

	virtual bool init(void) X_OVERRIDE;
	virtual void shutDown(void) X_OVERRIDE;
	virtual void release(void) X_OVERRIDE;

	virtual void Update(void) X_OVERRIDE;

	virtual bool ExecuteFile(const char* FileName, bool silent, bool forceReload) X_OVERRIDE;
	virtual bool UnLoadScript(const char* FileName) X_OVERRIDE;
	virtual void UnloadScripts() X_OVERRIDE;
	virtual bool ReloadScript(const char* FileName, bool silent) X_OVERRIDE;
	virtual bool ReloadScripts() X_OVERRIDE;
	virtual void ListLoadedScripts(void) X_OVERRIDE;
	virtual void LogStackTrace(void) X_OVERRIDE;
	virtual void SetGlobalAny(const char* Key, const ScriptValue& val) X_OVERRIDE;


	virtual HSCRIPTFUNCTION GetFunctionPtr(const char* sFuncName) X_OVERRIDE;
	virtual	HSCRIPTFUNCTION GetFunctionPtr(const char* sTableName, const char* sFuncName) X_OVERRIDE;

	virtual HSCRIPTFUNCTION AddFuncRef(HSCRIPTFUNCTION f) X_OVERRIDE;
	virtual bool CompareFuncRef(HSCRIPTFUNCTION f1, HSCRIPTFUNCTION f2) X_OVERRIDE;
	virtual void ReleaseFunc(HSCRIPTFUNCTION f) X_OVERRIDE;

	virtual IScriptTable* CreateTable(bool bEmpty = false) X_OVERRIDE;

	virtual bool GetGlobalAny(const char* Key, ScriptValue& any) X_OVERRIDE;

	virtual IScriptTable* CreateUserData(void* ptr, size_t size) X_OVERRIDE;

	virtual void OnScriptError(const char* fmt, ...) X_OVERRIDE;

	// ~IScriptSys

	// IXHotReload
	
	virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;

	// ~IXHotReload



	bool GetRecursiveAny(IScriptTable* pTable, const core::StackString<256>& key, ScriptValue& any);

	void PushAny(const ScriptValue &var);
	bool PopAny(ScriptValue& var);
	bool ToAny(ScriptValue& var, int index);
	void PushVec3(const Vec3f& vec);
	bool ToVec3(Vec3f& vec, int index);

	void PushTable(IScriptTable* pTable);
	void AttachTable(IScriptTable* pTable);

	bool DumpStateToFile(const char* name);

	X_INLINE lua_State* getLuaState(void) {
		return L;
	}

private:

	bool ExecuteFile_Internal(const core::Path<char>& path, bool silent);
	bool ExecuteBuffer(const char* sBuffer, size_t nSize, const char* Description);

	void TraceScriptError();


	void addFileName(const char* name);
	void removeFileName(const char* name);


	static int ErrorHandler(lua_State *L);

private:
	bool initialised_;
	lua_State* L;

	core::IFileSys* pFileSys_;
	XScriptBinds binds_;

	ScriptFileList fileList_;

	int c_script_draw_memory_stats_;
};


X_NAMESPACE_END

#endif // !_X_SCRIPT_SYS_H_
