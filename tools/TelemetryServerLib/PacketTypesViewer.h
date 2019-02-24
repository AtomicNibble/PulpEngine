#pragma once

#include <Time/DateTimeStamp.h>


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

struct AppTraceListHdr : public PacketBase
{
    tt_int32 num;
};

struct AppTraceListData
{
    tt_int32 traceId;
    core::DateTimeStamp date;
    char name[MAX_STRING_LEN];
    char buildInfo[MAX_STRING_LEN];
};

struct TraceInfo : public PacketBase
{
    tt_int32 traceId;
    tt_int64 numZones;
    tt_int64 numTicks;
    tt_int64 numAllocations;
    tt_int64 durationMicro;
};

struct OpenTrace : public PacketBase
{
    tt_int32 traceId;

    // TEMP
    char appName[MAX_STRING_LEN];
    char name[MAX_STRING_LEN];
};

struct OpenTraceResp : public PacketBase
{
    tt_int8 handle;
};

struct QueryTraceTicks : public PacketBase
{
    tt_int8 handle;

    tt_int32 tickIdx;
    tt_int32 num;      // -1 for all
};

struct QueryTraceZones : public PacketBase
{
    tt_int8 handle;

    tt_int64 start;
    tt_int64 end;   // -1 for ubounded
};

struct QueryTraceStrings : public PacketBase
{
    tt_int8 handle;

    // currently just return them all don't think it's every going to be that much data.
};

struct QueryServerStatsReponse
{
    tt_int32 activeTraces;
    tt_int64 bpsIngest;
    tt_int64 storageUsed;
};


X_NAMESPACE_END