#include "stdafx.h"
#include "AnimManager.h"

#include <IConsole.h>
#include <IFileSys.h>
#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>

#include <../../tools/AnimLib/AnimLib.h>

X_NAMESPACE_BEGIN(anim)


AnimManager::AnimManager(core::MemoryArenaBase* arena, core::MemoryArenaBase* blockArena) :
	arena_(arena),
	blockArena_(blockArena),
	anims_(arena, sizeof(AnimResource), X_ALIGN_OF(AnimResource), "AnimPool"),
	requestQueue_(arena),
	pendingRequests_(arena)
{

}

AnimManager::~AnimManager()
{

}

void AnimManager::registerCmds(void)
{
	ADD_COMMAND_MEMBER("listAnims", this, AnimManager, &AnimManager::Cmd_ListAnims, core::VarFlag::SYSTEM, "List all the loaded anims");
	ADD_COMMAND_MEMBER("animReload", this, AnimManager, &AnimManager::Cmd_ReloadAnim, core::VarFlag::SYSTEM, "Reload a anim <name>");

}

void AnimManager::registerVars(void)
{

}

bool AnimManager::init(void)
{

	gEnv->pHotReload->addfileType(this, ANIM_FILE_EXTENSION);



	return true;
}

void AnimManager::shutDown(void)
{

	gEnv->pHotReload->unregisterListener(this);


	freeDanglingAnims();
}

bool AnimManager::asyncInitFinalize(void)
{


	return true;
}

void AnimManager::dispatchPendingLoads(void)
{
	core::CriticalSection::ScopedLock lock(loadReqLock_);

	while (requestQueue_.isNotEmpty())
	{
		auto* pAnim = requestQueue_.peek();
		X_ASSERT(pAnim->getStatus() == core::LoadStatus::Loading, "Incorrect status")(pAnim->getStatus());
		auto loadReq = core::makeUnique<AnimLoadRequest>(arena_, pAnim);

		// dispatch IO 
		dispatchLoadRequest(loadReq.get());

		pendingRequests_.emplace_back(loadReq.release());

		requestQueue_.pop();
	}
}

Anim* AnimManager::findAnim(const char* pAnimName) const
{
	core::string name(pAnimName);
	core::ScopedLock<AnimContainer::ThreadPolicy> lock(anims_.getThreadPolicy());

	AnimResource* pAnim = anims_.findAsset(name);
	if (pAnim) {
		return pAnim;
	}

	X_WARNING("AnimManager", "Failed to find anim: \"%s\"", pAnimName);
	return nullptr;
}


Anim* AnimManager::loadAnim(const char* pAnimName)
{
	X_ASSERT_NOT_NULL(pAnimName);
	X_ASSERT(core::strUtil::FileExtension(pAnimName) == nullptr, "Extension not allowed")(pAnimName);

	core::string name(pAnimName);
	core::ScopedLock<AnimContainer::ThreadPolicy> lock(anims_.getThreadPolicy());

	AnimResource* pAnimRes = anims_.findAsset(name);
	if (pAnimRes)
	{
		// inc ref count.
		pAnimRes->addReference();
		return pAnimRes;
	}

	// we create a anim and give it back
	pAnimRes = anims_.createAsset(name, name, arena_);

	// add to list of anims that need loading.
	queueLoadRequest(pAnimRes);

	return pAnimRes;
}


void AnimManager::releaseAnim(Anim* pAnim)
{
	AnimResource* pAnimRes = static_cast<AnimResource*>(pAnim);
	if (pAnimRes->removeReference() == 0)
	{
		releaseResources(pAnimRes);

		anims_.releaseAsset(pAnimRes);
	}
}

void AnimManager::reloadAnim(const char* pName)
{
	X_UNUSED(pName);
	X_ASSERT_NOT_IMPLEMENTED();

}

void AnimManager::listAnims(const char* pSearchPatten) const
{
	core::ScopedLock<AnimContainer::ThreadPolicy> lock(anims_.getThreadPolicy());

	core::Array<AnimResource*> sorted_anims(g_3dEngineArena);
	sorted_anims.setGranularity(anims_.size());

	for (const auto& anim : anims_)
	{
		auto* pAnimRes = anim.second;

		if (!pSearchPatten || core::strUtil::WildCompare(pSearchPatten, pAnimRes->getName()))
		{
			sorted_anims.push_back(pAnimRes);
		}
	}

	std::sort(sorted_anims.begin(), sorted_anims.end(), [](AnimResource* a, AnimResource* b) {
		const auto& nameA = a->getName();
		const auto& nameB = b->getName();
		return nameA.compareInt(nameB) < 0;
	}
	);

	X_LOG0("Anim", "------------ ^8Anims(%" PRIuS ")^7 ---------------", sorted_anims.size());

	for (const auto* pAnim : sorted_anims)
	{
		X_LOG0("Anim", "^2%-32s^7 Refs:^2%i", pAnim->getName().c_str(), pAnim->getRefCount());
	}

	X_LOG0("Anim", "------------ ^8Anims End^7 --------------");
}

// ------------------------------------

void AnimManager::freeDanglingAnims(void)
{
	{
		core::ScopedLock<AnimContainer::ThreadPolicy> lock(anims_.getThreadPolicy());

		// any left?
		for (const auto& a : anims_)
		{
			auto* pAnimRes = a.second;
			const auto& name = pAnimRes->getName();
			X_WARNING("Anim", "\"%s\" was not deleted. refs: %" PRIi32, name.c_str(), pAnimRes->getRefCount());

			releaseResources(pAnimRes);
		}
	}

	anims_.free();
}

void AnimManager::releaseResources(Anim* pAnim)
{
	X_UNUSED(pAnim);


}

// ------------------------------------


void AnimManager::queueLoadRequest(AnimResource* pAnim)
{
	X_ASSERT(pAnim->getName().isNotEmpty(), "Anim has no name")();

	core::CriticalSection::ScopedLock lock(loadReqLock_);

	// can we know it's not in this queue just from status?
	// like if it's complete it could be in this status
	auto status = pAnim->getStatus();
	if (status == core::LoadStatus::Complete || status == core::LoadStatus::Loading)
	{
		X_WARNING("Anim", "Redundant load request requested: \"%s\"", pAnim->getName().c_str());
		return;
	}

	X_ASSERT(!requestQueue_.contains(pAnim), "Queue already contains asset")(pAnim);

	pAnim->addReference(); // prevent instance sweep
	pAnim->setStatus(core::LoadStatus::Loading);
	requestQueue_.push(pAnim);
}


void AnimManager::dispatchLoadRequest(AnimLoadRequest* pLoadReq)
{
	pLoadReq->dispatchTime = core::StopWatch::GetTimeNow();

	core::Path<char> path;
	path /= "anims";
	path /= pLoadReq->pAnim->getName();
	path.setExtension(".anim");

	// dispatch a read request baby!
	core::IoRequestOpen open;
	open.callback.Bind<AnimManager, &AnimManager::IoRequestCallback>(this);
	open.pUserData = pLoadReq;
	open.mode = core::fileMode::READ;
	open.path = path;

	gEnv->pFileSys->AddIoRequestToQue(open);
}

void AnimManager::onLoadRequestFail(AnimLoadRequest* pLoadReq)
{
	auto* pAnim = pLoadReq->pAnim;

	pAnim->setStatus(core::LoadStatus::Error);

	loadRequestCleanup(pLoadReq);
}

void AnimManager::loadRequestCleanup(AnimLoadRequest* pLoadReq)
{
	pLoadReq->loadTime = core::StopWatch::GetTimeNow();

	X_LOG0("Anim", "Anim loaded in: ^6%fms", (pLoadReq->loadTime - pLoadReq->dispatchTime).GetMilliSeconds());
	{
		core::CriticalSection::ScopedLock lock(loadReqLock_);
		pendingRequests_.remove(pLoadReq);
	}

	if (pLoadReq->pFile) {
		gEnv->pFileSys->AddCloseRequestToQue(pLoadReq->pFile);
	}

	// release our ref.
	releaseAnim(pLoadReq->pAnim);

	X_DELETE(pLoadReq, arena_);

	loadCond_.NotifyAll();
}


void AnimManager::IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
	core::XFileAsync* pFile, uint32_t bytesTransferred)
{
	core::IoRequest::Enum requestType = pRequest->getType();
	AnimLoadRequest* pLoadReq = pRequest->getUserData<AnimLoadRequest>();
	Anim* pAnim = pLoadReq->pAnim;

	if (requestType == core::IoRequest::OPEN)
	{
		if (!pFile)
		{
			X_ERROR("Anim", "Failed to load: \"%s\"", pAnim->getName().c_str());
			onLoadRequestFail(pLoadReq);
			return;
		}

		pLoadReq->pFile = pFile;

		// read the header.
		core::IoRequestRead read;
		read.callback.Bind<AnimManager, &AnimManager::IoRequestCallback>(this);
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

		if (pReadReq->pBuf == &pLoadReq->hdr)
		{
			if (bytesTransferred != sizeof(pLoadReq->hdr))
			{
				X_ERROR("Anim", "Failed to read anim header. Got: 0x%" PRIx32 " need: 0x%" PRIxS, bytesTransferred, sizeof(pLoadReq->hdr));
				onLoadRequestFail(pLoadReq);
				return;
			}

			if (!pLoadReq->hdr.isValid())
			{
				X_ERROR("Anim", "\"%s\" anim header is invalid", pAnim->getName().c_str());
				onLoadRequestFail(pLoadReq);
				return;
			}

			// we need to allocate memory and dispatch a read request for it.
			uint32_t datasize = pLoadReq->hdr.dataSize;
			X_ASSERT(datasize > 0, "Datasize must be positive")(datasize);
			pLoadReq->data = core::makeUnique<uint8_t[]>(blockArena_, datasize, 16);

			core::IoRequestRead read;
			read.callback.Bind<AnimManager, &AnimManager::IoRequestCallback>(this);
			read.pUserData = pLoadReq;
			read.pFile = pFile;
			read.offset = sizeof(pLoadReq->hdr);
			read.dataSize = datasize;
			read.pBuf = pLoadReq->data.ptr();
			gEnv->pFileSys->AddIoRequestToQue(read);
		}
		else
		{
			if (bytesTransferred != pLoadReq->hdr.dataSize)
			{
				X_ERROR("Anim", "Failed to read anim data. Got: 0x%" PRIx32 " need: 0x%" PRIx32, bytesTransferred, pLoadReq->hdr.dataSize);
				onLoadRequestFail(pLoadReq);
				return;
			}

			gEnv->pJobSys->CreateMemberJobAndRun<AnimManager>(this, &AnimManager::ProcessData_job,
				pLoadReq JOB_SYS_SUB_ARG(core::profiler::SubSys::ENGINE3D));
		}
	}

}

void AnimManager::ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData)
{
	X_UNUSED(jobSys, threadIdx, pJob, pData);

	AnimLoadRequest* pLoadReq = static_cast<AnimLoadRequest*>(X_ASSERT_NOT_NULL(pData));
	Anim* pAnim = pLoadReq->pAnim;

	pAnim->processData(pLoadReq->hdr, std::move(pLoadReq->data));

	loadRequestCleanup(pLoadReq);
}


// -------------------------------------


void AnimManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys, name);


}


// -------------------------------------

void AnimManager::Cmd_ListAnims(core::IConsoleCmdArgs* pCmd)
{
	// optional search criteria
	const char* pSearchPatten = nullptr;

	if (pCmd->GetArgCount() >= 2) {
		pSearchPatten = pCmd->GetArg(1);
	}

	listAnims(pSearchPatten);
}

void AnimManager::Cmd_ReloadAnim(core::IConsoleCmdArgs* pCmd)
{
	if (pCmd->GetArgCount() < 2) {
		X_WARNING("Anim", "reloadAnim <name>");
		return;
	}

	const char* pName = pCmd->GetArg(1);

	reloadAnim(pName);
}


X_NAMESPACE_END