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

#include <Time\StopWatch.h>

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
    state_(State::UnInit),
    io_(arena),
    vid_(arena),
    audio_(arena),
    pTexture_(nullptr)
{
    core::zero_object(vidHdr_);
    core::zero_object(audioHdr_);
}

Video::~Video()
{
    // TODO: work out way to unregister this safly.
    // since if we delte a sound object, then get a event for it, we crash :(
    if (audio_.sndObj != sound::INVALID_OBJECT_ID) {
        gEnv->pSound->unRegisterObject(audio_.sndObj);
    }

    if (io_.pFile) {
        gEnv->pFileSys->AddCloseRequestToQue(io_.pFile);
    }

    auto err = vpx_codec_destroy(&vid_.codec);
    if (err != VPX_CODEC_OK) {
        X_ERROR("Video", "Failed to destory decoder: %s", vpx_codec_err_to_string(err));
    }

    vorbis_block_clear(&audio_.vorbisBlock);
    vorbis_dsp_clear(&audio_.vorbisDsp);
    vorbis_info_clear(&audio_.vorbisInfo);
    vorbis_comment_clear(&audio_.vorbisComment);
}

void Video::play(void)
{
    X_ASSERT(state_ == State::Init, "")();


    state_ = State::Buffering;
    dispatchRead();
}

void Video::pause(void)
{
    X_ASSERT_NOT_IMPLEMENTED();
}

void Video::update(const core::FrameTimeData& frameTimeInfo)
{

    // TODO: this assumes header finished loading.
    if (!pTexture_) {
        pTexture_ = gEnv->pRender->createTexture(
            name_.c_str(),
            Vec2i(vidHdr_.pixelWidth, vidHdr_.pixelHeight),
            texture::Texturefmt::B8G8R8A8,
            render::BufUsage::PER_FRAME);
    }

    if (state_ != State::Buffering && state_ != State::Playing) {
        return;
    }

    dispatchRead();

    // dispatch decode jobs.
    processIOData();

    if (state_ == State::Playing)
    {
        auto dt = frameTimeInfo.unscaledDeltas[core::Timer::GAME];
        playTime_ += dt;
    }
  
    if (state_ == State::Buffering)
    {
        auto& channel0 = audio_.audioRingBuffers[0];

        if (vid_.availFrames.size() == vid_.availFrames.capacity() && channel0.size() > 0)
        {
            if (audio_.sndObj == sound::INVALID_OBJECT_ID) {
                audio_.sndObj = gEnv->pSound->registerObject("VideoAudio");
            }

            sound::AudioBufferDelegate del;
            del.Bind<Video, &Video::audioDataRequest>(this);
            audio_.sndPlayingId = gEnv->pSound->playVideoAudio(audioHdr_.channels, audioHdr_.sampleFreq, del, audio_.sndObj);

            state_ = State::Playing;
        }
    }

    if (playTime_ >= duration_)
    {
        gEnv->pSound->stopVideoAudio(audio_.sndPlayingId);

        // TODO: validate we played everything.

        state_ = State::Finished;
    }
}

void Video::processIOData(void)
{
    core::CriticalSection::ScopedLock lock(io_.cs);

    // add any complete packets to the queues.
    if(io_.fileBlocksLeft)
    {
        int32_t offset = io_.bufferReadOffset;
        const int32_t readOffset = safe_static_cast<int32_t>(io_.ringBuffer.tell());
        const int32_t ringAvail = safe_static_cast<int32_t>(io_.ringBuffer.size());

        while (io_.fileBlocksLeft > 0 && io_.ringBuffer.size() > (sizeof(BlockHdr) + offset))
        {
            auto hdr = io_.ringBuffer.peek<BlockHdr>(offset);
            X_ASSERT(hdr.type < TrackType::ENUM_COUNT, "Invalid type")(hdr.type);

            // got this block?
            if (ringAvail < hdr.blockSize + offset) {
                break;
            }

            auto& que = io_.trackQueues[hdr.type];
            if (que.freeSpace() == 0) {
                break;
            }

            que.push(readOffset + offset);

            offset += sizeof(hdr) + hdr.blockSize;

            --io_.fileBlocksLeft;
        }

        io_.bufferReadOffset = offset;
    }

    auto& audioQueue = io_.trackQueues[TrackType::Audio];
    auto& videoQueue = io_.trackQueues[TrackType::Video];

    const auto& channel0 = audio_.audioRingBuffers.front();

    // Audio
    if (channel0.size() < AUDIO_RING_MAX_FILL && audio_.pDecodeJob == nullptr)
    {
        if (audioQueue.isNotEmpty())
        {
            audio_.pDecodeJob = gEnv->pJobSys->CreateMemberJobAndRun<Video>(this, &Video::decodeAudio_job,
                nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::VIDEO));
        }
        else if(channel0.isEos())
        {
            X_WARNING("Video", "Audio queue starvation");
        }
    }

    // Video
    if (vid_.availFrames.freeSpace() > 0 && vid_.pDecodeJob == nullptr)
    {
        if (videoQueue.isNotEmpty())
        {
            vid_.pDecodeJob = gEnv->pJobSys->CreateMemberJobAndRun<Video>(this, &Video::decodeVideo_job,
                nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::VIDEO));
        }
        else
        {
            // are we starved?
            // Finished all the blocks?
            if (vid_.processedBlocks != vidHdr_.numBlocks) {
                X_WARNING("Video", "Decode buffer starved: \"%s\"", name_.c_str());
            }
        }
    }
}



void Video::appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket)
{
    if (vid_.availFrames.isEmpty()) {
        X_WARNING("Video", "No frame to present");
        return;
    }

    auto* pFrame = vid_.availFrames.peek();

    auto ellapsed = static_cast<int32_t>(playTime_.GetMilliSeconds());
    auto delta = pFrame->displayTime - ellapsed;

    // if it's negative frame is late by delta MS.
    if (delta > 10) {
        X_WARNING("Video", "Frame time till display: %" PRIi32, delta);
        return;
    }

    X_LOG0("Video", "Frame display delta: %" PRIi32 " ellapsed: %" PRIi32, delta, ellapsed);

#if 0
    // we want to submit a draw
    render::Commands::CopyTextureBufferData* pCopyCmd = bucket.addCommand<render::Commands::CopyTextureBufferData>(0, 0);
    pCopyCmd->textureId = pTexture_->getTexID();
    pCopyCmd->pData = pFrame->decoded.data();
    pCopyCmd->size = safe_static_cast<uint32_t>(pFrame->decoded.size());
#endif

    vid_.pLockedFrame = pFrame;
}

void Video::releaseFrame(void)
{
    if (!vid_.pLockedFrame) {
        return;
    }

    X_ASSERT(vid_.availFrames.isNotEmpty(), "Avail frames is empty")();
    X_ASSERT(vid_.pLockedFrame == vid_.availFrames.peek(), "Locked frame is not the top one")(vid_.pLockedFrame);

    vid_.pLockedFrame = nullptr;
    vid_.availFrames.pop();
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
        X_ERROR("Video", "Audio has unsupported channel count: %" PRIi32, hdr.audio.channels);
        return false;
    }

    vidHdr_ = hdr.video;
    audioHdr_ = hdr.audio;

    auto durationMS = hdr.durationNS / 1000000;
    auto durationS = durationMS / 1000;

    duration_ = core::TimeVal::fromMS(durationMS);

    // TODO: wrong
    frameRate_ = safe_static_cast<int32_t>(hdr.video.numBlocks / durationS);
    io_.pFile = pFile;

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

    vpx_codec_dec_cfg_t vpx_config = { 0 };
    vpx_config.w = vidHdr_.pixelWidth;
    vpx_config.h = vidHdr_.pixelHeight;
    vpx_config.threads = 1;

    vpx_codec_iface_t* pInterface = nullptr;  
    
    if (vidHdr_.fmt == VideoFmt::VP8) {
        pInterface  = vpx_codec_vp8_dx();
    }
    else if (vidHdr_.fmt == VideoFmt::VP9) {
        pInterface = vpx_codec_vp9_dx();
    }
    else {
        X_ASSERT_UNREACHABLE();
    }


    int flags = 0;
    if (vpx_codec_dec_init(&vid_.codec, pInterface, &vpx_config, flags)) {
        X_ERROR("Video", "Failed to initialize decoder: %s", vpx_codec_error_detail(&vid_.codec));
        return false;
    }

    io_.fileBlocksLeft = vidHdr_.numBlocks + audioHdr_.numBlocks;
    io_.fileLength = pFile->fileSize();
    io_.fileOffset = sizeof(hdr) + audioHdrSize;
    io_.reqBuffer.resize(safe_static_cast<size_t>(core::Min<uint64>(io_.fileLength, IO_REQUEST_SIZE)));
  
    vid_.encodedBlock.reserve(vidHdr_.largestBlockBytes);
    const auto frameBytes = vidHdr_.pixelWidth * (vidHdr_.pixelHeight * 4);
    for (auto& frame : vid_.frames) {
        frame.decoded.resize(frameBytes);
    }

    audio_.encodedAudioFrame.reserve(audioHdr_.largestBlockBytes);

    state_ = State::Init;
    return true;
}

bool Video::processVorbisHeader(core::span<uint8_t> data)
{
    auto decodeHeader = [&](const uint8_t* pData, size_t length) -> bool
    {
        bool bos = audio_.oggPacketCount == 0;
        bool eos = false;

        ogg_packet pkt = buildOggPacket(pData, length, bos, eos, 0, audio_.oggPacketCount);
        int r = vorbis_synthesis_headerin(&audio_.vorbisInfo, &audio_.vorbisComment, &pkt);

        ++audio_.oggPacketCount;

        return r == 0;
    };

    if (data.size_bytes() < 3) {
        X_ERROR("Video", "Failed to process audio headers");
        return false;
    }

    vorbis_info_init(&audio_.vorbisInfo);
    vorbis_comment_init(&audio_.vorbisComment);

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

    if (audio_.vorbisInfo.channels > VIDEO_MAX_AUDIO_CHANNELS) {
        X_ERROR("Video", "Audio has unsupported channel count: %" PRIi32, audio_.vorbisInfo.channels);
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

    if (vorbis_synthesis_init(&audio_.vorbisDsp, &audio_.vorbisInfo) != 0) {
        X_ERROR("Video", "Failed to init audio synthesis");
        return false;
    }

    if (vorbis_block_init(&audio_.vorbisDsp, &audio_.vorbisBlock) != 0) {
        X_ERROR("Video", "Failed to init audio block");
        return false;
    }

    return true;
}


sound::BufferResult::Enum Video::audioDataRequest(sound::AudioBuffer& ab)
{
    core::CriticalSection::ScopedLock lock(audio_.audioCs);

    X_ASSERT(ab.numChannels() <= VIDEO_MAX_AUDIO_CHANNELS, "Invalid channel count")(ab.numChannels());
    X_ASSERT(ab.numChannels() == audio_.vorbisInfo.channels, "Channel count mismatch")(ab.numChannels(), audio_.vorbisInfo.channels);

    auto& bufs = audio_.audioRingBuffers;
    auto& channel0 = bufs.front();

    if (channel0.isEos()) {

        // are we done?
        if (audio_.processedBlocks == audioHdr_.numBlocks) {
            return sound::BufferResult::NoMoreData;
        }

        return sound::BufferResult::NoDataReady;
    }

    auto availFrames = safe_static_cast<int32_t>(channel0.size() / sizeof(float));
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
    return sound::BufferResult::DataReady;
}

void Video::validateAudioBufferSizes(void) const
{
#if X_ENABLE_ASSERTIONS
    {
        auto& bufs = audio_.audioRingBuffers;
        auto allSame = std::all_of(bufs.begin() + 1, bufs.begin() + audioHdr_.channels, [&](const AudioRingBufferChannelArr::value_type& b) {
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
        core::CriticalSection::ScopedLock lock(io_.cs);

        // the io buffer has loaded, copy into ring buffer.
        X_ASSERT(io_.ringBuffer.freeSpace() >= pReadReq->dataSize, "No space to put IO data")(io_.ringBuffer.freeSpace(), pReadReq->dataSize);

        io_.ringBuffer.write(pBuf, bytesTransferred);
        io_.requestPending = false;

        if (io_.ringBuffer.freeSpace() >= IO_REQUEST_SIZE) {
            dispatchRead();
        }
    }
}

void Video::dispatchRead(void)
{
    core::CriticalSection::ScopedLock lock(io_.cs);

    // we have one IO buffer currently.
    // could add multiple if decide it's worth dispatching multiple requests.
    // i think having atleast 2 active would be worth it.
    if (io_.requestPending) {
        return;
    }

    if (io_.ringBuffer.freeSpace() < IO_REQUEST_SIZE) {
        return;
    }

    auto bytesLeft = io_.fileLength - io_.fileOffset;
    if (bytesLeft == 0) {
        return;
    }

    io_.requestPending = true;

    auto requestSize = core::Min<uint64_t>(IO_REQUEST_SIZE, bytesLeft);

    X_ASSERT(io_.reqBuffer.size() == IO_REQUEST_SIZE, "Buffer size mismatch")();

    core::IoRequestRead req;
    req.pFile = io_.pFile;
    req.callback.Bind<Video, &Video::IoRequestCallback>(this);
    req.pBuf = io_.reqBuffer.data();
    req.dataSize = safe_static_cast<uint32_t>(requestSize);
    req.offset = io_.fileOffset;
    gEnv->pFileSys->AddIoRequestToQue(req);

    io_.fileOffset += requestSize;
}

void Video::validateQueues(void)
{
#if X_ENABLE_ASSERTIONS
    core::CriticalSection::ScopedLock lock(io_.cs);

    // make sure none of the queues have been seeked past.
    for (size_t i = 0; i < io_.trackQueues.size(); i++) {
        auto& queue = io_.trackQueues[i];

        if (queue.isEmpty()) {
            continue;
        }

        auto absoluteOffset = queue.peek();
        auto readOffset = safe_static_cast<int32_t>(io_.ringBuffer.tell());

        if (absoluteOffset < readOffset)
        {
            auto diff = readOffset - absoluteOffset;
            X_ASSERT(false, "Item in queue has been skipped")(absoluteOffset, readOffset, diff);
            // what item?
            auto hdr = io_.ringBuffer.peek<BlockHdr>(readOffset);
            X_ASSERT(hdr.type < TrackType::ENUM_COUNT, "Invalid type")(hdr.type);
        }
    }
#endif // !X_ENABLE_ASSERTIONS
};

void Video::seekIoBuffer(int32_t numBytes)
{
    validateQueues();

    X_ASSERT(numBytes <= io_.bufferReadOffset, "")(numBytes, io_.bufferReadOffset);
    io_.ringBuffer.skip(numBytes);
    io_.bufferReadOffset -= numBytes;

    validateQueues();
};

void Video::popProcessed(TrackType::Enum type)
{
    core::CriticalSection::ScopedLock lock(io_.cs);
    
    int32_t skippableBytes = 0;

    auto& queue = io_.trackQueues[type];
    if (queue.isNotEmpty())
    {
        auto absoluteOffset = queue.peek();
        auto offset = io_.ringBuffer.absoluteToRelativeOffset(absoluteOffset);

        skippableBytes = safe_static_cast<int32_t>(offset);
        if (skippableBytes == 0) {
            // unprossed item right at start.
            return;
        }
    }

    // while we have processed blocks of this type skip them.
    while (io_.ringBuffer.size() > (sizeof(BlockHdr)))
    {
        auto hdr = io_.ringBuffer.peek<BlockHdr>();
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
        else if (io_.bufferReadOffset > 0)
        {
            seekIoBuffer(skipSize);
        }
        else
        {
            break;
        }
    }
}

bool Video::decodeAudioPacket(void)
{
    X_ASSERT(audio_.oggPacketCount >= 3, "Headers not parsed")(audio_.oggPacketCount);
    X_ASSERT(audio_.encodedAudioFrame.isNotEmpty(), "Audio packet empty")();

    ogg_packet pkt = buildOggPacket(audio_.encodedAudioFrame.data(), audio_.encodedAudioFrame.size(), false, false, -1, audio_.oggPacketCount++);
    bool firstPacket = audio_.oggPacketCount == 4;

    // TODO: use block size to calculate PCM size.
    // auto size = vorbis_packet_blocksize(&audio_.vorbisInfo, &pkt);

    int vsRest = vorbis_synthesis(&audio_.vorbisBlock, &pkt);
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

    if (vorbis_synthesis_blockin(&audio_.vorbisDsp, &audio_.vorbisBlock)) {
        X_ERROR("Video", "Failed to synth audio block");
        return false;
    }

    float** pPcm = nullptr;
    int32_t frames = vorbis_synthesis_pcmout(&audio_.vorbisDsp, &pPcm);
    if (frames == 0 && firstPacket)
    {

    }

    uint32_t channels = audio_.vorbisDsp.vi->channels;

    X_ASSERT(channels <= 2, "Unsupported channel count")(channels);

    while (frames > 0)
    {
        {
            core::CriticalSection::ScopedLock lock(audio_.audioCs);

            // TODO: handle full buffers.
            // can we back out of this packet and be like don't decode anymore?
            // if we pre decode a fixed amount of time tho it should give us a defined amount of output.
            // the problem is we need to provide slop, for the biggest packet.
            for (uint32_t i = 0; i < channels; i++)
            {
                float* pPcmChannel = pPcm[i];
                audio_.audioRingBuffers[i].write(pPcmChannel, frames);
            }

            validateAudioBufferSizes();
        }

        audio_.oggFramesDecoded += frames;

        if (vorbis_synthesis_read(&audio_.vorbisDsp, frames)) {
            X_ERROR("Video", "Failed to read synth audio");
            return false;
        }

        frames = vorbis_synthesis_pcmout(&audio_.vorbisDsp, &pPcm);
    }

    return true;
}

void Video::decodeAudio_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys, threadIdx, pJob, pData);

    auto& audioQueue = io_.trackQueues[TrackType::Audio];
    auto& channel0 = audio_.audioRingBuffers[0];

    // process packets till we either run out of packets or output buffer full.
    io_.cs.Enter();

    while(audioQueue.isNotEmpty() && channel0.size() < AUDIO_RING_MAX_FILL)
    {
        auto absoluteOffset = audioQueue.peek();
        auto offset = io_.ringBuffer.absoluteToRelativeOffset(absoluteOffset);

        auto hdr = io_.ringBuffer.peek<BlockHdr>(offset);
        X_ASSERT(hdr.type == TrackType::Audio, "Incorrect type")();

        audio_.encodedAudioFrame.resize(hdr.blockSize);
        io_.ringBuffer.peek(offset + sizeof(BlockHdr), audio_.encodedAudioFrame.data(), audio_.encodedAudioFrame.size());

        io_.cs.Leave();

        if (!decodeAudioPacket()) {
            X_ERROR("Vidoe", "Failed to decode audio packet");
        }

        io_.cs.Enter();

        ++audio_.processedBlocks;

        const int32_t blockSize = sizeof(BlockHdr) + hdr.blockSize;

        audioQueue.pop();

        if (offset == 0)
        {
            seekIoBuffer(blockSize);
            popProcessed(TrackType::Video);
        }
    }

    io_.cs.Leave();

    audio_.pDecodeJob = nullptr;
}

bool Video::decodeVideo(void)
{
    auto processImg = [&](vpx_image_t* pImg, Frame& frame) -> bool {
        if (pImg->fmt != VPX_IMG_FMT_I420) {
            X_ERROR("Video", "Failed to get frame: \"%s\"", vpx_codec_error_detail(&vid_.codec));
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

        auto requiredSize = (w * 4) * h;
        X_ASSERT(requiredSize == frame.decoded.size(), "Size mismatch")(requiredSize, frame.decoded.size());

        // only fails with bad args.
        int res = libyuv::I420ToARGB(
            pSrcY, strideY,
            pSrcU, strideU,
            pSrcV, strideV,
            frame.decoded.data(),
            w * 4,
            w,
            h);

        X_ASSERT(res == 0, "Failed to convert video data")(res);
        return true;
    };

    auto decodeBlock = [&]() -> bool {
        if (vpx_codec_decode(&vid_.codec, vid_.encodedBlock.data(), static_cast<uint32_t>(vid_.encodedBlock.size()), nullptr, 0)) {
            X_ERROR("Video", "Failed to decode frame: \"%s\"", vpx_codec_error_detail(&vid_.codec));
            return false;
        }

        vid_.curDisplayTimeMS = vid_.displayTimeMS;
        vid_.encodedBlock.clear();
        return true;
    };

    X_ASSERT(vid_.availFrames.freeSpace() > 0, "decode frame called, when no avalible frame buffers")(vid_.availFrames.size(), vid_.availFrames.capacity());

    // any left over frames?
    if (!vid_.vpxFrameIter) {
        if (!decodeBlock()) {
            return false;
        }
    }

retry:
    while (1)
    {
        vpx_image_t* pImg = vpx_codec_get_frame(&vid_.codec, &vid_.vpxFrameIter);
        if (!pImg) {

            vid_.vpxFrameIter = nullptr;

            // no block to decode.
            if (vid_.encodedBlock.isEmpty()) {
                return true;
            }

            if (!decodeBlock()) {
                return false;
            }

            goto retry;
        }

        auto& frame = vid_.frames[vid_.frameIdx % NUM_FRAME_BUFFERS];

        if (!processImg(pImg, frame)) {
            return false;
        }

        frame.displayTime = vid_.curDisplayTimeMS;

        ++vid_.frameIdx;

        vid_.availFrames.push(&frame);
        if (vid_.availFrames.freeSpace() == 0) {
            return true;
        }
    }

    return true;
}

void Video::decodeVideo_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys, threadIdx, pJob, pData);

    io_.cs.Enter();

    auto& videoQueue = io_.trackQueues[TrackType::Video];

    // decode frames till we filled frame buffer.
    while (vid_.availFrames.freeSpace() > 0 && videoQueue.isNotEmpty())
    {
        auto absoluteOffset = videoQueue.peek();
        auto offset = io_.ringBuffer.absoluteToRelativeOffset(absoluteOffset);

        auto hdr = io_.ringBuffer.peek<BlockHdr>(offset);
        X_ASSERT(hdr.type == TrackType::Video, "Incorrect type")();

        vid_.encodedBlock.resize(hdr.blockSize);
        io_.ringBuffer.peek(offset + sizeof(BlockHdr), vid_.encodedBlock.data(), vid_.encodedBlock.size());

        io_.cs.Leave();

        vid_.displayTimeMS = hdr.timeMS;

        ++vid_.processedBlocks;

        if (!decodeVideo()) {
            X_ERROR("Video", "Failed to decode video frame");
        }

        io_.cs.Enter();

        const int32_t blockSize = sizeof(BlockHdr) + hdr.blockSize;

        videoQueue.pop();

        if (offset == 0)
        {
            seekIoBuffer(blockSize);
            popProcessed(TrackType::Audio);
        }

        validateQueues();
    }

    io_.cs.Leave();

    vid_.pDecodeJob = nullptr;
}

Vec2f Video::drawDebug(engine::IPrimativeContext* pPrim, Vec2f pos) const
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
        stats_.queueSizes[i].push_back(safe_static_cast<int16_t>(io_.trackQueues[i].size()));
    }

    stats_.audioBufferSize.push_back(safe_static_cast<int32_t>(audio_.audioRingBuffers.front().size()));
    stats_.ioBufferSize.push_back(safe_static_cast<int32_t>(io_.ringBuffer.size()));

    // build linera array
    typedef core::FixedArray<float, FRAME_HISTORY_SIZE> PlotData;

    PlotData queueSizeData[TrackType::ENUM_COUNT];
    PlotData audioBufferData;
    PlotData ioBufferData;

    for (int32_t i = 0; i < TrackType::ENUM_COUNT; i++) {
        auto& queue = stats_.queueSizes[i];
        auto& data = queueSizeData[i];

        for (auto val : queue) {
            data.push_back(static_cast<float>(val));
        }
    }

    for (auto val : stats_.audioBufferSize) {
        audioBufferData.push_back(static_cast<float>(val));
    }

    for (auto val : stats_.ioBufferSize) {
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


    core::HumanSize::Str sizeStr0, sizeStr1, sizeStr2;
    core::StackString<64> txt;

    core::HumanDuration::Str durStr0, durStr1;
    txt.setFmt("Duration: %s - %s (%" PRIi32 "fps) - %s", core::HumanDuration::toString(durStr0, playTime_.GetMilliSeconds()),
        core::HumanDuration::toString(durStr1, duration_.GetMilliSeconds()), frameRate_, State::ToString(state_));

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;

    txt.setFmt("Audio: BR %s %" PRIi32 "hz(%" PRIi16 ") %" PRIi16 " chan %" PRIi32 " / %" PRIi32, core::HumanSize::toString(sizeStr0, audio_.vorbisInfo.bitrate_nominal),
        audioHdr_.sampleFreq, audioHdr_.bitDepth, audioHdr_.channels, audio_.processedBlocks, audioHdr_.numBlocks);

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;

    txt.setFmt("Video: %" PRIi32 "x%" PRIi32 " %" PRIi32 " / %" PRIi32, vidHdr_.pixelWidth, vidHdr_.pixelHeight,
        vid_.processedBlocks, vidHdr_.numBlocks);

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;

    size_t memUsage = 0;
    memUsage += io_.ringBuffer.capacity();
    memUsage += io_.reqBuffer.capacity();
    memUsage += vid_.encodedBlock.capacity();

    for (auto& frame : vid_.frames) {
        memUsage += frame.decoded.capacity();
    }

    for (const auto& tq : io_.trackQueues) {
        memUsage += tq.capacity() * sizeof(IntQueue::Type);
    }

    memUsage += audio_.encodedAudioFrame.capacity();

    for (const auto& arb : audio_.audioRingBuffers) {
        memUsage += arb.capacity();
    }

    txt.setFmt("Mem Usage: %s", core::HumanSize::toString(sizeStr0, memUsage));

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;

    auto totalblocks = vidHdr_.numBlocks + audioHdr_.numBlocks;

    txt.setFmt("Buffer: %s/%s %" PRIi32 "-%" PRIi32 " File: %s", core::HumanSize::toString(sizeStr0, io_.ringBuffer.size()),
        core::HumanSize::toString(sizeStr1, IO_RING_BUFFER_SIZE), totalblocks - io_.fileBlocksLeft, totalblocks,
        core::HumanSize::toString(sizeStr2, io_.fileLength));

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;

    pPrim->drawGraph(r, ioBufferData.begin(), ioBufferData.end(), Col_Orange, 0.f, static_cast<float>(IO_RING_BUFFER_SIZE));
    r += offset;

    for (int32_t i = 0; i < TrackType::ENUM_COUNT; i++)
    {
        txt.setFmt("%s: %" PRIuS "/%" PRIuS, TrackType::ToString(i), io_.trackQueues[i].size(), FRAME_QUEUE_SIZE);
        
        auto& data = queueSizeData[i];
        pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
        r += textOff;
        pPrim->drawGraph(r, data.begin(), data.end(), Col_Orange, 0.f, static_cast<float>(FRAME_QUEUE_SIZE));
        r += offset;
    }

    txt.setFmt("AudioBuf: %s/%s",
        core::HumanSize::toString(sizeStr0, audio_.audioRingBuffers.front().size()),
        core::HumanSize::toString(sizeStr1, AUDIO_RING_DECODED_BUFFER_SIZE));

    pPrim->drawText(Vec3f(r.getUpperLeft()), ctx, txt.begin(), txt.end());
    r += textOff;
    pPrim->drawGraph(r, audioBufferData.begin(), audioBufferData.end(), Col_Orange, 0.f, static_cast<float>(AUDIO_RING_DECODED_BUFFER_SIZE));


    return size;
#endif // !X_ENABLE_VIDEO_DEBUG
}

X_NAMESPACE_END