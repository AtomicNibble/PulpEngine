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
    userCmds_[writeFrame_[playerIndex] % BUFFER_SIZE][playerIndex] = cmd;
    writeFrame_[playerIndex]++;
}

void UserCmdMan::resetPlayer(int32_t playerIndex)
{
    for (int32_t i = 0; i < BUFFER_SIZE; i++) {
        core::zero_object(userCmds_[i][playerIndex]);
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

void UserCmdMan::readUserCmdToBs(core::FixedBitStreamBase& bs, int32_t playerIndex)
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
    core::TimeVal lastTime = lastCmd.gameTime;

    // we get sent redundant usercmds so need to find new ones.
    core::FixedArray<UserCmd, MAX_USERCMD_SEND> userCmds;

    for (int32_t i = 0; i < num; i++)
    {
        UserCmd cmd;
        cmd.fromBitStream(bs);

        if (cmd.gameTime > lastTime)
        {
            lastTime = cmd.gameTime;
            userCmds.append(cmd);
        }
        else if (cmd.gameTime == 0_tv)
        {
            X_WARNING("Net", "Recived user cmd with game time of zero");
        }
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