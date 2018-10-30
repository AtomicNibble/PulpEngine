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
#include "Console.h"

X_USING_NAMESPACE;

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

    core::FrameData frameData;
    if (pWindow_->Hasfocus()) {
        frameData.flags.Set(core::FrameFlag::HAS_FOCUS);
    }

    assetLoader_.dispatchPendingLoads();
    assetLoader_.update();

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

    // dispatch the core events.
    pCoreEventDispatcher_->pumpEvents();

    // get input events for this frame
    if (env_.pInput) {
        env_.pInput->update(frameData.input);
    }

    {
        // dispatch the input events.
        for (auto& ev : frameData.input.events)
        {
            // Alt-enter toggle fullscreen.
            if (ev.deviceType == input::InputDeviceType::KEYBOARD && ev.keyId == input::KeyId::ENTER &&
                ev.modifiers.IsSet(input::ModifiersMasks::Alt) && ev.action == input::InputState::PRESSED) {
                
                toggleFullscreen(); 
                continue;
            }

            if (static_cast<core::XConsole*>(env_.pConsole)->onInputEvent(ev)) {
                continue;
            }

#if X_ENABLE_PROFILER
            if (pProfiler_ && pProfiler_->onInputEvent(ev)) {
                continue;
            }
#endif // !X_ENABLE_PROFILER

            gEnv->pGame->onInputEvent(ev);
        }

    }


    // top job that we can use to wait for the chain of jobs to complete.
    Job* pSyncJob = jobSys.CreateEmtpyJob(JOB_SYS_SUB_ARG_SINGLE(core::profiler::SubSys::CORE));
    {
        // start a job to handler any file changes and create reload child jobs.
#if X_ENABLE_DIR_WATCHER
            Job* pDirectoryWatchProcess = jobSys.CreateMemberJobAsChild<XCore>(pSyncJob, this, &XCore::Job_DirectoryWatcher, nullptr JOB_SYS_SUB_ARG(core::profiler::SubSys::CORE));
            jobSys.Run(pDirectoryWatchProcess);
#endif // !X_ENABLE_DIR_WATCHER

    }

    env_.pConsole->dispatchRepeateInputEvents(frameData.timeInfo);
    // runs any commands that got submitted via input or config / other things.
    // so basically this is the only place that triggers command callbacks and modified callbacks.
    env_.pConsole->runCmds();


    jobSys.Run(pSyncJob);
    jobSys.Wait(pSyncJob);

    if (env_.p3DEngine) {
        env_.p3DEngine->update(frameData);
    }

    if (env_.pVideoSys) {
        env_.pVideoSys->update(frameData.timeInfo);
    }

    if (env_.pGame) {
        env_.pGame->update(frameData);
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
        env_.pScriptSys->update(frameData);
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

        engine::IPrimativeContext* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::PROFILE);

        const float padding = 10;
        const float yOffset = 30;

        Vec2f pos(padding, yOffset + padding);

#if X_ENABLE_PROFILER
        // draw me all the profile wins!
        if (pProfiler_) {
            pos = pProfiler_->Render(pPrim, pos, frameData.timeInfo, env_.pJobSys);
        }

#endif // !X_ENABLE_PROFILER

        if (env_.pVideoSys) {
            pos += env_.pVideoSys->drawDebug(pPrim, pos);
        }
        if (env_.pSound) {
            pos += env_.pSound->drawDebug(pPrim, pos);
        }

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
