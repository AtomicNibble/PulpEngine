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
	pFile_(nullptr),
	fileOffset_(0),
	fileLength_(0),
	ringBuffer_(arena, RING_BUFFER_SIZE),
	ioBuffer_(arena),
	encodedFrame_(arena),
	decodedFrame_(arena),
	pTexture_(nullptr)
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
	/*
		we need to tick..

		we need to keep filling the IO buffer untill it's full or we have read the whole file.
		we should not wait a frame before dispatching each io request.
		so after each IO request just see if we can dispach another, so IO is not limited by fps.
		
		when the io buffer is full tho, we will need to trigger IO request after eating data from the iobuffer.

		when we have enougth data to decode a frame we decode it.
		only if we have a fre ebuffer to decode into.
		might be nice to have two decoded frame buffers, so can have next ready.

		we only want to update the gpu buffer at the frame rate.
		so i need to track when the camel came to play.



	*/

	// TODO: this assumes header finished loading.
	if (!pTexture_) {
		pTexture_ = gEnv->pRender->createTexture(
			name_.c_str(),
			Vec2i(width_, height_),
			texture::Texturefmt::B8G8R8A8
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
		if (ringBuffer_.size() > sizeof(IVFFrameHdr))
		{
			auto& hdr = ringBuffer_.peek<IVFFrameHdr>();

			if (ringBuffer_.size() >= (hdr.frameDataSize + sizeof(IVFFrameHdr)))
			{
				// we have a full frame.
				ringBuffer_.skip(sizeof(IVFFrameHdr));

				encodedFrame_.resize(hdr.frameDataSize);

				ringBuffer_.read(encodedFrame_.data(), encodedFrame_.size());

				// we now want to decode the frame.
				gEnv->pJobSys->CreateMemberJobAndRun<Video>(this, &Video::DecodeFrame_job,
					nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::VIDEO));
			}
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

	vpx_codec_iface_t* interface = vpx_codec_vp8_dx();

	int flags = 0;
	if (vpx_codec_dec_init(&codec_, interface, nullptr, flags)) {
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


		if (ringBuffer_.freeSpace() >= IO_BUFFER_SIZE) {
			dispatchRead();
		}
#if 0
		// look for full frame.
		if (ringBuffer_.size() > sizeof(IVFFrameHdr))
		{
			auto& hdr = ringBuffer_.peek<IVFFrameHdr>();

			if (ringBuffer_.size() >= (hdr.frameDataSize + sizeof(IVFFrameHdr)))
			{
				// we have a full frame.
				ringBuffer_.skip(sizeof(IVFFrameHdr));

				encodedFrame_.resize(hdr.frameDataSize);

				ringBuffer_.read(encodedFrame_.data(), encodedFrame_.size());

				// we now want to decode the frame.

				gEnv->pJobSys->CreateMemberJobAndRun<Video>(this, &Video::DecodeFrame_job,
					nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::VIDEO));
			}
		}
#endif
	}

	/*
		1. read the header.
		2. allocate buffer for IO requests.
		3. when data is loaded, copy it into ring buffer, and dispatch another request
			- only one in flight request at a time.
		4. when the ring buffer has a full frame in it, copy the frame to temp memory.
		5. dispatch a job to decode this frame.
		6. dispatch a job to convert the decoded data to RGBA.
			- when finished, we are able to decode another frame.

		7. we have a frame.

		8. copy the decoded frame to device texture.
			- means we need to decoded frame to stay in memory for basically a whole frame.
					which means we can't decode the next one till that has happend.

			- if the video is been played in multiple locations, should just use same texture.
			- but if we have multiple textures it's okay for now.

			- who will create the device texture?
			- i think the video can create texture, that way we can just submit update commands here.

		so now we create and update a device texture.
		we have a texture id that can be used to render with.
		we just need to make it work with materials.

		like a mterial needs to say i want 'this' videos texture please.
		well we need like a techdef in order to setup the correct state and video params.
		then the material needs to select the def.

		how do i link this fucking texture lol.
		when i load the material i should work it out and set the texture id.

		techdefs can refrence textures that will only exsist at runtime, just like the fonts.
		but we must wire them in, the 3dengine needs todo this.

		the material needs to store what video tho.
		how should the engine know this kinda shit needs to be done.



	*/


}

void Video::dispatchRead(void)
{
	core::CriticalSection::ScopedLock lock(cs_);

	if (ringBuffer_.freeSpace() < IO_BUFFER_SIZE) {
		return;
	}

	auto bytesLeft = fileLength_ - fileOffset_;
	if (bytesLeft == 0) {
		return;
	}

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