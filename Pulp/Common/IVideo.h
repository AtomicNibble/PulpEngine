#pragma once

#include <IRender.h>
#include <IAsyncLoad.h>
#include <IConverterModule.h>

X_NAMESPACE_DECLARE(core,
                    struct FrameTimeData;)

X_NAMESPACE_BEGIN(video)

static const uint32_t VIDEO_VERSION = 1;
static const uint32_t VIDEO_FOURCC = X_TAG('v', 'i', 'd', ' ');
static const uint32_t VIDEO_MAX_TRACK = 1; // video
static const uint32_t VIDEO_MAX_AUDIO_TRACK = 4;

static const uint32_t VIDEO_MAX_AUDIO_CHANNELS = 2;

static const char* VIDEO_FILE_EXTENSION = "vid";

static const uint32_t VIDEO_MAX_LOADED = 4;

struct IVideoLib : public IConverter
{
};

struct IVideo
{
    virtual ~IVideo() = default;

    virtual render::TexID getTextureID(void) const X_ABSTRACT;
};

struct IVideoSys : public core::IEngineSysBase
    , public core::IAssetLoader
{
    using core::IAssetLoader::waitForLoad;

    virtual ~IVideoSys() = default;

    virtual void update(const core::FrameTimeData& frameTimeInfo) X_ABSTRACT;
    virtual void unlockBuffers(void) X_ABSTRACT;

    virtual IVideo* findVideo(const char* pVideoName) const X_ABSTRACT;
    virtual IVideo* loadVideo(const char* pVideoName) X_ABSTRACT;

    virtual void releaseVideo(IVideo* pVid) X_ABSTRACT;
    virtual bool waitForLoad(IVideo* pVideo) X_ABSTRACT;

    virtual void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const X_ABSTRACT;
};

// file types

X_DECLARE_ENUM8(TrackType)(
    Video,
    Audio
);

X_DECLARE_ENUM8(VideoFmt)(
    VP8,
    VP9
);

X_DECLARE_ENUM8(AudioFmt)(
    Vorbis
);

X_PACK_PUSH(1)

struct BlockHdr
{
    TrackType::Enum type;
    bool isKey;
    int32_t timeMS;
    int32_t blockSize;
};

struct VideoTrackHdr
{
    VideoFmt::Enum fmt;
    int16_t frameRate;
    int16_t pixelWidth;
    int16_t pixelHeight;
    int32_t numBlocks;
    int32_t largestBlockBytes;
};

struct AudioTrackHdr
{
    AudioFmt::Enum fmt;
    int8_t channels;
    int16_t bitDepth;
    int32_t sampleFreq;
    int32_t numBlocks;
    int32_t largestBlockBytes;

    int32_t deflatedHdrSize;
    int32_t inflatedHdrSize;
};

struct BufferSizes
{
    int32_t maxBytesFor100MS;
    int32_t maxBytesFor250MS;
    int32_t maxBytesFor500MS;
};

struct VideoHeader
{
    uint32 fourCC;
    uint8 version;

    // video has leading 6 bytes, so no padding after version.
    VideoTrackHdr video;
    AudioTrackHdr audio;

    int64_t durationNS;

    // the max amount of compressed data needed for a 500ms buffer.
    BufferSizes bufferInfo;

    X_INLINE bool isValid(void) const {
        return fourCC == VIDEO_FOURCC;
    }
};

X_PACK_POP

X_ENSURE_SIZE(BlockHdr, 10);
X_ENSURE_SIZE(VideoHeader, 64);


X_NAMESPACE_END
