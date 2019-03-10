#pragma once

#include <Time/DateTimeStamp.h>
#include <Util/Guid.h>

X_NAMESPACE_BEGIN(telemetry)

constexpr size_t MAX_TRACES_OPEN_PER_CLIENT = 8;


struct DataStreamTypeViewer
{
    enum Enum : tt_uint8
    {
        TraceZoneSegmentTicks = DataStreamType::Num,
        TraceZoneSegmentZones,
    };
};

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

struct QueryTraceInfo : public PacketBase
{
    core::Guid guid;
};


struct QueryTraceInfoResp : public PacketBase
{
    core::Guid guid;
    TraceStats stats;
};

struct OpenTrace : public PacketBase
{
    core::Guid guid;
};

struct OpenTraceResp : public PacketBase
{
    core::Guid guid;
    TraceStats stats;
    uint64_t ticksPerMicro;
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

struct ReqTraceZoneSegment : public PacketBase
{
    tt_int8 handle;

    tt_int32 tickIdx;
    tt_int32 max;      // -1 for all
    // tt_int64 startMicro;
    // tt_int64 endMicro;
};

struct DataPacketBaseViewer
{
    DataStreamTypeViewer::Enum type;
};

struct ReqTraceZoneSegmentRespTicks : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
};

struct ReqTraceZoneSegmentRespZones : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
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