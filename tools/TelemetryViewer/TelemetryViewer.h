#pragma once


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
    Client(core::MemoryArenaBase* arena);

    bool isConnected(void) const;

    VersionInfo serverVer;
    platform::SOCKET socket;

    // Server compress it's responses.
    int32_t cmpBufferOffset;
    int8_t cmpRingBuf[COMPRESSION_RING_BUFFER_SIZE];

    core::Compression::LZ4StreamDecode lz4DecodeStream;

    // apps
    telemetry::TraceAppArr apps;
};


bool run(Client& client);

X_NAMESPACE_END