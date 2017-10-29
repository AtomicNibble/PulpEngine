#include "stdafx.h"
#include "XVideo.h"

#include "IVFTypes.h"


X_NAMESPACE_BEGIN(video)


Video::Video(core::string name, core::MemoryArenaBase* arena) :
	core::AssetBase(name, assetDb::AssetType::VIDEO),
	width_(0),
	height_(0),
	frameRate_(0),
	timeScale_(0),
	numFrames_(0),
	signal_(true),
	pFile_(nullptr),
	fileOffset_(0),
	fileLength_(0),
	readBlock_(0),
	writeBlock_(0),
	totalBlocks_(0),
	loadedBuffers_(arena),
	buffer_(arena)
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

	// lets just ring buffer this in, .
	fileLength_ = pFile->fileSize();
	fileOffset_ = sizeof(IVFHdr);

	buffer_.resize(BUFFER_SIZE);
	totalBlocks_ = safe_static_cast<int32_t>(buffer_.size() / IO_REQUEST_GRAN);

	loadedBuffers_.resize(totalBlocks_);
	std::fill(loadedBuffers_.begin(), loadedBuffers_.end(), false);

	dispatchReads();
	return true;
}


void Video::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	X_ASSERT(pRequest->getType() == core::IoRequest::READ, "Unexpected request type")();

	auto* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);
	uint8_t* pBuf = static_cast<uint8_t*>(pReadReq->pBuf);

	auto bufferIdx = (pBuf - buffer_.data()) / IO_REQUEST_GRAN;

	{
		core::CriticalSection::ScopedLock lock(cs_);
		loadedBuffers_[bufferIdx] = true;

		--loadingBlocks_;
	}

	signal_.raise();
}

int32_t Video::avalibleBlocks(void) const
{
	if (readBlock_ > writeBlock_) {
		return (totalBlocks_ - readBlock_) + writeBlock_;
	}

	return writeBlock_ - readBlock_;
}


void Video::dispatchReads(void)
{
	// if the file read offset matches the base read + buffer size.
	// we have nothing to dispatch.


	auto numRequests = core::Min(IO_REQUEST_ACTIVE, totalBlocks_);

	{
		core::CriticalSection::ScopedLock lock(cs_);

		if (readBlock_ == writeBlock_)
		{
			// assume empty.
		}
		else if (readBlock_ < writeBlock_)
		{
			numRequests = (readBlock_ - readBlock_);
		}
		else if (readBlock_ > writeBlock_)
		{
			auto toEnd = (totalBlocks_ - readBlock_);
			auto toWrite = writeBlock_;

			numRequests = toEnd + toWrite;
		}
	}

	core::IoRequestRead req;
	req.pFile = pFile_;
	req.callback.Bind<Video, &Video::IoRequestCallback>(this);

	for (int32_t i = 0; i < numRequests; i++)
	{
		auto bufferIdx = (readBlock_ + i) % totalBlocks_;
		auto left = fileLength_ - fileOffset_;
		auto requestSize = core::Min<uint64_t>(IO_REQUEST_GRAN, left);

		{
			core::CriticalSection::ScopedLock lock(cs_);
			loadedBuffers_[bufferIdx] = false; // mark buffer invalid.
		}

		req.pBuf = &buffer_[bufferIdx * IO_REQUEST_GRAN];
		req.dataSize = safe_static_cast<uint32_t>(requestSize);
		req.offset = fileOffset_;
		gEnv->pFileSys->AddIoRequestToQue(req);

		fileOffset_ += requestSize;
	}
}


X_NAMESPACE_END