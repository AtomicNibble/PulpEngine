#include "stdafx.h"
#include "Multiplayer.h"
#include "UserNetMappings.h"

#include <Containers/FixedBitStream.h>

X_NAMESPACE_BEGIN(game)

Multiplayer::Multiplayer() :
    state_(GameState::NONE)
{

}

void Multiplayer::update(net::IPeer* pPeer, net::ILobby* pGameLobby, const UserNetMappings& unm)
{
    X_UNUSED(pGameLobby);

    // get all the pings yo.
    // this should just map with players.
    // maybe we should wait for spawn.
    for (size_t i = 0; i < unm.lobbyUserGuids.size(); i++)
    {
        if (!unm.lobbyUserGuids[i].isValid()) {
            continue;
        }

        auto sysHandle = unm.sysHandles[i];

        net::PingInfo pingInfo;
        if (pPeer->getPingInfo(sysHandle, pingInfo)) {
            continue;
        }

        playerStates_[i].ping = pingInfo.avg;
    }

    switch (state_)
    {
        case GameState::GAME:
            break;

        case GameState::SCOREBOARD:
            break;
    }


}

void Multiplayer::readFromSnapShot(core::FixedBitStreamBase& bs)
{
    bs.read(playerStates_.data(), playerStates_.size());

}

void Multiplayer::writeToSnapShot(core::FixedBitStreamBase& bs)
{
    bs.write(playerStates_.data(), playerStates_.size());

}


X_NAMESPACE_END