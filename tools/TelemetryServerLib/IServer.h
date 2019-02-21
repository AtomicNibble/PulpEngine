#pragma once

#include <Util/UniquePointer.h>
#include <Containers/Array.h>

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


struct Trace
{
    Trace() :
        ticksPerMicro(0)
    {}

    core::Path<> dbPath;
    core::string name;
    core::string date;
    core::string buildInfo;
    core::string cmdLine;
    uint64_t ticksPerMicro;
};

using TraceArr = core::Array<Trace>;

struct TraceApp
{

    TraceApp(core::MemoryArenaBase* arena) :
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