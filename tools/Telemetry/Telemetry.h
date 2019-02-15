#pragma once

#include <../TelemetryCommon/Types.h>
#include "../TelemetryCommon/Compiler.h" // for export defs

#ifndef TTELEMETRY_ENABLED
#define TTELEMETRY_ENABLED 1
#endif // TTELEMETRY_ENABLED

#ifndef TTELEMETRY_LINK 
#define TTELEMETRY_LINK 1
#endif // TTELEMETRY_LINK 

#define __TELEMETRY_UNIQUE_NAME_HELPER_0(_0, _1)  _0##_1
#define __TELEMETRY_UNIQUE_NAME_HELPER(_0, _1) __TELEMETRY_UNIQUE_NAME_HELPER_0(_0, _1)
#define __TELEMETRY_UNIQUE_NAME(name) __TELEMETRY_UNIQUE_NAME_HELPER(name, __LINE__)

enum class LogType
{
    Msg,
    Warning,
    Error
};

using LogFunction = void(*)(void* pUserData, LogType type, const char* pMsgNullTerm, tt_int32 lenWithoutTerm);
using TraceContexHandle = tt_uintptr;


inline TraceContexHandle INVALID_TRACE_CONTEX = 0;

enum TtConnectionType
{
    Tcp
};

enum TtError
{
    Ok,
    Error,
    InvalidParam,
    InvalidContex,
    ArenaTooSmall,
    NetNotInit,

    HandeshakeFail
};

struct TtCallStack
{
    static const unsigned int MAX_FRAMES = 7;

    void* frames[MAX_FRAMES];
};

struct TtSourceInfo
{
    TtSourceInfo() = default;
    TtSourceInfo(const TtSourceInfo& oth) = default;
    inline TtSourceInfo(const char* const pFile, const char* const pFunction, int line) :
        pFile_(pFile),
        pFunction_(pFunction),
        line_(line)
    {}

    TtSourceInfo& operator =(const TtSourceInfo& oth) = default;

    const char* pFile_;
    const char* pFunction_;
    int line_;
};

#define TT_SOURCE_INFO TtSourceInfo(__FILE__, __FUNCTION__, __LINE__)

#ifdef __cplusplus
extern "C" 
{
#endif

#if TTELEMETRY_LINK
#define __TELEM_API_BLANK(func) 
#else
#define __TELEM_API_BLANK(func) func
#endif

#define __TELEM_API_VOID(name, ...) TELEMETRYLIB_EXPORT void  name(__VA_ARGS__); \
        __TELEM_API_BLANK(inline void __blank##name(__VA_ARGS__) {})

#define __TELEM_API_BOOL(name, ...) TELEMETRYLIB_EXPORT bool name(__VA_ARGS__); \
        __TELEM_API_BLANK(inline bool __blank##name(__VA_ARGS__) { return true; })

#define __TELEM_API_ERR(name, ...) TELEMETRYLIB_EXPORT TtError name(__VA_ARGS__); \
        __TELEM_API_BLANK(inline TtError __blank##name(__VA_ARGS__) { return TtError::Ok; })

#pragma warning( push )
#pragma warning(disable: 4100) // unused param (caused by the blank functions).

    __TELEM_API_BOOL(TelemInit);
    __TELEM_API_VOID(TelemShutDown);

    // Context
    __TELEM_API_ERR(TelemInitializeContext, TraceContexHandle& ctx, void* pArena, tt_size bufLen);
    __TELEM_API_VOID(TelemShutdownContext, TraceContexHandle ctx);

    __TELEM_API_VOID(TelemSetContextLogFunc, TraceContexHandle ctx, LogFunction func, void* pUserData);

    __TELEM_API_ERR(TelemOpen, TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
        tt_uint16 serverPort, TtConnectionType conType, tt_int32 timeoutMS);

    __TELEM_API_BOOL(TelemClose, TraceContexHandle ctx);

    __TELEM_API_VOID(TelemTick, TraceContexHandle ctx);
    __TELEM_API_VOID(TelemFlush, TraceContexHandle ctx);
    __TELEM_API_VOID(TelemUpdateSymbolData, TraceContexHandle ctx);

    __TELEM_API_VOID(TelemPause, TraceContexHandle ctx, bool pause);
    __TELEM_API_BOOL(TelemIsPaused, TraceContexHandle ctx);

    // Thread
    __TELEM_API_VOID(TelemSetThreadName, TraceContexHandle ctx, tt_uint32 threadID, const char* pName);

    // Callstack
    __TELEM_API_BOOL(TelemGetCallStack, TraceContexHandle ctx, TtCallStack& stackOut);
    __TELEM_API_VOID(TelemSendCallStack, TraceContexHandle ctx, const TtCallStack* pStack);

    // Zones
    __TELEM_API_VOID(TelemEnter, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pZoneName);
    __TELEM_API_VOID(TelemEnterEx, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pZoneName);
    __TELEM_API_VOID(TelemLeave, TraceContexHandle ctx);
    __TELEM_API_VOID(TelemLeaveEx, TraceContexHandle ctx, tt_uint64 matchId);

    // Lock util
    __TELEM_API_VOID(TelemSetLockName, TraceContexHandle ctx, const void* pPtr, const char* pLockName);
    __TELEM_API_VOID(TelemTryLock, TraceContexHandle ctx, const void* pPtr, const char* pDescription);
    __TELEM_API_VOID(TelemTryLockEx, TraceContexHandle ctx, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const void* pPtr, const char* pDescription);
    __TELEM_API_VOID(TelemEndTryLock, TraceContexHandle ctx, const void* pPtr, TtLockResult result);
    __TELEM_API_VOID(TelemEndTryLockEx, TraceContexHandle ctx, tt_uint64 matchId, const void* pPtr, TtLockResult result);
    __TELEM_API_VOID(TelemSetLockState, TraceContexHandle ctx, const void* pPtr, TtLockState state);
    __TELEM_API_VOID(TelemSignalLockCount, TraceContexHandle ctx, const void* pPtr, tt_int32 count);

    // Some allocation tracking.
    __TELEM_API_VOID(TelemAlloc, TraceContexHandle ctx, void* pPtr, tt_size size);
    __TELEM_API_VOID(TelemFree, TraceContexHandle ctx, void* pPtr);

    __TELEM_API_VOID(TelemPlot, TraceContexHandle ctx, TtPlotType type, float value, const char* pName);
    __TELEM_API_VOID(TelemPlotF32, TraceContexHandle ctx, TtPlotType type, float value, const char* pName);
    __TELEM_API_VOID(TelemPlotF64, TraceContexHandle ctx, TtPlotType type, double value, const char* pName);
    __TELEM_API_VOID(TelemPlotI32, TraceContexHandle ctx, TtPlotType type, tt_int32 value, const char* pName);
    __TELEM_API_VOID(TelemPlotU32, TraceContexHandle ctx, TtPlotType type, tt_int64 value, const char* pName);
    __TELEM_API_VOID(TelemPlotI64, TraceContexHandle ctx, TtPlotType type, tt_uint32 value, const char* pName);
    __TELEM_API_VOID(TelemPlotU64, TraceContexHandle ctx, TtPlotType type, tt_uint64 value, const char* pName);

#pragma warning( pop )

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#ifdef __cplusplus

namespace telem
{
    using ContexHandle = TraceContexHandle;
    using ConnectionType = TtConnectionType;
    using LockResult = TtLockResult;
    using LockState = TtLockState;
    using LockState = TtLockState;
    using PlotType = TtPlotType;
    using Error = TtError;

#if TTELEMETRY_LINK
#define __TELEM_FUNC_NAME(name) name
#else
#define __TELEM_FUNC_NAME(name) telem::gTelemApi.p##name

    struct TelemetryAPI
    {
        #define __TELEM_RESOLVE(name) p##name= (decltype(name)*)::GetProcAddress(hLib_, # name); if(!p##name) { return false; }
        #define __TELEM_SET_BLANK(name) p##name= __blank##name;
        #define __TELEM_FUNC_PTR(name) decltype(name)* p##name;

        TelemetryAPI() {
            memset(this, 0, sizeof(*this));
            setBlank();
        }

        bool loadModule()
        {
            hLib_ = ::LoadLibraryW(L"engine_TelemetryLib.dll");
            if (!hLib_) {
                return false;
            }

            __TELEM_RESOLVE(TelemInit);
            __TELEM_RESOLVE(TelemShutDown);
            __TELEM_RESOLVE(TelemInitializeContext);
            __TELEM_RESOLVE(TelemShutdownContext);
            __TELEM_RESOLVE(TelemSetContextLogFunc);
            __TELEM_RESOLVE(TelemOpen);
            __TELEM_RESOLVE(TelemClose);
            __TELEM_RESOLVE(TelemTick);
            __TELEM_RESOLVE(TelemFlush);
            __TELEM_RESOLVE(TelemUpdateSymbolData);
            __TELEM_RESOLVE(TelemPause);
            __TELEM_RESOLVE(TelemIsPaused);
            __TELEM_RESOLVE(TelemSetThreadName);
            __TELEM_RESOLVE(TelemEnter);
            __TELEM_RESOLVE(TelemEnterEx);
            __TELEM_RESOLVE(TelemLeave);
            __TELEM_RESOLVE(TelemLeaveEx);
            __TELEM_RESOLVE(TelemSetLockName);
            __TELEM_RESOLVE(TelemTryLock);
            __TELEM_RESOLVE(TelemTryLockEx);
            __TELEM_RESOLVE(TelemEndTryLock);
            __TELEM_RESOLVE(TelemEndTryLockEx);
            __TELEM_RESOLVE(TelemSetLockState);
            __TELEM_RESOLVE(TelemSignalLockCount);
            __TELEM_RESOLVE(TelemAlloc);
            __TELEM_RESOLVE(TelemFree);
            return true;
        }

        void setBlank()
        {
            __TELEM_SET_BLANK(TelemInit);
            __TELEM_SET_BLANK(TelemShutDown);
            __TELEM_SET_BLANK(TelemInitializeContext);
            __TELEM_SET_BLANK(TelemShutdownContext);
            __TELEM_SET_BLANK(TelemSetContextLogFunc);
            __TELEM_SET_BLANK(TelemOpen);
            __TELEM_SET_BLANK(TelemClose);
            __TELEM_SET_BLANK(TelemTick);
            __TELEM_SET_BLANK(TelemFlush);
            __TELEM_SET_BLANK(TelemUpdateSymbolData);
            __TELEM_SET_BLANK(TelemPause);
            __TELEM_SET_BLANK(TelemIsPaused);
            __TELEM_SET_BLANK(TelemSetThreadName);
            __TELEM_SET_BLANK(TelemEnter);
            __TELEM_SET_BLANK(TelemEnterEx);
            __TELEM_SET_BLANK(TelemLeave);
            __TELEM_SET_BLANK(TelemLeaveEx);
            __TELEM_SET_BLANK(TelemSetLockName);
            __TELEM_SET_BLANK(TelemTryLock);
            __TELEM_SET_BLANK(TelemTryLockEx);
            __TELEM_SET_BLANK(TelemEndTryLock);
            __TELEM_SET_BLANK(TelemEndTryLockEx);
            __TELEM_SET_BLANK(TelemSetLockState);
            __TELEM_SET_BLANK(TelemSignalLockCount);
            __TELEM_SET_BLANK(TelemAlloc);
            __TELEM_SET_BLANK(TelemFree);
        }

        void unLoad()
        {
            if (hLib_) {
                ::FreeLibrary(hLib_);
            }

            memset(this, 0, sizeof(*this));
        }

        __TELEM_FUNC_PTR(TelemInit);
        __TELEM_FUNC_PTR(TelemShutDown);
        __TELEM_FUNC_PTR(TelemInitializeContext);
        __TELEM_FUNC_PTR(TelemShutdownContext);
        __TELEM_FUNC_PTR(TelemSetContextLogFunc);
        __TELEM_FUNC_PTR(TelemOpen);
        __TELEM_FUNC_PTR(TelemClose);
        __TELEM_FUNC_PTR(TelemTick);
        __TELEM_FUNC_PTR(TelemFlush);
        __TELEM_FUNC_PTR(TelemUpdateSymbolData);
        __TELEM_FUNC_PTR(TelemPause);
        __TELEM_FUNC_PTR(TelemIsPaused);
        __TELEM_FUNC_PTR(TelemSetThreadName);
        __TELEM_FUNC_PTR(TelemEnter);
        __TELEM_FUNC_PTR(TelemEnterEx);
        __TELEM_FUNC_PTR(TelemLeave);
        __TELEM_FUNC_PTR(TelemLeaveEx);
        __TELEM_FUNC_PTR(TelemSetLockName);
        __TELEM_FUNC_PTR(TelemTryLock);
        __TELEM_FUNC_PTR(TelemTryLockEx);
        __TELEM_FUNC_PTR(TelemEndTryLock);
        __TELEM_FUNC_PTR(TelemEndTryLockEx);
        __TELEM_FUNC_PTR(TelemSetLockState);
        __TELEM_FUNC_PTR(TelemSignalLockCount);
        __TELEM_FUNC_PTR(TelemAlloc);
        __TELEM_FUNC_PTR(TelemFree);

    private:
        HMODULE hLib_;
    };

    // if you have multiple dll's this will be diffrent.
    // so you would have to call it in each dll init.
    // but also means you can conditionally enable telemetry for various modules.
    // it's not safe to resole functions during a zone.
    static TelemetryAPI gTelemApi;

#endif // TTELEMETRY_LINK

    struct ScopedZone
    {
        inline ScopedZone(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pLabel) :
            ctx_(ctx)
        {
            __TELEM_FUNC_NAME(TelemEnter)(ctx, sourceInfo, pLabel);
        }

        inline ~ScopedZone() {
            __TELEM_FUNC_NAME(TelemLeave)(ctx_);
        }

    private:
        TraceContexHandle ctx_;
    };

    struct ScopedZoneFilterd
    {
        inline ScopedZoneFilterd(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64 minMicroSec, const char* pLabel) :
            ctx_(ctx)
        {
            __TELEM_FUNC_NAME(TelemEnterEx)(ctx, sourceInfo, matchId_, minMicroSec, pLabel);
        }

        inline ~ScopedZoneFilterd() {
            __TELEM_FUNC_NAME(TelemLeaveEx)(ctx_, matchId_);
        }

    private:
        TraceContexHandle ctx_;
        tt_uint64 matchId_;
    };


} // namespace telem

#endif // __cplusplus

#if TTELEMETRY_ENABLED

#if TTELEMETRY_LINK
#define ttLoadLibary() true
#else
#define ttLoadLibary() telem::gTelemApi.loadModule()
#endif // TTELEMETRY_LINK

#define ttZone(ctx, pLabel) telem::ScopedZone __TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, TT_SOURCE_INFO, pLabel);
#define ttZoneFilterd(ctx, minMicroSec, pLabel) telem::ScopedZoneFilterd __TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, TT_SOURCE_INFO, minMicroSec, pLabel);

#define ttInit() __TELEM_FUNC_NAME(TelemInit)()
#define ttShutDown() __TELEM_FUNC_NAME(TelemShutDown)()

// Context
#define ttInitializeContext(out, pBuf, bufLen) __TELEM_FUNC_NAME(TelemInitializeContext)(out, pBuf, bufLen)
#define ttShutdownContext(ctx) __TELEM_FUNC_NAME(TelemShutdownContext)(ctx)

#define ttSetContextLogFunc(ctx, func, pUserData) __TELEM_FUNC_NAME(TelemSetContextLogFunc)(ctx, func, pUserData);

#define ttOpen(ctx, pAppName, pBuildInfo, pServerAddress, serverPort, conType, timeoutMS) \
    __TELEM_FUNC_NAME(TelemOpen)(ctx, pAppName, pBuildInfo, pServerAddress, serverPort, conType, timeoutMS)

#define ttClose(ctx) __TELEM_FUNC_NAME(TelemClose)(ctx)

#define ttTick(ctx) __TELEM_FUNC_NAME(TelemTick)(ctx)
#define ttFlush(ctx) __TELEM_FUNC_NAME(TelemFlush)(ctx)
#define ttUpdateSymbolData(ctx) __TELEM_FUNC_NAME(TelemUpdateSymbolData)(ctx)

#define ttPause(ctx, pause) __TELEM_FUNC_NAME(TelemPause)(ctx, pause)
#define ttIsPaused(ctx) __TELEM_FUNC_NAME(TelemIsPaused)(ctx)

// Thread
#define ttSetThreadName(ctx, threadID, pName) TelemSetThreadName(ctx, threadID, pName);

#define ttGetCallStack(ctx, stackOut) TelemGetCallStack(ctx, stackOut)
#define ttSendCallStack(ctx, pStack) TelemSendCallStack(ctx, pStack)

// Zones
#define ttEnter(ctx, pZoneName) __TELEM_FUNC_NAME(TelemEnter)(ctx, TT_SOURCE_INFO, pZoneName);
#define ttEnterEx(ctx, matchIdOut, minMicroSec, pZoneName) __TELEM_FUNC_NAME(TelemEnterEx)(ctx, TT_SOURCE_INFO, matchIdOut, minMicroSec, pZoneName);
#define ttLeave(ctx) __TELEM_FUNC_NAME(TelemLeave)(ctx);
#define ttLeaveEx(ctx, matchId) __TELEM_FUNC_NAME(TelemLeaveEx)(ctx, matchId);


// Lock util
#define ttSetLockName(ctx, pPtr, pLockName) __TELEM_FUNC_NAME(TelemSetLockName)(ctx, pPtr, pLockName);
#define ttTryLock(ctx, pPtr, pDescription) __TELEM_FUNC_NAME(TelemTryLock)(ctx, pPtr, pDescription);
#define ttTryLockEx(ctx, matchIdOut, minMicroSec, pPtr, pDescription) __TELEM_FUNC_NAME(TelemTryLock)(ctx, matchIdOut, minMicroSec, pPtr, pDescription);
#define ttEndTryLock(ctx, pPtr, result) __TELEM_FUNC_NAME(TelemEndTryLock)(ctx, pPtr, result);
#define ttEndTryLockEx(ctx, matchIdOut, pPtr, result) __TELEM_FUNC_NAME(TelemEndTryLockEx)(ctx, matchIdOut, pPtr, result);
#define ttSetLockState(ctx, pPtr, state) __TELEM_FUNC_NAME(TelemSetLockState)(ctx, pPtr, state);
#define ttSignalLockCount(ctx, pPtr, count) __TELEM_FUNC_NAME(TelemSignalLockCount)(ctx, pPtr, count);

// Some allocation tracking.
#define ttAlloc(ctx, pPtr, size) __TELEM_FUNC_NAME(TelemAlloc)(ctx, pPtr, size);
#define ttFree(ctx, pPtr) __TELEM_FUNC_NAME(TelemFree)(ctx, pPtr);

#define ttPlot(ctx, type, value, pName);
#define ttPlotF32(ctx, type, value, pName);
#define ttPlotF64(ctx, type, value, pName);
#define ttPlotI32(ctx, type, value, pName);
#define ttPlotU32(ctx, type, value, pName);
#define ttPlotI64(ctx, type, value, pName);
#define ttPlotU64(ctx, type, value, pName);


#else // TTELEMETRY_ENABLED

#define ttLoadLibary() true

#define ttZone(...)
#define ttZoneFilterd(...)

#define ttInit() true
#define ttShutDown() 

#define ttSetContextLogFunc(...)

// Context
#define ttInitializeContext(...)
#define ttShutdownContext(...)

#define ttOpen(...)

#define ttClose(...)

#define ttTick(...)
#define ttFlush(...)
#define ttUpdateSymbolData(...)

#define ttPause(...)
#define ttIsPaused(...)

// Thread
#define ttSetThreadName(...);

#define ttGetCallStack(...)
#define ttSendCallStack(...)

// Zones
#define ttEnter(...);
#define ttEnterEx(...);
#define ttLeave(...);
#define ttLeaveEx(...);


// Lock util
#define ttSetLockName(...);
#define ttTryLock(...);
#define ttEndTryLock(...);
#define ttSetLockState(...);
#define ttSignalLockCount(...);

#endif // TTELEMETRY_ENABLED