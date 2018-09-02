#pragma once

#include <Assets\AssetBase.h>
#include <Threading\Signal.h>

#include <Containers\FixedByteStreamRing.h>
#include <Time\TimeVal.h>

X_NAMESPACE_DECLARE(core,
                    struct IoRequestBase;
                    struct XFileAsync);

X_NAMESPACE_BEGIN(video)

struct IVFHdr;

struct FrameData
{
    size_t dataSize;
    const uint8_t* pData;
};

X_DECLARE_ENUM8(VideoState)
(
    Playing,
    Stopped);

class Video : public core::AssetBase
    , public IVideo
{
    const size_t IO_BUFFER_SIZE = 1024 * 128;
    const size_t RING_BUFFER_SIZE = 1024 * 1024 * 1; // 1MB

    typedef core::Array<uint8_t, core::ArrayAlignedAllocatorFixed<uint8_t, 64>, core::growStrat::Multiply> DataVec;

public:
    Video(core::string name, core::MemoryArenaBase* arena);
    virtual ~Video();

    void update(const core::FrameTimeData& frameTimeInfo);
    void appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket);

    render::TexID getTextureID(void) const X_FINAL;

    bool processHdr(core::XFileAsync* pFile, const IVFHdr& hdr);

    X_INLINE VideoState::Enum getState(void) const;
    X_INLINE uint16_t getWidth(void) const;
    X_INLINE uint16_t getHeight(void) const;
    X_INLINE uint32_t getFps(void) const;
    X_INLINE uint32_t getNumFrames(void) const;
    X_INLINE uint32_t getCurFrame(void) const;
    X_INLINE size_t getBufferSize(void) const;
    X_INLINE bool hasFrame(void) const;

private:
    void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
        core::XFileAsync* pFile, uint32_t bytesTransferred);

    void dispatchRead(void);

    bool decodeFrame(void);

private:
    void decodeFrame_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
    vpx_codec_ctx_t codec_;

    uint16_t width_;
    uint16_t height_;
    uint32_t frameRate_;
    uint32_t timeScale_;
    uint32_t numFrames_;
    uint32_t curFrame_;

    core::TimeVal timeSinceLastFrame_;
    core::TimeVal timePerFrame_;

    VideoState::Enum state_;
    bool presentFrame_;
    bool isStarved_;
    bool ioRequestPending_;

    core::V2::Job* pDecodeJob_;

    // Loading stuff
    core::CriticalSection cs_;

    core::XFileAsync* pFile_;

    uint64_t fileOffset_; // the file offset we last read from.
    uint64_t fileLength_; // the total file length;

    core::FixedByteStreamRingOwning ringBuffer_; // buffer that hold pending IO data.

    DataVec ioBuffer_;     // file data read into here
    DataVec encodedFrame_; // a frame that's about to be decoded is place here.
    DataVec decodedFrame_; // a decoded frame, read for uploading to gpu.

    // device
    render::IDeviceTexture* pTexture_;
};

X_NAMESPACE_END

#include "XVideo.inl"