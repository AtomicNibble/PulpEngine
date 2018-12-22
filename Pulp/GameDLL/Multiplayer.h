#pragma once

#include <INetwork.h>

X_NAMESPACE_DECLARE(core, class FixedBitStreamBase);


X_NAMESPACE_BEGIN(game)

class Multiplayer
{
    X_DECLARE_ENUM(GameState)(
        NONE,
        GAME,
        SCOREBOARD        
    );

    struct PlayerState
    {
        PlayerState() :
            ping(0),
            kills(0)
        {
        }

        int32_t ping;   // the server tells us the players pings.
        int32_t kills;
    };

    using PlayerStateArr = std::array< PlayerState, net::MAX_PLAYERS>;

public:
    Multiplayer();

    void update(void);

    void readFromSnapShot(core::FixedBitStreamBase& bs);
    void writeToSnapShot(core::FixedBitStreamBase& bs);


private:
    GameState::Enum state_;
    GameState::Enum nextState_;

    PlayerStateArr playerStates_;
};


X_NAMESPACE_END