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
    tt_int64 numZones;
    tt_int64 numTicks;
    tt_int64 durationMicro;
    // will either need to store free in seperate table or manually count them.
    // tt_int64 numAllocations;
};

struct Trace
{
    Trace() :
        ticksPerMicro(0),
        active(false)
    {}

    bool active;
    core::Guid guid;
    uint64_t ticksPerMicro;
    core::DateTimeStamp date;
    core::string hostName;
    core::string buildInfo;
    core::string cmdLine;
    core::Path<> dbPath;
};

using TraceArr = core::Array<Trace>;

struct TraceApp
{
    TraceApp(const TelemFixedStr& appName, core::MemoryArenaBase* arena) :
        appName(appName),
        traces(arena)
    {}

    TelemFixedStr appName;
    TraceArr traces;
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