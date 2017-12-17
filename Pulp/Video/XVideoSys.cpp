#include "stdafx.h"
#include "XVideoSys.h"
#include "XVideo.h"

#include <Threading\JobSystem2.h>
#include <IConsole.h>


X_NAMESPACE_BEGIN(video)


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

bool XVideoSys::waitForLoad(core::AssetBase* pVideo)
{
	X_ASSERT(pVideo->getType() == assetDb::AssetType::VIDEO, "Invalid asset passed")();

	if (pVideo->isLoaded()) {
		return true;
	}

	return waitForLoad(static_cast<IVideo*>(static_cast<Video*>(pVideo)));
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
		X_LOG0("Video", "^2%-32s^7 dim:^2%ix%i^7 fps:^2%i^7 frame:^2%i/%i^7 bufferSize:^2%s^7 state:^2%s^7 Refs:^2%i",
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


X_NAMESPACE_END