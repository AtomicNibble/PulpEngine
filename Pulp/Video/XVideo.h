#pragma once

#include <Assets\AssetBase.h>
#include <Threading\Signal.h>
#include <Threading\FixedThreadQue.h>

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


class VideoVars;

X_DECLARE_ENUM(State)
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
    typedef core::FixedThreadQue<FrameArr::value_type*, NUM_FRAME_BUFFERS, core::CriticalSection> FramePtrThreadQueue;

    // Debug
    static constexpr int32_t FRAME_HISTORY_SIZE = 128;

    template<typename T>
    using FrameHistory = core::FixedRingBuffer<T, FRAME_HISTORY_SIZE>;

    template<typename T>
    using TrackFrameHistory = std::array<FrameHistory<T>, TrackType::ENUM_COUNT>;

public:
    Video(core::string name, const VideoVars& vars, core::MemoryArenaBase* arena);
    virtual ~Video();

    void play(void);
    void pause(void);

    void update(const core::FrameTimeData& frameTimeInfo);
    void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket);
    void releaseFrame(void);

    render::TexID getTextureID(void) const X_FINAL;

    bool processHdr(core::XFileAsync* pFile, core::span<uint8_t> data);

    Vec2f drawDebug(engine::IPrimativeContext* pPrim, Vec2f pos) const;

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
    const VideoVars& vars_;

    VideoTrackHdr vidHdr_;
    AudioTrackHdr audioHdr_;

    int32_t frameRate_;

    core::TimeVal playTime_;
    core::TimeVal duration_;

    State::Enum state_;

    // Render texture
    render::IDeviceTexture* pTexture_;

    struct IoFields
    {
        IoFields(core::MemoryArenaBase* arena) :
            pFile(nullptr),
            requestPending(false),
            fileOffset(0),
            fileLength(0),
            fileBlocksLeft(0),
            ringBuffer(arena, IO_RING_BUFFER_SIZE),
            bufferReadOffset(0),
            reqBuffer(arena)
        {
        }

        // IO Stuff
        core::CriticalSection cs;
        core::XFileAsync* pFile;

        bool requestPending;
        bool _pad[3];

        uint64_t fileOffset;       // the file offset we last read from.
        uint64_t fileLength;       // the total file length;
        int32_t fileBlocksLeft;    //

        DataVec reqBuffer;                           // file data read into here, then moved into ringBuffer_
        volatile int32_t bufferReadOffset;

        core::FixedByteStreamRingOwning ringBuffer;  // buffer holding loaded IO data, ready for processing.

        // inline
        TrackQueues trackQueues; // packet buffer queues
    };

    struct VideoFields
    {
        VideoFields(core::MemoryArenaBase* arena) :
            pDecodeJob(nullptr),
            pLockedFrame(nullptr),
            frames{
            arena,
            arena,
            arena
            },
            frameIdx(0),
            processedBlocks(0),
            encodedBlock(arena),
            displayTimeMS(0),
            curDisplayTimeMS(0)
        {
            core::zero_object(codec);
        }

        core::V2::Job* pDecodeJob;

        Frame* pLockedFrame;
        FramePtrThreadQueue availFrames;
        FrameArr frames;
        int32_t frameIdx;
        int32_t processedBlocks;

        DataVec encodedBlock;

        int32_t displayTimeMS; // the disaply time of the encode block.
        int32_t curDisplayTimeMS;

        vpx_codec_ctx_t codec;
        vpx_codec_iter_t vpxFrameIter;
    };

    struct AudioFields
    {
        AudioFields(core::MemoryArenaBase* arena) :
            pDecodeJob(nullptr),
            oggPacketCount(0),
            oggFramesDecoded(0),
            processedBlocks(0),
            encodedAudioFrame(arena),
            audioRingBuffers{ {
                { arena, AUDIO_RING_DECODED_BUFFER_SIZE },
                { arena, AUDIO_RING_DECODED_BUFFER_SIZE }
            } },
            sndPlayingId(sound::INVALID_PLAYING_ID),
            sndObj(sound::INVALID_OBJECT_ID)
        {
            core::zero_object(vorbisInfo);
            core::zero_object(vorbisComment);
            core::zero_object(vorbisDsp);
            core::zero_object(vorbisBlock);
        }

        core::V2::Job* pDecodeJob;

        int64_t oggPacketCount;
        int64_t oggFramesDecoded;
        int32_t processedBlocks;

        vorbis_info      vorbisInfo;       // struct that stores all the static vorbis bitstream settings
        vorbis_comment   vorbisComment;    // struct that stores all the bitstream user comments
        vorbis_dsp_state vorbisDsp;        // central working state for the packet->PCM decoder
        vorbis_block     vorbisBlock;      // local working space for packet->PCM decode

        core::CriticalSection audioCs;
        DataVec encodedAudioFrame;                         // encoded audio
        AudioRingBufferChannelArr audioRingBuffers;        // decoded audio, ready for sound system.

        sound::PlayingID sndPlayingId;
        sound::SndObjectHandle sndObj;
    };

    // shove in structs and align to prevent false sharing.
    X_ALIGNED_SYMBOL(IoFields io_, 64);
    X_ALIGNED_SYMBOL(VideoFields vid_, 64);
    X_ALIGNED_SYMBOL(AudioFields audio_, 64);


    // show me the goat.
#if X_ENABLE_VIDEO_DEBUG

    struct Stats 
    {
        TrackFrameHistory<int16_t> queueSizes;
        FrameHistory<int32_t> audioBufferSize;
        FrameHistory<int32_t> ioBufferSize;
    };

    // mutable so we can be sure we are read only for all other fields in debug view.
    mutable Stats stats_;
#endif // !X_ENABLE_VIDEO_DEBUG
};

X_NAMESPACE_END

#include "XVideo.inl"