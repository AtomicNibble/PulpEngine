#pragma once

#include <Threading/Signal.h>

X_NAMESPACE_BEGIN(telemetry)

struct Region
{
    bool active = false;
    int64_t start;
    int64_t end;
};

struct Animation
{
    bool active = false;
    int64_t start0, start1;
    int64_t end0, end1;
    double progress;
    double lenMod;
};

// --------------------------------

struct ZoneData
{
    // 4
    uint32_t threadID;

    // 16
    uint64_t start;
    uint64_t end;
};

struct TickData
{
    int64_t start;
    int64_t end;

    int64_t startNano;
    int64_t endNano;
};

struct ZoneSegmentThread
{
    // this is all the zones for this thread.
    using ZoneDataArr = core::ArrayGrowMultiply<ZoneData>;

public:
    ZoneSegmentThread(core::MemoryArenaBase* arena) :
        zones(arena)
    {}

    ZoneDataArr zones;
};

struct ZoneSegment
{
    using ZoneSegmentThreadArr = core::ArrayGrowMultiply<ZoneSegmentThread>;
    using TickDataArr = core::ArrayGrowMultiply<TickData>;

public:
    ZoneSegment(core::MemoryArenaBase* arena) :
        ticks(arena),
        threads(arena)
    {}

    // what is this segment for?
    // well it has to be for a number of ticks.

    TickDataArr ticks;
    ZoneSegmentThreadArr threads;
};


struct TraceView
{
    using ZoneSegmentArr = core::Array<ZoneSegment>;

public:
    TraceView(core::Guid guid, TraceStats stats, tt_int8 handle, core::MemoryArenaBase* arena) :
        guid(guid),
        stats(stats),
        handle(handle),
        segments(arena)
    {
        open_ = true;

    //    numFrames_ = 1024 * 1024;
        frameStart_ = 0;
        frameScale_ = 0;

        zvStart_ = 1000 * 1;
        zvEnd_ = 1000 * 10;

        zvHeight_ = 0;
        zvScroll_ = 0;

        tabName.set("Trace - Zones");
    }

    int64_t GetTimeBegin(void) const 
    {
        // we always start from zero?
        return 0;
    }

    int64_t GetVisiableNS(void) const
    {
        return zvEnd_ - zvStart_;
    }

    int64_t GetVisibleStartNS(void) const
    {
        return zvStart_ - GetTimeBegin();
    }


    core::StackString<64,char> tabName;

    bool paused_; // don't auto scroll
    bool open_;
    bool _pad[2];

    core::Guid guid;
    TraceStats stats;
    tt_int8 handle;

    // int32_t numFrames_;
    int32_t frameStart_;
    int32_t frameScale_;

    int64_t zvStart_;
    int64_t zvEnd_;

    int32_t zvHeight_;
    int32_t zvScroll_;

    Region highlight_;
    Region highlightZoom_;
    Animation zoomAnim_;

    ZoneSegmentArr segments;
};

using GuidTraceStats = std::pair<core::Guid, TraceStats>;

struct Client
{
    using GuidTraceStatsArr = core::Array<GuidTraceStats>;
    using TraceViewArr = core::Array<TraceView>;

    enum class ConnectionState {
        Offline,
        Connecting,
        Connected,
    };

    Client(core::MemoryArenaBase* arena);

    bool isConnected(void) const;
    void sendDataToServer(const void* pData, int32_t len);

    core::StackString256 addr;
    uint16_t port;
    ConnectionState conState;
    core::Signal connectSignal;

    platform::SOCKET socket;
    VersionInfo serverVer;

    // Server compress it's responses.
    int32_t cmpBufferOffset;
    int8_t cmpRingBuf[COMPRESSION_RING_BUFFER_SIZE];

    core::Compression::LZ4StreamDecode lz4DecodeStream;

    // stuff set by background thread..?
    char _pad[64]; // todo align.

    core::CriticalSection dataCS;

    TraceAppArr apps;

    GuidTraceStatsArr traceStats;
    TraceViewArr views;
};


bool run(Client& client);

X_NAMESPACE_END