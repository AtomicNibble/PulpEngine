#pragma once

#include <Time\TimeVal.h>

#include <IInput.h>
#include <ITimer.h>

#include <Math\XViewPort.h>

X_NAMESPACE_BEGIN(core)

X_DECLARE_FLAGS(FrameFlag)
(
    HAS_FOCUS

);

struct FrameInput
{
    // These should be used when showing a visible cursor.
    // Stuff like player view should just be driven from relative mouse events.
    // This is because the visible cursor can just jump, eg alt tab or touch screen.
    Vec2i cusorPos;
    Vec2i cusorPosClient;

    // this is just a buffer where this frames input events get stored.
    // you should not read from this, but instead register a input listner.
    // as that respects priority and filtering.
    input::InputEventBuffer events;
};

struct FrameTimeData
{
    TimeVal startTimeReal;      // real frame start time
    TimeVal startTimeRealative; // unscaled relative time to start of timer system.

    TimeVal deltas[ITimer::Timer::ENUM_COUNT];
    TimeVal unscaledDeltas[ITimer::Timer::ENUM_COUNT];
    TimeVal ellapsed[ITimer::Timer::ENUM_COUNT];
};

X_DISABLE_WARNING(4324)

struct FrameView
{
    Vec2i displayRes;
    Vec2i renderRes;
    XViewPort viewport;

    XCamera cam; // camera pos for this frame.

    X_ALIGN16_MATRIX44F(viewMatrix);
    X_ALIGN16_MATRIX44F(viewMatrixInv);
    X_ALIGN16_MATRIX44F(projMatrix);
    X_ALIGN16_MATRIX44F(viewProjMatrix);
    X_ALIGN16_MATRIX44F(viewProjInvMatrix);

    // for 2d
    X_ALIGN16_MATRIX44F(viewMatrixOrtho);
    X_ALIGN16_MATRIX44F(projMatrixOrtho);
    X_ALIGN16_MATRIX44F(viewProjMatrixOrth);
};

X_ENABLE_WARNING(4324)

struct FrameData
{
    typedef Flags<FrameFlag> FrameFlags;

    FrameTimeData timeInfo;
    FrameFlags flags;
    FrameInput input;

    FrameView view;
};

X_NAMESPACE_END
