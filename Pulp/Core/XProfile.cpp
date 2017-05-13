#include "stdafx.h"
#include "XProfile.h"

#include <Time\StopWatch.h>
#include <Util\Process.h>
#include <Threading\JobSystem2.h>

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

		ProfileTimer::CalculateCPUSpeed();

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
			X_UNUSED(frameTimeInfo);

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

		float keyHeight = 30.f;
		const float maxHeightPerThread = 50.f;

		const int32_t visibleMS = vars_.jobSysThreadMS();
		const uint32_t numThread = pJobSys->GetThreadCount();
		const uint32_t numThreadQueues = pJobSys->GetQeueCount();
		const float threadInfoXOffset = 85.f;
		const float threadInfoX = xStart + padding + threadInfoXOffset;
		const float threadInfoY = yStart + ctx.size.y + (padding * 2);
		const float threadInfoHeight = core::Min<float>((height - (ctx.size.y + padding) - (keyHeight + padding)) - 20, maxHeightPerThread * numThreadQueues);
		const float threadInfoWidth = (width - (padding * 2)) - threadInfoXOffset;

		const float widthPerMS = threadInfoWidth / visibleMS;
		const float threadInfoEntryHeight = threadInfoHeight / numThreadQueues;


		core::StackString512 txt;
		txt.appendFmt("JobsRun: %" PRIi32 " JobsStolen: %" PRIi32 " JobsAssited: %" PRIi32,
			frameStats.jobsRun, frameStats.jobsStolen, frameStats.jobsAssited);

		engine::IPrimativeContext* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::PROFILE);

		std::array<int32_t, V2::JobSystem::HW_THREAD_MAX> jobCounts;

		for (uint32_t i = 0; i < numThreadQueues; i++)
		{
			if (!timeLines[i]) {
				continue;
			}

			jobCounts[i] = timeLines[i]->getHistory()[profileIdx].bottom_;
		}

		// draw a gird spliting up the horizontal space in to time.
		{
			// background
			pPrim->drawQuad(
				xStart, 
				yStart, 
				width, 
				threadInfoHeight + (threadInfoY - yStart) + (keyHeight * 2),
				Color(0.1f, 0.1f, 0.1f, 0.8f)
			);


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

			// draw how many jobs each thread ran
			{
				ctx.flags.Set(font::DrawTextFlag::RIGHT);

				for (size_t i = 0; i < numThreadQueues; i++)
				{
					str.setFmt("%i", jobCounts[i]);
					pPrim->drawText(threadInfoX - padding, threadInfoY + (i* threadInfoEntryHeight), ctx, str.c_str());
				}

				ctx.flags.Remove(font::DrawTextFlag::RIGHT);


			}

			// draw the color key
			{
				float keyY = threadInfoY + (threadInfoHeight + keyHeight);
				float keyX = threadInfoX + padding;

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

			ctx.flags.Set(font::DrawTextFlag::CENTER);
			pPrim->drawText(xStart + (width / 2), yStart + padding, ctx, L"Profiler");
			ctx.flags.Remove(font::DrawTextFlag::CENTER);
			pPrim->drawText(xStart + padding, yStart + padding, ctx, txt.begin(), txt.end());

			X_ENABLE_WARNING(4127)
		}

		for (uint32_t i = 0; i < numThreadQueues; i++)
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
	
		const int32_t visibleMS = vars_.jobSysThreadMS();
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




X_NAMESPACE_END
