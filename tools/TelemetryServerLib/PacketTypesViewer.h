#pragma once

#include <Time/DateTimeStamp.h>
#include <Util/Guid.h>

X_NAMESPACE_BEGIN(telemetry)

constexpr size_t MAX_TRACES_OPEN_PER_CLIENT = 8;


struct ConnectionRequestViewerHdr : public PacketBase
{
    VersionInfo viewerVer;
};


struct AppsListHdr : public PacketBase
{
    tt_int32 num;
};

struct AppsListData
{
    tt_int32 numTraces;
    char appName[MAX_STRING_LEN];
};

struct AppTraceListData
{
    bool active;
    core::Guid guid;
    uint64_t ticksPerMicro;
    core::DateTimeStamp date;
    char hostName[MAX_STRING_LEN];
    char buildInfo[MAX_STRING_LEN];
};

struct TraceInfo : public PacketBase
{
    core::Guid guid;
    tt_int64 numZones;
    tt_int64 numTicks;
    tt_int64 numAllocations;
    tt_int64 durationMicro;
};

struct OpenTrace : public PacketBase
{
    core::Guid guid;
};

struct OpenTraceResp : public PacketBase
{
    core::Guid guid;
    tt_int8 handle;
};

struct ReqTraceTicks : public PacketBase
{
    tt_int8 handle;

    tt_int32 tickIdx;
    tt_int32 num;      // -1 for all
};

struct ReqTraceZones : public PacketBase
{
    tt_int8 handle;

    tt_int64 start;
    tt_int64 end;   // -1 for ubounded
};

struct ReqTraceStrings : public PacketBase
{
    tt_int8 handle;

    // currently just return them all don't think it's every going to be that much data.
};

#if 0
struct QueryServerStatsReponse
{
    tt_int32 activeTraces;
    tt_int64 bpsIngest;
    tt_int64 storageUsed;
};
#endif


X_NAMESPACE_END