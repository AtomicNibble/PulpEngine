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
                    class Material;);

X_NAMESPACE_BEGIN(core)

struct FrameTimeData;
struct XFileStats;

namespace profiler
{
    class XProfileSys : public IProfiler
        , public ICoreEventListener
    {
        struct SubSystemInfo
        {
            SubSystemInfo()
            {
                pName = nullptr;
                pWarningMat = nullptr;
            }

            const char* pName;
            engine::Material* pWarningMat;
            Color8u col;
        };

        typedef std::pair<core::TimeVal, engine::Material*> WarningEntry;
        typedef core::Array<WarningEntry> WarningArr;

        typedef std::array<SubSystemInfo, profiler::SubSys::ENUM_COUNT> SubSystemInfoArr;
        typedef std::array<core::TimeVal::TimeType, profiler::SubSys::ENUM_COUNT> SubSystemTimeArr;
        typedef core::Array<XProfileData*> ProfilerDataPtrArr;
        typedef core::Array<XProfileDataHistory*> ProfilerDataHistoryPtrArr;

        typedef ProfilerHistory<int64_t, 64> FrameTimes;

    public:
        XProfileSys(core::MemoryArenaBase* arena);
        ~XProfileSys() X_FINAL;

        void registerVars(void);
        void registerCmds(void);

        bool init(ICore* pCore);
        bool loadRenderResources(void);
        void shutDown(void);

        bool asyncInitFinalize(void);

        // IProfiler
        void AddProfileData(XProfileData* pData) X_FINAL;

        void ScopeBegin(XProfileScope* pScope) X_FINAL;
        void ScopeEnd(XProfileScope* pScope) X_FINAL;
        // ~IProfiler

        void OnFrameBegin(const FrameTimeData& frameTimeInfo);
        void OnFrameEnd(void);
        bool onInputEvent(const input::InputEvent& event);

        void Render(const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys);

        X_INLINE const ProfilerVars& getVars(void) const;

    private:
        // ICoreEventListener
        void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_FINAL;

    private:
        void UpdateProfileData(void);
        Vec2f RenderStartupData(Vec2f pos);
        Vec2f RenderArenaTree(Vec2f pos, core::MemoryArenaBase* arena);
        Vec2f RenderStr(Vec2f pos, const wchar_t* pTitle, const core::StackString512& str);

        static size_t countChildren_r(core::MemoryArenaBase* arena);

#if X_ENABLE_JOBSYS_PROFILER
        Vec2f RenderJobSystem(Vec2f pos, const FrameTimeData& frameTimeInfo, core::V2::JobSystem* pJobSys, int32_t profileIdx);
        void DrawThreadInfo(const FrameTimeData& frameTimeInfo, engine::IPrimativeContext* pPrim, float xStart, float yStart, float width, float height,
            const core::V2::JobQueueHistory::FrameHistory& history, SubSystemTimeArr& subTimesOut);

        void DrawSubsysInfo(engine::IPrimativeContext* pPrim, float xStart, float yStart, float width, float height,
            const SubSystemTimeArr& subTimes);

#endif // !X_ENABLE_JOBSYS_PROFILER

#if X_ENABLE_PROFILER_WARNINGS
        void drawWarnings(void);
#endif // !X_ENABLE_PROFILER_WARNINGS

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

        uint64_t frameStartTime_;
        uint64_t frameTime_;
        uint64_t totalTime_;
        FrameTimes frameTimeHistory_;

        ProfilerDataPtrArr profilerData_;
        ProfilerDataHistoryPtrArr profilerHistoryData_;

#if X_ENABLE_PROFILER_WARNINGS

        engine::Material* pWarnTotalMem_;
        engine::Material* pWarnTextureMem_;
        engine::Material* pWarnSndMem_;
        engine::Material* pWarnFileSys_;

        WarningArr warningList_;
#endif // !X_ENABLE_PROFILER_WARNINGS

        bool eventListerReg_;
    };

} // namespace profiler

X_NAMESPACE_END

#include "XProfile.inl"

#endif //!X_ENABLE_PROFILER
#endif // !_X_PROFILE_H_
