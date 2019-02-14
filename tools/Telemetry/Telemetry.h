#pragma once

#include <../TelemetryCommon/Types.h>

#define TTELEMETRY_ENABLED 1
#define TTELEMETRY_LINK 1

#define X_TELEMETRY_UNIQUE_NAME_HELPER_0(_0, _1)  _0##_1
#define X_TELEMETRY_UNIQUE_NAME_HELPER(_0, _1) X_TELEMETRY_UNIQUE_NAME_HELPER_0(_0, _1)
#define X_TELEMETRY_UNIQUE_NAME(name) X_TELEMETRY_UNIQUE_NAME_HELPER(name, __LINE__)

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
    // meow
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
#define TELEM_API_BLANK(func) 
#else
#define TELEM_API_BLANK(func) func
#endif

#define TELEM_API_VOID(name, ...) TELEMETRYLIB_EXPORT void  name(__VA_ARGS__); \
        TELEM_API_BLANK(inline void __blank##name(__VA_ARGS__) {})

#define TELEM_API_BOOL(name, ...) TELEMETRYLIB_EXPORT bool name(__VA_ARGS__); \
        TELEM_API_BLANK(inline bool __blank##name(__VA_ARGS__) { return true; })

#define TELEM_API_ERR(name, ...) TELEMETRYLIB_EXPORT TtError name(__VA_ARGS__); \
        TELEM_API_BLANK(inline TtError __blank##name(__VA_ARGS__) { return TtError::Ok; })

#pragma warning( push )
#pragma warning(disable: 4100) // unused param (caused by the blank functions).

    TELEM_API_BOOL(TelemInit);
    TELEM_API_VOID(TelemShutDown);

    // Context
    TELEM_API_ERR(TelemInitializeContext, TraceContexHandle& ctx, void* pArena, tt_size bufLen);
    TELEM_API_VOID(TelemShutdownContext, TraceContexHandle ctx);

    TELEM_API_VOID(TelemSetContextLogFunc, TraceContexHandle ctx, LogFunction func, void* pUserData);

    TELEM_API_ERR(TelemOpen, TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
        tt_uint16 serverPort, TtConnectionType conType, tt_int32 timeoutMS);

    TELEM_API_BOOL(TelemClose, TraceContexHandle ctx);

    TELEM_API_VOID(TelemTick, TraceContexHandle ctx);
    TELEM_API_VOID(TelemFlush, TraceContexHandle ctx);
    TELEM_API_VOID(TelemUpdateSymbolData, TraceContexHandle ctx);

    TELEM_API_VOID(TelemPause, TraceContexHandle ctx, bool pause);
    TELEM_API_BOOL(TelemIsPaused, TraceContexHandle ctx);

    // Thread
    TELEM_API_VOID(TelemSetThreadName, TraceContexHandle ctx, tt_uint32 threadID, const char* pName);

    // Callstack
    TELEM_API_BOOL(TelemGetCallStack, TraceContexHandle ctx, TtCallStack& stackOut);
    TELEM_API_BOOL(TelemSendCallStack, TraceContexHandle ctx, const TtCallStack& stackOut);

    // Zones
    TELEM_API_VOID(TelemEnter, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pZoneName);
    TELEM_API_VOID(TelemEnterEx, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pZoneName);
    TELEM_API_VOID(TelemLeave, TraceContexHandle ctx);
    TELEM_API_VOID(TelemLeaveEx, TraceContexHandle ctx, tt_uint64 matchId);

    // Lock util
    TELEM_API_VOID(TelemSetLockName, TraceContexHandle ctx, const void* pPtr, const char* pLockName);
    TELEM_API_VOID(TelemTryLock, TraceContexHandle ctx, const void* pPtr, const char* pDescription);
    TELEM_API_VOID(TelemTryLockEx, TraceContexHandle ctx, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const void* pPtr, const char* pDescription);
    TELEM_API_VOID(TelemEndTryLock, TraceContexHandle ctx, const void* pPtr, TtLockResult result);
    TELEM_API_VOID(TelemEndTryLockEx, TraceContexHandle ctx, tt_uint64 matchId, const void* pPtr, TtLockResult result);
    TELEM_API_VOID(TelemSetLockState, TraceContexHandle ctx, const void* pPtr, TtLockState state);
    TELEM_API_VOID(TelemSignalLockCount, TraceContexHandle ctx, const void* pPtr, tt_int32 count);

    // Some allocation tracking.
    TELEM_API_VOID(TelemAlloc, TraceContexHandle ctx, void* pPtr, tt_size size);
    TELEM_API_VOID(TelemFree, TraceContexHandle ctx, void* pPtr);

    TELEM_API_VOID(TelemPlot, TraceContexHandle ctx, TtPlotType type, float value, const char* pName);
    TELEM_API_VOID(TelemPlotF32, TraceContexHandle ctx, TtPlotType type, float value, const char* pName);
    TELEM_API_VOID(TelemPlotF64, TraceContexHandle ctx, TtPlotType type, double value, const char* pName);
    TELEM_API_VOID(TelemPlotI32, TraceContexHandle ctx, TtPlotType type, tt_int32 value, const char* pName);
    TELEM_API_VOID(TelemPlotU32, TraceContexHandle ctx, TtPlotType type, tt_int64 value, const char* pName);
    TELEM_API_VOID(TelemPlotI64, TraceContexHandle ctx, TtPlotType type, tt_uint32 value, const char* pName);
    TELEM_API_VOID(TelemPlotU64, TraceContexHandle ctx, TtPlotType type, tt_uint64 value, const char* pName);

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
#define TELEM_FUNC_NAME(name) name
#else
#define TELEM_FUNC_NAME(name) telem::gTelemApi.p##name

    struct TelemetryAPI
    {
        #define TELEM_RESOLVE(name) p##name= (decltype(name)*)::GetProcAddress(hLib_, # name); if(!p##name) { return false; }
        #define TELEM_SET_BLANK(name) p##name= __blank##name;
        #define TELEM_FUNC_PTR(name) decltype(name)* p##name;

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

            TELEM_RESOLVE(TelemInit);
            TELEM_RESOLVE(TelemShutDown);
            TELEM_RESOLVE(TelemInitializeContext);
            TELEM_RESOLVE(TelemShutdownContext);
            TELEM_RESOLVE(TelemSetContextLogFunc);
            TELEM_RESOLVE(TelemOpen);
            TELEM_RESOLVE(TelemClose);
            TELEM_RESOLVE(TelemTick);
            TELEM_RESOLVE(TelemFlush);
            TELEM_RESOLVE(TelemUpdateSymbolData);
            TELEM_RESOLVE(TelemPause);
            TELEM_RESOLVE(TelemIsPaused);
            TELEM_RESOLVE(TelemSetThreadName);
            TELEM_RESOLVE(TelemEnter);
            TELEM_RESOLVE(TelemEnterEx);
            TELEM_RESOLVE(TelemLeave);
            TELEM_RESOLVE(TelemLeaveEx);
            TELEM_RESOLVE(TelemSetLockName);
            TELEM_RESOLVE(TelemTryLock);
            TELEM_RESOLVE(TelemTryLockEx);
            TELEM_RESOLVE(TelemEndTryLock);
            TELEM_RESOLVE(TelemEndTryLockEx);
            TELEM_RESOLVE(TelemSetLockState);
            TELEM_RESOLVE(TelemSignalLockCount);
            TELEM_RESOLVE(TelemAlloc);
            TELEM_RESOLVE(TelemFree);
            return true;
        }

        void setBlank()
        {
            TELEM_SET_BLANK(TelemInit);
            TELEM_SET_BLANK(TelemShutDown);
            TELEM_SET_BLANK(TelemInitializeContext);
            TELEM_SET_BLANK(TelemShutdownContext);
            TELEM_SET_BLANK(TelemSetContextLogFunc);
            TELEM_SET_BLANK(TelemOpen);
            TELEM_SET_BLANK(TelemClose);
            TELEM_SET_BLANK(TelemTick);
            TELEM_SET_BLANK(TelemFlush);
            TELEM_SET_BLANK(TelemUpdateSymbolData);
            TELEM_SET_BLANK(TelemPause);
            TELEM_SET_BLANK(TelemIsPaused);
            TELEM_SET_BLANK(TelemSetThreadName);
            TELEM_SET_BLANK(TelemEnter);
            TELEM_SET_BLANK(TelemEnterEx);
            TELEM_SET_BLANK(TelemLeave);
            TELEM_SET_BLANK(TelemLeaveEx);
            TELEM_SET_BLANK(TelemSetLockName);
            TELEM_SET_BLANK(TelemTryLock);
            TELEM_SET_BLANK(TelemTryLockEx);
            TELEM_SET_BLANK(TelemEndTryLock);
            TELEM_SET_BLANK(TelemEndTryLockEx);
            TELEM_SET_BLANK(TelemSetLockState);
            TELEM_SET_BLANK(TelemSignalLockCount);
            TELEM_SET_BLANK(TelemAlloc);
            TELEM_SET_BLANK(TelemFree);
        }

        void unLoad()
        {
            if (hLib_) {
                ::FreeLibrary(hLib_);
            }

            memset(this, 0, sizeof(*this));
        }

        TELEM_FUNC_PTR(TelemInit);
        TELEM_FUNC_PTR(TelemShutDown);
        TELEM_FUNC_PTR(TelemInitializeContext);
        TELEM_FUNC_PTR(TelemShutdownContext);
        TELEM_FUNC_PTR(TelemSetContextLogFunc);
        TELEM_FUNC_PTR(TelemOpen);
        TELEM_FUNC_PTR(TelemClose);
        TELEM_FUNC_PTR(TelemTick);
        TELEM_FUNC_PTR(TelemFlush);
        TELEM_FUNC_PTR(TelemUpdateSymbolData);
        TELEM_FUNC_PTR(TelemPause);
        TELEM_FUNC_PTR(TelemIsPaused);
        TELEM_FUNC_PTR(TelemSetThreadName);
        TELEM_FUNC_PTR(TelemEnter);
        TELEM_FUNC_PTR(TelemEnterEx);
        TELEM_FUNC_PTR(TelemLeave);
        TELEM_FUNC_PTR(TelemLeaveEx);
        TELEM_FUNC_PTR(TelemSetLockName);
        TELEM_FUNC_PTR(TelemTryLock);
        TELEM_FUNC_PTR(TelemTryLockEx);
        TELEM_FUNC_PTR(TelemEndTryLock);
        TELEM_FUNC_PTR(TelemEndTryLockEx);
        TELEM_FUNC_PTR(TelemSetLockState);
        TELEM_FUNC_PTR(TelemSignalLockCount);
        TELEM_FUNC_PTR(TelemAlloc);
        TELEM_FUNC_PTR(TelemFree);

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
            TELEM_FUNC_NAME(TelemEnter)(ctx, sourceInfo, pLabel);
        }

        inline ~ScopedZone() {
            TELEM_FUNC_NAME(TelemLeave)(ctx_);
        }

    private:
        TraceContexHandle ctx_;
    };

    struct ScopedZoneFilterd
    {
        inline ScopedZoneFilterd(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64 minMicroSec, const char* pLabel) :
            ctx_(ctx)
        {
            TELEM_FUNC_NAME(TelemEnterEx)(ctx, sourceInfo, matchId_, minMicroSec, pLabel);
        }

        inline ~ScopedZoneFilterd() {
            TELEM_FUNC_NAME(TelemLeaveEx)(ctx_, matchId_);
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

#define ttZone(ctx, pLabel) telem::ScopedZone X_TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, TT_SOURCE_INFO, pLabel);
#define ttZoneFilterd(ctx, minMicroSec, pLabel) telem::ScopedZoneFilterd X_TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, TT_SOURCE_INFO, minMicroSec, pLabel);

#define ttInit() TELEM_FUNC_NAME(TelemInit)()
#define ttShutDown() TELEM_FUNC_NAME(TelemShutDown)()

// Context
#define ttInitializeContext(out, pBuf, bufLen) TELEM_FUNC_NAME(TelemInitializeContext)(out, pBuf, bufLen)
#define ttShutdownContext(ctx) TELEM_FUNC_NAME(TelemShutdownContext)(ctx)

#define ttSetContextLogFunc(ctx, func, pUserData) TELEM_FUNC_NAME(TelemSetContextLogFunc)(ctx, func, pUserData);

#define ttOpen(ctx, pAppName, pBuildInfo, pServerAddress, serverPort, conType, timeoutMS) \
    TELEM_FUNC_NAME(TelemOpen)(ctx, pAppName, pBuildInfo, pServerAddress, serverPort, conType, timeoutMS)

#define ttClose(ctx) TELEM_FUNC_NAME(TelemClose)(ctx)

#define ttTick(ctx) TELEM_FUNC_NAME(TelemTick)(ctx)
#define ttFlush(ctx) TELEM_FUNC_NAME(TelemFlush)(ctx)
#define ttUpdateSymbolData(ctx) TELEM_FUNC_NAME(TelemUpdateSymbolData)(ctx)

#define ttPause(ctx, pause) TELEM_FUNC_NAME(TelemPause)(ctx, pause)
#define ttIsPaused(ctx) TELEM_FUNC_NAME(TelemIsPaused)(ctx)

// Thread
#define ttSetThreadName(ctx, threadID, pName);

// Zones
#define ttEnter(ctx, pZoneName) TELEM_FUNC_NAME(TelemEnter)(ctx, TT_SOURCE_INFO, pZoneName);
#define ttEnterEx(ctx, matchIdOut, minMicroSec, pZoneName) TELEM_FUNC_NAME(TelemEnterEx)(ctx, TT_SOURCE_INFO, matchIdOut, minMicroSec, pZoneName);
#define ttLeave(ctx) TELEM_FUNC_NAME(TelemLeave)(ctx);
#define ttLeaveEx(ctx, matchId) TELEM_FUNC_NAME(TelemLeaveEx)(ctx, matchId);


// Lock util
#define ttSetLockName(ctx, pPtr, pLockName) TELEM_FUNC_NAME(TelemSetLockName)(ctx, pPtr, pLockName);
#define ttTryLock(ctx, pPtr, pDescription) TELEM_FUNC_NAME(TelemTryLock)(ctx, pPtr, pDescription);
#define ttTryLockEx(ctx, matchIdOut, minMicroSec, pPtr, pDescription) TELEM_FUNC_NAME(TelemTryLock)(ctx, matchIdOut, minMicroSec, pPtr, pDescription);
#define ttEndTryLock(ctx, pPtr, result) TELEM_FUNC_NAME(TelemEndTryLock)(ctx, pPtr, result);
#define ttEndTryLockEx(ctx, matchIdOut, pPtr, result) TELEM_FUNC_NAME(TelemEndTryLockEx)(ctx, matchIdOut, pPtr, result);
#define ttSetLockState(ctx, pPtr, state) TELEM_FUNC_NAME(TelemSetLockState)(ctx, pPtr, state);
#define ttSignalLockCount(ctx, pPtr, count) TELEM_FUNC_NAME(TelemSignalLockCount)(ctx, pPtr, count);

// Some allocation tracking.
#define ttAlloc(ctx, pPtr, size) TELEM_FUNC_NAME(TelemAlloc)(ctx, pPtr, size);
#define ttFree(ctx, pPtr) TELEM_FUNC_NAME(TelemFree)(ctx, pPtr);

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