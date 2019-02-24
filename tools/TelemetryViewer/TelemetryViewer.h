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
    TraceView() {
        core::zero_this(this);

        numFrames_ = 1024 * 1024;
        frameScale_ = 0;
        zvEnd_ = 100;
    }

    int32_t numFrames_;
    int32_t frameStart_;
    int32_t frameScale_;

    int64_t zvStart_;
    int64_t zvEnd_;

    int zvHeight_;
    int zvScroll_;

    Region highlight_;
    Region highlightZoom_;
    Animation zoomAnim_;

    bool paused_;
};



struct Client
{
    enum class ConnectionState {
        Offline,
        Connecting,
        Connected,
    };

    Client(core::MemoryArenaBase* arena);

    bool isConnected(void) const;

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

    telemetry::TraceAppArr apps;
};


bool run(Client& client);

X_NAMESPACE_END