#pragma once



// define some types.
using tt_int8 = char;
using tt_int16 = short;
using tt_int32 = int;
using tt_int64 = long long;

using tt_uint8 = unsigned char;
using tt_uint16 = unsigned short;
using tt_uint32 = unsigned int;
using tt_uint64 = unsigned long long;

#ifdef X_64

using tt_intptr = __int64;
using tt_uintptr = unsigned __int64;
using tt_ptrdiff = __int64;
using tt_size = unsigned __int64;

#else

using tt_intptr = int;
using tt_uintptr = unsigned int;
using tt_ptrdiff = int;
using tt_size = unsigned int;

#endif

static_assert(sizeof(tt_uintptr) == sizeof(void*), "Size missmatch");

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
};

#ifdef __cplusplus
extern "C" 
{
#endif

    // Loads the telemty module
    // bool ttLoadModule(void);


    TELEMETRYLIB_EXPORT bool ttInit(void);
    TELEMETRYLIB_EXPORT void ttShutDown(void);

    // Context
    TELEMETRYLIB_EXPORT bool ttInitializeContext(TraceContexHandle& out, void* pBuf, tt_size bufLen);
    TELEMETRYLIB_EXPORT void ttShutdownContext(TraceContexHandle ctx);

    TELEMETRYLIB_EXPORT TtError ttOpen(TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
        tt_uint16 serverPort, TtConnectionType conType, tt_int32 timeoutMS);

    TELEMETRYLIB_EXPORT bool ttClose(TraceContexHandle ctx);

    TELEMETRYLIB_EXPORT bool ttTick(TraceContexHandle ctx);
    TELEMETRYLIB_EXPORT bool ttFlush(TraceContexHandle ctx);
    TELEMETRYLIB_EXPORT void ttUpdateSymbolData(TraceContexHandle ctx);

    TELEMETRYLIB_EXPORT void ttPause(TraceContexHandle ctx, bool pause);
    TELEMETRYLIB_EXPORT bool ttIsPaused(TraceContexHandle ctx);

    // Thread
    TELEMETRYLIB_EXPORT void ttSetThreadName(TraceContexHandle ctx, tt_uint32 threadID, const char* pName);

    // TODO : make these scoped helpers.
    void ttZone(TraceContexHandle ctx, const char* pLabel);
    void ttZoneFilterd(TraceContexHandle ctx, tt_uint64 minMicroSec, const char* pLabel);

    // Zones
    TELEMETRYLIB_EXPORT void ttEnter(TraceContexHandle ctx, const char* pZoneName);
    TELEMETRYLIB_EXPORT void ttEnterEx(TraceContexHandle ctx, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pZoneName);
    TELEMETRYLIB_EXPORT void ttLeave(TraceContexHandle ctx);
    TELEMETRYLIB_EXPORT void ttLeaveEx(TraceContexHandle ctx, tt_uint64 matchId);

    // Lock util
    TELEMETRYLIB_EXPORT void ttSetLockName(TraceContexHandle ctx, tt_uint32 threadID, const char* pName);
    TELEMETRYLIB_EXPORT void ttTryLock(TraceContexHandle cxx, const void* pPtr, const char* pLockName);
    TELEMETRYLIB_EXPORT void ttEndTryLock(TraceContexHandle ctx, const void* pPtr, TtLockResult result);
    TELEMETRYLIB_EXPORT void ttSetLockState(TraceContexHandle ctx, const void* pPtr, TtLockState state, const char* pLabel);
    TELEMETRYLIB_EXPORT void ttSignalLockCount(TraceContexHandle ctx, const void* pPtr, tt_int32 count, const char* pLabel);

    // Some allocation tracking.
    void ttAlloc(TraceContexHandle ctx, void* pPtr, tt_size size);
    void ttFree(TraceContexHandle ctx, void* pPtr);

    void tmPlot(TraceContexHandle ctx, TtPlotType type, float value, const char* pName);
    void tmPlotF32(TraceContexHandle ctx, TtPlotType type, float value, const char* pName);
    void tmPlotF64(TraceContexHandle ctx, TtPlotType type, double value, const char* pName);
    void tmPlotI32(TraceContexHandle ctx, TtPlotType type, tt_int32 value, const char* pName);
    void tmPlotU32(TraceContexHandle ctx, TtPlotType type, tt_int64 value, const char* pName);
    void tmPlotI64(TraceContexHandle ctx, TtPlotType type, tt_uint32 value, const char* pName);
    void tmPlotU64(TraceContexHandle ctx, TtPlotType type, tt_uint64 value, const char* pName);


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


    inline bool Init(void)
    {
        return ttInit();
    }

    inline void ShutDown(void)
    {
        ttShutDown();
    }

    // Context
    inline bool InitializeContext(ContexHandle& out, void* pBuf, tt_size bufLen)
    {
        return ttInitializeContext(out, pBuf, bufLen);
    }

    inline void ShutdownContext(ContexHandle ctx)
    {
        ttShutdownContext(ctx);
    }

    inline TtError Open(ContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
        tt_uint16 serverPort, ConnectionType conType, tt_int32 timeoutMS)
    {
        return ttOpen(ctx, pAppName, pBuildInfo, pServerAddress, serverPort, conType, timeoutMS);
    }

    inline bool Close(ContexHandle ctx)
    {
        return ttClose(ctx);
    }


} // namespace telem

#endif // __cplusplus
