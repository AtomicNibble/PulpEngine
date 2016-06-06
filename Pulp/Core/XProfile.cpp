#include "stdafx.h"
#include "XProfile.h"

#include "Profile\ProfilerTypes.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(core)

namespace
{
	#pragma intrinsic(__rdtsc)

	namespace ProfileTimer
	{
		static uint64_t g_cpuspeed = 1000000;

		static inline uint64_t getTicks() {
			return __rdtsc();
		}
		static float32_t toMS(const uint64_t &t) {
			return float32_t(t) / g_cpuspeed;

		//	return float64_t(t) / float64_t(3200.0 * 1000000.0);
		}

		void CalculateCPUSpeed()
		{
			uint64_t start, elapsed;
			const DWORD dwDelay = 100;

			// We want absolute maximum priority
			DWORD dwPriorityClass = GetPriorityClass(GetCurrentProcess());
			int nPriority = GetThreadPriority(GetCurrentThread());
			SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
			PotatoSleep(0);	// Give up the rest of our timeslice so we don't get a context switch

			start = getTicks();
			PotatoSleep(dwDelay);
			elapsed = (getTicks() - start);

			// Reset priority and get speed
			SetThreadPriority(GetCurrentThread(), nPriority);
			SetPriorityClass(GetCurrentProcess(), dwPriorityClass);
			
			g_cpuspeed = 3200000; //  (elapsed * 100);
		}
		// 3200000000
		// 3184599450000000
		// 3193773880
		// 3198000000
		// 3198813020
		// 1000000000 - 1.0
		// 2800000000 - 2.8
		// 3200000000 - 3.2
		// 3200000000
	}

};

XProfileSys* XProfileSys::s_this = nullptr;

XProfileSys::XProfileSys() :
	pCore_(nullptr),
	profiles_(nullptr),
	displayInfo_(nullptr),
	// --------
	frameStartTime_(0),
	frameTime_(0),
	totalTime_(0)
{
	s_this = this;

//	profiles_.reserve(512);
}

XProfileSys::~XProfileSys()
{

}

void XProfileSys::Init(ICore* pCore)
{
	pCore_ = pCore;

	profiles_.setArena(gEnv->pArena, 512);
	displayInfo_.setArena(gEnv->pArena, 512);

	gEnv->profileScopeStart = &ScopeStart;
	gEnv->profileScopeEnd = &ScopeEnd;

	ProfileTimer::CalculateCPUSpeed();

	core::zero_object(subSystemInfo_);
	subSystemInfo_[ProfileSubSys::CORE].name = "Core";
	subSystemInfo_[ProfileSubSys::ENGINE3D].name = "3DEngine";
	subSystemInfo_[ProfileSubSys::FONT].name = "Font";
	subSystemInfo_[ProfileSubSys::INPUT].name = "Input";
	subSystemInfo_[ProfileSubSys::RENDER].name = "Render";
	subSystemInfo_[ProfileSubSys::SCRIPT].name = "Script";
	subSystemInfo_[ProfileSubSys::SOUND].name = "Sound";
	subSystemInfo_[ProfileSubSys::GAME].name = "Game";
	subSystemInfo_[ProfileSubSys::PHYSICS].name = "Physics";
	subSystemInfo_[ProfileSubSys::NETWORK].name = "Network";
	subSystemInfo_[ProfileSubSys::UNCLASSIFIED].name = "UnClassified";

#if X_DEBUG
	// check i've not forgot to add one.
	// all things i can potentital forget to do, should 
	// be detected in debug mode. no fails here.
	uint32_t i;
	for (i = 0; i < ProfileSubSys::ENUM_COUNT; i++)
	{
		X_ASSERT_NOT_NULL(subSystemInfo_[i].name);
	}

#endif // !X_DEBUG

	// register some vars.
	
	ADD_CVAR_REF("profile_draw", s_drawProfileInfo_, 0, 0, 1, 
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Display profiler info. (visible items enabled via profile_draw_* vars)");
	ADD_CVAR_REF("profile_draw_when_console_expanded", s_drawProfileInfoWhenConsoleExpaned_, 1, 0, 1, 
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Display profiler even when console is expanded");
	ADD_CVAR_REF("profile_draw_subsystems", s_drawSubsystems_, 1, 0, 1, 
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Display profiler subsystem block");
	ADD_CVAR_REF("profile_draw_meminfo", s_drawMemInfo_, 1, 0, 1, 
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Display profiler mem info blocks");
	ADD_CVAR_REF("profile_draw_stats_table", s_drawStats_, 1, 0, 1,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Display profiler stats table block");
	ADD_CVAR_REF("profile_draw_frame_time_graph", s_drawFrameTimeBar_, 1, 0, 1, 
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Display profiler frame time bar");
}



void XProfileSys::AddProfileData(XProfileData* pData)
{
	// profile data is static and only added 
	// first time the profile scope is hit.
	// profilers for code that is not run won't be added.

	profiles_.push_back(pData);
}

void XProfileSys::FrameBegin(void)
{
	if (!isEnabled()) {
		return;
	}

	// get kinky.
	frameStartTime_ = ProfileTimer::getTicks();
}

void XProfileSys::FrameEnd(void)
{
	if (!isEnabled()) {
		return;
	}

	uint64_t end = ProfileTimer::getTicks();

	// update some time stats.
	frameTime_ = end - frameStartTime_;
	totalTime_ += frameTime_;


	frameTimeHistory_.append(ProfileTimer::toMS(frameTime_));

	DisplayProfileData();
	UpdateProfileData();
}



void XProfileSys::AddProfileDisplayData_r(XProfileData* pData, int lvl)
{
	ProfileDisplayInfo info;
	info.depth = lvl;
	info.pData = pData;

	displayInfo_.push_back(info);

	// find children.
	size_t i;
	for (i=0; i<profiles_.size(); i++)
	{
		XProfileData* pChildData = profiles_[i];

		if (pChildData->pParent_ == pData) {
			AddProfileDisplayData_r(pChildData, lvl + 1);
		}
	}
}




void XProfileSys::DisplayProfileData(void)
{
	size_t i;

	displayInfo_.reserve(profiles_.size());
	displayInfo_.clear();

	// we only add data with no parent.
	// since the tree will add the rest.
	for (i = 0; i<profiles_.size(); i++)
	{
		XProfileData* pData = profiles_[i];

		if (pData->pParent_ == nullptr) {
			AddProfileDisplayData_r(pData, 0);
		}
	}
}

void XProfileSys::UpdateProfileData(void)
{
	size_t i;
	for (i = 0; i<profiles_.size(); i++)
	{
		XProfileData* pData = profiles_[i];

		// add frame values
		pData->sumTime_ += pData->time_;
		pData->sumTimeSelf_ += pData->timeSelf_;

		// create 'ruff time values' (but acurate in relation to other times)
		const float fTotalTime = ProfileTimer::toMS(pData->time_);
		const float fSelfTime = ProfileTimer::toMS(pData->timeSelf_);

		// log values.
		pData->totalTimeHistory_.append(fTotalTime);
		pData->selfTimeHistory_.append(fSelfTime);
		pData->callCountHistory_.append(pData->callCount_);

		// sub me up.
		subSystemInfo_[pData->subSystem_].selfTime += fSelfTime;

		// clear frame values
		pData->time_ = 0llu;
		pData->timeSelf_ = 0llu;
		pData->callCount_ = 0;
	}
}

// Static

void XProfileSys::ScopeStart(XProfileScope* pProScope)
{
	s_this->callstack_.Push(pProScope);

	pProScope->start_ = ProfileTimer::getTicks();
	pProScope->excludeTime_ = 0llu;
}

void XProfileSys::ScopeEnd(XProfileScope* pProScope)
{
	XProfileData* data = pProScope->pData_;

	uint64_t end = ProfileTimer::getTicks();
	uint64_t totalTime = (end - pProScope->start_);

	uint64_t selftime = totalTime - pProScope->excludeTime_;

	data->time_ += totalTime;
	data->timeSelf_ += selftime;
	data->callCount_++;

	s_this->callstack_.Pop(pProScope);

	if (pProScope->pParent_)
	{
		// If we have parent, add this counter total time to parent exclude time.
		pProScope->pParent_->excludeTime_ += totalTime;
		if (!data->pParent_ && pProScope->pParent_->pData_)
		{
			pProScope->pParent_->pData_->hasChildren_ = 1;
			data->pParent_ = pProScope->pParent_->pData_;
		}
	}
	else
	{
		pProScope->pParent_ = nullptr;
	}
}

// ~Static



X_NAMESPACE_END
