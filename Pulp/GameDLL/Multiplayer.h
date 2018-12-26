#pragma once

#include <INetwork.h>

#include <Containers/FixedFifo.h>

X_NAMESPACE_DECLARE(core, class FixedBitStreamBase);

X_NAMESPACE_BEGIN(game)

struct UserNetMappings;

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

    static const size_t NUM_CHAT_LINES = 6;

    struct ChatLine
    {
        ChatLine(core::string line) :
            line(line)
        {
        }

        core::string line;
        core::TimeVal ellapsed;
    };

    using ChatLineFiFo = core::FixedFifo<ChatLine, NUM_CHAT_LINES>;
    using PlayerStateArr = std::array< PlayerState, net::MAX_PLAYERS>;

public:
    Multiplayer();

    void update(net::IPeer* pPeer, const UserNetMappings& unm);

    void draw(core::FrameTimeData& time, engine::IPrimativeContext* pPrim);

    void readFromSnapShot(core::FixedBitStreamBase& bs);
    void writeToSnapShot(core::FixedBitStreamBase& bs);

    void addChatLine(core::string line);

private:
    void drawChat(engine::IPrimativeContext* pPrim);
    void updateChat(core::TimeVal dt);

    void drawLeaderboard(engine::IPrimativeContext* pPrim);

private:
    GameState::Enum state_;
    GameState::Enum nextState_;

    PlayerStateArr playerStates_;

    // Chitty chat
    core::TimeVal chatTime_;
    ChatLineFiFo chatLines_;
};


X_NAMESPACE_END