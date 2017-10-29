#include "stdafx.h"
#include "XVideo.h"

#include <algorithm>

X_NAMESPACE_BEGIN(video)



MkvReader::MkvReader(core::XFileAsync* pFile, core::MemoryArenaBase* arena) :
	signal_(true),
	buffer_(arena),
	loadedBuffers_(arena),
	pFile_(X_ASSERT_NOT_NULL(pFile)),
	readFileOffset_(0),
	offset_(0),
	readBlock_(0),
	writeBlock_(0),
	totalBlocks_(0)
{
	length_ = pFile->fileSize();

	// if the video file is really small basically make a buffer big enougth for it.
	if (length_ < BUFFER_SIZE)
	{
		auto bufferSize = core::bitUtil::RoundUpToMultiple<size_t>(length_, IO_REQUEST_GRAN);

		buffer_.resize(bufferSize);
	}
	else
	{
		buffer_.resize(BUFFER_SIZE);
	}

	X_ASSERT((buffer_.size() % IO_REQUEST_GRAN) == 0, "Buffer must be a multiple of request gran")();

	totalBlocks_ = safe_static_cast<int32_t>(buffer_.size() / IO_REQUEST_GRAN);

	loadedBuffers_.resize(totalBlocks_);
	std::fill(loadedBuffers_.begin(), loadedBuffers_.end(), false);
}

int32_t MkvReader::avalibleBlocks(void) const
{
	if (readBlock_ > writeBlock_) {
		return (totalBlocks_ - readBlock_) + writeBlock_;
	}

	return writeBlock_ - readBlock_;
}

int MkvReader::Read(long long offset, long len, unsigned char* buf)
{

	if (offset < 0 || len < 0) {
		return -1;
	}
	if (len == 0) {
		return 0;
	}
	if (static_cast<uint64_t>(offset) >= length_) {
		return -1;
	}


	auto readOffset = safe_static_cast<int32_t>(offset - readFileOffset_);
	auto blockOffset = readOffset / IO_REQUEST_GRAN;
	auto readBlocks = (len / IO_REQUEST_GRAN) + 1;
	auto requiredBlocks = (blockOffset + readBlocks);

	auto avail = avalibleBlocks();

	if(avail < requiredBlocks)
	{
		fillBufffer();

		// we need more blocks than we have.
		auto need = requiredBlocks - avail;

		for (int32_t i=0; i<need; ++i)
		{
			// wait for next block to load.
			while (!loadedBuffers_[writeBlock_]) {
				signal_.wait();
			}

			loadedBuffers_[writeBlock_] = false;

			++writeBlock_;
			if (writeBlock_ == totalBlocks_) {
				writeBlock_ = 0;
			}
		}
	}

	X_ASSERT(avalibleBlocks() >= requiredBlocks, "We don't have enougth blocks")(avalibleBlocks(), requiredBlocks);

	// looking back too far?
	if (offset < readFileOffset_) {
		return -1;
	}

	auto* pReadStart = &buffer_[readBlock_ * IO_REQUEST_GRAN];

	if (readBlock_ + requiredBlocks < totalBlocks_)
	{
		std::memcpy(buf, pReadStart + readOffset, len);
		return 0;
	}
	else
	{
		X_ASSERT_NOT_IMPLEMENTED();
	}


	int goatt = 0;
	goatt = 1;

#if 0
	pFile_->seek(offset, core::SeekMode::SET);

	auto bytesRead = pFile_->read(buf, len);
	if (bytesRead < static_cast<size_t>(len)) {
		return -1;
	}
#endif

	return 0;
}

int MkvReader::Length(long long* total, long long* available)
{
	if (total) {
		*total = safe_static_cast<long long>(length_);
	}

	if (available) {
		*available = safe_static_cast<long long>(length_);
	}

	return 0;
}

void MkvReader::IoCallback(core::IFileSys&, const core::IoRequestBase* pReq, core::XFileAsync*, uint32_t)
{
	X_ASSERT(pReq->getType() == core::IoRequest::READ, "Unexpected request type")();

	auto* pReadReq = static_cast<const core::IoRequestRead*>(pReq);
	uint8_t* pBuf = static_cast<uint8_t*>(pReadReq->pBuf);

	auto bufferIdx = (pBuf - buffer_.data()) / IO_REQUEST_GRAN;

	{
		core::CriticalSection::ScopedLock lock(cs_);

		loadedBuffers_[bufferIdx] = true;
	}

	signal_.raise();
}

void MkvReader::fillBufffer()
{
	// if the file read offset matches the base read + buffer size.
	// we have nothing to dispatch.
	if (offset_ == readFileOffset_ + buffer_.size()) {
		return;
	}

	auto numRequests = totalBlocks_;

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
	req.callback.Bind<MkvReader, &MkvReader::IoCallback>(this);

	for (int32_t i = 0; i < numRequests; i++)
	{
		auto bufferIdx = (readBlock_ + i) % totalBlocks_;
		auto left = length_ - offset_;
		auto requestSize = core::Min<uint64_t>(IO_REQUEST_GRAN, left);

		{
			core::CriticalSection::ScopedLock lock(cs_);
			loadedBuffers_[bufferIdx] = false; // mark buffer invalid.
		}

		req.pBuf = &buffer_[bufferIdx * IO_REQUEST_GRAN];
		req.dataSize = safe_static_cast<uint32_t>(requestSize);
		req.offset = offset_;
		gEnv->pFileSys->AddIoRequestToQue(req);

		offset_ += requestSize;
	}
}

// -----------------------------------------------

XVideoWebm::XVideoWebm(core::XFileAsync* pFile, core::MemoryArenaBase* arena) :
	reader_(pFile, arena),
	pSegment_(nullptr),
	pCluster_(nullptr),
	pBlock_(nullptr),
	pBlockEntry_(nullptr),
	videoTrackIndex_(-1),
	blockFrameIndex_(-1),
	timestampNs_(0),
	reachedEos_(0),
	isKeyFrame_(0),
	width_(0),
	height_(0),
	buffer_(arena)
{

}

bool XVideoWebm::parse(void)
{
	mkvparser::EBMLHeader ebmlHeader;

	long long pos = 0;

	auto ret = ebmlHeader.Parse(&reader_, pos);
	if (ret < 0) {
		X_ERROR("Video", "BMLHeader failed to parse");
		return false;
	}

	ret = mkvparser::Segment::CreateInstance(&reader_, pos, pSegment_);
	if (ret) {
		X_ERROR("Video", "Segment::CreateInstance() failed.");
		return false;
	}

	ret = pSegment_->ParseHeaders();
	if (ret != 0) {
		X_ERROR("Video", "Segment header parse failed");
		return false;
	}

	const auto* pTracks = pSegment_->GetTracks();
	const mkvparser::VideoTrack* pVideoTrack = nullptr;

	for (uint32_t i = 0; i < pTracks->GetTracksCount(); ++i) 
	{
		const auto* pTrack = pTracks->GetTrackByIndex(i);
		if (pTrack->GetType() == mkvparser::Track::kVideo)
		{
			pVideoTrack = static_cast<const mkvparser::VideoTrack*>(pTrack);
			videoTrackIndex_ = static_cast<int32_t>(pTrack->GetNumber());
			break;
		}
	}

	if (!pVideoTrack) {
		return false;
	}


	if (core::strUtil::IsEqual(pVideoTrack->GetCodecId(), "V_VP8")) {
	//	vpx_ctx->fourcc = VP8_FOURCC;
	}
	else if (core::strUtil::IsEqual(pVideoTrack->GetCodecId(), "V_VP9")) {
	//	vpx_ctx->fourcc = VP9_FOURCC;
		return false;
	}
	else {
		return false;
	}

	pCluster_ = pSegment_->GetFirst();

	width_ = safe_static_cast<int32_t>(pVideoTrack->GetWidth());
	height_ = safe_static_cast<int32_t>(pVideoTrack->GetHeight());

	if (!guessFramerate()) {
		return false;
	}

	return true;
}

bool XVideoWebm::guessFramerate(void)
{
	uint32_t i = 0;

	while (timestampNs_ < 1000000000 && i < 50) {
		if (readFrame()) {
			break;
		}
		++i;
	}

	framerate_.numerator = (i - 1) * 1000000;
	framerate_.denominator = safe_static_cast<int32_t>(timestampNs_ / 1000);
	return true;
}



bool XVideoWebm::readFrame(void)
{
	if (reachedEos_) {
		return false;
	}

	bool blockEntryEos = false;

	const mkvparser::Cluster* pCluster = pCluster_;
	const mkvparser::Block* pBlock = pBlock_;
	const mkvparser::BlockEntry* pBlockEntry = pBlockEntry_;

	do 
	{
		long status = 0;
		bool get_new_block = false;

		if (!pBlockEntry && !blockEntryEos) 
		{
			status = pCluster->GetFirst(pBlockEntry);
			get_new_block = true;
		}
		else if (blockEntryEos || pBlockEntry->EOS())
		{
			pCluster = pSegment_->GetNext(pCluster);
			if (!pCluster || pCluster->EOS()) {
				buffer_.clear();
				reachedEos_ = 1;
				return false;
			}

			status = pCluster->GetFirst(pBlockEntry);
			blockEntryEos = false;
			get_new_block = true;
		}
		else if (!pBlock || blockFrameIndex_ == pBlock->GetFrameCount() || pBlock->GetTrackNumber() != videoTrackIndex_)
		{
			status = pCluster->GetNext(pBlockEntry, pBlockEntry);
			if (!pBlockEntry|| pBlockEntry->EOS()) {
				blockEntryEos = true;
				continue;
			}

			get_new_block = true;
		}

		if (status || !pBlockEntry) {
			return false;
		}

		if (get_new_block) {
			pBlock = pBlockEntry->GetBlock();
			if (!pBlock) {
				return false;
			}
			
			blockFrameIndex_ = 0;
		}

	} while (blockEntryEos || pBlock->GetTrackNumber() != videoTrackIndex_);

	// update members.
	pCluster_ = pCluster;
	pBlockEntry_ = pBlockEntry;
	pBlock_ = pBlock;


	const mkvparser::Block::Frame &frame = pBlock->GetFrame(blockFrameIndex_);
	++blockFrameIndex_;

	buffer_.resize(frame.len);

	timestampNs_ = pBlock->GetTime(pCluster);
	isKeyFrame_ = pBlock->IsKey();

	if (frame.Read(&reader_, buffer_.data()) != 0) {
		return false;
	}

	return true;
}

const DataVec& XVideoWebm::getFrameData(void) const
{
	return buffer_;
}

// -----------------------------------------------


XVideoSys::XVideoSys(core::MemoryArenaBase* arena)
{

}

void XVideoSys::registerVars(void)
{

}

void XVideoSys::registerCmds(void)
{

}

bool XVideoSys::init(void)
{
	vpx_codec_iface_t* interface = vpx_codec_vp8_dx();

	// Initialize codec
	int flags = 0;
	if (vpx_codec_dec_init(&codec_, interface, nullptr, flags)) {
		printf("Failed to initialize decoder\n");
		return false;
	}


	loadVideo("test2");

	return true;
}

void XVideoSys::shutDown(void)
{

}

void XVideoSys::release(void)
{

}

void XVideoSys::loadVideo(const char* pFileName)
{
	core::fileModeFlags mode;
	mode.Set(core::fileMode::READ);
	mode.Set(core::fileMode::RANDOM_ACCESS);
	mode.Set(core::fileMode::SHARE);

	core::Path<char> path;
	path /= "videos";
	path /= pFileName;
	path.setExtension(VIDEO_FILE_EXTENSION);

	auto pFile = gEnv->pFileSys->openFileAsync(path.c_str(), mode);


	DataVec dst(g_VideoArena);

	XVideoWebm video(pFile, g_VideoArena);
	video.parse();

	do
	{
		auto& data = video.getFrameData();

		if (vpx_codec_decode(&codec_, data.data(), static_cast<uint32_t>(data.size()), nullptr, 0))
		{
			const char* pDetail = vpx_codec_error_detail(&codec_);
			break;
		}
		
		vpx_codec_iter_t iter = nullptr;
		vpx_image_t* pImg = vpx_codec_get_frame(&codec_, &iter);
		if (!pImg) {
			continue;
		}

		const int w = pImg->d_w;
		const int h = pImg->d_h;
		
		const int strideY = pImg->stride[VPX_PLANE_Y];
		const int strideU = pImg->stride[VPX_PLANE_U];
		const int strideV = pImg->stride[VPX_PLANE_V];

		const uint8_t* pSrcY = pImg->planes[VPX_PLANE_Y];
		const uint8_t* pSrcU = pImg->planes[VPX_PLANE_U];
		const uint8_t* pSrcV = pImg->planes[VPX_PLANE_V];

		dst.resize((w * 4) * h);

		int res = libyuv::I420ToARGB(
			pSrcY, strideY,
			pSrcU, strideU,
			pSrcV, strideV,
			dst.data(),
			w * 4,
			w,
			h
		);

		pImg = nullptr;

	} while (video.readFrame());

}


X_NAMESPACE_END