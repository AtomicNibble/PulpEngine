#pragma once

#ifndef _X_PROFILE_H_
#define _X_PROFILE_H_
#if X_ENABLE_PROFILER

#include <Containers\Array.h>
#include <Threading\JobSystem2.h>
#include <Time\TimeVal.h>

#include "Vars\ProfilerVars.h"

#include <IInput.h>


X_NAMESPACE_DECLARE(render, struct IRender);

X_NAMESPACE_DECLARE(engine,
	class IPrimativeContext;
);

X_NAMESPACE_BEGIN(core)

struct FrameTimeData;

namespace profiler
{

	class XProfileSys : 
		public IProfiler,
		public ICoreEventListener,
		public input::IInputEventListner
	{
		struct SubSystemInfo
		{
			const char* pName;
			Colorf col;
		};

		typedef std::array<SubSystemInfo, profiler::SubSys::ENUM_COUNT> SubSystemInfoArr;
		typedef core::Array<XProfileData*>	ProfilerDataPtrArr;
		typedef core::Array<XProfileDataHistory*>	ProfilerDataHistoryPtrArr;


	public:
		XProfileSys(core::MemoryArenaBase* arena);
		~XProfileSys() X_FINAL;

		void registerVars(void);
		void registerCmds(void);

		bool init(ICore* pCore);
		bool loadRenderResources(void);
		void shutDown(void);

		// IProfiler
		void AddProfileData(XProfileData* pData) X_FINAL;

		void ScopeBegin(XProfileScope* pScope) X_FINAL;
		void ScopeEnd(XProfileScope* pScope) X_FINAL;
		// ~IProfiler

		void OnFrameBegin(const FrameTimeData& frameTimeInfo);
		void OnFrameEnd(void);

		void Render(const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys);

		X_INLINE const ProfilerVars& getVars(void) const;


	private:
		// ICoreEventListener		
		void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam);

		// IInputEventListner
		bool OnInputEvent(const input::InputEvent& event) X_FINAL;
		bool OnInputEventChar(const input::InputEvent& event) X_FINAL;
		int32_t GetInputPriority(void) const X_FINAL;

	private:
		void UpdateProfileData(void);
		Vec2f RenderStartupData(Vec2f pos);
		Vec2f RenderMemoryInfo(Vec2f pos, const wchar_t* pTitle, const core::MemoryAllocatorStatistics& stats);

#if X_ENABLE_JOBSYS_PROFILER
		Vec2f RenderJobSystem(Vec2f pos, const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys, int32_t profileIdx);
		void DrawThreadInfo(const FrameTimeData& frameTimeInfo, engine::IPrimativeContext* pPrim, float xStart, float yStart, float width, float height,
			const core::V2::JobQueueHistory::FrameHistory& history);

#endif // !X_ENABLE_JOBSYS_PROFILER

	private:
		ProfilerVars vars_;

		SubSystemInfoArr subSystemInfo_;

		// rendering stuff
		Vec2i renderRes_;
		font::IFont* pFont_;
		int32_t frameOffset_;

		X_DISABLE_WARNING(4324);
		input::InputEvent repeatEvent_;
		X_ENABLE_WARNING(4324);

		TimeVal repeatEventInterval_;
		TimeVal repeatEventInitialDelay_;
		TimeVal repeatEventTimer_; // the time a repeat event will be trigger.


		ProfilerDataPtrArr profilerData_;
		ProfilerDataHistoryPtrArr profilerHistoryData_;
	};

} // namespace profiler

X_NAMESPACE_END

#include "XProfile.inl"

#endif //!X_ENABLE_PROFILER
#endif // !_X_PROFILE_H_


