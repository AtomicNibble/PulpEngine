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

X_PACK_PUSH(1)

struct BlockHdr
{
    TrackType::Enum type;
    bool isKey;
    int32_t timeMS;
    int32_t blockSize;
};

#if 0
struct ClusterHdr
{
    int32_t timeMS;
    int32_t durationMS;
    int32_t numBlocks;
};
#endif

struct VideoTrackHdr
{
    int16_t pixelWidth;
    int16_t pixelHeight;
    int32_t numFrames;
    int32_t largestFrameBytes;
};

struct AudioTrackHdr
{
    int16_t channels;
    int16_t bitDepth;
    int32_t sampleFreq;
    int32_t numFrames;
    int32_t largestFrameBytes;

    int32_t deflatedHdrSize;
    int32_t inflatedHdrSize;
};

struct VideoHeader
{
    uint32 fourCC;
    uint8 version;
    uint8_t _pad[3];

    int64_t durationNS;

    VideoTrackHdr video;
    AudioTrackHdr audio;

    X_INLINE bool isValid(void) const {
        return fourCC == VIDEO_FOURCC;
    }
};

X_PACK_POP

X_ENSURE_SIZE(BlockHdr, 10);
// X_ENSURE_SIZE(ClusterHdr, 12);
X_ENSURE_SIZE(VideoHeader, 52);


X_NAMESPACE_END
