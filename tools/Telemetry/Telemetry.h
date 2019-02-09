#pragma once

// #define X_64 1

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


using TraceContexHandle = tt_uintptr;

inline TraceContexHandle INVALID_TRACE_CONTEX = 0;

enum ConnectionType
{
    ReliableUdp
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

enum class TtError
{
    Ok,
    InvalidParam,
    InvalidContex
};

// Loads the telemty module
// bool ttLoadModule(void);

bool ttInit(void);
void ttShutDown(void);

// Context
bool ttInitializeContext(TraceContexHandle& out, void* pBuf, tt_size bufLen);
void ttShutdownContext(TraceContexHandle ctx);

TtError ttOpen(TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
    ConnectionType conType, tt_uint16 serverPort, tt_int32 timeoutMS);

bool ttClose(TraceContexHandle ctx);

bool ttTick(TraceContexHandle ctx);
bool ttFlush(TraceContexHandle ctx);
void ttUpdateSymbolData(TraceContexHandle ctx);

void ttPause(TraceContexHandle ctx, bool pause);
bool ttIsPaused(TraceContexHandle ctx);

// Thread
void ttSetThreadName(TraceContexHandle ctx, tt_uint32 threadID, const char* pName);

// TODO : make these scoped helpers.
void ttZone(TraceContexHandle ctx, const char* pLabel);
void ttZoneFilterd(TraceContexHandle ctx, tt_uint64 minMicroSec, const char* pLabel);

// Zones
void ttEnter(TraceContexHandle ctx, const char* pZoneName);
void ttEnterEx(TraceContexHandle ctx, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pZoneName);
void ttLeave(TraceContexHandle ctx);
void ttLeaveEx(TraceContexHandle ctx, tt_uint64 matchId);


// Lock util
void ttSetLockName(TraceContexHandle ctx, tt_uint32 threadID, const char* pName);
void ttTryLock(TraceContexHandle cxx, const void* pPtr, const char* pLockName);
void ttEndTryLock(TraceContexHandle ctx, const void* pPtr, TtLockResult result);
void ttSetLockState(TraceContexHandle ctx, const void* pPtr, TtLockState state, const char* pLabel);
void ttSignalLockCount(TraceContexHandle ctx, const void* pPtr, tt_int32 count, const char* pLabel);

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

