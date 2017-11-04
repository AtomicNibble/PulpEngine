#include "stdafx.h"
#include "XVideoSys.h"
#include "XVideo.h"

#include <Threading\JobSystem2.h>
#include <IConsole.h>

#include <algorithm>

X_NAMESPACE_BEGIN(video)

#if 0

BufferedReader::BufferedReader(core::XFileAsync* pFile, core::MemoryArenaBase* arena) :
	signal_(true),
	buffer_(arena),
	loadedBuffers_(arena),
	pFile_(X_ASSERT_NOT_NULL(pFile)),
	readBlockOffset_(0),
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

int32_t BufferedReader::avalibleBlocks(void) const
{
	if (readBlock_ > writeBlock_) {
		return (totalBlocks_ - readBlock_) + writeBlock_;
	}

	return writeBlock_ - readBlock_;
}

int BufferedReader::Read(long long offset, long len, unsigned char* buf)
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


void BufferedReader::IoCallback(core::IFileSys&, const core::IoRequestBase* pReq, core::XFileAsync*, uint32_t)
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

void BufferedReader::fillBufffer()
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
	req.callback.Bind<BufferedReader, &BufferedReader::IoCallback>(this);

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

#endif

// -----------------------------------------------



// -----------------------------------------------


XVideoSys::XVideoSys(core::MemoryArenaBase* arena) :
	arena_(arena),
	blockArena_(arena),
	videos_(arena, sizeof(VideoResource), X_ALIGN_OF(VideoResource), "VideoPool"),
	pendingRequests_(arena)
{

}

void XVideoSys::registerVars(void)
{

}

void XVideoSys::registerCmds(void)
{
	ADD_COMMAND_MEMBER("listVideos", this, XVideoSys, &XVideoSys::Cmd_ListVideo, core::VarFlag::SYSTEM, 
		"List all the loaded videos");

}


bool XVideoSys::init(void)
{

	auto* pVideo = loadVideo("movie");

	waitForLoad(pVideo);

	return true;
}

void XVideoSys::shutDown(void)
{

}

void XVideoSys::release(void)
{

}

void XVideoSys::update(const core::FrameTimeData& frameTimeInfo)
{
	core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

	for (const auto& video : videos_)
	{
		auto* pVideoRes = video.second;

		pVideoRes->update(frameTimeInfo);
	}
}

void XVideoSys::appendDirtyBuffers(render::CommandBucket<uint32_t>& bucket) const
{
	core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

	for (const auto& video : videos_)
	{
		auto* pVideoRes = video.second;

		if (pVideoRes->hasFrame()) {
			pVideoRes->appendDirtyBuffers(bucket);
		}
	}
}


IVideo* XVideoSys::findVideo(const char* pVideoName) const
{
	core::string name(pVideoName);
	core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

	auto* pVideo = videos_.findAsset(name);
	if (pVideo) {
		return pVideo;
	}

	X_WARNING("VidManager", "Failed to find video: \"%s\"", pVideoName);
	return nullptr;
}

IVideo* XVideoSys::loadVideo(const char* pVideoName)
{
	X_ASSERT_NOT_NULL(pVideoName);
	X_ASSERT(core::strUtil::FileExtension(pVideoName) == nullptr, "Extension not allowed")(pVideoName);

	core::string name(pVideoName);
	core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

	VideoResource* pVideoRes = videos_.findAsset(name);
	if (pVideoRes)
	{
		pVideoRes->addReference();
		return pVideoRes;
	}

	pVideoRes = videos_.createAsset(name, name, blockArena_);

	queueLoadRequest(pVideoRes);

	return pVideoRes;
}

void XVideoSys::releaseVideo(IVideo* pVideo)
{
	VideoResource* pVideoRes = static_cast<VideoResource*>(pVideo);
	if (pVideoRes->removeReference() == 0)
	{
		videos_.releaseAsset(pVideoRes);
	}
}

bool XVideoSys::waitForLoad(IVideo* pIVideo)
{
	Video* pVideo = static_cast<Video*>(pIVideo);

	{
		// we lock to see if loading as the setting of loading is performed inside this lock.
		core::CriticalSection::ScopedLock lock(loadReqLock_);
		while (pVideo->getStatus() == core::LoadStatus::Loading)
		{
			loadCond_.Wait(loadReqLock_);
		}
	}

	// did we fail? or never sent a dispatch?
	auto status = pVideo->getStatus();
	if (status == core::LoadStatus::Complete)
	{
		return true;
	}
	else if (status == core::LoadStatus::Error)
	{
		return false;
	}
	else if (status == core::LoadStatus::NotLoaded)
	{
		// request never sent?
	}

	X_ASSERT_UNREACHABLE();
	return false;
}


void XVideoSys::queueLoadRequest(VideoResource* pVideoRes)
{
	// for video don't bother que, just dispatch now.

	pVideoRes->addReference(); // prevent instance sweep
	pVideoRes->setStatus(core::LoadStatus::Loading);

	core::CriticalSection::ScopedLock lock(loadReqLock_);

	auto loadReq = core::makeUnique<VideoLoadRequest>(arena_, pVideoRes);

	dispatchLoadRequest(loadReq.get());

	pendingRequests_.emplace_back(loadReq.release());
}


void XVideoSys::dispatchLoadRequest(VideoLoadRequest* pLoadReq)
{
	core::Path<char> path;
	path /= "videos";
	path /= pLoadReq->pVideo->getName();
	path.setExtension(VIDEO_FILE_EXTENSION);

	core::IoRequestOpen open;
	open.callback.Bind<XVideoSys, &XVideoSys::IoRequestCallback>(this);
	open.pUserData = pLoadReq;
	open.mode = core::fileMode::READ;
	open.path = path;

	gEnv->pFileSys->AddIoRequestToQue(open);
}


void XVideoSys::onLoadRequestFail(VideoLoadRequest* pLoadReq)
{
	auto* pVideo = pLoadReq->pVideo;

	pVideo->setStatus(core::LoadStatus::Error);

	// we only close file on error, we keep it open for streaming.
	if (pLoadReq->pFile) {
		gEnv->pFileSys->AddCloseRequestToQue(pLoadReq->pFile);
	}

	loadRequestCleanup(pLoadReq);
}

void XVideoSys::loadRequestCleanup(VideoLoadRequest* pLoadReq)
{
	{
		core::CriticalSection::ScopedLock lock(loadReqLock_);
		pendingRequests_.remove(pLoadReq);
	}

	// release our ref.
	releaseVideo(pLoadReq->pVideo);

	X_DELETE(pLoadReq, arena_);

	loadCond_.NotifyAll();
}

void XVideoSys::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	core::IoRequest::Enum requestType = pRequest->getType();
	VideoLoadRequest* pLoadReq = pRequest->getUserData<VideoLoadRequest>();
	auto* pVideo = pLoadReq->pVideo;

	if (requestType == core::IoRequest::OPEN)
	{
		if (!pFile)
		{
			X_ERROR("Video", "Failed to load: \"%s\"", pVideo->getName().c_str());
			onLoadRequestFail(pLoadReq);
			return;
		}

		pLoadReq->pFile = pFile;

		// read the header.
		core::IoRequestRead read;
		read.callback.Bind<XVideoSys, &XVideoSys::IoRequestCallback>(this);
		read.pUserData = pLoadReq;
		read.pFile = pFile;
		read.dataSize = sizeof(pLoadReq->hdr);
		read.offset = 0;
		read.pBuf = &pLoadReq->hdr;
		fileSys.AddIoRequestToQue(read);
	}
	else if (requestType == core::IoRequest::READ)
	{
		const core::IoRequestRead* pReadReq = static_cast<const core::IoRequestRead*>(pRequest);

		if (pReadReq->pBuf != &pLoadReq->hdr)
		{
			X_ASSERT_UNREACHABLE();

		}
		if (bytesTransferred != sizeof(pLoadReq->hdr))
		{
			X_ERROR("Video", "Failed to read video header. Got: 0x%" PRIx32 " need: 0x%" PRIxS, bytesTransferred, sizeof(pLoadReq->hdr));
			onLoadRequestFail(pLoadReq);
			return;
		}

		if (!pLoadReq->hdr.isValid())
		{
			X_ERROR("Video", "\"%s\" video header is invalid", pVideo->getName().c_str());
			onLoadRequestFail(pLoadReq);
			return;
		}


		gEnv->pJobSys->CreateMemberJobAndRun<XVideoSys>(this, &XVideoSys::ProcessData_job,
			pLoadReq JOB_SYS_SUB_ARG(core::profiler::SubSys::VIDEO));
	}
	else
	{
		X_ASSERT_UNREACHABLE();
	}
}

void XVideoSys::ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys, threadIdx, pJob, pData);

	auto* pLoadReq = static_cast<VideoLoadRequest*>(X_ASSERT_NOT_NULL(pData));
	auto* pVideo = pLoadReq->pVideo;

	if (pVideo->processHdr(pLoadReq->pFile, pLoadReq->hdr))
	{
		pVideo->setStatus(core::LoadStatus::Complete);
	}
	else
	{
		pVideo->setStatus(core::LoadStatus::Error);
	}

	loadRequestCleanup(pLoadReq);
}

void XVideoSys::listVideos(const char* pSearchPatten) const
{
	core::ScopedLock<VideoContainer::ThreadPolicy> lock(videos_.getThreadPolicy());

	core::Array<VideoResource*> sorted_videos(arena_);
	sorted_videos.setGranularity(videos_.size());

	for (const auto& video : videos_)
	{
		auto* pVideoRes = video.second;

		if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, pVideoRes->getName()))
		{
			sorted_videos.push_back(pVideoRes);
		}
	}

	std::sort(sorted_videos.begin(), sorted_videos.end(), [](VideoResource* a, VideoResource* b) {
		const auto& nameA = a->getName();
		const auto& nameB = b->getName();
		return nameA.compareInt(nameB) < 0;
	}
	);

	X_LOG0("Video", "------------ ^8Videos(%" PRIuS ")^7 ---------------", sorted_videos.size());

	core::HumanSize::Str sizeStr;
	for (const auto* pVideo : sorted_videos)
	{
		X_LOG0("Video", "^2%-32s^7 dim:^2%ix%i^7 fps:^2%i^7 frame:^2%i/%i^7 bufferSize: ^2%s^7 state: ^2%s^7 Refs:^2%i",
			pVideo->getName(), pVideo->getWidth(), pVideo->getHeight(), pVideo->getFps(),
			pVideo->getCurFrame(), pVideo->getNumFrames(),
			core::HumanSize::toString(sizeStr, pVideo->getBufferSize()),
			VideoState::ToString(pVideo->getState()),
			pVideo->getRefCount());
	}

	X_LOG0("Video", "------------ ^8Videos End^7 --------------");
}


void XVideoSys::Cmd_ListVideo(core::IConsoleCmdArgs* pCmd)
{
	// optional search criteria
	const char* pSearchPatten = nullptr;

	if (pCmd->GetArgCount() >= 2) {
		pSearchPatten = pCmd->GetArg(1);
	}

	listVideos(pSearchPatten);
}



#if 0
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

	XVideo video(pFile, g_VideoArena);
	video.parse();

	do
	{
		FrameData frame;

		if (!video.getFrame(frame)) {
			return;
		}

		if (vpx_codec_decode(&codec_, frame.pData, static_cast<uint32_t>(frame.dataSize), nullptr, 0))
		{
			const char* pDetail = vpx_codec_error_detail(&codec_);
			break;
		}
		

		video.discardFrame(frame);


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
#endif

X_NAMESPACE_END