#pragma once

#include "UserCmd.h"

X_NAMESPACE_BEGIN(game)

//
// Manages user commands for multiple clients, keeping a history.
//
class UserCmdMan
{
    static const size_t BUFFER_SIZE = 64;
    // we create sets of UserCmds for each player at each read index.
    // just so if all clients are currenton same write index, near in memory.
    typedef std::array<UserCmd, MAX_PLAYERS> UserCmdAPlayerArr;
    typedef std::array<UserCmdAPlayerArr, BUFFER_SIZE> UserCmdBuf;
    typedef std::array<int32_t, MAX_PLAYERS> IndexArr;

public:
    UserCmdMan();

    void addUserCmdForPlayer(int32_t playerIndex, const UserCmd& cmd);
    void resetPlayer(int32_t playerIndex);

    const UserCmd& newestUserCmdForPlayer(int32_t playerIndex);
    const UserCmd& getUserCmdForPlayer(int32_t playerIndex);

    X_INLINE size_t getNumUnreadFrames(int32_t playerIndex);

private:
    IndexArr writeFrame_;
    IndexArr readFrame_;
    UserCmdBuf userCmds_; // buffers for each player.
};

X_INLINE size_t UserCmdMan::getNumUnreadFrames(int32_t playerIndex)
{
    return safe_static_cast<size_t>((writeFrame_[playerIndex] - 1) - readFrame_[playerIndex]);
}

X_NAMESPACE_END