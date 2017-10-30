#include "stdafx.h"
#include "XVideo.h"

#include "IVFTypes.h"

#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(video)


Video::Video(core::string name, core::MemoryArenaBase* arena) :
	core::AssetBase(name, assetDb::AssetType::VIDEO),
	width_(0),
	height_(0),
	frameRate_(0),
	timeScale_(0),
	numFrames_(0),
	pFile_(nullptr),
	fileOffset_(0),
	fileLength_(0),
	ringBuffer_(arena, RING_BUFFER_SIZE),
	ioBuffer_(arena),
	encodedFrame_(arena),
	decodedFrame_(arena)
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
		X_ASSERT(ringBuffer_.freeSpace() > pReadReq->dataSize, "No space to put IO data")();

		ringBuffer_.write(pBuf, bytesTransferred);

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

		3. as the data gets loaded from file, copy it i
			- the decoding is done in a job, max 1 at a time.
			- data will continue to get read from file.
		4

	*/


}

void Video::dispatchRead(void)
{
	core::CriticalSection::ScopedLock lock(cs_);

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

	if (vpx_codec_decode(&codec_, encodedFrame_.data(), static_cast<uint32_t>(encodedFrame_.size()), nullptr, 0))
	{
		const char* pDetail = vpx_codec_error_detail(&codec_);
		return false;
	}

	vpx_codec_iter_t iter = nullptr;
	vpx_image_t* pImg = vpx_codec_get_frame(&codec_, &iter);
	if (!pImg) {

		return false;
	}

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
		// ?
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