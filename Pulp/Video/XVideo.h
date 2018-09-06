#pragma once

#include <Assets\AssetBase.h>
#include <Threading\Signal.h>

#include <Containers\FixedByteStreamRing.h>
#include <Containers\FixedFifo.h>
#include <Time\TimeVal.h>

#include <ISound.h>

X_NAMESPACE_DECLARE(core,
                    struct IoRequestBase;
                    struct XFileAsync);

X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext);

X_NAMESPACE_BEGIN(video)

struct IVFHdr;

struct FrameData
{
    size_t dataSize;
    const uint8_t* pData;
};

X_DECLARE_ENUM8(State)
(
    UnInit,
    Init,
    Buffering,
    Playing,
    Stopped,
    Finished
);

class Video : public core::AssetBase
    , public IVideo
{
    static constexpr size_t NUM_FRAME_BUFFERS = 3;

    static constexpr size_t IO_REQUEST_SIZE = 1024 * 128;
    static constexpr size_t IO_RING_BUFFER_SIZE = 1024 * 1024 * 2; // 2MB
    static_assert(IO_RING_BUFFER_SIZE % IO_REQUEST_SIZE == 0, "Ring buffer not a multiple of IO request size");

    // if 44100 that's (44100 * 4) = 176400 bytes per second per channel
    static constexpr size_t AUDIO_RING_DECODED_BUFFER_SIZE = 1024 * 256; // 256KB just over 1 second buffer.
    static constexpr size_t AUDIO_RING_MAX_FILL = 1024 * 192; // don't decode any more if this much data.

    static constexpr size_t FRAME_QUEUE_SIZE = 128 * 4;


    template<typename T>
    using ArrayFixedBaseAlign = core::Array<T, core::ArrayAlignedAllocatorFixed<T, 64>, core::growStrat::Multiply>;

    typedef ArrayFixedBaseAlign<uint8_t> DataVec;
    typedef core::FixedFifo<int32_t, FRAME_QUEUE_SIZE> IntQueue;

    typedef std::array<core::FixedByteStreamRingOwning, VIDEO_MAX_AUDIO_CHANNELS> AudioRingBufferChannelArr;
    typedef std::array<IntQueue, TrackType::ENUM_COUNT> TrackQueues;
    typedef std::array<int32_t, TrackType::ENUM_COUNT> TrackIntArr;


    // Debug
    static constexpr int32_t FRAME_HISTORY_SIZE = 128;

    template<typename T>
    using FrameHistory = core::FixedRingBuffer<T, FRAME_HISTORY_SIZE>;

    template<typename T>
    using TrackFrameHistory = std::array<FrameHistory<T>, TrackType::ENUM_COUNT>;

    struct Frame
    {
        Frame(core::MemoryArenaBase* arena) :
            displayTime(0),
            decoded(arena)
        {}

        int32_t displayTime;
        DataVec decoded; // a decoded frame, read for uploading to gpu.
    };

    typedef std::array<Frame, NUM_FRAME_BUFFERS> FrameArr;
    typedef core::FixedFifo<FrameArr::value_type*, NUM_FRAME_BUFFERS> FramePtrQueue;

public:
    Video(core::string name, core::MemoryArenaBase* arena);
    virtual ~Video();

    void play(void);
    void pause(void);

    void update(const core::FrameTimeData& frameTimeInfo);
    void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket);
    void releaseFrame(void);

    render::TexID getTextureID(void) const X_FINAL;

    bool processHdr(core::XFileAsync* pFile, core::span<uint8_t> data);

    Vec2f drawDebug(engine::IPrimativeContext* pPrim, Vec2f pos);

    X_INLINE State::Enum getState(void) const;
    X_INLINE uint16_t getWidth(void) const;
    X_INLINE uint16_t getHeight(void) const;
    X_INLINE uint32_t getFps(void) const;
    X_INLINE size_t getIOBufferSize(void) const;
    X_INLINE bool hasFrame(void) const;

private:
    void processIOData(void);

    bool processVorbisHeader(core::span<uint8_t> data);
    void validateAudioBufferSizes(void) const;

    sound::BufferResult::Enum audioDataRequest(sound::AudioBuffer& ab);

    void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
        core::XFileAsync* pFile, uint32_t bytesTransferred);

    void dispatchRead(void);

    void validateQueues(void);
    void seekIoBuffer(int32_t numBytes);
    void popProcessed(TrackType::Enum type);

    bool decodeAudioPacket(void);
    bool decodeVideo(void);

private:
    void decodeAudio_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void decodeVideo_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
    vpx_codec_ctx_t codec_;

    VideoTrackHdr video_;
    AudioTrackHdr audio_;

    int32_t frameRate_;

    core::TimeVal playTime_;
    core::TimeVal duration_;

    State::Enum state_;
    bool ioRequestPending_;
    bool _pad[2];

    core::V2::Job* pDecodeAudioJob_;
    core::V2::Job* pDecodeVideoJob_;

    // Render texture
    render::IDeviceTexture* pTexture_;

    // IO Stuff
    core::CriticalSection ioCs_;
    core::XFileAsync* pFile_;

    uint64_t fileOffset_;       // the file offset we last read from.
    uint64_t fileLength_;       // the total file length;
    int32_t fileBlocksLeft_;    //

    TrackIntArr processedBlocks_;   

    core::FixedByteStreamRingOwning ioRingBuffer_;  // buffer holding loaded IO data, ready for processing.
    DataVec ioReqBuffer_;                           // file data read into here, then moved into ringBuffer_
    int32_t ioBufferReadOffset_;
    TrackQueues trackQueues_; // packet buffer queues

    // Video buffers
    core::CriticalSection frameCs_;
    Frame* pLockedFrame_;
    FramePtrQueue availFrames_;
    int32_t frameIdx_;
    FrameArr frames_;

    int32_t displayTimeMS_; // the disaply time of the encode block.
    DataVec encodedBlock_;

    int32_t curDisplayTimeMS_;
    vpx_codec_iter_t vpxFrameIter_;

    // sound stuff
    int64_t oggPacketCount_;
    int64_t oggFramesDecoded_;

    vorbis_info      vorbisInfo_;       // struct that stores all the static vorbis bitstream settings
    vorbis_comment   vorbisComment_;    // struct that stores all the bitstream user comments
    vorbis_dsp_state vorbisDsp_;        // central working state for the packet->PCM decoder
    vorbis_block     vorbisBlock_;      // local working space for packet->PCM decode

    core::CriticalSection audioCs_;
    DataVec encodedAudioFrame_;                         // encoded audio
    AudioRingBufferChannelArr audioRingBuffers_;        // decoded audio, ready for sound system.

    sound::PlayingID sndPlayingId_;
    sound::SndObjectHandle sndObj_;

    // show me the goat.
#if X_ENABLE_VIDEO_DEBUG
    TrackFrameHistory<int16_t> queueSizes_;
    FrameHistory<int32_t> audioBufferSize_;
    FrameHistory<int32_t> ioBufferSize_;
#endif // !X_ENABLE_VIDEO_DEBUG
};

X_NAMESPACE_END

#include "XVideo.inl"