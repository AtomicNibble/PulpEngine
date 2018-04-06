#include "stdafx.h"
#include "Timer.h"

#include <ICore.h>
#include <IConsole.h>
#include <IFrameData.h>

#include "SystemTimer.h"

X_NAMESPACE_BEGIN(core)

namespace
{
    X_INLINE int64_t scaleTime(int64_t time, float32_t scale)
    {
        const double timeD = static_cast<double>(time);
        const double scaleD = static_cast<double>(scale);

        const double result = timeD * scaleD;

        return static_cast<int64_t>(result);
    }

} // namespace

XTimer::XTimer() :
    timeScale_(1.f),
    timeScaleUi_(1.f),
    maxFrameTimeDelta_(0),
    ticksPerSec_(0),
    debugTime_(0),
    maxFps_(0),
    deltaBufIdx_(0)
{
    core::zero_object(accumalatedTime_);
}

bool XTimer::init(ICore* pCore)
{
    X_UNUSED(pCore);
    ticksPerSec_ = SysTimer::GetTickPerSec();
    maxFrameTimeDelta_ = ticksPerSec_ / 5; // 0.2f
    timeScale_ = 1;
    timeScaleUi_ = 1;

    reset();

    ADD_CVAR_REF("time_debug", debugTime_, 0, 0, 1, VarFlag::SYSTEM, "Time debugging");
    core::ICVar* pVar = ADD_CVAR_FLOAT("time_max_frametime", 0.20f, 0.f, 10000.f, VarFlag::SYSTEM, "max time a frame can take (unscaled)");
    ADD_CVAR_REF("time_scale", timeScale_, 1.f, 0, 2000.f, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED | VarFlag::CHEAT, "scale time of each frame");
    ADD_CVAR_REF("time_scale_ui", timeScaleUi_, 1.f, 0, 2000.f, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED | VarFlag::CHEAT, "scale time of each UI frame");
    ADD_CVAR_REF("maxfps", maxFps_, 24, 0, 1000, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Max fps 0=unlimated");

    if (pVar) {
        core::ConsoleVarFunc del;
        del.Bind<XTimer, &XTimer::OnMaxFrameTimeChanged>(this);
        pVar->SetOnChangeCallback(del);
    }

    return true;
}

//////////////////////////////////////////////////

void XTimer::reset(void)
{
    baseTime_ = SysTimer::Get();
    lastFrameStartTime_ = 0;
    currentTime_ = 0;

    core::zero_object(accumalatedTime_);
}

//////////////////////////////////////////////////

void XTimer::OnFrameBegin(core::FrameTimeData& frameTime)
{
    const int64_t now = SysTimer::Get();

    currentTime_ = now - baseTime_;

    int64_t frameDelta = currentTime_ - lastFrameStartTime_;
    int64_t frameDeltaCapped = core::Min(frameDelta, maxFrameTimeDelta_);

    if (frameDelta != frameDeltaCapped && debugTime_) {
        const float32_t deltaMs = SysTimer::ToMilliSeconds(frameDelta);
        const float32_t cappedDeltaMs = SysTimer::ToMilliSeconds(frameDeltaCapped);
        X_LOG0("Time", "Frame delta was capped. delta: %f cappedDelta: %f", deltaMs, cappedDeltaMs);
    }

    if (maxFps_ != 0) {
        // sleep the diffrence.
        const int64_t targetTicks = (ticksPerSec_ / maxFps_);

        if (frameDelta < targetTicks) {
            const int64_t sleepTicks = targetTicks - frameDelta;
            const float sleepMs = math<float>::abs(SysTimer::ToMilliSeconds(sleepTicks));

            if (debugTime_) {
                X_LOG0("Timer", "Sleeping for %gms to limit frame rate.", sleepMs);
            }

            core::Thread::Sleep(static_cast<uint32_t>(sleepMs));

            frameDelta = targetTicks;
            frameDeltaCapped = targetTicks;
        }
    }

    frameTime.startTimeReal.SetValue(now);
    frameTime.startTimeRealative.SetValue(currentTime_);

    // set the unscaled timers
    frameTime.unscaledDeltas[Timer::GAME].SetValue(frameDeltaCapped);
    frameTime.unscaledDeltas[Timer::UI].SetValue(frameDeltaCapped);

    // now we need to scale each of the timers.
    {
        const int64_t gameTime = scaleTime(frameDeltaCapped, timeScale_);
        accumalatedTime_[Timer::GAME] += gameTime;
        frameTime.deltas[Timer::GAME].SetValue(gameTime);
    }
    {
        const int64_t uiTime = scaleTime(frameDeltaCapped, timeScaleUi_);
        accumalatedTime_[Timer::UI] += uiTime;
        frameTime.deltas[Timer::UI].SetValue(uiTime);
    }

    // i want the accumalated scaled time.
    frameTime.ellapsed[Timer::GAME].SetValue(accumalatedTime_[Timer::GAME]);
    frameTime.ellapsed[Timer::UI].SetValue(accumalatedTime_[Timer::UI]);

    updateAvgFrameTime(TimeVal(frameDeltaCapped));

    // set last time for use next frame.
    lastFrameStartTime_ = currentTime_;

    if (debugTime_) {
        X_LOG0("Timer", "Cur=%" PRId64 " Now=%" PRId64 " Game=%f UI=%f",
            currentTime_,
            now,
            frameTime.deltas[Timer::GAME].GetMilliSeconds(),
            frameTime.deltas[Timer::UI].GetMilliSeconds());
    }
}

TimeVal XTimer::GetTimeNowNoScale(void) const
{
    int64 now = SysTimer::Get();

    return TimeVal(now - baseTime_);
}

TimeVal XTimer::GetTimeNowReal(void) const
{
    return TimeVal(SysTimer::Get());
}

TimeVal XTimer::GetAvgFrameTime(void) const
{
    return avgTime_;
}

float XTimer::GetAvgFrameRate(void)
{
    return 1.f / avgTime_.GetSeconds();
}

float XTimer::GetScale(Timer::Enum timer)
{
    if (timer == Timer::GAME) {
        return timeScale_;
    }
    else if (timer == Timer::UI) {
        return timeScaleUi_;
    }

    X_ASSERT_UNREACHABLE();
    return 0.f;
}

void XTimer::OnMaxFrameTimeChanged(core::ICVar* pVar)
{
    const float val = pVar->GetFloat();

    // turn it into ticks.
    maxFrameTimeDelta_ = static_cast<int64_t>(ticksPerSec_ * val);
}

void XTimer::updateAvgFrameTime(TimeVal delta)
{
    deltaSum_ -= deltaBuf_[deltaBufIdx_];
    deltaSum_ += delta;

    deltaBuf_[deltaBufIdx_] = delta;

    if (++deltaBufIdx_ == NUM_DELTAS) {
        deltaBufIdx_ = 0;
    }

    avgTime_.SetValue(static_cast<int64_t>(static_cast<double>(deltaSum_.GetValue()) / NUM_DELTAS));
}

X_NAMESPACE_END