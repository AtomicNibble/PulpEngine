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

    timeScale_[Timer::GAME] = 1.f;
    timeScale_[Timer::UI] = 1.f;

    reset();

    ADD_CVAR_REF("time_debug", debugTime_, 0, 0, 1, VarFlag::SYSTEM, "Time debugging");
    ADD_CVAR_REF("time_scale", timeScale_[Timer::GAME], 1.f, 0, 2000.f, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED | VarFlag::CHEAT, "scale time of each frame");
    ADD_CVAR_REF("time_scale_ui", timeScale_[Timer::UI], 1.f, 0, 2000.f, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED | VarFlag::CHEAT, "scale time of each UI frame");
    ADD_CVAR_REF("maxfps", maxFps_, 24, 0, 1000, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Max fps 0=unlimated");

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
    int64_t now = SysTimer::Get();
    currentTime_ = now - baseTime_;

    const int64_t realFrameDelta = currentTime_ - lastFrameStartTime_;

    if (maxFps_ != 0) 
    {
        // sleep the diffrence.
        const int64_t targetTicks = (ticksPerSec_ / maxFps_);

        if (realFrameDelta < targetTicks) {
            const int64_t sleepTicks = targetTicks - realFrameDelta;
            const float sleepMs = math<float>::abs(SysTimer::ToMilliSeconds(sleepTicks));

            if (debugTime_) {
                X_LOG0("Timer", "Sleeping for %gms to limit frame rate.", sleepMs);
            }

            core::Thread::sleep(static_cast<uint32_t>(sleepMs));

            // how long did we actually sleep for.
            now = SysTimer::Get();
            currentTime_ = now - baseTime_;
        }
    }

    const int64_t frameDelta = currentTime_ - lastFrameStartTime_;

    frameTime.startTimeReal.SetValue(now);
    frameTime.startTimeRealative.SetValue(currentTime_);

    // set the unscaled timers
    frameTime.unscaledDeltas[Timer::GAME].SetValue(frameDelta);
    frameTime.unscaledDeltas[Timer::UI].SetValue(frameDelta);

    // now we need to scale each of the timers.
    {
        const int64_t gameTime = scaleTime(frameDelta, timeScale_[Timer::GAME]);
        accumalatedTime_[Timer::GAME] += gameTime;
        frameTime.deltas[Timer::GAME].SetValue(gameTime);
    }
    {
        const int64_t uiTime = scaleTime(frameDelta, timeScale_[Timer::UI]);
        accumalatedTime_[Timer::UI] += uiTime;
        frameTime.deltas[Timer::UI].SetValue(uiTime);
    }

    // i want the accumalated scaled time.
    frameTime.ellapsed[Timer::GAME].SetValue(accumalatedTime_[Timer::GAME]);
    frameTime.ellapsed[Timer::UI].SetValue(accumalatedTime_[Timer::UI]);

    updateAvgFrameTime(TimeVal(frameDelta));

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
    return timeScale_[timer];
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