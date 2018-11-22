#pragma once

#include <UserCmd.h>
#include <INetwork.h>

X_NAMESPACE_BEGIN(game)

//
// Manages user commands for multiple clients, keeping a history.
//
class GameVars;

class UserCmdMan
{
    static const size_t BUFFER_SIZE = 64;
    // we create sets of UserCmds for each player at each read index.
    // just so if all clients are currently on same write index, near in memory.
    typedef std::array<net::UserCmd, net::MAX_PLAYERS> UserCmdPlayerArr;
    typedef std::array<UserCmdPlayerArr, BUFFER_SIZE> UserCmdBuf;
    typedef std::array<int32_t, net::MAX_PLAYERS> IndexArr;

public:
    UserCmdMan(GameVars& vars);

    void addUserCmdForPlayer(int32_t playerIndex, const net::UserCmd& cmd);
    void resetPlayer(int32_t playerIndex);

    const net::UserCmd& newestUserCmdForPlayer(int32_t playerIndex);
    const net::UserCmd& getUserCmdForPlayer(int32_t playerIndex);

    void writeUserCmdToBs(core::FixedBitStreamBase& bs, int32_t max, int32_t playerIndex) const;
    void readUserCmdToBs(core::FixedBitStreamBase& bs, int32_t playerIndex);

    X_INLINE size_t getNumUnreadFrames(int32_t playerIndex) const;
    X_INLINE bool hasUnreadFrames(int32_t playerIndex) const;

private:
    GameVars& vars_;
    IndexArr writeFrame_;
    IndexArr readFrame_;
    UserCmdBuf userCmds_; // buffers for each player.
};

X_INLINE size_t UserCmdMan::getNumUnreadFrames(int32_t playerIndex) const
{
    return safe_static_cast<size_t>((writeFrame_[playerIndex] - 1) - readFrame_[playerIndex]);
}

X_INLINE bool UserCmdMan::hasUnreadFrames(int32_t playerIndex) const
{
    return readFrame_[playerIndex] < (writeFrame_[playerIndex] - 1);
}

X_NAMESPACE_END