#include "EngineCommon.h"
#include "UserCmdMan.h"

#include <Containers\FixedBitStream.h>
#include <Time\TimeLiterals.h>


X_NAMESPACE_BEGIN(net)


UserCmdMan::UserCmdMan()
{
    writeFrame_.fill(0);
    readFrame_.fill(-1);
}

void UserCmdMan::addUserCmdForPlayer(int32_t playerIndex, const UserCmd& cmd)
{
    auto freeSlots = getNumFreeSlots(playerIndex);
    if (freeSlots < 1) {

        // Set to middle of buffer as a temp fix until we can catch the client up correctly
        readFrame_[playerIndex] = (writeFrame_[playerIndex] - BUFFER_SIZE / 2);
        X_ASSERT(getNumFreeSlots(playerIndex), "No free slots")(getNumFreeSlots(playerIndex));

        X_WARNING("Net", "Usercmd buffer overflow for index %" PRIi32, playerIndex);
    }

    userCmds_[writeFrame_[playerIndex] % BUFFER_SIZE][playerIndex] = cmd;
    writeFrame_[playerIndex]++;
}

void UserCmdMan::resetPlayer(int32_t playerIndex)
{
    for (int32_t i = 0; i < BUFFER_SIZE; i++) {
        userCmds_[i][playerIndex].clear();
    }

    writeFrame_[playerIndex] = 0;
    readFrame_[playerIndex] = -1;
}

const UserCmd& UserCmdMan::newestUserCmdForPlayer(int32_t playerIndex)
{
    int32_t index = core::Max(writeFrame_[playerIndex] - 1, 0);
    return userCmds_[index % BUFFER_SIZE][playerIndex];
}

const UserCmd& UserCmdMan::getUserCmdForPlayer(int32_t playerIndex)
{
    if (readFrame_[playerIndex] < writeFrame_[playerIndex] - 1) {
        readFrame_[playerIndex]++;
    }

    //grab the next command in the readFrame buffer
    int32_t index = readFrame_[playerIndex];
    auto& result = userCmds_[index % BUFFER_SIZE][playerIndex];
    return result;
}

int32_t UserCmdMan::getNextUserCmdClientTimeMSForPlayer(int32_t playerIndex) const
{
    X_ASSERT(hasUnreadFrames(playerIndex), "Can't client time for cmd buffer is empty")(playerIndex);

    int32_t index = readFrame_[playerIndex];
    auto& cmd = userCmds_[index % BUFFER_SIZE][playerIndex];

    return cmd.clientGameTimeMS;
}

void UserCmdMan::writeUserCmdToBs(core::FixedBitStreamBase& bs, int32_t max, int32_t playerIndex) const
{
    if (max > MAX_USERCMD_SEND) {
        X_ERROR("Net", "tried to write %" PRIi32 " user cmd max is %" PRIi32, max, MAX_USERCMD_SEND);
        max = MAX_USERCMD_SEND;
    }

    // extract as many valid user cmds we have for the player, regardless of readFrame_
    auto startIdx = core::Max(writeFrame_[playerIndex] - core::Min<int32_t>(max, BUFFER_SIZE), 0_i32);
    auto num = writeFrame_[playerIndex] - startIdx;

    bs.write(safe_static_cast<int16_t>(num));

    for (int32_t i = 0; i < num; i++)
    {
        int32_t idx = (startIdx + i) % BUFFER_SIZE;
        auto& cmd = userCmds_[idx][playerIndex];
        
        cmd.writeToBitStream(bs);
    }
}

void UserCmdMan::readUserCmdFromBs(core::FixedBitStreamBase& bs, int32_t playerIndex)
{
    // meow.
    int32_t num = bs.read<int16_t>();
    if (num == 0) {
        X_ERROR("Net", "Recived 0 user cmds for player %" PRIi32 " ignoring", playerIndex);
        return;
    }

    // just ignore the bad client!
    if (num > net::MAX_USERCMD_SEND) {
        X_ERROR("Net", "Recived too many user cmds for player %" PRIi32 " ignoring", playerIndex);
        return;
    }

    // the last user cmd we got.
    auto& lastCmd = newestUserCmdForPlayer(playerIndex);
    int32_t lastTime = lastCmd.clientGameTimeMS;

    // we get sent redundant usercmds so need to find new ones.
    core::FixedArray<UserCmd, MAX_USERCMD_SEND> userCmds;

    for (int32_t i = 0; i < num; i++)
    {
        UserCmd cmd;
        cmd.fromBitStream(bs);

        if (cmd.clientGameTimeMS > lastTime)
        {
            lastTime = cmd.clientGameTimeMS;
            userCmds.append(cmd);
        }
        else if (cmd.clientGameTimeMS == 0)
        {
            X_WARNING("Net", "Recived user cmd with game time of zero");
        }
    }

    if (userCmds.isEmpty()) {
        X_WARNING("Net", "Recived no new user cmds for player %" PRIi32, playerIndex);
    }

#if 0
    if (vars_.userCmdDebug())
    {
        X_LOG0("Net", "Recived %" PRIuS " new usrCmds from player %" PRIi32, userCmds.size(), playerIndex);
    }
#endif

    // these are added for 0-N so in order.
    for (auto& cmd : userCmds)
    {
        addUserCmdForPlayer(playerIndex, cmd);
    }
}


X_NAMESPACE_END