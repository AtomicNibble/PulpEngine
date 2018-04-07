#pragma once

#ifndef _X_TIMER_H_
#define _X_TIMER_H_

#include <ITimer.h>

X_NAMESPACE_DECLARE(core,
                    struct ICVar);

X_NAMESPACE_BEGIN(core)

class XTimer : public ITimer
{
    static const size_t NUM_DELTAS = 64;

public:
    XTimer();
    ~XTimer() X_OVERRIDE = default;

    bool init(ICore* pCore) X_FINAL;

    // reset the timers, so delta from start time is zero.
    void reset(void) X_FINAL;

    void OnFrameBegin(core::FrameTimeData& frameTime) X_FINAL;

    TimeVal GetTimeNowNoScale(void) const X_FINAL;

    // returns the time now relative to base time.
    TimeVal GetTimeNowReal(void) const X_FINAL;

    TimeVal GetAvgFrameTime(void) const X_FINAL;
    float GetAvgFrameRate(void) X_FINAL;

    float GetScale(Timer::Enum timer) X_FINAL;

private:
    void OnMaxFrameTimeChanged(core::ICVar* pVar);

    void updateAvgFrameTime(TimeVal delta);

private:
    int64_t baseTime_;                           // time we started / reset
    int64_t lastFrameStartTime_;                 // start time relative to base time
    int64_t currentTime_;                        // relative to base.
    int64_t accumalatedTime_[Timer::ENUM_COUNT]; // accumalted scaled time

    float32_t timeScale_;       // scale for game
    float32_t timeScaleUi_;     // scale for ui
    int64_t maxFrameTimeDelta_; // cap delta's that exceed this value.
    int64_t ticksPerSec_;

    int32_t debugTime_;
    int32_t maxFps_;

    size_t deltaBufIdx_;
    TimeVal avgTime_;
    TimeVal deltaSum_;
    TimeVal deltaBuf_[NUM_DELTAS];
};

X_NAMESPACE_END

#endif // !_X_TIMER_H_
