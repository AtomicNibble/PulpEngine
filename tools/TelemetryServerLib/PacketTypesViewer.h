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
        TraceZoneSegmentLockStates,
        TraceZoneSegmentLockTry,
        TraceStringsInfo,
        TraceStrings,
        TraceThreadNames,
        TraceThreadGroups,
        TraceLocks,
        TraceLockNames,
        TraceZoneTree,
        TraceMessages
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
    uint64_t unixTimestamp;
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
    uint64_t unixTimestamp;
    uint32_t workerThreadID;
    tt_int8 handle;
};

struct ReqTraceStrings : public PacketBase
{
    tt_int8 handle;
    // currently just return them all don't think it's every going to be that much data.
};

struct ReqTraceThreadNames : public PacketBase
{
    tt_int8 handle;
};

struct ReqTraceThreadGroups : public PacketBase
{
    tt_int8 handle;
};

struct ReqTraceLockNames : public PacketBase
{
    tt_int8 handle;
};

struct ReqTraceLocks : public PacketBase
{
    tt_int8 handle;
};

struct ReqTraceZoneSegment : public PacketBase
{
    tt_int8 handle;

    tt_int64 startNano;
    tt_int64 endNano;
};

struct ReqTraceZoneTree : public PacketBase
{
    tt_int8 handle;
    tt_int32 frameIdx;
};

struct ReqTraceZoneMessages : public PacketBase
{
    tt_int8 handle;

    tt_int64 startNano;
    tt_int64 endNano;
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

struct ReqTraceZoneSegmentRespLockTry : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
};

struct ReqTraceZoneSegmentRespLockStates : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
};

struct ReqTraceLocksResp : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
};

struct TraceLockData
{
    tt_uint64 id;
};


struct ReqTraceStringsRespInfo : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
    tt_int32 strDataSize;
    tt_int32 minId;
    tt_int32 maxId;
};


struct ReqTraceStringsResp : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
};

struct TraceStringHdr
{
    tt_uint16 id;
    tt_uint16 length;
};


struct ReqTraceThreadNamesResp : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
};

struct TraceThreadNameData
{
    tt_uint32 threadId;
    tt_int64  timeTicks;
    tt_uint16 strIdx;
};

struct ReqTraceThreadGroupsResp : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
};

struct TraceThreadGroupData
{
    tt_uint32 threadId;
    tt_int32 groupId;
    tt_int32 sortVal;
};

struct ReqTraceLockNamesResp : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
};

struct TraceLockNameData
{
    tt_uint64 lockId;
    tt_int64  timeTicks;
    tt_uint16 strIdx;
};

struct ReqTraceZoneTreeResp : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 frameIdx;
    tt_int32 num;
};

struct TraceZoneTreeData
{
    tt_int32 parentId;
    tt_uint32 strIdx;
    tt_int64 totalTicks;
};

struct ReqTraceMessagesResp : public DataPacketBaseViewer
{
    tt_int8 handle;
    tt_int32 num;
};

struct TraceMessagesData
{
    TtLogType::Enum type;
    tt_int64  timeTicks;
    tt_uint32 strIdx;
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
