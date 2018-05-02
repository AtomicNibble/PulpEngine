#include "stdafx.h"
#include "UserCmdMan.h"

X_NAMESPACE_BEGIN(game)

UserCmdMan::UserCmdMan()
{
    writeFrame_.fill(0);
    readFrame_.fill(-1);
}

void UserCmdMan::addUserCmdForPlayer(int32_t playerIndex, const net::UserCmd& cmd)
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

const net::UserCmd& UserCmdMan::newestUserCmdForPlayer(int32_t playerIndex)
{
    int32_t index = core::Max(writeFrame_[playerIndex] - 1, 0);
    return userCmds_[index % BUFFER_SIZE][playerIndex];
}

const net::UserCmd& UserCmdMan::getUserCmdForPlayer(int32_t playerIndex)
{
    if (readFrame_[playerIndex] < writeFrame_[playerIndex] - 1) {
        readFrame_[playerIndex]++;
    }

    //grab the next command in the readFrame buffer
    int32_t index = readFrame_[playerIndex];
    auto& result = userCmds_[index % BUFFER_SIZE][playerIndex];
    return result;
}

X_NAMESPACE_END