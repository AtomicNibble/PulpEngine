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
    // 16
    int64_t startNano;
    int64_t endNano;

    // 16
    int64_t startTicks;
    int64_t endTicks;
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
    ZoneSegmentThread(uint32_t id, core::MemoryArenaBase* arena) :
        id(id),
        zones(arena)
    {}

    uint32_t id;

    ZoneDataArr zones;
};

struct ZoneSegment
{
    using ZoneSegmentThreadArr = core::ArrayGrowMultiply<ZoneSegmentThread>;
    using TickDataArr = core::ArrayGrowMultiply<TickData>;

public:
    ZoneSegment(core::MemoryArenaBase* arena) :
        startNano(0),
        endNano(0),
        ticks(arena),
        threads(arena)
    {}

    // this should just be for a range of time.
    // it may have no ticks, should probs move ticks out of this then.
    int64_t startNano;
    int64_t endNano;

    TickDataArr ticks;
    ZoneSegmentThreadArr threads;
};


struct TraceView
{
    using ZoneSegmentArr = core::Array<ZoneSegment>;

public:
    TraceView(core::Guid guid, uint64_t ticksPerMicro, TraceStats stats, int8_t handle, core::MemoryArenaBase* arena) :
        guid(guid),
        ticksPerMicro(ticksPerMicro),
        stats(stats),
        handle(handle),
        segments(arena)
    {
        open_ = true;

    //    numFrames_ = 1024 * 1024;
    //    frameStart_ = 0;
    //    frameScale_ = 0;

        zvStartNS_ = 1000 * 1000 * 800;
        zvEndNS_ = 1000 * 1000 * 1500;

        zvHeight_ = 0;
        zvScroll_ = 0;

        tabName.set("Trace - Zones");
    }

    X_INLINE int64_t GetTimeBegin(void) const
    {
        // we always start from zero?
        return 0;
    }

    X_INLINE int64_t GetVisiableNS(void) const
    {
        return zvEndNS_ - zvStartNS_;
    }

    X_INLINE int64_t GetVisibleStartNS(void) const
    {
        return zvStartNS_ - GetTimeBegin();
    }

    X_INLINE uint64_t ticksToNano(uint64_t tsc) const
    {
        const uint64_t whole = (tsc / ticksPerMicro) * 1000;
        const uint64_t part = (tsc % ticksPerMicro) * 1000 / ticksPerMicro;

        return whole + part;
    }

    core::StackString<64,char> tabName;

    bool paused_; // don't auto scroll
    bool open_;
    bool _pad[2];

    core::Guid guid;
    uint64_t ticksPerMicro;
    TraceStats stats;
    int8_t handle;

    // int32_t numFrames_;
    // int32_t frameStart_;
    // int32_t frameScale_;

    int64_t zvStartNS_;
    int64_t zvEndNS_;

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