#pragma once

#include <UserCmd.h>
#include <INetwork.h>

X_NAMESPACE_BEGIN(net)

//
// Manages user commands for multiple clients, keeping a history.
//
class UserCmdMan
{
    static const size_t BUFFER_SIZE = 64;
    // we create sets of UserCmds for each player at each read index.
    // just so if all clients are currently on same write index, near in memory.
    typedef std::array<UserCmd, MAX_PLAYERS> UserCmdPlayerArr;
    typedef std::array<UserCmdPlayerArr, BUFFER_SIZE> UserCmdBuf;
    typedef std::array<int32_t, MAX_PLAYERS> IndexArr;

public:
    UserCmdMan();

    void addUserCmdForPlayer(int32_t playerIndex, const UserCmd& cmd);
    void resetPlayer(int32_t playerIndex);

    const UserCmd& newestUserCmdForPlayer(int32_t playerIndex);
    const UserCmd& getUserCmdForPlayer(int32_t playerIndex);
    int32_t getNextUserCmdClientTimeMSForPlayer(int32_t playerIndex) const;

    void writeUserCmdToBs(core::FixedBitStreamBase& bs, int32_t max, int32_t playerIndex) const;
    void readUserCmdFromBs(core::FixedBitStreamBase& bs, int32_t playerIndex);

    X_INLINE size_t getNumFreeSlots(int32_t playerIndex) const;
    X_INLINE size_t getNumUnreadFrames(int32_t playerIndex) const;
    X_INLINE bool hasUnreadFrames(int32_t playerIndex) const;

private:
    IndexArr writeFrame_;
    IndexArr readFrame_;
    UserCmdBuf userCmds_; // buffers for each player.
};


X_INLINE size_t UserCmdMan::getNumFreeSlots(int32_t playerIndex) const
{
    return BUFFER_SIZE - safe_static_cast<size_t>((writeFrame_[playerIndex]) - readFrame_[playerIndex]);
}

X_INLINE size_t UserCmdMan::getNumUnreadFrames(int32_t playerIndex) const
{
    return safe_static_cast<size_t>((writeFrame_[playerIndex] - 1) - readFrame_[playerIndex]);
}

X_INLINE bool UserCmdMan::hasUnreadFrames(int32_t playerIndex) const
{
    return readFrame_[playerIndex] < (writeFrame_[playerIndex] - 1);
}

X_NAMESPACE_END