#pragma once

#ifndef _X_PROFILE_H_
#define _X_PROFILE_H_

#include <Containers\Array.h>
#include <Threading\JobSystem2.h>

#include "Vars\ProfilerVars.h"

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
		public ICoreEventListener
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

		void OnFrameBegin(void);
		void OnFrameEnd(void);

		void Render(const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys);

		X_INLINE const ProfilerVars& getVars(void) const;


	private:
		// ICoreEventListener		
		void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam);

	private:
		void UpdateProfileData(void);


#if X_ENABLE_JOBSYS_PROFILER
		void RenderJobSystem(const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys, int32_t profileIdx);
		void DrawThreadInfo(const FrameTimeData& frameTimeInfo, engine::IPrimativeContext* pPrim, float xStart, float yStart, float width, float height,
			const core::V2::JobQueueHistory::FrameHistory& history);

#endif // !X_ENABLE_JOBSYS_PROFILER

	private:
		ProfilerVars vars_;

		SubSystemInfoArr subSystemInfo_;

		// rendering stuff
		Vec2i renderRes_;
		font::IFont* pFont_;

		ProfilerDataPtrArr profilerData_;
		ProfilerDataHistoryPtrArr profilerHistoryData_;

		uint64_t frameStartTime_;
		uint64_t frameTime_;
		uint64_t totalTime_;
	};

} // namespace profiler

X_NAMESPACE_END

#include "XProfile.inl"

#endif // !_X_PROFILE_H_


