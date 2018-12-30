#pragma once

#include <INetwork.h>

#include <Containers/FixedFifo.h>
#include <Containers/FixedBitStream.h>

X_NAMESPACE_DECLARE(core, class FixedBitStreamBase);

X_NAMESPACE_BEGIN(game)

class GameVars;
struct UserNetMappings;

class Multiplayer
{
    X_DECLARE_ENUM(Event)(
        // We don't need events for a player joining, since the client already knows when a player joins / leaves
        // based on the snapshot.

        PLY_KILLED,     // killed by someone
        PLY_DIED,       // player fell off a cliff or something.
        PLY_LEFT        // nobody liked you anyway!
    );

    X_DECLARE_ENUM(GameState)(
        NONE,
        GAME,
        SCOREBOARD        
    );

    struct PlayerState
    {
        PlayerState() :
            ping(0),
            points(0),
            kills(0),
            headshots(0),
            deaths(0)
        {
        }

        int32_t ping;   // the server tells us the players pings.
        int32_t points;
        int32_t kills;
        int32_t headshots;
        int32_t deaths;
    };

    static const size_t NUM_CHAT_LINES = 6;
    static const size_t NUM_EVENT_LINES = 6;

    struct TextLine
    {
        TextLine(core::string_view line) :
            line(line.data(), line.length())
        {
        }

        TextLine(core::string line) :
            line(line)
        {
        }

        core::string line;
        core::TimeVal ellapsed;
    };

    using ChatLineFiFo = core::FixedFifo<TextLine, NUM_CHAT_LINES>;
    using EventLineFiFo = core::FixedFifo<TextLine, NUM_EVENT_LINES>;
    using PlayerStateArr = std::array<PlayerState, net::MAX_PLAYERS>;

public:
    using ChatPacketBs = core::FixedBitStreamStack<net::MAX_USERNAME_LEN + net::MAX_CHAT_MSG_LEN + 16>;
    using EventPacketBs = core::FixedBitStreamStack<64>;

public:
    Multiplayer(GameVars& vars, net::ISession* pSession);

    void update(net::IPeer* pPeer, const UserNetMappings& unm);

    void drawChat(core::FrameTimeData& time, engine::IPrimativeContext* pPrim);
    void drawEvents(core::FrameTimeData& time, engine::IPrimativeContext* pPrim);
    void drawLeaderboard(const UserNetMappings& unm, engine::IPrimativeContext* pPrim);

    void readFromSnapShot(core::FixedBitStreamBase& bs);
    void writeToSnapShot(core::FixedBitStreamBase& bs);

    void playerSpawned(const UserNetMappings& unm, int32_t localIndex);
    void playerDeath(const UserNetMappings& unm, int32_t playerIdx, entity::EntityId attacker);
    void playerLeft(const UserNetMappings& unm, int32_t localIndex);
    void handleChatMsg(core::string_view name, core::string_view msg);
    void handleEvent(const UserNetMappings& unm, core::FixedBitStreamBase& bs);

    void addChatLine(core::string_view name, core::string_view msg);
private:
    void addEventLine(core::string_view line);

    void drawChat(engine::IPrimativeContext* pPrim);
    void drawEvents(engine::IPrimativeContext* pPrim);
    void updateChat(core::TimeVal dt);
    void updateEvents(core::TimeVal dt);

    void postEvent(const UserNetMappings& unm, Event::Enum evt, int32_t param0, int32_t param1);

public:
    static void buildChatPacket(ChatPacketBs& bs, core::string_view name, core::string_view msg);

private:
    GameVars& vars_;
    net::ISession* pSession_;

    GameState::Enum state_;
    GameState::Enum nextState_;

    PlayerStateArr playerStates_;

    // Chitty chat
    ChatLineFiFo chatLines_;
    EventLineFiFo eventLines_;
};


X_NAMESPACE_END