#pragma once

#include <../TelemetryCommon/Types.h>

#define TTELEMETRY_ENABLED 1

#define X_TELEMETRY_UNIQUE_NAME_HELPER(_0, _1)  _0##_1
#define X_TELEMETRY_UNIQUE_NAME(name) X_TELEMETRY_UNIQUE_NAME_HELPER(name, __LINE__)

using TraceContexHandle = tt_uintptr;
inline TraceContexHandle INVALID_TRACE_CONTEX = 0;

enum TtConnectionType
{
    Tcp
};

enum TtLockResult
{
    Acquired,
    Fail
};

enum TtLockState
{
    Locked,
    Released
};

// Plots
enum TtPlotType
{
    Time,
    Time_us,
    Time_clocks,
    Time_cycles,
    Integer,
    Percentage_computed,
    Percentage_direct,
    Untyped
};

enum TtError
{
    Ok,
    Error,
    InvalidParam,
    InvalidContex,

    ConnectionRejected
};

#ifdef __cplusplus
extern "C" 
{
#endif

    // Loads the telemty module
    // bool ttLoadModule(void);

    TELEMETRYLIB_EXPORT bool TelemInit(void);
    TELEMETRYLIB_EXPORT void TelemShutDown(void);

    // Context
    TELEMETRYLIB_EXPORT bool TelemInitializeContext(TraceContexHandle& out, void* pBuf, tt_size bufLen);
    TELEMETRYLIB_EXPORT void TelemShutdownContext(TraceContexHandle ctx);

    TELEMETRYLIB_EXPORT TtError TelemOpen(TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
        tt_uint16 serverPort, TtConnectionType conType, tt_int32 timeoutMS);

    TELEMETRYLIB_EXPORT bool TelemClose(TraceContexHandle ctx);

    TELEMETRYLIB_EXPORT bool TelemTick(TraceContexHandle ctx);
    TELEMETRYLIB_EXPORT bool TelemFlush(TraceContexHandle ctx);
    TELEMETRYLIB_EXPORT void TelemUpdateSymbolData(TraceContexHandle ctx);

    TELEMETRYLIB_EXPORT void TelemPause(TraceContexHandle ctx, bool pause);
    TELEMETRYLIB_EXPORT bool TelemIsPaused(TraceContexHandle ctx);

    // Thread
    TELEMETRYLIB_EXPORT void TelemSetThreadName(TraceContexHandle ctx, tt_uint32 threadID, const char* pName);

    // Zones
    TELEMETRYLIB_EXPORT void TelemEnter(TraceContexHandle ctx, const char* pZoneName);
    TELEMETRYLIB_EXPORT void TelemEnterEx(TraceContexHandle ctx, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pZoneName);
    TELEMETRYLIB_EXPORT void TelemLeave(TraceContexHandle ctx);
    TELEMETRYLIB_EXPORT void TelemLeaveEx(TraceContexHandle ctx, tt_uint64 matchId);

    // Lock util
    TELEMETRYLIB_EXPORT void TelemSetLockName(TraceContexHandle ctx, tt_uint32 threadID, const char* pName);
    TELEMETRYLIB_EXPORT void TelemTryLock(TraceContexHandle ctx, const void* pPtr, const char* pLockName);
    TELEMETRYLIB_EXPORT void TelemEndTryLock(TraceContexHandle ctx, const void* pPtr, TtLockResult result);
    TELEMETRYLIB_EXPORT void TelemSetLockState(TraceContexHandle ctx, const void* pPtr, TtLockState state, const char* pLabel);
    TELEMETRYLIB_EXPORT void TelemSignalLockCount(TraceContexHandle ctx, const void* pPtr, tt_int32 count, const char* pLabel);

    // Some allocation tracking.
    void TelemAlloc(TraceContexHandle ctx, void* pPtr, tt_size size);
    void TelemFree(TraceContexHandle ctx, void* pPtr);

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
        ScopedZone(TraceContexHandle ctx, const char* pLabel) :
            ctx_(ctx)
        {
            TelemEnter(ctx, pLabel);
        }

        ~ScopedZone() {
            TelemLeave(ctx_);
        }

    private:
        TraceContexHandle ctx_;
    };

    struct ScopedZoneFilterd
    {
        ScopedZoneFilterd(TraceContexHandle ctx, tt_uint64 minMicroSec, const char* pLabel) :
            ctx_(ctx)
        {
            TelemEnterEx(ctx, matchId_, minMicroSec, pLabel);
        }

        ~ScopedZoneFilterd() {
            TelemLeaveEx(ctx_, matchId_);
        }

    private:
        TraceContexHandle ctx_;
        tt_uint64 matchId_;
    };

} // namespace telem

#endif // __cplusplus

#if TTELEMETRY_ENABLED

#define ttZone(ctx, pLabel) telem::ScopedZone X_TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, pLabel);
#define ttZoneFilterd(ctx, minMicroSec, pLabel) telem::ScopedZoneFilterd X_TELEMETRY_UNIQUE_NAME(scopedzone_)(ctx, minMicroSec, pLabel);

#define ttInit() TelemInit()
#define ttShutDown() TelemShutDown()

// Context
#define ttInitializeContext(out, pBuf, bufLen) TelemInitializeContext(out, pBuf, bufLen)
#define ttShutdownContext(ctx) TelemShutdownContext(ctx)

#define ttOpen(ctx, pAppName, pBuildInfo, pServerAddress, serverPort, conType, timeoutMS) \
    TelemOpen(ctx, pAppName, pBuildInfo, pServerAddress, serverPort, conType, timeoutMS)

#define ttClose(ctx) TelemClose(ctx)

#define ttTick(ctx) TelemTic(ctx)
#define ttFlush(ctx) TelemFlusg(ctx)
#define ttUpdateSymbolData(ctx) TelemUpdateSymbolData(ctx)

#define ttPause(ctx, pause) TelemPause(ctx, pause)
#define ttIsPaused(ctx) TelemIsPaused(ctx)

// Thread
#define ttSetThreadName(ctx, threadID, pName);

// Zones
#define ttEnter(ctx, pZoneName);
#define ttEnterEx(ctx, matchIdOut, minMicroSec, pZoneName);
#define ttLeave(ctx);
#define ttLeaveEx(ctx, matchId);


// Lock util
#define ttSetLockName(ctx, threadID, pName);
#define ttTryLock(ctx, pPtr, pLockName);
#define ttEndTryLock(ctx, pPtr, result);
#define ttSetLockState(ctx, pPtr, state, pLabel);
#define ttSignalLockCount(ctx, pPtr, count, pLabel);

// Some allocation tracking.
#define ttAlloc(ctx, pPtr, size);
#define ttFree(ctx, pPtr);

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