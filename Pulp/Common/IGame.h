#pragma once

#ifndef X_GAME_I_H_
#define X_GAME_I_H_

static const uint32_t MAX_ENTS = 1 << 9;


X_NAMESPACE_DECLARE(core,
    struct FrameData);

X_NAMESPACE_DECLARE(input,
    struct InputEvent);

X_NAMESPACE_BEGIN(game)

struct IGame
{
    virtual ~IGame() = default;

    virtual void registerVars(void) X_ABSTRACT;
    virtual void registerCmds(void) X_ABSTRACT;

    virtual bool init(void) X_ABSTRACT;
    virtual bool shutDown(void) X_ABSTRACT;
    virtual void release(void) X_ABSTRACT;

    virtual bool asyncInitFinalize(void) X_ABSTRACT;

    virtual bool update(core::FrameData& frame) X_ABSTRACT;
    virtual bool onInputEvent(const input::InputEvent& event) X_ABSTRACT;


    virtual int32_t getLocalClientIdx(void) const X_ABSTRACT;
};

X_NAMESPACE_END

#endif // !X_GAME_I_H_
