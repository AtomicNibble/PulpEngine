#include "stdafx.h"
#include "Core.h"

#include <IConsole.h>
#include <IRender.h>
#include <I3DEngine.h>
#include <IGame.h>
#include <ISound.h>
#include <IScriptSys.h>
#include <IPhysics.h>
#include <IVideo.h>

#include <IFrameData.h>

#include <Threading\JobSystem2.h>
#include <Platform\Window.h>

#include "CoreEventDispatcher.h"

X_USING_NAMESPACE;

// we have a load of things todo.
/*
Get input. (input events)
Check dir watcher for changed files.
Console?
3dEngine:
animations
loading of assets
culling
greating drawcalls.
sounds
scripts sys
physics


Do we want to be fancy and depending on how much spare frame tiem we have for the target fps, allow more streaming jobs to happen.
Also instead of sleeping we could be running physcis steps...
*/

bool XCore::RunGameLoop(void)
{
    env_.state_ = CoreGlobals::State::RUNNING;

    while (PumpMessages()) {
        X_PROFILE_BEGIN("GameLoop", core::profiler::SubSys::GAME);

        Update();
    }
    return true;
}

bool XCore::Update(void)
{
    X_PROFILE_BEGIN("CoreUpdate", core::profiler::SubSys::CORE);
    using namespace core::V2;

    assetLoader_.dispatchPendingLoads();
    assetLoader_.update();

    auto width = vars_.getWinWidth();
    auto height = vars_.getWinHeight();

    core::FrameData frameData;
    frameData.flags.Set(core::FrameFlag::HAS_FOCUS);
    frameData.view.viewport.set(width, height);
    frameData.view.viewport.setZ(0.f, 1.f);

    // get time deltas for this frame.
    time_.OnFrameBegin(frameData.timeInfo);

#if X_ENABLE_PROFILER
    if (pProfiler_) {
        pProfiler_->OnFrameBegin(frameData.timeInfo);
    }
#endif // !X_ENABLE_PROFILER

    JobSystem& jobSys = *env_.pJobSys;

    {
        bool paused = false;

#if X_ENABLE_PROFILER
        if (pProfiler_) {
            paused = pProfiler_->getVars().isPaused();
        }
#endif // !X_ENABLE_PROFILER

        jobSys.OnFrameBegin(paused);
    }


    // get input events for this frame
    if (env_.pInput) {
        env_.pInput->update(frameData.input);
    }

    // dispatch the core events.
    pEventDispatcher_->pumpEvents();

    if (env_.pVideoSys) {
        env_.pVideoSys->update(frameData.timeInfo);
    }

    // top job that we can use to wait for the chain of jobs to complete.
    Job* pSyncJob = jobSys.CreateEmtpyJob(JOB_SYS_SUB_ARG_SINGLE(core::profiler::SubSys::CORE));
    {
        // start a job to handler any file chnages and create relaod child jobs.
        Job* pDirectoryWatchProcess = jobSys.CreateMemberJobAsChild<XCore>(pSyncJob, this, &XCore::Job_DirectoryWatcher, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::CORE));
        jobSys.Run(pDirectoryWatchProcess);

        // create a job for syncing all input related jobs.
        Job* pInputSync = jobSys.CreateEmtpyJobAsChild(pSyncJob JOB_SYS_SUB_ARG(core::profiler::SubSys::CORE));
        {
            Job* pPostInputFrame = jobSys.CreateMemberJobAsChild<XCore>(pInputSync, this, &XCore::Job_PostInputFrame, &frameData JOB_SYS_SUB_ARG(core::profiler::SubSys::CORE));
            Job* pConsoleUpdates = jobSys.CreateMemberJobAsChild<XCore>(pInputSync, this, &XCore::Job_ConsoleUpdates, &frameData.timeInfo JOB_SYS_SUB_ARG(core::profiler::SubSys::CORE));

            // we run console updates after input events have been posted.
            jobSys.AddContinuation(pPostInputFrame, pConsoleUpdates);

            jobSys.Run(pPostInputFrame);
        }

        jobSys.Run(pInputSync);
    }

    jobSys.Run(pSyncJob);
    jobSys.Wait(pSyncJob);

    if (env_.pGame) {
        env_.pGame->update(frameData);
    }

    if (env_.p3DEngine) {
        env_.p3DEngine->update(frameData);
    }

    if (env_.pPhysics) {
        env_.pPhysics->onTickPreRender(
            frameData.timeInfo.deltas[core::ITimer::Timer::GAME],
            AABB(Vec3f::zero(), 2000.f));
    }

    // we could update the sound system while rendering on gpu.
    if (env_.pSound) {
        env_.pSound->update(frameData);
    }

    if (env_.pScriptSys) {
        env_.pScriptSys->update();
    }

    RenderBegin(frameData);
    RenderEnd(frameData);

#if 1
    static core::TimeVal start = time_.GetTimeNowNoScale();
    core::TimeVal time = time_.GetTimeNowNoScale();

    float val = time.GetDifferenceInSeconds(start);
    if (val >= 0.95f) {
        start = time;

        float fps = time_.GetAvgFrameRate();
        core::TimeVal frametime = time_.GetAvgFrameTime();

        core::StackString<128> title;
        title.clear();
        title.appendFmt(X_ENGINE_NAME " Engine " X_CPUSTRING " (fps:%" PRIi32 ", %gms) Game: %" PRId64 "(x%g) UI: %" PRId64 "(x%g)",
            static_cast<int>(fps),
            frametime.GetMilliSeconds(),
            frameData.timeInfo.deltas[core::Timer::GAME].GetMilliSecondsAsInt64(),
            time_.GetScale(core::Timer::GAME),
            frameData.timeInfo.deltas[core::Timer::UI].GetMilliSecondsAsInt64(),
            time_.GetScale(core::Timer::UI));

        pWindow_->SetTitle(title.c_str());
    }

    int goat = 0;
    goat = 2;
#endif
    return true;
}

void XCore::RenderBegin(core::FrameData& frameData)
{
    X_PROFILE_BEGIN("CoreRenderBegin", core::profiler::SubSys::CORE);
    X_UNUSED(frameData);

    env_.pRender->renderBegin();
    env_.p3DEngine->onFrameBegin(frameData);
}

void XCore::RenderEnd(core::FrameData& frameData)
{
    {
        X_PROFILE_BEGIN("CoreRenderEnd", core::profiler::SubSys::CORE);

        if (env_.pPhysics) {
            env_.pPhysics->onTickPostRender(frameData.timeInfo.deltas[core::ITimer::Timer::GAME]);
        }

#if X_ENABLE_PROFILER

        // draw me all the profile wins!
        if (pProfiler_) {
            pProfiler_->Render(frameData.timeInfo, env_.pJobSys);
        }

#endif // !X_ENABLE_PROFILER

        if (core::IConsole* pConsole = GetIConsole()) {
            pConsole->draw(frameData.timeInfo);
        }

        env_.pRender->renderEnd();
    }

#if X_ENABLE_PROFILER

    // End
    if (pProfiler_) {
        pProfiler_->OnFrameEnd();
    }

#endif // !X_ENABLE_PROFILER
}
