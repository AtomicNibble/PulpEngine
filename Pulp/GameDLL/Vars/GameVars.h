#pragma once

#include "PlayerVars.h"

X_NAMESPACE_BEGIN(game)

class GameVars
{
public:
    GameVars();
    ~GameVars() = default;

    void registerVars(void);

    X_INLINE int32_t userCmdDebug(void) const;
    X_INLINE bool userCmdDrawDebug(void) const;
    X_INLINE int32_t userCmdClientReplay(void) const;

    X_INLINE core::ICVar* getFovVar(void) const;

    PlayerVars player;

private:
    // TODO: should these really be network vars?
    int32_t userCmdDebug_;
    int32_t userCmdDrawDebug_;
    int32_t userCmdClientReplay_;

    core::ICVar* pFovVar_;
};

X_NAMESPACE_END

#include "GameVars.inl"