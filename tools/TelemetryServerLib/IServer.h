#pragma once

#include <Containers/Array.h>

#include <Util/UniquePointer.h>
#include <Util/Guid.h>
#include <Time/DateTimeStamp.h>

#include <../TelemetryCommon/TelemetryCommonLib.h>


#if !defined(TELEM_SRV_EXPORT)

#if !defined(X_LIB)
#define TELEM_SRV_EXPORT X_IMPORT
#else
#define TELEM_SRV_EXPORT
#endif

#endif


X_NAMESPACE_BEGIN(telemetry)

using TelemFixedStr = core::StackString<MAX_STRING_LEN, char>;

struct TraceStats
{
    core::Guid guid;
    tt_int64 numStrings;
    tt_int64 numZones;
    tt_int64 numTicks;
    tt_int64 numLockTry;
    tt_int64 durationNano;
    // will either need to store free in seperate table or manually count them.
    // tt_int64 numAllocations;
};

struct TraceInfo
{
    TraceInfo() :
        ticksPerMicro(0),
        ticksPerMs(0),
        active(false)
    {}


    TELEM_INLINE int64_t ticksToNano(tt_int64 tsc) const
    {
        // This is correct using ticksPerMicro to work out nano.
        // TODO: switch this to ticksPerMs to get better accuracy.
        const tt_int64 whole = (tsc / ticksPerMicro) * 1000;
        const tt_int64 part = (tsc % ticksPerMicro) * 1000 / ticksPerMicro;

        return whole + part;
    }

    TELEM_INLINE int64_t nanoToTicks(tt_int64 nano) const
    {
        const tt_int64 whole = nano * (ticksPerMicro / 1000);
        const tt_int64 part = (nano * (ticksPerMicro % 1000)) / 1000;

        return whole + part;
    }

    bool active;
    core::Guid guid;
    uint64_t ticksPerMicro;
    uint64_t ticksPerMs;
    core::DateTimeStamp date;
    core::string hostName;
    core::string buildInfo;
    core::string cmdLine;
    core::Path<> dbPath;
};

using TraceInfoArr = core::Array<TraceInfo>;

struct TraceApp
{
    TraceApp(const TelemFixedStr& appName, core::MemoryArenaBase* arena) :
        appName(appName),
        traces(arena)
    {}

    TelemFixedStr appName;
    TraceInfoArr traces;
};

using TraceAppArr = core::Array<TraceApp>;


struct ITelemServer
{
    virtual ~ITelemServer() = default;

    virtual bool loadApps() X_ABSTRACT;
    virtual bool listen() X_ABSTRACT;
};

TELEM_SRV_EXPORT core::UniquePointer<ITelemServer> createServer(core::MemoryArenaBase* arena);


X_NAMESPACE_END
