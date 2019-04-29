#pragma once

#include "../TelemetryCommon/Types.h"
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

#ifdef _MSC_VER

#define __TELEM_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#define __TELEM_EXPAND(x) x
#define __TELEM_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#define __TELEM_EXPAND_ARGS_PRIVATE(...) __TELEM_EXPAND(__TELEM_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define __TELEM_ARG_COUNT(...)  __TELEM_EXPAND_ARGS_PRIVATE(__TELEM_ARGS_AUGMENTER(__VA_ARGS__))

#else 

#define __TELEM_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#define __TELEM_ARG_COUNT(...) __TELEM_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#endif // _MSC_VER


struct TtLogType
{
    enum Enum : tt_uint8
    {
        Msg,
        Warning,
        Error
    };
};

typedef TtLogType TtMsgType;

struct TTFlag
{
    enum Enum : tt_uint8
    {
        DropData = 1, // Used for profiling overhead, data is not sent to server.
    };
};

// IO callbacks.
using FileOpenFunc = TtFileHandle(*)(const char*);
using FileCloseFunc = void(*)(TtFileHandle);
using FileWriteFunc = tt_int32(*)(TtFileHandle, const void*, tt_int32);

inline TtFileHandle TELEM_INVALID_HANDLE = 0;


using LogFunction = void(*)(void* pUserData, TtLogType::Enum type, const char* pMsgNullTerm, tt_int32 lenWithoutTerm);
using TraceContexHandle = tt_uintptr;

inline TraceContexHandle INVALID_TRACE_CONTEX = 0;

enum TtConnectionType
{
    Tcp,
    File
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
    static const tt_uint32 MAX_FRAMES = 31;

    TtCallStack() {
        id = -1;
        num = 0;
    }

    tt_int32 id;
    tt_int32 num;
    void* frames[MAX_FRAMES];
};

TELEM_PACK_PUSH(4)

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

TELEM_PACK_POP;

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

#define __TELEM_API_INT(name, ...) TELEMETRYLIB_EXPORT tt_int32 name(__VA_ARGS__); \
        __TELEM_API_BLANK(inline tt_int32 __blank##name(__VA_ARGS__) { return -1; })

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
        TtConnectionType conType, tt_uint16 serverPort, tt_int32 timeoutMS);

    __TELEM_API_BOOL(TelemClose, TraceContexHandle ctx);

    __TELEM_API_VOID(TelemTick, TraceContexHandle ctx);
    __TELEM_API_VOID(TelemFlush, TraceContexHandle ctx);
    __TELEM_API_VOID(TelemUpdateSymbolData, TraceContexHandle ctx);

    __TELEM_API_VOID(TelemPause, TraceContexHandle ctx, bool pause);
    __TELEM_API_BOOL(TelemIsPaused, TraceContexHandle ctx);

    __TELEM_API_VOID(TelemSetFlag, TraceContexHandle ctx, TTFlag::Enum flag, bool set);

    // Thread
    __TELEM_API_VOID(TelemSetThreadName, TraceContexHandle ctx, tt_uint32 threadID, const char* pFmtString, tt_int32 numArgs, ...);

    // Callstack
    __TELEM_API_INT(TelemGetCallStack, TraceContexHandle ctx, TtCallStack& stackOut);
    __TELEM_API_INT(TelemSendCallStack, TraceContexHandle ctx, const TtCallStack* pStack);
    __TELEM_API_VOID(TelemSendCallStackSkip, TraceContexHandle ctx, const TtCallStack* pStack, tt_int32 numToSkip);

    // Zones
    __TELEM_API_VOID(TelemEnter, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemEnterEx, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemLeave, TraceContexHandle ctx);
    __TELEM_API_VOID(TelemLeaveEx, TraceContexHandle ctx, tt_uint64 matchId);

    // Lock util
    __TELEM_API_VOID(TelemSetLockName, TraceContexHandle ctx, const void* pPtr, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemTryLock, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const void* pPtr, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemTryLockEx, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const void* pPtr, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemEndTryLock, TraceContexHandle ctx, const void* pPtr, TtLockResult::Enum result);
    __TELEM_API_VOID(TelemEndTryLockEx, TraceContexHandle ctx, tt_uint64 matchId, const void* pPtr, TtLockResult::Enum result);
    __TELEM_API_VOID(TelemSetLockState, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const void* pPtr, TtLockState::Enum state);
    __TELEM_API_VOID(TelemSignalLockCount, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const void* pPtr, tt_int32 count);

    // Some allocation tracking.
    __TELEM_API_VOID(TelemAlloc, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, void* pPtr, tt_size allocSize, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemFree, TraceContexHandle ctx, const TtSourceInfo& sourceInfo, void* pPtr);

    __TELEM_API_VOID(TelemPlotF32, TraceContexHandle ctx, TtPlotType::Enum type, float value, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemPlotF64, TraceContexHandle ctx, TtPlotType::Enum type, double value, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemPlotI32, TraceContexHandle ctx, TtPlotType::Enum type, tt_int32 value, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemPlotI64, TraceContexHandle ctx, TtPlotType::Enum type, tt_int64 value, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemPlotU32, TraceContexHandle ctx, TtPlotType::Enum type, tt_uint32 value, const char* pFmtString, tt_int32 numArgs, ...);
    __TELEM_API_VOID(TelemPlotU64, TraceContexHandle ctx, TtPlotType::Enum type, tt_uint64 value, const char* pFmtString, tt_int32 numArgs, ...);

    __TELEM_API_VOID(TelemMessage, TraceContexHandle ctx, TtMsgType::Enum type, const char* pFmtString, tt_int32 numArgs, ...);

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
            __TELEM_RESOLVE(TelemSetFlag);
            __TELEM_RESOLVE(TelemSetThreadName);
            __TELEM_RESOLVE(TelemGetCallStack);
            __TELEM_RESOLVE(TelemSendCallStack);
            __TELEM_RESOLVE(TelemSendCallStackSkip);
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
            __TELEM_RESOLVE(TelemPlotF32);
            __TELEM_RESOLVE(TelemPlotF64);
            __TELEM_RESOLVE(TelemPlotI32);
            __TELEM_RESOLVE(TelemPlotI64);
            __TELEM_RESOLVE(TelemPlotU32);
            __TELEM_RESOLVE(TelemPlotU64);
            __TELEM_RESOLVE(TelemMessage);
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
            __TELEM_SET_BLANK(TelemSetFlag);
            __TELEM_SET_BLANK(TelemSetThreadName);
            __TELEM_SET_BLANK(TelemGetCallStack);
            __TELEM_SET_BLANK(TelemSendCallStack);
            __TELEM_SET_BLANK(TelemSendCallStackSkip);
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
            __TELEM_SET_BLANK(TelemPlotF32);
            __TELEM_SET_BLANK(TelemPlotF64);
            __TELEM_SET_BLANK(TelemPlotI32);
            __TELEM_SET_BLANK(TelemPlotI64);
            __TELEM_SET_BLANK(TelemPlotU32);
            __TELEM_SET_BLANK(TelemPlotU64);
            __TELEM_SET_BLANK(TelemMessage);
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
        __TELEM_FUNC_PTR(TelemSetFlag);
        __TELEM_FUNC_PTR(TelemSetThreadName);
        __TELEM_FUNC_PTR(TelemGetCallStack);
        __TELEM_FUNC_PTR(TelemSendCallStack);
        __TELEM_FUNC_PTR(TelemSendCallStackSkip);
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
        __TELEM_FUNC_PTR(TelemPlotF32);
        __TELEM_FUNC_PTR(TelemPlotF64);
        __TELEM_FUNC_PTR(TelemPlotI32);
        __TELEM_FUNC_PTR(TelemPlotI64);
        __TELEM_FUNC_PTR(TelemPlotU32);
        __TELEM_FUNC_PTR(TelemPlotU64);
        __TELEM_FUNC_PTR(TelemMessage);

    private:
        HMODULE hLib_;
    };

    // if you have multiple dll's this will be diffrent.
    // so you would have to call it in each dll init.
    // but also means you can conditionally enable telemetry for various modules.
    // it's not safe to resole functions during a zone for the same module.
    static TelemetryAPI gTelemApi;

#endif // TTELEMETRY_LINK

    struct ScopedZone
    {
        // Is having this overload helpful for compile time?
        inline ScopedZone(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pFormat) :
            ctx_(ctx)
        {
            __TELEM_FUNC_NAME(TelemEnter)(ctx, sourceInfo, pFormat, 0);
        }

        template<typename ... Args>
        inline ScopedZone(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pFormat, Args&& ... args) :
            ctx_(ctx)
        {
            const std::size_t num = sizeof...(Args);
            __TELEM_FUNC_NAME(TelemEnter)(ctx, sourceInfo, pFormat, num, std::forward<Args>(args) ...);
        }

        inline ~ScopedZone() {
            __TELEM_FUNC_NAME(TelemLeave)(ctx_);
        }

    private:
        TraceContexHandle ctx_;
    };

    struct ScopedZoneFilterd
    {
        // Is having this overload helpful for compile time?
        inline ScopedZoneFilterd(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64 minMicroSec, const char* pFormat) :
            ctx_(ctx)
        {
            __TELEM_FUNC_NAME(TelemEnterEx)(ctx, sourceInfo, matchId_, minMicroSec, pFormat, 0);
        }

        template<typename ... Args>
        inline ScopedZoneFilterd(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64 minMicroSec, const char* pFormat, Args&& ... args) :
            ctx_(ctx)
        {
            const std::size_t num = sizeof...(Args);
            __TELEM_FUNC_NAME(TelemEnterEx)(ctx, sourceInfo, matchId_, minMicroSec, pFormat, num, std::forward<Args>(args) ...);
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

#define ttZone(ctx, pFmtString, ...) telem::ScopedZone __TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, TT_SOURCE_INFO, pFmtString, __VA_ARGS__)
#define ttZoneFilterd(ctx, minMicroSec, pFmtString, ...) telem::ScopedZoneFilterd __TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, TT_SOURCE_INFO, minMicroSec, pFmtString, __VA_ARGS__)

#define ttZoneFunction(ctx) ttZone(ctx, __FUNCTION__)
#define ttZoneFunctionFilterd(ctx, minMicroSec) ttZoneFilterd(ctx, minMicroSec, __FUNCTION__)

#define ttInit() __TELEM_FUNC_NAME(TelemInit)()
#define ttShutDown() __TELEM_FUNC_NAME(TelemShutDown)()

// Context
#define ttInitializeContext(out, pBuf, bufLen) __TELEM_FUNC_NAME(TelemInitializeContext)(out, pBuf, bufLen)
#define ttShutdownContext(ctx) __TELEM_FUNC_NAME(TelemShutdownContext)(ctx)

#define ttSetContextLogFunc(ctx, func, pUserData) __TELEM_FUNC_NAME(TelemSetContextLogFunc)(ctx, func, pUserData)

#define ttOpen(ctx, pAppName, pBuildInfo, pServerAddress, conType, serverPort, timeoutMS) \
    __TELEM_FUNC_NAME(TelemOpen)(ctx, pAppName, pBuildInfo, pServerAddress, conType, serverPort, timeoutMS)

#define ttClose(ctx) __TELEM_FUNC_NAME(TelemClose)(ctx)

#define ttTick(ctx) __TELEM_FUNC_NAME(TelemTick)(ctx)
#define ttFlush(ctx) __TELEM_FUNC_NAME(TelemFlush)(ctx)
#define ttUpdateSymbolData(ctx) __TELEM_FUNC_NAME(TelemUpdateSymbolData)(ctx)

#define ttPause(ctx, pause) __TELEM_FUNC_NAME(TelemPause)(ctx, pause)
#define ttIsPaused(ctx) __TELEM_FUNC_NAME(TelemIsPaused)(ctx)

#define ttSetFlag(ctx, flag, set) __TELEM_FUNC_NAME(TelemSetFlag)(ctx, flag, set)

// Thread
#define ttSetThreadName(ctx, threadID, pFmtString, ...) __TELEM_FUNC_NAME(TelemSetThreadName)(ctx, threadID, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

#define ttGetCallStack(ctx, stackOut) __TELEM_FUNC_NAME(TelemGetCallStack)(ctx, stackOut)
#define ttSendCallStack(ctx, pStack) __TELEM_FUNC_NAME(TelemSendCallStack)(ctx, pStack)
#define ttSendCallStackSkip(ctx, pStack, numToSkip) __TELEM_FUNC_NAME(TelemSendCallStackSkip)(ctx, pStack, numToSkip)

// Zones
#define ttEnter(ctx, pFmtString, ...) __TELEM_FUNC_NAME(TelemEnter)(ctx, TT_SOURCE_INFO, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttEnterEx(ctx, matchIdOut, minMicroSec, pFmtString, ...) __TELEM_FUNC_NAME(TelemEnterEx)(ctx, TT_SOURCE_INFO, matchIdOut, minMicroSec, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttLeave(ctx) __TELEM_FUNC_NAME(TelemLeave)(ctx)
#define ttLeaveEx(ctx, matchId) __TELEM_FUNC_NAME(TelemLeaveEx)(ctx, matchId)


// Lock util
#define ttSetLockName(ctx, pPtr, pFmtString, ...) __TELEM_FUNC_NAME(TelemSetLockName)(ctx, pPtr, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttTryLock(ctx, pPtr, pFmtString, ...) __TELEM_FUNC_NAME(TelemTryLock)(ctx, TT_SOURCE_INFO, pPtr, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttTryLockEx(ctx, matchIdOut, minMicroSec, pPtr, pFmtString, ...) __TELEM_FUNC_NAME(TelemTryLock)(ctx, TT_SOURCE_INFO, matchIdOut, minMicroSec, pPtr, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttEndTryLock(ctx, pPtr, result) __TELEM_FUNC_NAME(TelemEndTryLock)(ctx, pPtr, result)
#define ttEndTryLockEx(ctx, matchIdOut, pPtr, result) __TELEM_FUNC_NAME(TelemEndTryLockEx)(ctx, matchIdOut, pPtr, result)
#define ttSetLockState(ctx, pPtr, state) __TELEM_FUNC_NAME(TelemSetLockState)(ctx, TT_SOURCE_INFO, pPtr, state)
#define ttSignalLockCount(ctx, pPtr, count) __TELEM_FUNC_NAME(TelemSignalLockCount)(ctx, TT_SOURCE_INFO, pPtr, count)

// Some allocation tracking.
#define ttAlloc(ctx, pPtr, size, pFmtString, ...) __TELEM_FUNC_NAME(TelemAlloc)(ctx, TT_SOURCE_INFO, pPtr, size, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttFree(ctx, pPtr) __TELEM_FUNC_NAME(TelemFree)(ctx, TT_SOURCE_INFO, pPtr)

#define ttPlot(ctx, type, value, pFmtString, ...)    ttPlotF32(ctx, type, value, pFmtString, __VA_ARGS__);
#define ttPlotF32(ctx, type, value, pFmtString, ...)  __TELEM_FUNC_NAME(TelemPlotF32)(ctx, type, value, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttPlotF64(ctx, type, value, pFmtString, ...)  __TELEM_FUNC_NAME(TelemPlotF64)(ctx, type, value, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttPlotI32(ctx, type, value, pFmtString, ...)  __TELEM_FUNC_NAME(TelemPlotI32)(ctx, type, value, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttPlotU32(ctx, type, value, pFmtString, ...)  __TELEM_FUNC_NAME(TelemPlotU32)(ctx, type, value, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttPlotI64(ctx, type, value, pFmtString, ...)  __TELEM_FUNC_NAME(TelemPlotI64)(ctx, type, value, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttPlotU64(ctx, type, value, pFmtString, ...)  __TELEM_FUNC_NAME(TelemPlotU64)(ctx, type, value, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)

#define ttMessage(ctx, type, pFmtString, ...)  __TELEM_FUNC_NAME(TelemMessage)(ctx, type, pFmtString, __TELEM_ARG_COUNT(__VA_ARGS__), __VA_ARGS__)
#define ttLog(ctx, pFmtString, ...) ttMessage(ctx, TtLogType::Msg, pFmtString, __VA_ARGS__)
#define ttWarning(ctx, pFmtString, ...) ttMessage(ctx, TtLogType::Warning, pFmtString, __VA_ARGS__)
#define ttError(ctx, pFmtString, ...) ttMessage(ctx, TtLogType::Error, pFmtString,  __VA_ARGS__)


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

#define ttSetFlag(...)

// Thread
#define ttSetThreadName(...);

#define ttGetCallStack(...)
#define ttSendCallStack(...)
#define ttSendCallStackSkip(...)

// Zones
#define ttEnter(...);
#define ttEnterEx(...);
#define ttLeave(...);
#define ttLeaveEx(...);


// Lock util
#define ttSetLockName(...);
#define ttTryLock(...);
#define ttTryLockEx(...);
#define ttEndTryLock(...);
#define ttEndTryLockEx(...);
#define ttSetLockState(...);
#define ttSignalLockCount(...);

#define ttAlloc(...);
#define ttFree(...);

#define ttPlot(...);
#define ttPlotF32(...);
#define ttPlotF64(...);
#define ttPlotI32(...);
#define ttPlotU32(...);
#define ttPlotI64(...);
#define ttPlotU64(...);

#define ttMessage(...);
#define ttLog(...);
#define ttWarning(...);
#define ttError(...);

#endif // TTELEMETRY_ENABLED
