#include "stdafx.h"
#include "XProfile.h"

#include <Time\StopWatch.h>
#include <Util\Process.h>

#include <IFrameData.h>
#include <I3DEngine.h>
#include <IPrimativeContext.h>
#include <IFont.h>

#include "Profile\ProfilerTypes.h"


X_NAMESPACE_BEGIN(core)

namespace
{
	#pragma intrinsic(__rdtsc)

	namespace ProfileTimer
	{
		static uint64_t g_cpuspeed = 1000000;

		X_INLINE uint64_t getTicks(void)
		{
			return __rdtsc();
		}

		X_INLINE uint64_t getTicksFlush(void)
		{
			int temp[4];
			__cpuid(temp, 0);
			return __rdtsc();
		}

		X_INLINE float32_t toMS(const uint64_t &t) 
		{
			return float32_t(t) / g_cpuspeed;
		}

		void CalculateCPUSpeed()
		{
			uint64_t start, elapsed;
			const uint32_t delayMS = 1;

			core::Process pro = core::Process::GetCurrent();

			// We want absolute maximum priority
			const auto priorityClass = pro.GetPriorityClass();
			const auto curThreadPri = core::Thread::GetPriority();
			
			pro.SetPriorityClass(core::Process::Priority::REALTIME);
			core::Thread::SetPriority(core::Thread::Priority::REALTIME);
			core::Thread::Sleep(0);	// Give up the rest of our timeslice so we don't get a context switch

			core::StopWatch time;

			auto overhead = time.GetMilliSeconds();
			time.Start();

			start = getTicksFlush();
			core::Thread::Sleep(delayMS);
			elapsed = (getTicksFlush() - start);

			auto elapsedTime = time.GetMilliSeconds() - (overhead);

			// Reset priority and get speed
			core::Thread::SetPriority(curThreadPri);
			pro.SetPriorityClass(priorityClass);
			
			auto ticksPerMS = static_cast<uint64_t>(static_cast<double>(elapsed) / static_cast<double>(elapsedTime));

			g_cpuspeed = ticksPerMS;
			// g_cpuspeed = 3200000;
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

#if 1

namespace profiler
{

	XProfileSys::XProfileSys(core::MemoryArenaBase* arena) :
		pFont_(nullptr),
		profilerData_(arena),
		profilerHistoryData_(arena),
		frameStartTime_(0),
		frameTime_(0),
		totalTime_(0)
	{
		
	}

	XProfileSys::~XProfileSys()
	{

	}

	void XProfileSys::registerVars(void)
	{
		vars_.RegisterVars();
	}

	void XProfileSys::registerCmds(void)
	{

	}


	bool XProfileSys::init(ICore* pCore)
	{
		X_LOG0("ProfileSys", "Starting");
		X_UNUSED(pCore);

		pCore->GetCoreEventDispatcher()->RegisterListener(this);


		core::zero_object(subSystemInfo_);
		subSystemInfo_[profiler::SubSys::CORE].pName = "Core";
		subSystemInfo_[profiler::SubSys::ENGINE3D].pName = "3DEngine";
		subSystemInfo_[profiler::SubSys::FONT].pName = "Font";
		subSystemInfo_[profiler::SubSys::INPUT].pName = "Input";
		subSystemInfo_[profiler::SubSys::RENDER].pName = "Render";
		subSystemInfo_[profiler::SubSys::SCRIPT].pName = "Script";
		subSystemInfo_[profiler::SubSys::SOUND].pName = "Sound";
		subSystemInfo_[profiler::SubSys::GAME].pName = "Game";
		subSystemInfo_[profiler::SubSys::PHYSICS].pName = "Physics";
		subSystemInfo_[profiler::SubSys::NETWORK].pName = "Network";
		subSystemInfo_[profiler::SubSys::UNCLASSIFIED].pName = "UnClassified";
		subSystemInfo_[profiler::SubSys::TOOL].pName = "Tool";
		subSystemInfo_[profiler::SubSys::UNITTEST].pName = "UnitTests";

		subSystemInfo_[profiler::SubSys::CORE].col = Col_Red;
		subSystemInfo_[profiler::SubSys::ENGINE3D].col = Col_Orange;
		subSystemInfo_[profiler::SubSys::FONT].col = Col_Yellow;
		subSystemInfo_[profiler::SubSys::INPUT].col = Col_Lime;
		subSystemInfo_[profiler::SubSys::RENDER].col = Col_Green;
		subSystemInfo_[profiler::SubSys::SCRIPT].col = Col_Cyan;
		subSystemInfo_[profiler::SubSys::SOUND].col = Col_Blue;
		subSystemInfo_[profiler::SubSys::GAME].col = Col_Purple;
		subSystemInfo_[profiler::SubSys::PHYSICS].col = Col_Magenta;
		subSystemInfo_[profiler::SubSys::NETWORK].col = Col_Pink;
		subSystemInfo_[profiler::SubSys::UNCLASSIFIED].col = Col_Coral;
		subSystemInfo_[profiler::SubSys::TOOL].col = Col_Teal;
		subSystemInfo_[profiler::SubSys::UNITTEST].col = Col_Olive;

		return true;
	}


	void XProfileSys::shutDown(void)
	{
		// ...
	}

	bool XProfileSys::loadRenderResources(void)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pFontSys);

		pFont_ = gEnv->pFontSys->GetFont("default");

		return pFont_ != nullptr;
	}


	void XProfileSys::AddProfileData(XProfileData* pData)
	{
		if (pData->getType() == XProfileData::Type::SingleShot)
		{
			X_ASSERT(profilerData_.find(pData) == decltype(profilerData_)::invalid_index, "Data node already added")();
			profilerData_.emplace_back(pData);
		}
		else
		{
			auto* pDataHistory = static_cast<decltype(profilerHistoryData_)::Type>(pData);
			X_ASSERT(profilerHistoryData_.find(pDataHistory) == decltype(profilerHistoryData_)::invalid_index, "Data node already added")();
			profilerHistoryData_.emplace_back(pDataHistory);
		}
	}


	void XProfileSys::ScopeBegin(XProfileScope* pScope)
	{
		pScope->start_ = ProfileTimer::getTicks();
		pScope->excludeTime_ = 0llu;
	}

	void XProfileSys::ScopeEnd(XProfileScope* pScope)
	{
		XProfileData* data = pScope->pData_;

		uint64_t end = ProfileTimer::getTicks();
		uint64_t totalTime = (end - pScope->start_);

		uint64_t selftime = totalTime - pScope->excludeTime_;

		data->time_ += totalTime;
		data->timeSelf_ += selftime;
		data->callCount_++;

		// in order to support callstacks i would need to make thread specific stacks.
		// otherwise shit will get messy.
		// for now i just won't bother with stacks.

#if 0
		if (pScope->pParent_)
		{
			XProfileScope* pScopeParent = pScope->pParent_;

			// If we have parent, add this counter total time to parent exclude time.
			pScopeParent->excludeTime_ += totalTime;
			if (!data->pParent_ && pScopeParent->pData_)
			{
				pScopeParent->pData_->hasChildren_ = 1;
				data->pParent_ = pScopeParent->pData_;
			}
		}
#endif
	}

	void XProfileSys::OnFrameBegin(void)
	{
		if (vars_.isPaused()) {
			return;
		}

		frameStartTime_ = ProfileTimer::getTicks();
	}

	void XProfileSys::OnFrameEnd(void)
	{
		if (vars_.isPaused()) {
			return;
		}

		uint64_t end = ProfileTimer::getTicks();

		// update some time stats.
		frameTime_ = end - frameStartTime_;
		totalTime_ += frameTime_;

		UpdateProfileData();
	}


	void XProfileSys::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
	{
		if (event == CoreEvent::RENDER_RES_CHANGED)
		{
			renderRes_.x = static_cast<int32_t>(wparam);
			renderRes_.y = static_cast<int32_t>(lparam);
		}
	}

	void XProfileSys::UpdateProfileData(void)
	{
		for(auto* pData : profilerHistoryData_)
		{
			pData->onFrameBegin();
		}
	}

	void XProfileSys::Render(const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys)
	{
		if (pJobSys && vars_.drawProfileInfo())
		{
#if X_ENABLE_JOBSYS_PROFILER

			int32_t profilerIdx = pJobSys->getCurrentProfilerIdx();

			// so we want to draw one before the current index.
			if (profilerIdx == 0) {
				profilerIdx =core::V2::JOBSYS_HISTORY_COUNT - 1;
			}
			profilerIdx = math<int32_t>::clamp(profilerIdx - 1, 0, core::V2::JOBSYS_HISTORY_COUNT);

			RenderJobSystem(frameTimeInfo, pJobSys, profilerIdx);
#else


#endif // !X_ENABLE_JOBSYS_PROFILER
		}
	}

#if X_ENABLE_JOBSYS_PROFILER

	void XProfileSys::RenderJobSystem(const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys, int32_t profileIdx)
	{
		X_UNUSED(pJobSys);
		X_UNUSED(profileIdx);
		auto& stats = pJobSys->GetStats();
		auto& frameStats = stats[profileIdx];
		auto& timeLines = pJobSys->GetTimeLines();

		// well my goaty pickle trimmer.
		// i want something sorta full screen and sexy.
		// give me the dimensions!
		const float border = 15;
		const float padding = 10;
		const float yOffset = 30;

		const float xStart = border;
		const float yStart = border + yOffset;
		const float width = renderRes_.x - (border * 2);
		const float height = (renderRes_.y - (border * 2) - yOffset);
	
		font::TextDrawContext ctx;
		ctx.pFont = pFont_;
		ctx.effectId = 0;
		ctx.SetColor(Col_White);
		ctx.SetSize(Vec2f(16.f,16.f));

		engine::IPrimativeContext* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::PROFILE);

		pPrim->drawQuad(xStart, yStart, width, height, Color(0.1f, 0.1f, 0.1f, 0.8f));

		core::StackString512 txt;
		txt.appendFmt("JobsRun: %" PRIi32 " JobsStolen: %" PRIi32 " JobsAssited: %" PRIi32, 
			frameStats.jobsRun, frameStats.jobsStolen, frameStats.jobsAssited);

		ctx.flags.Set(font::DrawTextFlag::CENTER);
		pPrim->drawText(xStart + (width / 2), yStart + padding, ctx, L"Profiler");
		ctx.flags.Remove(font::DrawTextFlag::CENTER);
		pPrim->drawText(xStart + padding, yStart + padding, ctx, txt.begin(), txt.end());

		// draw the color key table.
		float keyHeight = 30.f;

		{
			float keyBegin = yStart + (height - keyHeight);
			float keyY = keyBegin;
			float keyX = xStart + padding;

			for (uint32_t i = 0; i < core::profiler::SubSys::ENUM_COUNT; i++)
			{
				if (i == core::profiler::SubSys::UNITTEST) {
					continue;
				}

				ctx.col = subSystemInfo_[i].col;

				const char* pBegin = subSystemInfo_[i].pName;
				const char* pEnd = pBegin + core::strUtil::strlen(subSystemInfo_[i].pName);

				auto txtSize = pFont_->GetTextSize(pBegin, pEnd, ctx);

				pPrim->drawText(keyX, keyY, ctx, subSystemInfo_[i].pName);

				keyX += txtSize.x + 10.f;
			}
		}

		const size_t visibleMS = 16;
		const uint32_t numThread = pJobSys->GetThreadCount() + 1;
		const float threadInfoXOffset = 85.f;
		const float threadInfoX = xStart + padding + threadInfoXOffset;
		const float threadInfoY = yStart + ctx.size.y + (padding * 2);
		const float threadInfoHeight = (height - (ctx.size.y + padding) - (keyHeight + padding)) - 20;
		const float threadInfoWidth = (width - (padding * 2)) - threadInfoXOffset;


		const float widthPerMS = threadInfoWidth / visibleMS;
		const float threadInfoEntryHeight = threadInfoHeight / numThread;

		// draw a gird spliting up the horizontal space in to time.
		{

			for (size_t i = 0; i < visibleMS + 1; i++)
			{
				Vec3f top(threadInfoX + (i * widthPerMS), threadInfoY, 1);
				Vec3f bottom(threadInfoX + (i * widthPerMS), threadInfoY + threadInfoHeight, 1);

				pPrim->drawLine(top, bottom, Colorf(0.2f, 0.2f, 0.2f));
			}

			X_DISABLE_WARNING(4127)

			ctx.col = Col_Dimgray;

			core::StackString<64> str;
			for (size_t i = 1; i < visibleMS; i++)
			{
				if (visibleMS > 16)
				{
					if (!(i & 1) || (i + 1) == visibleMS) {
						continue;
					}
				}

				str.setFmt("%ims", i);
				pPrim->drawText(threadInfoX + (i * widthPerMS), threadInfoY + threadInfoHeight, ctx, str.c_str());
			}

			for (size_t i = 0; i < numThread; i++)
			{
				str.setFmt("Thread %i", i);
				pPrim->drawText(threadInfoX - threadInfoXOffset, threadInfoY + (i* threadInfoEntryHeight), ctx, str.c_str());
			}

			X_ENABLE_WARNING(4127)
		}

		for (uint32_t i = 0; i < numThread; i++)
		{		
			if (!timeLines[i]) {
				continue;
			}

			DrawThreadInfo(
				frameTimeInfo,
				pPrim,
				threadInfoX,
				threadInfoY + (i * threadInfoEntryHeight),
				threadInfoWidth,
				threadInfoEntryHeight - (padding * 2),
				timeLines[i]->getHistory()[profileIdx]
			);
		}

	}

	void XProfileSys::DrawThreadInfo(const FrameTimeData& frameTimeInfo, 
		engine::IPrimativeContext* pPrim , float xStart, float yStart, float width, float height,
		const core::V2::JobQueueHistory::FrameHistory& history)
	{
		pPrim->drawQuad(xStart, yStart, width, height, Color(0.55f, 0.35f, 0.35f, 0.1f));

		X_UNUSED(frameTimeInfo);
		X_UNUSED(history);

		if (history.bottom_ == 0) {
			return;
		}
	
		const size_t visibleMS = 16;
		const float widthPerMS = width / visibleMS;
		const float quadsize = 15.f;

		// i want to draw the history of this thread at correct based on start time relative to
		// start time, so that I can visualize 'bubbles'.
		// this means i need to know when the frame started.

		const auto frameStartTime = history.start;

		for (int32_t idx = 0; idx < history.bottom_; idx++)
		{
			const auto& entry = history.entryes_[idx];
			const auto timeOffset = entry.start - frameStartTime;

			float entryStart = timeOffset.GetMilliSeconds() * widthPerMS;
			float entryWidth = (entry.end - entry.start).GetMilliSeconds() * widthPerMS;

			pPrim->drawQuad(xStart + entryStart, yStart, entryWidth, quadsize, subSystemInfo_[entry.subsystem].col);
		}
	}

#endif // !X_ENABLE_JOBSYS_PROFILER

} // namespace profiler


#else


XProfileSys* XProfileSys::s_this = nullptr;

XProfileSys::XProfileSys(core::MemoryArenaBase* arena) :
	pCore_(nullptr),
	pFont_(nullptr),
	pRender_(nullptr),
	pPrimCon_(nullptr),
	profiles_(arena),
	displayInfo_(arena),
	// --------
	frameStartTime_(0),
	frameTime_(0),
	totalTime_(0)
{
	s_this = this;

}

XProfileSys::~XProfileSys()
{

}

void XProfileSys::registerVars(void)
{
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

void XProfileSys::registerCmds(void)
{

}



bool XProfileSys::init(ICore* pCore)
{
	X_LOG0("ProfileSys", "Starting");

	core::StopWatch time;

	pCore_ = pCore;

	profiles_.reserve(512);
	displayInfo_.reserve(512);

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
	subSystemInfo_[ProfileSubSys::TOOL].name = "Tool";
	subSystemInfo_[ProfileSubSys::UNITTEST].name = "UnitTests";

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


	X_LOG0("ProfileSys", "Init ^6%gms", time.GetMilliSeconds());
	return true;
}


void XProfileSys::shutDown(void)
{
	// ...
	gEnv->profileScopeStart = nullptr;
	gEnv->profileScopeEnd = nullptr;
}

bool XProfileSys::loadRenderResources(void)
{
	font::IFontSys* pFontSys = pCore_->GetIFontSys();
	if (!pFontSys) {
		return false;
	}

	pFont_ = pFontSys->GetFont("default");
	if (!pFont_) {
		return false;
	}

	return true;
}


void XProfileSys::AddProfileData(XProfileData* pData)
{
	// profile data is static and only added 
	// first time the profile scope is hit.
	// profilers for code that is not run won't be added.

	profiles_.push_back(pData);
}

void XProfileSys::OnFrameBegin(void)
{
	if (!isEnabled()) {
		return;
	}

	// get kinky.
	frameStartTime_ = ProfileTimer::getTicks();
}

void XProfileSys::OnFrameEnd(void)
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

#endif


X_NAMESPACE_END
