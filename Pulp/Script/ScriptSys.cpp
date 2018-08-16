#include "stdafx.h"
#include "ScriptSys.h"
#include "ScriptTable.h"
#include "TableDump.h"
#include "ScriptBinds.h"
#include "Script.h"

#include "binds\ScriptBinds_core.h"
#include "binds\ScriptBinds_io.h"
#include "binds\ScriptBinds_script.h"

#include <ICore.h>
#include <IFileSys.h>
#include <IRender.h>
#include <IConsole.h>
#include <IFrameData.h>

#include <Threading\JobSystem2.h>
#include <Memory\VirtualMem.h>
#include <String\StringTokenizer.h>

#include <Assets\AssetLoader.h>

X_NAMESPACE_BEGIN(script)

using namespace lua;

namespace
{
    static const size_t POOL_ALLOC_MAX = 1024 * 4; // script tables
    static const size_t POOL_ALLOCATION_SIZE = sizeof(XScriptTable);
    static const size_t POOL_ALLOCATION_ALIGN = X_ALIGN_OF(XScriptTable);

} // namespace

XScriptSys::XScriptSys(core::MemoryArenaBase* arena) :
    arena_(arena),
    pAssetLoader_(nullptr),
    L(nullptr),
    poolAllocator_(PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE) * POOL_ALLOC_MAX,
        core::VirtualMem::GetPageSize() * 4,
        0,
        PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE),
        PoolArena::getMemoryAlignmentRequirement(POOL_ALLOCATION_ALIGN),
        PoolArena::getMemoryOffsetRequirement()),
    poolArena_(&poolAllocator_, "TablePool"),
    scriptTables_(arena),
    numCallParams_(-1),
    scriptBinds_(arena),
    baseBinds_(arena),
    scripts_(arena, sizeof(ScriptResource), X_ALIGN_OF(ScriptResource), "ScriptPool"),
    completedLoads_(arena)
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
    ADD_COMMAND_MEMBER("listScriptBinds", this, XScriptSys, &XScriptSys::listBinds, core::VarFlag::SYSTEM, "List script binds");
    ADD_COMMAND_MEMBER("listScipts", this, XScriptSys, &XScriptSys::listScripts, core::VarFlag::SYSTEM, "List loaded script files");

    ADD_COMMAND_MEMBER("scriptDumpState", this, XScriptSys, &XScriptSys::dumpState, core::VarFlag::SYSTEM, "Dump the lua state to a file <filename>");
}

bool XScriptSys::init(void)
{
    X_PROFILE_NO_HISTORY_BEGIN("ScriptSysInit", core::profiler::SubSys::SCRIPT);

    X_LOG0("Script", "Starting script system");

    pAssetLoader_ = gEnv->pCore->GetAssetLoader();
    pAssetLoader_->registerAssetType(assetDb::AssetType::SCRIPT, this, SCRIPT_FILE_EXTENSION);

    L = luaL_newstate();

    lua::StateView view(L);
    view.setPanic(myLuaPanic);
    view.openLibs(
        lua::libs(
            lua::lib::Base | lua::lib::Math | lua::lib::Table | lua::lib::String | lua::lib::Bit32 | lua::lib::Jit));

    stack::push(L, myErrorHandler);
    errrorHandler_ = stack::pop_to_ref(L);

    // Janky!
    XScriptTable::L = L;
    XScriptTable::pScriptSystem_ = this;

    baseBinds_.append(X_NEW(XBinds_Script, arena_, "ScriptBinds")(this));
    baseBinds_.append(X_NEW(XBinds_Core, arena_, "CoreBinds")(this));
    baseBinds_.append(X_NEW(XBinds_Io, arena_, "IoBinds")(this));

    for (auto* pBind : baseBinds_) {
        pBind->bind(gEnv->pCore);
    }

    setGlobalValue("timeMS", 0.f);
    setGlobalValue("uiTimeMS", 0.f);
    setGlobalValue("timeDeltaMS", 0.f);
    setGlobalValue("uiTimeDeltaMS", 0.f);

    loadScript("main");

    return true;
}

void XScriptSys::shutDown(void)
{
    X_LOG0("ScriptSys", "Shutting Down");

    for (auto* pBind : scriptBinds_) {
        X_DELETE(pBind, arena_);
    }
    for (auto* pBind : baseBinds_) {
        X_DELETE(pBind, arena_);
    }

    scriptBinds_.clear();
    baseBinds_.clear();

    freeDangling();

    if (L != nullptr) {
        lua_close(L);
        L = nullptr;
    }

}

void XScriptSys::release(void)
{
    X_DELETE(this, g_ScriptArena);
}

void XScriptSys::update(core::FrameData& frame)
{
    X_PROFILE_BEGIN("ScriptUpdate", core::profiler::SubSys::SCRIPT);

    auto ti = frame.timeInfo;

    auto timeGame = ti.ellapsed[core::ITimer::Timer::GAME].GetMilliSeconds();
    auto timeMS = ti.ellapsed[core::ITimer::Timer::UI].GetMilliSeconds();
    auto timeDeltaGame = ti.deltas[core::ITimer::Timer::GAME].GetMilliSeconds();
    auto timeDeltaMS = ti.deltas[core::ITimer::Timer::UI].GetMilliSeconds();

    setGlobalValue("timeMS", timeGame);
    setGlobalValue("uiTimeMS", timeMS);
    setGlobalValue("timeDeltaMS", timeDeltaGame);
    setGlobalValue("uiTimeDeltaMS", timeDeltaMS);

    processLoadedScritpts();

    {
        X_PROFILE_BEGIN("Lua GC", core::profiler::SubSys::SCRIPT);
        state::gc_step(L, vars_.gcStepSize());
    }
}


void XScriptSys::processLoadedScritpts(void)
{
    while (completedLoads_.isNotEmpty())
    {
        X_LUA_CHECK_STACK(L);

        Script* pScript = completedLoads_.peek();

        // if the script already fully run script processing.
        if (pScript->getLastCallResult() == lua::CallResult::Ok) {
            completedLoads_.pop();
            continue;
        }

        X_LOG0("ScriptSys", "Processing loaded script: \"%s\"", pScript->getName().c_str());

        // the script can either error.
        // fail on dependancy
        // or run okay.
        // we only don't remove if we are waiting for a depedancy,
        if (!processLoadedScript(pScript)) {
            // failed?

        }

        if (!pScript->hasPendingInclude()) {
            completedLoads_.pop();
        }
        else {
            break;
        }
    }
}

// return true if not 'error'
bool XScriptSys::processLoadedScript(Script* pScript)
{
    X_ASSERT(pScript->getStatus() == core::LoadStatus::Complete, "Script was not loaded")(pScript->getStatus());
    X_ASSERT(pScript->getLastCallResult() == lua::CallResult::None || pScript->getLastCallResult() == lua::CallResult::TryAgain, "Unexpected call result")(pScript->getLastCallResult());

    if (pScript->hasPendingInclude()) {
        X_ASSERT(pScript->getLastCallResult() == lua::CallResult::TryAgain, "Unexpected call result")(pScript->getLastCallResult());

        auto* pInclude = X_ASSERT_NOT_NULL(pScript->getPendingInclude());
        if (pInclude->getStatus() == core::LoadStatus::Error) {
            X_ERROR("Script", "\"%s\" depeancy failed to load: \"%s\"", pScript->getName().c_str(), pInclude->getName().c_str());
            return false;
        }

        // if our include has not loaded from disk yet, wait.
        if (pInclude->getStatus() != core::LoadStatus::Complete) {
            return true;
        }

        if (!processLoadedScript(pInclude)) {
            X_ERROR("Script", "\"%s\" depeancy failed to load: \"%s\"", pScript->getName().c_str(), pInclude->getName().c_str());
            return false;
        }

        // may not of been a error, but could have a child include.
        if (pInclude->getLastCallResult() != lua::CallResult::Ok) {
            return true;
        }

        // sanity check the dependancy
        X_ASSERT(pInclude->getStatus() == core::LoadStatus::Complete, "Script is not loaded")(pInclude->getStatus());
        X_ASSERT(pInclude->getLastCallResult() == lua::CallResult::Ok, "Script did not run ok")(pInclude->getLastCallResult());
        X_ASSERT(!pInclude->hasPendingInclude(), "Script has pending include")(pInclude->hasPendingInclude());
        pScript->setPendingInclude(nullptr);
    }

    X_ASSERT(!pScript->hasPendingInclude(), "Script has pending include")(pScript->hasPendingInclude());
    X_LUA_CHECK_STACK(L);

    auto chunk = pScript->getChunk(L);
    if (chunk == lua::Ref::Nil) {
        return false;
    }

    auto result = stack::pcall(L, 0, LUA_MULTRET, errrorHandler_, chunk);

    pScript->setLastCallResult(result);

    // script had a include, that's not loaded, rip.
    // we must dispatch a load and wait for the result of that.
    if (result == CallResult::TryAgain) {

        X_ASSERT(stack::get_type(L) == Type::String, "Type should be string")(stack::get_type(L));

        size_t length;
        const char* pMissingFiles = stack::as_string(L, &length);
        
        proicessMissingIncludes(pScript, pMissingFiles, pMissingFiles + length);

        stack::pop(L);
        return true;
    }

    if (result != CallResult::Ok) {
        X_ERROR("Script", "\"%s\" failed to exec: \"%s\"", pScript->getName().c_str(), CallResult::ToString(result));
        stack::pop(L);
        return false;
    }

    return true;
}

IScript* XScriptSys::findScript(const char* pFileName)
{
    core::Path<char> nameNoExt(pFileName);
    nameNoExt.removeExtension();

    core::string name(nameNoExt.begin(), nameNoExt.end());

    core::ScopedLock<ScriptContainer::ThreadPolicy> lock(scripts_.getThreadPolicy());

    ScriptResource* pScript = scripts_.findAsset(name);
    if (pScript) {
        return pScript;
    }

    return nullptr;
}

IScript* XScriptSys::loadScript(const char* pFileName)
{
    // I allow extension to be passed, unlike other assets, since it's allowed in includes.
    core::Path<char> nameNoExt(pFileName);
    nameNoExt.removeExtension();

    core::string name(nameNoExt.begin(), nameNoExt.end());

    core::ScopedLock<ScriptContainer::ThreadPolicy> lock(scripts_.getThreadPolicy());

    ScriptResource* pScriptRes = scripts_.findAsset(name);
    if (pScriptRes) {
        pScriptRes->addReference();
        return pScriptRes;
    }

    pScriptRes = scripts_.createAsset(name, arena_, name);

    addLoadRequest(pScriptRes);

    return pScriptRes;
}

bool XScriptSys::waitForLoad(core::AssetBase* pScript)
{
    X_ASSERT(pScript->getType() == assetDb::AssetType::SCRIPT, "Invalid asset passed")();

    if (pScript->isLoaded()) {
        return true;
    }

    return waitForLoad(static_cast<IScript*>(static_cast<Script*>(pScript)));
}

bool XScriptSys::waitForLoad(IScript* pIScript)
{
    Script* pScript = static_cast<Script*>(pIScript);

    return pAssetLoader_->waitForLoad(pScript);
}

int32_t XScriptSys::onInclude(IFunctionHandler* pH)
{
    core::StackString512 missing;

    auto num = pH->getParamCount();
    for (int32_t i = 0; i < num; i++)
    {
        const char* pFileName = nullptr;
        if (!pH->getParam(i + 1, pFileName)) {
            return pH->endFunction();
        }

        Script* pScriptRes = static_cast<Script*>(findScript(pFileName));
        if (!pScriptRes || !pScriptRes->isLoaded()) {
            if (missing.isNotEmpty()) {
                missing.append(";");
            }

            missing.append(pFileName);
        }
    }

    if (missing.isEmpty()) {
        return pH->endFunction();
    }

    // push the files.
    stack::push(L, missing.c_str(), missing.length());

    lua_tryagain(L);

    return pH->endFunction();
}

void XScriptSys::proicessMissingIncludes(Script* pScript, const char* pBegin, const char* pEnd)
{
    X_ASSERT_NOT_NULL(pBegin);
    X_ASSERT_NOT_NULL(pEnd);

    core::StringTokenizer<char> tokens(pBegin, pEnd, ';');
    core::StringRange<> range(nullptr, nullptr);

    while (tokens.extractToken(range))
    {
        core::StackString256 name(range.begin(), range.end());
        Script* pInclude = static_cast<Script*>(loadScript(name.c_str()));

        pScript->setPendingInclude(X_ASSERT_NOT_NULL(pInclude));
    }
}


bool XScriptSys::executeBuffer(const char* pBegin, const char* pEnd, const char* pDesc)
{
    lua::StateView state(L);

    if (!state.loadScript(pBegin, pEnd, pDesc)) {
        return false;
    }

    int base = stack::top(state);
    stack::push_ref(state, errrorHandler_);
    stack::move_top_to(state, base); // move the error hander before the script.

    auto status = stack::pcall(state, 0, LUA_MULTRET, base);

    stack::remove(state, base);

    if (status != CallResult::Ok) {
        stack::pop(L);
    }

    return true;
}

bool XScriptSys::loadBufferToTable(const char* pBegin, const char* pEnd, const char* pDesc, IScriptTable* pITable)
{
    X_LUA_CHECK_STACK(L);

    lua::StateView state(L);

    if (!state.loadScript(pBegin, pEnd, pDesc)) {
        return false;
    }

    int base = stack::top(state);
    stack::push_ref(state, errrorHandler_);
    stack::move_top_to(state, base); // move the error hander before the script.

    auto result = stack::pcall(state, 0, LUA_MULTRET, base);
   
    // remove error handler.
    stack::remove(state, base);

    auto type = stack::get_type(L);

    if (result == CallResult::TryAgain)
    {
        X_ASSERT(stack::get_type(L) == Type::String, "Type should be string")(stack::get_type(L));
        const char* pMissingFiles = stack::as_string(L);
        X_UNUSED(pMissingFiles);

        stack::pop(L);
        return false;
    }
    
    if (result != CallResult::Ok) 
    {
        X_ERROR("Script", "failed to exec: \"%s\"", CallResult::ToString(result));
        stack::pop(L);
        return false;
    }
    
    if (type != Type::Table)
    {
        X_ERROR("Script", "Not a table: \"%s\"", Type::ToString(type));
        stack::pop(L);
        return false;
    }

    auto* pTable = static_cast<XScriptTable*>(pITable);
    pTable->attach();
    return true;
}

bool XScriptSys::runScriptInSandbox(const char* pBegin, const char* pEnd) const
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

    if (status != CallResult::Ok) {
        stack::pop(L);
    }

    return true;
}

ScriptFunctionHandle XScriptSys::getFunctionPtr(const char* pFuncName)
{
    X_LUA_CHECK_STACK(L);

    stack::push_global(L, pFuncName);
    if (!stack::isfunction(L)) {
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
    X_ASSERT(stack::get_type(L) == Type::Function, "type should be function")(stack::get_type(L));
    const void* f1p = stack::as_pointer(L);
    stack::pop(L);

    stack::push_ref(L, f1);
    X_ASSERT(stack::get_type(L) == Type::Function, "type should be function")(stack::get_type(L));
    const void* f2p = stack::as_pointer(L);
    stack::pop(L);

    return f1p == f2p;
}

void XScriptSys::releaseFunc(ScriptFunctionHandle f)
{
    X_LUA_CHECK_STACK(L);

    if (f != INVALID_HANLDE) {
#ifdef _DEBUG
        stack::push_ref(L, f);
        X_ASSERT(stack::get_type(L) == Type::Function, "type should be function")(stack::get_type(L));
        stack::pop(L);
#endif

        state::remove_ref(L, f);
    }
}

IScriptTable* XScriptSys::createTable(bool empty)
{
    XScriptTable* pTable = allocTable();
    if (!empty) {
        pTable->createNew();
    }

    return pTable;
}

IScriptBinds* XScriptSys::createScriptBind(void)
{
    XScriptBinds* pScriptBase = X_NEW(XScriptBinds, arena_, "ScriptBase")(this);

    scriptBinds_.push_back(pScriptBase);

    return pScriptBase;
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
    if (pSep) {
        ScriptValue globalAny;
        core::StackString<256> key1(pKey, pSep);

        getGlobalValue(key1.c_str(), globalAny);
        if (globalAny.getType() == Type::Table) {
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

bool XScriptSys::call(ScriptFunctionHandle f)
{
    if (!beginCall(f)) {
        return false;
    }

    numCallParams_ = 0;
    return endCall(0);
}

bool XScriptSys::beginCall(ScriptFunctionHandle f)
{
    X_ASSERT(numCallParams_ < 0, "Begin called when in the middle of a function call block")(numCallParams_);

    if (f == INVALID_HANLDE) {
        return false;
    }

    stack::push_ref(L, f);

    X_ASSERT(stack::isfunction(L), "Invalid function handle")(f, stack::get_type(L));
    return true;
}

bool XScriptSys::beginCall(const char* pFunName)
{
    X_ASSERT(numCallParams_ < 0, "Begin called when in the middle of a function call block")(numCallParams_);

    stack::push_global(L, pFunName);

    if (stack::isfunction(L)) {
        X_ERROR("Script", "Function \"%s\" not found.", pFunName);
        return false;
    }

    numCallParams_ = 0;
    return true;
}

bool XScriptSys::beginCall(const char* pTableName, const char* pFunName)
{
    X_ASSERT(numCallParams_ < 0, "Begin called when in the middle of a function call block")(numCallParams_);

    stack::push_global(L, pTableName);

    if (!stack::istable(L)) {
        X_ERROR("Script", "Table \"%s\" not found", pTableName);
        return false;
    }

    stack::push_table_value(L, -2, pFunName);
    stack::remove(L, -2);

    if (!stack::isfunction(L)) {
        X_ERROR("Script", "Function \"%s\" not found on table: \"%s\"", pFunName, pTableName);
        return false;
    }

    numCallParams_ = 0;
    return true;
}

bool XScriptSys::beginCall(IScriptTable* pTable, const char* pFunName)
{
    X_ASSERT(numCallParams_ < 0, "Begin called when in the middle of a function call block")(numCallParams_);

    pushTable(pTable);

    stack::push_table_value(L, -2, pFunName);
    stack::remove(L, -2);

    if (!stack::isfunction(L)) {
        X_ERROR("Script", "Function \"%s\" not found on table: %p", pFunName, pTable);
        return false;
    }

    numCallParams_ = 0;
    return true;
}

void XScriptSys::pushCallArg(const ScriptValue& any)
{
    X_ASSERT(numCallParams_ >= 0, "PushFuncArg called without a valid begin call")(numCallParams_);

    pushAny(any);
    ++numCallParams_;
}

bool XScriptSys::endCall(int32_t numReturnValues)
{
    X_ASSERT(numCallParams_ >= 0, "endCall called without a valid begin call")(numCallParams_);

    int32_t fucIndex = stack::top(L) - numCallParams_;

    stack::push_ref(L, errrorHandler_);
    stack::move_top_to(L, fucIndex);

    auto status = stack::pcall(L, numCallParams_, numReturnValues, fucIndex);

    stack::remove(L, fucIndex);

    numCallParams_ = -1;

    if (status != CallResult::Ok) {
        X_ERROR("Script", "Function call failed: %s", CallResult::ToString(status));
        return false;
    }

    return true;
}

bool XScriptSys::endCall(void)
{
    return endCall(0);
}

bool XScriptSys::endCall(ScriptValue& value)
{
    if (!endCall(1)) {
        return false;
    }

    return popAny(value);
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

void XScriptSys::logCallStack(void)
{
    lua::dumpCallStack(L);
}

XScriptTable* XScriptSys::allocTable(void)
{
    auto* pTable = X_NEW(XScriptTable, &poolArena_, "ScriptTable");

    core::CriticalSection::ScopedLock lock(cs_);
    scriptTables_.push_back(pTable);

    return pTable;
}

void XScriptSys::freeTable(XScriptTable* pTable)
{
    {
        core::CriticalSection::ScopedLock lock(cs_);
        scriptTables_.remove(pTable);
    }
    X_DELETE(pTable, &poolArena_);
}

bool XScriptSys::getRecursiveAny(IScriptTable* pTable, const core::StackString<256>& key, ScriptValue& any)
{
    core::StackString<256> key1;
    core::StackString<256> key2;

    const char* pSep = key.find('.');
    if (pSep) {
        key1.set(key.begin(), pSep);
        key2.set(pSep + 1, key.end());
    }
    else {
        key1 = key;
    }

    ScriptValue localAny;
    if (!pTable->getValueAny(key1.c_str(), localAny)) {
        return false;
    }

    if (localAny.getType() == Type::Function && nullptr == pSep) {
        any = localAny;
        return true;
    }
    else if (localAny.getType() == Type::Table && nullptr != pSep) {
        return getRecursiveAny(localAny.pTable_, key2, any);
    }

    return false;
}

bool XScriptSys::popAny(ScriptValue& var)
{
    bool res = toAny(var, -1);
    stack::pop(L);
    return res;
}

void XScriptSys::pushAny(const ScriptValue& var)
{
    switch (var.getType()) {
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
            X_ASSERT(stack::get_type(L) == Type::Function, "type should be function")(stack::get_type(L));
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

X_INLINE void XScriptSys::pushTable(IScriptTable* pTable)
{
    static_cast<XScriptTable*>(pTable)->pushRef();
}

bool XScriptSys::toVec3(Vec3f& vec, int tableIndex)
{
    X_LUA_CHECK_STACK(L);

    if (tableIndex < 0) {
        tableIndex = stack::num(L) + tableIndex + 1;
    }

    if (stack::get_type(L, tableIndex) != Type::Table) {
        return false;
    }

    lua_Number x, y, z;
    stack::push_table_value(L, tableIndex, "x");

    if (stack::isnumber(L)) {
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

    auto luaType = stack::get_type(L, index);

    if (var.getType() != Type::None && !isTypeCompatible(var.getType(), luaType)) {
        X_WARNING("Script", "toAny type mismatch");
        return false;
    }

    switch (luaType) {
        case Type::Nil:
            var.type_ = Type::Nil;
            break;
        case Type::Boolean:
            var.bool_ = stack::as_bool(L, index) != 0;
            var.type_ = Type::Boolean;
            break;
        case Type::Pointer:
            var.pPtr_ = stack::as_pointer(L, index);
            var.type_ = Type::Handle;
            break;
        case Type::Number:
            var.number_ = static_cast<float>(stack::as_number(L, index));
            var.type_ = Type::Number;
            break;
        case Type::String: {
            size_t len = 0;
            var.str_.pStr = stack::as_string(L, index, &len);
            var.str_.len = safe_static_cast<int32_t>(len);
            var.type_ = Type::String;
        } break;
        case Type::Table:
        case Type::Userdata:
            if (!var.pTable_) {
                var.pTable_ = static_cast<XScriptSys*>(gEnv->pScriptSys)->allocTable();
            }
            stack::push_copy(L, index);
            static_cast<XScriptTable*>(var.pTable_)->attach();
            var.type_ = Type::Table;
            break;
        case Type::Function: {
            var.type_ = Type::Function;
            // Make reference to function.
            stack::push_copy(L, index);
            var.pFunction_ = refToScriptHandle(stack::pop_to_ref(L));
        } break;
        case LUA_TTHREAD:
        default:
            return false;
    }

    return true;
}

// ~IScriptSys

// -----------------------------------------------------------------

void XScriptSys::freeDangling(void)
{
    {
        core::ScopedLock<AssetContainer::ThreadPolicy> lock(scripts_.getThreadPolicy());

        for (const auto& m : scripts_) {
            auto* pScriptRes = m.second;
            const auto& name = pScriptRes->getName();

            X_WARNING("Script", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pScriptRes->getRefCount());
        }
    }

    scripts_.free();

    {
        core::CriticalSection::ScopedLock lock(cs_);
        if (scriptTables_.isNotEmpty())
        {
            X_WARNING("Script", "Cleaning up %" PRIuS " dangaling script tables", scriptTables_.size());

            for (auto* pTable : scriptTables_) {
                X_DELETE(pTable, &poolArena_);
            }
        }
    }
}

void XScriptSys::releaseScript(Script* pScript)
{
    ScriptResource* pScriptRes = reinterpret_cast<ScriptResource*>(pScript);
    if (pScriptRes->removeReference() == 0) {
        scripts_.releaseAsset(pScriptRes);
    }
}

void XScriptSys::addLoadRequest(ScriptResource* pScript)
{
    pScript->addReference(); // prevent instance sweep

    pAssetLoader_->addLoadRequest(pScript);
}

void XScriptSys::onLoadRequestFail(core::AssetBase* pAsset)
{
    X_UNUSED(pAsset);
}

bool XScriptSys::processData(core::AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize)
{
    auto* pScript = static_cast<Script*>(pAsset);

    if (!pScript->processData(std::move(data), dataSize)) {
        return false;
    }

    completedLoads_.push(pScript);
    return true;
}

bool XScriptSys::onFileChanged(const core::AssetName& assetName, const core::string& name)
{
    X_UNUSED(assetName, name);

    return true;
}

void XScriptSys::listBinds(void) const
{
    XScriptTableDumpConsole dumper;

    X_LOG0("Script", "--------------- ^8Binds^7 ----------------");

    for (auto* pBind : scriptBinds_) {
        X_LOG0("Script", "^2%s^7 = {", pBind->getGlobalName());
        {
            X_LOG_BULLET;
            pBind->getMethodsTable()->dump(&dumper);
        }
        X_LOG0("Script", "}");
    }

    X_LOG0("Script", "------------- ^8Binds End^7 --------------");
}

void XScriptSys::listScripts(const char* pSearchPatten) const
{
    core::ScopedLock<ScriptContainer::ThreadPolicy> lock(scripts_.getThreadPolicy());

    core::Array<ScriptResource*> sorted_scripts(arena_);
    sorted_scripts.setGranularity(scripts_.size());

    for (const auto& script : scripts_) {
        auto* pScriptRes = script.second;

        if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, pScriptRes->getName())) {
            sorted_scripts.push_back(pScriptRes);
        }
    }

    std::sort(sorted_scripts.begin(), sorted_scripts.end(), [](ScriptResource* a, ScriptResource* b) {
        const auto& nameA = a->getName();
        const auto& nameB = b->getName();
        return nameA.compareInt(nameB) < 0;
    });

    X_LOG0("Script", "------------ ^8Scripts(%" PRIuS ")^7 --------------", sorted_scripts.size());

    for (const auto* pScript : sorted_scripts) {
        X_LOG0("Script", "^2%-32s^7 Hash: ^20x%016" PRIx64 "^7 Status: ^2%s^7 Refs: ^2%" PRIi32,
            pScript->getName().c_str(), pScript->getHash(),
            lua::CallResult::ToString(pScript->getLastCallResult()), pScript->getRefCount());
    }

    X_LOG0("Script", "------------ ^8Scripts End^7 -------------");
}

void XScriptSys::listBinds(core::IConsoleCmdArgs* pArgs)
{
    X_UNUSED(pArgs);

    listBinds();
}

void XScriptSys::listScripts(core::IConsoleCmdArgs* pArgs)
{
    const char* pSearchPatten = nullptr;

    if (pArgs->GetArgCount() > 1) {
        pSearchPatten = pArgs->GetArg(1);
    }

    listScripts(pSearchPatten);
}

void XScriptSys::dumpState(core::IConsoleCmdArgs* pArgs)
{
    const char* pFileName = "lua_state.txt";
    if (pArgs->GetArgCount() > 1) {
        pFileName = pArgs->GetArg(1);
    }

    dumpStateToFile(L, pFileName);
}

X_NAMESPACE_END