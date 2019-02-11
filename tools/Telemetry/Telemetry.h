#pragma once

#include <../TelemetryCommon/Types.h>

#define TTELEMETRY_ENABLED 1

#define X_TELEMETRY_UNIQUE_NAME_HELPER_0(_0, _1)  _0##_1
#define X_TELEMETRY_UNIQUE_NAME_HELPER(_0, _1) X_TELEMETRY_UNIQUE_NAME_HELPER_0(_0, _1)
#define X_TELEMETRY_UNIQUE_NAME(name) X_TELEMETRY_UNIQUE_NAME_HELPER(name, __LINE__)

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

    // Loads the telemty module
    // bool ttLoadModule(void);

    TELEMETRYLIB_EXPORT bool TelemInit(void);
    TELEMETRYLIB_EXPORT void TelemShutDown(void);

    // Context
    TELEMETRYLIB_EXPORT TtError TelemInitializeContext(TraceContexHandle& out, void* pBuf, tt_size bufLen);
    TELEMETRYLIB_EXPORT void TelemShutdownContext(TraceContexHandle ctx);

    TELEMETRYLIB_EXPORT TtError TelemOpen(TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
        tt_uint16 serverPort, TtConnectionType conType, tt_int32 timeoutMS);

    TELEMETRYLIB_EXPORT bool TelemClose(TraceContexHandle ctx);

    TELEMETRYLIB_EXPORT void TelemTick(TraceContexHandle ctx);
    TELEMETRYLIB_EXPORT void TelemFlush(TraceContexHandle ctx);
    TELEMETRYLIB_EXPORT void TelemUpdateSymbolData(TraceContexHandle ctx);

    TELEMETRYLIB_EXPORT void TelemPause(TraceContexHandle ctx, bool pause);
    TELEMETRYLIB_EXPORT bool TelemIsPaused(TraceContexHandle ctx);

    // Thread
    TELEMETRYLIB_EXPORT void TelemSetThreadName(TraceContexHandle ctx, tt_uint32 threadID, const char* pName);

    // Zones
    TELEMETRYLIB_EXPORT void TelemEnter(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pZoneName);
    TELEMETRYLIB_EXPORT void TelemEnterEx(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pZoneName);
    TELEMETRYLIB_EXPORT void TelemLeave(TraceContexHandle ctx);
    TELEMETRYLIB_EXPORT void TelemLeaveEx(TraceContexHandle ctx, tt_uint64 matchId);

    // Lock util
    TELEMETRYLIB_EXPORT void TelemSetLockName(TraceContexHandle ctx, const void* pPtr, const char* pLockName);
    TELEMETRYLIB_EXPORT void TelemTryLock(TraceContexHandle ctx, const void* pPtr, const char* pDescription);
    TELEMETRYLIB_EXPORT void TelemTryLockEx(TraceContexHandle ctx, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const void* pPtr, const char* pDescription);
    TELEMETRYLIB_EXPORT void TelemEndTryLock(TraceContexHandle ctx, const void* pPtr, TtLockResult result);
    TELEMETRYLIB_EXPORT void TelemEndTryLockEx(TraceContexHandle ctx, tt_uint64 matchId, const void* pPtr, TtLockResult result);
    TELEMETRYLIB_EXPORT void TelemSetLockState(TraceContexHandle ctx, const void* pPtr, TtLockState state);
    TELEMETRYLIB_EXPORT void TelemSignalLockCount(TraceContexHandle ctx, const void* pPtr, tt_int32 count);

    // Some allocation tracking.
    TELEMETRYLIB_EXPORT void TelemAlloc(TraceContexHandle ctx, void* pPtr, tt_size size);
    TELEMETRYLIB_EXPORT void TelemFree(TraceContexHandle ctx, void* pPtr);

    void TelemPlot(TraceContexHandle ctx, TtPlotType type, float value, const char* pName);
    void TelemPlotF32(TraceContexHandle ctx, TtPlotType type, float value, const char* pName);
    void TelemPlotF64(TraceContexHandle ctx, TtPlotType type, double value, const char* pName);
    void TelemPlotI32(TraceContexHandle ctx, TtPlotType type, tt_int32 value, const char* pName);
    void TelemPlotU32(TraceContexHandle ctx, TtPlotType type, tt_int64 value, const char* pName);
    void TelemPlotI64(TraceContexHandle ctx, TtPlotType type, tt_uint32 value, const char* pName);
    void TelemPlotU64(TraceContexHandle ctx, TtPlotType type, tt_uint64 value, const char* pName);


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

    struct ScopedZone
    {
        inline ScopedZone(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pLabel) :
            ctx_(ctx)
        {
            TelemEnter(ctx, sourceInfo, pLabel);
        }

        inline ~ScopedZone() {
            TelemLeave(ctx_);
        }

    private:
        TraceContexHandle ctx_;
    };

    struct ScopedZoneFilterd
    {
        inline ScopedZoneFilterd(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64 minMicroSec, const char* pLabel) :
            ctx_(ctx)
        {
            TelemEnterEx(ctx, sourceInfo, matchId_, minMicroSec, pLabel);
        }

        inline ~ScopedZoneFilterd() {
            TelemLeaveEx(ctx_, matchId_);
        }

    private:
        TraceContexHandle ctx_;
        tt_uint64 matchId_;
    };

} // namespace telem

#endif // __cplusplus

#if TTELEMETRY_ENABLED

#define ttZone(ctx, pLabel) telem::ScopedZone X_TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, TT_SOURCE_INFO, pLabel);
#define ttZoneFilterd(ctx, minMicroSec, pLabel) telem::ScopedZoneFilterd X_TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, TT_SOURCE_INFO, minMicroSec, pLabel);

#define ttInit() TelemInit()
#define ttShutDown() TelemShutDown()

// Context
#define ttInitializeContext(out, pBuf, bufLen) TelemInitializeContext(out, pBuf, bufLen)
#define ttShutdownContext(ctx) TelemShutdownContext(ctx)

#define ttOpen(ctx, pAppName, pBuildInfo, pServerAddress, serverPort, conType, timeoutMS) \
    TelemOpen(ctx, pAppName, pBuildInfo, pServerAddress, serverPort, conType, timeoutMS)

#define ttClose(ctx) TelemClose(ctx)

#define ttTick(ctx) TelemTick(ctx)
#define ttFlush(ctx) TelemFlush(ctx)
#define ttUpdateSymbolData(ctx) TelemUpdateSymbolData(ctx)

#define ttPause(ctx, pause) TelemPause(ctx, pause)
#define ttIsPaused(ctx) TelemIsPaused(ctx)

// Thread
#define ttSetThreadName(ctx, threadID, pName);

// Zones
#define ttEnter(ctx, pZoneName) TelemEnter(ctx, TT_SOURCE_INFO, pZoneName);
#define ttEnterEx(ctx, matchIdOut, minMicroSec, pZoneName) TelemEnterEx(ctx, TT_SOURCE_INFO, matchIdOut, minMicroSec, pZoneName);
#define ttLeave(ctx) TelemLeave(ctx);
#define ttLeaveEx(ctx, matchId) TelemLeaveEx(ctx, matchId);


// Lock util
#define ttSetLockName(ctx, pPtr, pLockName) TelemSetLockName(ctx, pPtr, pLockName);
#define ttTryLock(ctx, pPtr, pDescription) TelemTryLock(ctx, pPtr, pDescription);
#define ttTryLockEx(ctx, matchIdOut, minMicroSec, pPtr, pDescription) TelemTryLock(ctx, matchIdOut, minMicroSec, pPtr, pDescription);
#define ttEndTryLock(ctx, pPtr, result) TelemEndTryLock(ctx, pPtr, result);
#define ttEndTryLockEx(ctx, matchIdOut, pPtr, result) TelemEndTryLockEx(ctx, matchIdOut, pPtr, result);
#define ttSetLockState(ctx, pPtr, state) TelemSetLockState(ctx, pPtr, state);
#define ttSignalLockCount(ctx, pPtr, count) TelemSignalLockCount(ctx, pPtr, count);

// Some allocation tracking.
#define ttAlloc(ctx, pPtr, size) TelemAlloc(ctx, pPtr, size);
#define ttFree(ctx, pPtr) TelemFree(ctx, pPtr);

#define ttPlot(ctx, type, value, pName);
#define ttPlotF32(ctx, type, value, pName);
#define ttPlotF64(ctx, type, value, pName);
#define ttPlotI32(ctx, type, value, pName);
#define ttPlotU32(ctx, type, value, pName);
#define ttPlotI64(ctx, type, value, pName);
#define ttPlotU64(ctx, type, value, pName);


#else // TTELEMETRY_ENABLED

#define ttZone(...)
#define ttZoneFilterd(...)

#define ttInit() true
#define ttShutDown() 

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