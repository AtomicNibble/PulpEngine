#include "stdafx.h"
#include "Multiplayer.h"

#include <Containers/FixedBitStream.h>

X_NAMESPACE_BEGIN(game)

Multiplayer::Multiplayer() :
    state_(GameState::NONE)
{

}

void Multiplayer::update(void)
{
    // i want to populate all the ping info for connected players.
    // which means i need the lobby and the peer.
    // i need some way to map netGuids to player idx tho.
    // think i might just have some for of player state that i can pass around.

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