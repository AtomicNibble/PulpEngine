#include "stdafx.h"
#include "XVideo.h"

#include "IVFTypes.h"

#include <Threading\JobSystem2.h>
#include <Memory\MemCursor.h>
#include <Compression\LZ4.h>

#include <String\HumanDuration.h>
#include <String\HumanSize.h>

#include <IRenderCommands.h>
#include <CmdBucket.h>
#include <IFrameData.h>

#include <ISound.h>

#include <IFont.h>
#include <IPrimativeContext.h>

X_NAMESPACE_BEGIN(video)

namespace
{
    ogg_packet buildOggPacket(const uint8_t* pData, size_t length,
        bool bos, bool eos, int64_t aGranulepos, int64_t packetNo)
    {
        ogg_packet packet;
        packet.packet = const_cast<unsigned char*>(pData);
        packet.bytes = safe_static_cast<long>(length);
        packet.b_o_s = bos;
        packet.e_o_s = eos;
        packet.granulepos = aGranulepos;
        packet.packetno = packetNo;
        return packet;
    };

} // namespace

Video::Video(core::string name, core::MemoryArenaBase* arena) :
    core::AssetBase(name, assetDb::AssetType::VIDEO),
    frameRate_(0),
    curFrame_(0),
    state_(VideoState::Playing),
    presentFrame_(false),
    isStarved_(false),
    ioRequestPending_(false),
    pFile_(nullptr),
    fileOffset_(0),
    fileLength_(0),
    ioRingBuffer_(arena, IO_RING_BUFFER_SIZE),
    ioReqBuffer_(arena),
    encodedFrame_(arena),
    decodedFrame_(arena),
    pTexture_(nullptr),
    pDecodeJob_(nullptr),
    oggPacketCount_(0),
    oggFramesDecoded_(0),
    encodedAudioFrame_(arena),
    audioRingBuffers_{{
        { arena, AUDIO_RING_DECODED_BUFFER_SIZE },
        { arena, AUDIO_RING_DECODED_BUFFER_SIZE }
    }}
{
    core::zero_object(codec_);
    core::zero_object(video_);
    core::zero_object(audio_);

    core::zero_object(vorbisInfo_);
    core::zero_object(vorbisComment_);
    core::zero_object(vorbisDsp_);
    core::zero_object(vorbisBlock_);

    ioBufferReadOffset_ = 0;

    blocksLeft_ = 0;
}

Video::~Video()
{
    if (pFile_) {
        gEnv->pFileSys->AddCloseRequestToQue(pFile_);
    }

    auto err = vpx_codec_destroy(&codec_);
    if (err != VPX_CODEC_OK) {
        X_ERROR("Video", "Failed to destory decoder: %s", vpx_codec_err_to_string(err));
    }

    vorbis_block_clear(&vorbisBlock_);
    vorbis_dsp_clear(&vorbisDsp_);
    vorbis_info_clear(&vorbisInfo_);
    vorbis_comment_clear(&vorbisComment_);
}

void Video::update(const core::FrameTimeData& frameTimeInfo)
{
    // TODO: this assumes header finished loading.
    if (!pTexture_) {
        pTexture_ = gEnv->pRender->createTexture(
            name_.c_str(),
            Vec2i(video_.pixelWidth, video_.pixelHeight),
            texture::Texturefmt::B8G8R8A8,
            render::BufUsage::PER_FRAME);
    }

    if (state_ == VideoState::Stopped) {
        return;
    }

    if (curFrame_ == video_.numFrames) {
        state_ = VideoState::Stopped;
    }

    auto validateQueues = [&]() {
#if X_ENABLE_ASSERTIONS
        // make sure none of the queues have been seeked past.
        for (int32_t i = 0; i < trackQueues_.size(); i++) {
            auto& queue = trackQueues_[i];

            if (queue.isEmpty()) {
                continue;
            }

            auto absoluteOffset = queue.peek();
            auto readOffset = ioRingBuffer_.tell();

            if (absoluteOffset < readOffset)
            {
                auto diff = readOffset - absoluteOffset;
                X_ASSERT(false, "Item in queue has been skipped")(absoluteOffset, readOffset, diff);
                // what item?
                auto hdr = ioRingBuffer_.peek<BlockHdr>(readOffset);
                X_ASSERT(hdr.type < TrackType::ENUM_COUNT, "Invalid type")(hdr.type);
            }
        }
#endif // !X_ENABLE_ASSERTIONS
    };

    auto seekIoBuffer = [&](int32_t numBytes) {
        validateQueues();

        X_ASSERT(numBytes < ioBufferReadOffset_, "")(numBytes, ioBufferReadOffset_);
        ioRingBuffer_.skip(numBytes);
        ioBufferReadOffset_ -= numBytes;

        validateQueues();
    };

    auto popProcessed = [&](TrackType::Enum type) {
        auto& queue = trackQueues_[type];

        int32_t skippableBytes = 0;

        if (queue.isNotEmpty())
        {
            auto absoluteOffset = queue.peek();
            auto offset = ioRingBuffer_.absoluteToRelativeOffset(absoluteOffset);

            skippableBytes = safe_static_cast<int32_t>(offset);
            if (skippableBytes == 0) {
                // unprossed item right at start.
                return;
            }
        }

        // while we have processed blocks of this type skip them.
        while (ioRingBuffer_.size() > (sizeof(BlockHdr)))
        {
            auto hdr = ioRingBuffer_.peek<BlockHdr>();
            X_ASSERT(hdr.type < TrackType::ENUM_COUNT, "Invalid type")(hdr.type);

            if (hdr.type != type) {
                break;
            }

            int32_t skipSize = sizeof(hdr) + hdr.blockSize;

            // if skippableBytes is set this is how many bytes till a unprocssed block.
            // otherwise we have processed eything up untill ioBufferReadOffset_ of same type.

            if (skippableBytes > 0)
            {
                seekIoBuffer(skipSize);
                skippableBytes -= skipSize;

                if (skippableBytes == 0) {
                    return;
                }

                X_ASSERT(skippableBytes >= 0, "Negative value")(skippableBytes);
            }
            else if (ioBufferReadOffset_ > 0)
            {
                seekIoBuffer(skipSize);
            }
            else
            {
                break;
            }
        }
    };

    core::CriticalSection::ScopedLock lock(cs_);

    // add any complete packets to the queues.
    {
        int32_t offset = ioBufferReadOffset_;
        int32_t readOffset = safe_static_cast<int32_t>(ioRingBuffer_.tell());

        while (blocksLeft_ > 0 && ioRingBuffer_.size() > (sizeof(BlockHdr) + offset))
        {
            auto hdr = ioRingBuffer_.peek<BlockHdr>(offset);
            X_ASSERT(hdr.type < TrackType::ENUM_COUNT, "Invalid type")(hdr.type);

            // got this block?
            if (ioRingBuffer_.size() < hdr.blockSize + offset) {
                break;
            }

            auto& que = trackQueues_[hdr.type];
            if (que.freeSpace() == 0) {
                break;
            }

            que.push(readOffset + offset);

            offset += sizeof(hdr) + hdr.blockSize;

            --blocksLeft_;
        }

        ioBufferReadOffset_ = offset;
    }

    auto& audioQueue = trackQueues_[TrackType::Audio];
    auto& videoQueue = trackQueues_[TrackType::Video];

    // any audio packets ready?
    if (audioQueue.isNotEmpty())
    {
        auto& channel0 = audioRingBuffers_[0];

        // space in the output buffers?
        while (audioQueue.isNotEmpty() && channel0.size() < 1024 * (256 + 128))
        {
            auto absoluteOffset = audioQueue.peek();
            auto offset = ioRingBuffer_.absoluteToRelativeOffset(absoluteOffset);

            auto hdr = ioRingBuffer_.peek<BlockHdr>(offset);
            X_ASSERT(hdr.type == TrackType::Audio, "Incorrect type")();

            encodedAudioFrame_.resize(hdr.blockSize);
            ioRingBuffer_.peek(offset + sizeof(BlockHdr), encodedAudioFrame_.data(), encodedAudioFrame_.size());

            decodeAudioPacket(core::make_span(encodedAudioFrame_));

            int32_t blockSize = sizeof(BlockHdr) + hdr.blockSize;

            audioQueue.pop();

            if (offset == 0)
            {
                seekIoBuffer(blockSize);
                popProcessed(TrackType::Video);
            }
        }

        if (audioQueue.isEmpty() && channel0.isEos()) {
            X_WARNING("Video", "Audio queue starvation");
        }
    }

    if (encodedFrame_.isEmpty()) 
    {
        X_ASSERT(pDecodeJob_ == nullptr, "Deocde job not null")(pDecodeJob_);

        if (videoQueue.isNotEmpty())
        {
            // the video should always be at start?
            // if not, it means we have a audio buffer still waiting to be processed that came before this video frame
            // sounds like something that should not happen..
            auto absoluteOffset = videoQueue.peek();
            auto offset = ioRingBuffer_.absoluteToRelativeOffset(absoluteOffset);

            auto hdr = ioRingBuffer_.peek<BlockHdr>(offset);
            X_ASSERT(hdr.type == TrackType::Video, "Incorrect type")();

            encodedFrame_.resize(hdr.blockSize);
            ioRingBuffer_.peek(offset + sizeof(BlockHdr), encodedFrame_.data(), encodedFrame_.size());

            // we now want to decode the frame.
            pDecodeJob_ = gEnv->pJobSys->CreateMemberJobAndRun<Video>(this, &Video::decodeFrame_job,
                nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::VIDEO));

            int32_t blockSize = sizeof(BlockHdr) + hdr.blockSize;

            videoQueue.pop();

            if (offset == 0)
            {
                seekIoBuffer(blockSize);
                popProcessed(TrackType::Audio);
            }
            else
            {
               // X_ASSERT_UNREACHABLE();
            }

            validateQueues();
        }

        validateQueues();

        if (!pDecodeJob_) {
            isStarved_ = true;
            X_WARNING("Video", "Decode buffer starved: \"%s\"", name_.c_str());
        }
        else {
            isStarved_ = false;
        }
    }

    // dispatch a read if needed.
    if (ioRingBuffer_.freeSpace() >= IO_REQUEST_SIZE) {
        dispatchRead();
    }

    // do we want to update buffer this frame?
    // if the video has fixed fps and engine has variable frame rate.
    // we want to know if enoguth time has passed to update the frame.
    auto dt = frameTimeInfo.deltas[core::Timer::GAME];

    timeSinceLastFrame_ += dt;
    playTime_ += dt;

    if (timeSinceLastFrame_ >= timePerFrame_) {
        timeSinceLastFrame_ = core::TimeVal(0ll);
        presentFrame_ = true;
        ++curFrame_;

        if (curFrame_ == video_.numFrames) {
            state_ = VideoState::Stopped;
        }
    }
}

void Video::appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket)
{
    if (!presentFrame_) {
        X_WARNING("Video", "No frame to present");
        return;
    }

    if (isStarved_) {
        return;
    }

    // stall untill the decode has finished.
    X_ASSERT_NOT_NULL(pDecodeJob_);

    gEnv->pJobSys->Wait(pDecodeJob_);
    pDecodeJob_ = nullptr;

    // we want to submit a draw
    render::Commands::CopyTextureBufferData* pCopyCmd = bucket.addCommand<render::Commands::CopyTextureBufferData>(0, 0);
    pCopyCmd->textureId = pTexture_->getTexID();
    pCopyCmd->pData = decodedFrame_.data();
    pCopyCmd->size = safe_static_cast<uint32_t>(decodedFrame_.size());

    core::CriticalSection::ScopedLock lock(cs_);

    // clearing this will start another decode next update.
    // if we wanted to start the decode now, we would need to double buffer.

    encodedFrame_.clear();
    presentFrame_ = false;
}

render::TexID Video::getTextureID(void) const
{
    X_ASSERT_NOT_NULL(pTexture_);
    return pTexture_->getTexID();
}

bool Video::processHdr(core::XFileAsync* pFile, core::span<uint8_t> data)
{
    if (data.size() < sizeof(VideoHeader)) {
        X_ASSERT_UNREACHABLE();
        X_ERROR("Video", "Initial buffer is too small");
        return false;
    }

    core::MemCursor cursor(data.data(), data.size_bytes());

    auto& hdr = *cursor.getSeekPtr<const VideoHeader>();
    if (!hdr.isValid()) {
        X_ERROR("Video", "Invalid header");
        return false;
    }

    if (hdr.version != VIDEO_VERSION) {
        X_ERROR("Video", "Version mismatch. Got: %" PRIu32 " Wanted: %" PRIu32, static_cast<uint32_t>(hdr.version), VIDEO_VERSION);
        return false;
    }

    if (hdr.audio.channels > VIDEO_MAX_AUDIO_CHANNELS) {
        X_ERROR("Video", "Audio has unsupported channel count: %" PRIi32, vorbisInfo_.channels);
        return false;
    }

    video_ = hdr.video;
    audio_ = hdr.audio;

    auto durationMS = hdr.durationNS / 1000000;
    auto durationS = durationMS / 1000;

    duration_ = core::TimeVal::fromMS(durationMS);

    frameRate_ = safe_static_cast<int32_t>(hdr.video.numFrames / durationS);
    pFile_ = pFile;

    // audio header HYPE!
    size_t audioHdrSize = hdr.audio.deflatedHdrSize;

    if (audioHdrSize > cursor.numBytesRemaning()) {
        // sad face we don't have enougth data for the audio header yet.
        // TODO: copy what we have into the file buffer and process it once we do.
        X_ASSERT_NOT_IMPLEMENTED();
    }
    else
    {
       // Audio header hype!
        auto* pSrc = cursor.getPtr<uint8_t>();

        // process me some ogg. MY MAN!
        DataVec inflated(g_VideoArena);
        inflated.resize(hdr.audio.inflatedHdrSize);

        // infalte directly into ogg buffer. :)
        if (!core::Compression::LZ4::inflate(pSrc, hdr.audio.deflatedHdrSize, inflated.data(), hdr.audio.inflatedHdrSize)) {
            X_ERROR("Video", "Failed to infalte audio data");
            return false;
        }

        if (!processVorbisHeader(core::make_span(inflated))) {
            X_ERROR("Video", "Failed to process audio headers");
            return false;
        }
    }

    vpx_codec_iface_t* pInterface = vpx_codec_vp8_dx();

    int flags = 0;
    if (vpx_codec_dec_init(&codec_, pInterface, nullptr, flags)) {
        X_ERROR("Video", "Failed to initialize decoder: %s", vpx_codec_error_detail(&codec_));
        return false;
    }

    blocksLeft_ = video_.numFrames + audio_.numFrames;

    fileLength_ = pFile->fileSize();
    fileOffset_ = sizeof(hdr) + audioHdrSize;

    ioReqBuffer_.resize(safe_static_cast<size_t>(core::Min<uint64>(fileLength_, IO_REQUEST_SIZE)));
  
    encodedAudioFrame_.reserve(audio_.largestFrameBytes);
    encodedFrame_.reserve(video_.largestFrameBytes);

    decodedFrame_.resize(video_.pixelWidth * (video_.pixelHeight * 4));

    int64_t ms = 1000 / frameRate_;
    timePerFrame_ = core::TimeVal::fromMS(ms);

#if 1
    auto obj = gEnv->pSound->registerObject();

    sound::AudioBufferDelegate del;
    del.Bind<Video, &Video::audioDataRequest>(this);
    gEnv->pSound->playVideoAudio(audio_.channels, audio_.sampleFreq, del, obj);
#endif

    dispatchRead();
    return true;
}

bool Video::processVorbisHeader(core::span<uint8_t> data)
{
    auto decodeHeader = [&](const uint8_t* pData, size_t length) -> bool
    {
        bool bos = oggPacketCount_ == 0;
        bool eos = false;

        ogg_packet pkt = buildOggPacket(pData, length, bos, eos, 0, oggPacketCount_);
        int r = vorbis_synthesis_headerin(&vorbisInfo_, &vorbisComment_, &pkt);

        ++oggPacketCount_;

        return r == 0;
    };

    if (data.size_bytes() < 3) {
        X_ERROR("Video", "Failed to process audio headers");
        return false;
    }

    vorbis_info_init(&vorbisInfo_);
    vorbis_comment_init(&vorbisComment_);


    auto* pData = data.data();

    int32_t numHeaders = pData[0] + 1;
    if (numHeaders != 3) {
        X_ERROR("Video", "Failed to process audio headers");
        return false;
    }

    int32_t idHdrSize = pData[1];
    int32_t commentHdrSize = pData[2];
    int32_t setupHeaderSize = safe_static_cast<int32_t>(data.size_bytes()) - idHdrSize - commentHdrSize;

    auto* pIdHeader = &pData[3];
    auto* pCommentHeader = pIdHeader + idHdrSize;
    auto* pSetupHeader = pCommentHeader + commentHdrSize;

    if (!decodeHeader(pIdHeader, idHdrSize)) {
        X_ERROR("Video", "Failed to decode audio info");
        return false;
    }

    if (vorbisInfo_.channels > VIDEO_MAX_AUDIO_CHANNELS) {
        X_ERROR("Video", "Audio has unsupported channel count: %" PRIi32, vorbisInfo_.channels);
        return false;
    }

    if (!decodeHeader(pCommentHeader, commentHdrSize)) {
        X_ERROR("Video", "Failed to decode audio comments");
        return false;
    }

    if (!decodeHeader(pSetupHeader, setupHeaderSize)) {
        X_ERROR("Video", "Failed to decode audio setup data");
        return false;
    }

    if (vorbis_synthesis_init(&vorbisDsp_, &vorbisInfo_) != 0) {
        X_ERROR("Video", "Failed to init audio synthesis");
        return false;
    }

    if (vorbis_block_init(&vorbisDsp_, &vorbisBlock_) != 0) {
        X_ERROR("Video", "Failed to init audio block");
        return false;
    }

    return true;
}

bool Video::decodeAudioPacket(core::span<uint8_t> data)
{
    X_ASSERT(oggPacketCount_ >= 3, "Headers not parsed")(oggPacketCount_);

    ogg_packet pkt = buildOggPacket(data.data(), data.size(), false, false, -1, oggPacketCount_++);
    bool firstPacket = oggPacketCount_ == 4;

    int vsRest = vorbis_synthesis(&vorbisBlock_, &pkt);
    if (vsRest)
    {
        if (vsRest == OV_ENOTAUDIO) {
            X_ERROR("Video", "Failed to synth audio block: not audio");
            return false;
        }
        else if (vsRest == OV_EBADPACKET) {
            X_ERROR("Video", "Failed to synth audio block: bad packet");
            return false;
        }
        
        X_ERROR("Video", "Failed to synth audio block: %" PRIi32, vsRest);
        return false;
    }

    if (vorbis_synthesis_blockin(&vorbisDsp_, &vorbisBlock_)) {
        X_ERROR("Video", "Failed to synth audio block");
        return false;
    }

    float** pPcm = nullptr;
    int32_t frames = vorbis_synthesis_pcmout(&vorbisDsp_, &pPcm);
    if (frames == 0 && firstPacket)
    {

    }

    uint32_t channels = vorbisDsp_.vi->channels;

    X_ASSERT(channels <= 2, "Unsupported channel count")(channels);

    while (frames > 0)
    {
        {
            core::CriticalSection::ScopedLock lock(audioCs_);

            // TODO: handle full buffers.
            // can we back out of this packet and be like don't decode anymore?
            // if we pre decode a fixed amount of time tho it should give us a defined amount of output.
            // the problem is we need to provide slop, for the biggest packet.
            for (uint32_t i = 0; i < channels; i++)
            {
                float* pPcmChannel = pPcm[i];
                audioRingBuffers_[i].write(pPcmChannel, frames);
            }

            validateAudioBufferSizes();
        }

        oggFramesDecoded_ += frames;

        if (vorbis_synthesis_read(&vorbisDsp_, frames)) {
            X_ERROR("Video", "Failed to read synth audio");
            return false;
        }

        frames = vorbis_synthesis_pcmout(&vorbisDsp_, &pPcm);
    }

    return true;
}


void Video::audioDataRequest(sound::AudioBuffer& ab)
{
    core::CriticalSection::ScopedLock lock(audioCs_);

    X_ASSERT(ab.numChannels() <= VIDEO_MAX_AUDIO_CHANNELS, "Invalid channel count")(ab.numChannels());
    X_ASSERT(ab.numChannels() == vorbisInfo_.channels, "Channel count mismatch")(ab.numChannels(), vorbisInfo_.channels);

    auto& bufs = audioRingBuffers_;

    auto availFrames = safe_static_cast<int32_t>(bufs.front().size() / sizeof(float));
    auto numFrames = core::Min(availFrames, ab.maxFrames());

    // What sweet song do you have for me today?
    // -> When I gape for you! MY nipples start twitching..
    // Not that kinda song.. O.O
    for (int32_t i = 0; i < ab.numChannels(); i++)
    {
        auto* pChannelDest = ab.getChannel(i);
        bufs[i].read(pChannelDest, numFrames);
    }

    ab.setValidFrames(numFrames);

    validateAudioBufferSizes();
}

void Video::validateAudioBufferSizes(void) const
{
#if X_ENABLE_ASSERTIONS
    {
        auto& bufs = audioRingBuffers_;
        auto allSame = std::all_of(bufs.begin() + 1, bufs.end(), [&](const AudioRingBufferChannelArr::value_type& b) {
            return b.size() == bufs.front().size();
        });

        X_ASSERT(allSame, "Channel buffers are not same size")(allSame);
    }
#endif // X_ENABLE_ASSERTIONS   
}

void Video::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
    core::XFileAsync* pFile, uint32_t bytesTransferred)
{
    X_ASSERT(pRequest->getType() == core::IoRequest::READ, "Unexpected request type")();

    auto* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);
    uint8_t* pBuf = static_cast<uint8_t*>(pReadReq->pBuf);

    {
        core::CriticalSection::ScopedLock lock(cs_);

        // the io buffer has loaded, copy into ring buffer.
        X_ASSERT(ioRingBuffer_.freeSpace() >= pReadReq->dataSize, "No space to put IO data")(ioRingBuffer_.freeSpace(), pReadReq->dataSize);

        ioRingBuffer_.write(pBuf, bytesTransferred);

        ioRequestPending_ = false;

        if (ioRingBuffer_.freeSpace() >= IO_REQUEST_SIZE) {
            dispatchRead();
        }
    }
}

void Video::dispatchRead(void)
{
    core::CriticalSection::ScopedLock lock(cs_);

    // we have one IO buffer currently.
    // could add multiple if decide it's worth dispatching multiple requests.
    // i think having atleast 2 active would be worth it.
    if (ioRequestPending_) {
        return;
    }

    if (ioRingBuffer_.freeSpace() < IO_REQUEST_SIZE) {
        return;
    }

    auto bytesLeft = fileLength_ - fileOffset_;
    if (bytesLeft == 0) {
        return;
    }

    ioRequestPending_ = true;

    auto requestSize = core::Min<uint64_t>(IO_REQUEST_SIZE, bytesLeft);

    X_ASSERT(ioReqBuffer_.size() == IO_REQUEST_SIZE, "Buffer size mismatch")();

    core::IoRequestRead req;
    req.pFile = pFile_;
    req.callback.Bind<Video, &Video::IoRequestCallback>(this);
    req.pBuf = ioReqBuffer_.data();
    req.dataSize = safe_static_cast<uint32_t>(requestSize);
    req.offset = fileOffset_;
    gEnv->pFileSys->AddIoRequestToQue(req);

    fileOffset_ += requestSize;
}

bool Video::decodeFrame(void)
{
    core::CriticalSection::ScopedLock lock(cs_);

    X_ASSERT(encodedFrame_.isNotEmpty(), "Encoded frame is empty")(encodedFrame_.size());

    if (vpx_codec_decode(&codec_, encodedFrame_.data(), static_cast<uint32_t>(encodedFrame_.size()), nullptr, 0)) {
        X_ERROR("Video", "Failed to decode frame: \"%s\"", vpx_codec_error_detail(&codec_));
        return false;
    }

    vpx_codec_iter_t iter = nullptr;
    vpx_image_t* pImg = vpx_codec_get_frame(&codec_, &iter);
    if (!pImg) {
        X_ERROR("Video", "Failed to get frame: \"%s\"", vpx_codec_error_detail(&codec_));
        return false;
    }

    vpx_img_flip(pImg);

    const int w = pImg->d_w;
    const int h = pImg->d_h;

    const int strideY = pImg->stride[VPX_PLANE_Y];
    const int strideU = pImg->stride[VPX_PLANE_U];
    const int strideV = pImg->stride[VPX_PLANE_V];

    const uint8_t* pSrcY = pImg->planes[VPX_PLANE_Y];
    const uint8_t* pSrcU = pImg->planes[VPX_PLANE_U];
    const uint8_t* pSrcV = pImg->planes[VPX_PLANE_V];

    decodedFrame_.resize((w * 4) * h);

    int res = libyuv::I420ToARGB(
        pSrcY, strideY,
        pSrcU, strideU,
        pSrcV, strideV,
        decodedFrame_.data(),
        w * 4,
        w,
        h);

    if (res != 0) {
        X_ERROR("Video", "Failed to convert decoded frame");
    }

    return true;
}

void Video::decodeFrame_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys, threadIdx, pJob, pData);

    if (!decodeFrame()) {
        X_ERROR("Video", "Failed to decode frame");
    }
}

Vec2f Video::drawDebug(engine::IPrimativeContext* pPrim, Vec2f pos)
{
#if !X_ENABLE_VIDEO_DEBUG
    X_UNUSED(pPrim, pos);

    return Vec2f::zero();
#else

    // draw me some shit!
    Vec2f size(600, 60);
    Rectf r(pos, pos + size);
        
    const float padding = 16.f;

    // update graphs.
    for (int32_t i = 0; i < TrackType::ENUM_COUNT; i++) {
        queueSizes_[i].push_back(safe_static_cast<int16_t>(trackQueues_[i].size()));
    }

    audioBufferSize_.push_back(safe_static_cast<int32_t>(audioRingBuffers_.front().size()));
    ioBufferSize_.push_back(safe_static_cast<int32_t>(ioRingBuffer_.size()));

    // build linera array
    typedef core::FixedArray<float, FRAME_HISTORY_SIZE> PlotData;

    PlotData queueSizeData[TrackType::ENUM_COUNT];
    PlotData audioBufferData;
    PlotData ioBufferData;

    for (int32_t i = 0; i < TrackType::ENUM_COUNT; i++) {
        auto& queue = queueSizes_[i];
        auto& data = queueSizeData[i];

        for (auto val : queue) {
            data.push_back(static_cast<float>(val));
        }
    }

    for (auto val : audioBufferSize_) {
        audioBufferData.push_back(static_cast<float>(val));
    }

    for (auto val : ioBufferSize_) {
        ioBufferData.push_back(static_cast<float>(val));
    }

    font::TextDrawContext ctx;
    ctx.col = Col_Wheat;
    ctx.pFont = gEnv->pFontSys->getDefault();
    ctx.size = Vec2f(16.f, 16.f);

    Vec2f textOff(0, ctx.size.y + 6.f);
    Vec2f offset(0, size.y + padding);

    {
        Rectf box(r);
        box.include(Vec2f(r.x1, r.y1 + (textOff.y * 3) + ((offset.y + textOff.y) * 4)));
        box.inflate(Vec2f(10.f, 16.f));

        pPrim->drawQuad(box, Color8u(2, 2, 2, 200));
    }


    core::HumanSize::Str sizeStr0, sizeStr1;
    core::StackString<64> txt;

    core::HumanDuration::Str durStr0, durStr1;
    txt.setFmt("Duration: %s - %s (%" PRIi32 "fps)", core::HumanDuration::toString(durStr0, playTime_.GetMilliSeconds()),
        core::HumanDuration::toString(durStr1, duration_.GetMilliSeconds()), frameRate_);

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;

    txt.setFmt("Audio: BR %s %" PRIi32 "hz(%" PRIi16 ") %" PRIi16 " chan" , core::HumanSize::toString(sizeStr0, vorbisInfo_.bitrate_nominal),
        audio_.sampleFreq, audio_.bitDepth, audio_.channels);

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;

    size_t memUsage = 0;
    memUsage += ioRingBuffer_.capacity();
    memUsage += ioReqBuffer_.capacity();
    memUsage += encodedFrame_.capacity();
    memUsage += decodedFrame_.capacity();

    for (const auto& tq : trackQueues_) {
        memUsage += tq.capacity() * sizeof(IntQueue::Type);
    }

    memUsage += encodedAudioFrame_.capacity();

    for (const auto& arb : audioRingBuffers_) {
        memUsage += arb.capacity();
    }

    txt.setFmt("Mem Usage: %s", core::HumanSize::toString(sizeStr0, memUsage));

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;

    txt.setFmt("Buffer: %s/%s", core::HumanSize::toString(sizeStr0, ioRingBuffer_.size()),
        core::HumanSize::toString(sizeStr1, IO_RING_BUFFER_SIZE));

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;

    pPrim->drawGraph(r, ioBufferData.begin(), ioBufferData.end(), Col_Orange, 0.f, static_cast<float>(IO_RING_BUFFER_SIZE));
    r += offset;

    for (int32_t i = 0; i < TrackType::ENUM_COUNT; i++)
    {
        txt.setFmt("%s: %" PRIuS "/%" PRIuS, TrackType::ToString(i), trackQueues_[i].size(), FRAME_QUEUE_SIZE);
        
        auto& data = queueSizeData[i];
        pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
        r += textOff;
        pPrim->drawGraph(r, data.begin(), data.end(), Col_Orange, 0.f, static_cast<float>(FRAME_QUEUE_SIZE));
        r += offset;
    }

    txt.setFmt("AudioBuf: %s/%s",
        core::HumanSize::toString(sizeStr0, audioRingBuffers_.front().size()),
        core::HumanSize::toString(sizeStr1, AUDIO_RING_DECODED_BUFFER_SIZE));

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;
    pPrim->drawGraph(r, audioBufferData.begin(), audioBufferData.end(), Col_Orange, 0.f, static_cast<float>(AUDIO_RING_DECODED_BUFFER_SIZE));


    return size;
#endif // !X_ENABLE_VIDEO_DEBUG
}

X_NAMESPACE_END