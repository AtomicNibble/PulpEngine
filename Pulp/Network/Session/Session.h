#pragma once


#include <Containers\Array.h>
#include <Time\TimeVal.h>

#include "SessionCallbacks.h"
#include "Lobby.h"

X_NAMESPACE_BEGIN(net)

/*

I want this to be like a game session.
The reason it's in network lib, is no matter what game you make you should use this session.
and single player works like a loopback server.

So this session is for handling current game state like in menu / in game / loading etc..
You submit useCmd's to the session and get snapshots back will all the ent info.

The session has no idea what's in a snap shot, it's just a blob of data, it's unto the gameDll to interperate the data.


The flow:
    PartyLobby -> GameLobby -> Loading -> InGame

Basically all players join a PartyLobby which may be hosted by the player.
Or a 3rd party service, once we want to start the game we need to move into a GameLobby.

the game lobby is hosted on the machine hosting the game.
Once the users have connected to that we can move to in game.
The players in the game lobby are the players in the game.


*/

X_DECLARE_ENUM(SessionState)(
    Idle,                           // menu
    CreateAndMoveToPartyLobby,      //  
    CreateAndMoveToGameLobby,       //
    PartyLobbyHost,                 //
    PartyLobbyPeer,                 // pleb
    GameLobbyHost,                  //
    GameLobbyPeer,                  // pleb
    ConnectAndMoveToParty,          //
    ConnectAndMoveToGame,           //
    Loading,                        //
    InGame                          //
);

struct UserCmd;
class SnapShot;

struct MatchParameters;

class SessionVars;

class Session : public ISession, ISessionCallbacks
{
    X_NO_COPY_MOVE_ALL(Session);

    using LobbyArr = std::array<Lobby,LobbyType::ENUM_COUNT>;

public:
    Session(SessionVars& vars, IPeer* pPeer, core::MemoryArenaBase* arena);

    void update(void) X_FINAL;

    void connect(SystemAddress address);

    void finishedLoading(void) X_FINAL; // tell the session we finished loading the map.
    bool hasFinishedLoading(void) const X_FINAL;

    void quitToMenu(void) X_FINAL;
    void createPartyLobby(const MatchParameters& parms) X_FINAL;
    void createMatch(const MatchParameters& parms) X_FINAL;
    void startMatch(void) X_FINAL;

    // if we are a peer, we send user cmds.
    void sendUserCmd(const UserCmd& snap) X_FINAL;
    // if we are a host and have peers we send snaps.
    void sendSnapShot(SnapShot&& snap) X_FINAL;

    const SnapShot* getSnapShot(void) X_FINAL;
    const ILobby* getLobby(LobbyType::Enum type) const X_FINAL;

    bool handleState(void);

    void sendChatMsg(const char* pMsg);

    X_INLINE IPeer* getPeer(void) const;

    X_INLINE SessionState::Enum getState(void) const;
    
    bool isHost(void) const X_FINAL;
    SessionStatus::Enum getStatus(void) const X_FINAL;
    const MatchParameters& getMatchParams(void) const X_FINAL;

    void drawDebug(engine::IPrimativeContext* pPrim) const X_FINAL;

private:
    void onLostConnectionToHost(void) X_FINAL;
    void onReciveSnapShot(SnapShot&& snap) X_FINAL;


private:
    void onReciveSnapShot(Packet* pPacket);
    void sendPacketToLobby(Packet* pPacket);

private:
    void setState(SessionState::Enum state);

private:

    bool stateIdle(void);
    bool stateCreateAndMoveToPartyLobby(void);
    bool stateCreateAndMoveToGameLobby(void);
    bool statePartyLobbyHost(void);
    bool statePartyLobbyPeer(void);
    bool stateGameLobbyHost(void);
    bool stateGameLobbyPeer(void);
    bool stateConnectAndMoveToParty(void);
    bool stateConnectAndMoveToGame(void);
    bool stateLoading(void);
    bool stateInGame(void);

    bool hasLobbyCreateCompleted(Lobby& lobby);
    bool handleConnectAndMoveToLobby(Lobby& lobby);
    void handleConnectionFailed(Lobby& lobby);

    void startLoading(void);

    bool readPackets(void);


private:
    SessionVars& vars_;
    IPeer* pPeer_;
    core::MemoryArenaBase* arena_;

    SessionState::Enum state_;

    LobbyArr lobbys_;

    core::FixedRingBuffer<SnapShot, 8> recivedSnaps_;
};


X_NAMESPACE_END

#include "Session.inl"