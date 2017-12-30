#include "stdafx.h"
#include "XVideo.h"

#include "IVFTypes.h"

#include <Threading\JobSystem2.h>

#include <IRenderCommands.h>
#include <CmdBucket.h>
#include <IFrameData.h>


X_NAMESPACE_BEGIN(video)


Video::Video(core::string name, core::MemoryArenaBase* arena) :
	core::AssetBase(name, assetDb::AssetType::VIDEO),
	width_(0),
	height_(0),
	frameRate_(0),
	timeScale_(0),
	numFrames_(0),
	curFrame_(0),
	state_(VideoState::Playing),
	presentFrame_(false),
	isStarved_(false),
	ioRequestPending_(false),
	pFile_(nullptr),
	fileOffset_(0),
	fileLength_(0),
	ringBuffer_(arena, RING_BUFFER_SIZE),
	ioBuffer_(arena),
	encodedFrame_(arena),
	decodedFrame_(arena),
	pTexture_(nullptr),
	pDecodeJob_(nullptr)
{

	core::zero_object(codec_);
}

Video::~Video()
{
	if (pFile_) {
		gEnv->pFileSys->AddCloseRequestToQue(pFile_);
	}

	auto err = vpx_codec_destroy(&codec_);
	if (err != VPX_CODEC_OK) {
		X_ERROR("Vid", "Failed to destory decoder: %s", vpx_codec_err_to_string(err));
	}
}

void Video::update(const core::FrameTimeData& frameTimeInfo)
{
	// TODO: this assumes header finished loading.
	if (!pTexture_) {
		pTexture_ = gEnv->pRender->createTexture(
			name_.c_str(),
			Vec2i(width_, height_),
			texture::Texturefmt::B8G8R8A8,
			render::BufUsage::PER_FRAME
		);
	}

	if (state_ == VideoState::Stopped) {
		return;
	}

	if (curFrame_ == numFrames_) {
		state_ = VideoState::Stopped;
	}

	core::CriticalSection::ScopedLock lock(cs_);

	// do we need a new encoded frame?
	if (encodedFrame_.isEmpty())
	{
		X_ASSERT(pDecodeJob_ == nullptr, "Deocde job not null")(pDecodeJob_);

		if (ringBuffer_.size() > sizeof(IVFFrameHdr))
		{
			auto& hdr = ringBuffer_.peek<IVFFrameHdr>();

			const auto totalFrameSize = (hdr.frameDataSize + sizeof(IVFFrameHdr));

			if (totalFrameSize  > RING_BUFFER_SIZE) {
				X_ERROR("Video", "Single frame in \"%s\" is bigger than buffer size", name_.c_str());
			}

			if (ringBuffer_.size() >= totalFrameSize)
			{
				// we have a full frame.
				ringBuffer_.skip(sizeof(IVFFrameHdr));

				encodedFrame_.resize(hdr.frameDataSize);

				ringBuffer_.read(encodedFrame_.data(), encodedFrame_.size());

				// we now want to decode the frame.
				pDecodeJob_ = gEnv->pJobSys->CreateMemberJobAndRun<Video>(this, &Video::DecodeFrame_job,
					nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::VIDEO));
			}
		}

		if (!pDecodeJob_) {
			isStarved_ = true;
			X_WARNING("Video", "Decode buffer starved: \"%s\"", name_.c_str());
		}
		else {
			isStarved_ = false;
		}
	}

	// dispatch a read if needed.
	if (ringBuffer_.freeSpace() >= IO_BUFFER_SIZE) {
		dispatchRead();
	}

	// do we want to update buffer this frame?
	// if the video has fixed fps and engine has variable frame rate.
	// we want to know if enoguth time has passed to update the frame.

	timeSinceLastFrame_ += frameTimeInfo.deltas[core::Timer::GAME];
	if (timeSinceLastFrame_ >= timePerFrame_)
	{
		timeSinceLastFrame_ = core::TimeVal(0ll);
		presentFrame_ = true;
		++curFrame_;

		if (curFrame_ == numFrames_)
		{
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


bool Video::processHdr(core::XFileAsync* pFile, const IVFHdr& hdr)
{
	width_ = hdr.width;
	height_ = hdr.height;
	frameRate_ = hdr.frameRate;
	timeScale_ = hdr.timeScale;
	numFrames_ = hdr.numFrames;
	pFile_ = pFile;

	if (hdr.codecFourCC != FOURCC('V', 'P', '8', '0')) {
		X_ERROR("Video", "Invalid codec: %" PRIu32, hdr.codecFourCC);
		return false;
	}

	vpx_codec_iface_t* pInterface = vpx_codec_vp8_dx();

	int flags = 0;
	if (vpx_codec_dec_init(&codec_, pInterface, nullptr, flags)) {
		X_ERROR("Vid", "Failed to initialize decoder: %s", vpx_codec_error_detail(&codec_));
		return false;
	}

	fileLength_ = pFile->fileSize();
	fileOffset_ = sizeof(IVFHdr);

	ioBuffer_.resize(safe_static_cast<size_t>(core::Min<uint64>(fileLength_, IO_BUFFER_SIZE)));
	decodedFrame_.resize(width_ * (height_ * 4));
	

	int64_t ms = 1000 / frameRate_;
	timePerFrame_ = core::TimeVal::fromMS(ms);
	
	dispatchRead();
	return true;
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
		X_ASSERT(ringBuffer_.freeSpace() >= pReadReq->dataSize, "No space to put IO data")(ringBuffer_.freeSpace(), pReadReq->dataSize);
	
		ringBuffer_.write(pBuf, bytesTransferred);

		ioRequestPending_ = false;

		if (ringBuffer_.freeSpace() >= IO_BUFFER_SIZE) {
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

	if (ringBuffer_.freeSpace() < IO_BUFFER_SIZE) {
		return;
	}

	auto bytesLeft = fileLength_ - fileOffset_;
	if (bytesLeft == 0) {
		return;
	}

	ioRequestPending_ = true;

	auto requestSize = core::Min<uint64_t>(IO_BUFFER_SIZE, bytesLeft);

	core::IoRequestRead req;
	req.pFile = pFile_;
	req.callback.Bind<Video, &Video::IoRequestCallback>(this);
	req.pBuf = ioBuffer_.data();
	req.dataSize = safe_static_cast<uint32_t>(requestSize);
	req.offset = fileOffset_;
	gEnv->pFileSys->AddIoRequestToQue(req);

	fileOffset_ += requestSize;
}


bool Video::decodeFrame(void)
{
	core::CriticalSection::ScopedLock lock(cs_);

	X_ASSERT(encodedFrame_.isNotEmpty(), "Encoded frame is empty")(encodedFrame_.size());

	if (vpx_codec_decode(&codec_, encodedFrame_.data(), static_cast<uint32_t>(encodedFrame_.size()), nullptr, 0))
	{
		X_ERROR("Vid", "Failed to decode frame: \"%s\"", vpx_codec_error_detail(&codec_));
		return false;
	}

	vpx_codec_iter_t iter = nullptr;
	vpx_image_t* pImg = vpx_codec_get_frame(&codec_, &iter);
	if (!pImg) {
		X_ERROR("Vid", "Failed to get frame: \"%s\"", vpx_codec_error_detail(&codec_));
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
		h
	);
	
	if (res != 0) {
		X_ERROR("Vid", "Failed to convert decoded frame");
	}

	return true;
}

void Video::DecodeFrame_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys, threadIdx, pJob, pData);

	if (!decodeFrame())
	{
		X_ERROR("Video", "Failed to decode frame");
	}
}

X_NAMESPACE_END