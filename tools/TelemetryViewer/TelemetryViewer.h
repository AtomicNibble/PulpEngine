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

struct TraceView
{
    TraceView(core::Guid guid, tt_int8 handle) :
        guid(guid),
        handle(handle)
    {
        open_ = true;

        numFrames_ = 1024 * 1024;
        frameStart_ = 0;
        frameScale_ = 0;

        zvStart_ = 0;
        zvEnd_ = 100;

        zvHeight_ = 0;
        zvScroll_ = 0;

        tabName.set("Trace - Zones");
    }

    core::StackString<64,char> tabName;

    bool paused_;
    bool open_;
    bool _pad[2];

    core::Guid guid;
    tt_int8 handle;

    int32_t numFrames_;
    int32_t frameStart_;
    int32_t frameScale_;

    int64_t zvStart_;
    int64_t zvEnd_;

    int32_t zvHeight_;
    int32_t zvScroll_;

    Region highlight_;
    Region highlightZoom_;
    Animation zoomAnim_;
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