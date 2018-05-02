#pragma once


#include <Containers\Array.h>
#include <Time\TimeVal.h>

struct UserCmd;
class SnapShot;

X_NAMESPACE_BEGIN(net)

/*

I want this to be like a game session.
The reason it's in network lib, is no matter what game you make you should use this session.
and single player works like a loopback server.

So this session is for handling current game state like in menu / in game / loading etc..
You submit useCmd's to the session and get snapshots back will all the ent info.

The session has no idea what's in a snap shot, it's just a blob of data, it's unto the gameDll to interperate the data.


*/


// Status is merged state.
X_DECLARE_ENUM(SessionStatus)(
    Idle,
    PartyLobby,
    GameLobby,
    Connecting,
    Loading,
    InGame
);

X_DECLARE_ENUM(SessionState)(
    Idle,
    CreateAndMoveToPartyLobby,
    CreateAndMoveToGameLobby,
    PartyLobbyHost,
    PartyLobbyPeer, // pleb
    GameLobbyHost,
    GameLobbyPeer, // pleb
    Connecting,
    Loading,
    InGame
);

X_DECLARE_ENUM(LobbyType)(
    Party,
    Game
);

X_DECLARE_ENUM(LobbyState)(
    Idle,
    Creating
);

X_DECLARE_FLAGS(MatchFlag)(
    Online    
);

typedef Flags<MatchFlag> MatchFlags;

struct MatchParameters
{
    int32_t numSlots;
    MatchFlags flags;

    core::string mapName;
};


struct LobbyUser
{
    NetGUID guid;
    SystemAddress address;

};

struct LobbyPeer
{
    bool loaded;
    bool inGame;

    core::TimeVal lastSnap;

    float snapHz;

    SystemAddress address;
};

class Lobby
{
    typedef core::Array<LobbyUser> LobbyUserArr;
    typedef core::Array<LobbyPeer> LobbyPeerArr;

public:
    Lobby(IPeer* pPeer, LobbyType::Enum type, core::MemoryArenaBase* arena);

    // if we are a peer, we send user cmds.
    void sendUserCmd(const UserCmd& snap);
    // if we are a host and have peers we send snaps.
    void sendSnapShot(const SnapShot& snap);

    bool handleState(void);

    void startHosting(const MatchParameters& parms);
  //  void sendMembersToLobby(Lobby& destLobby);
    void finishedLoading(void);

    void shutdown(void);

    X_INLINE LobbyState::Enum getState(void) const;
    X_INLINE bool isHost(void) const;
    X_INLINE bool isPeer(void) const;
    X_INLINE bool hasFinishedLoading(void) const;
    X_INLINE MatchFlags getMatchFlags(void) const;

    X_INLINE int32_t numUsers(void) const;
    X_INLINE int32_t numFreeSlots(void) const;
    X_INLINE bool isFull(void) const;


private:
    void setState(LobbyState::Enum state);

private:
    bool stateIdle(void);
    bool stateCreating(void);

private:
    void initStateLobbyHost(void);
    void clearUsers(void);

private:
    IPeer* pPeer_;
    LobbyType::Enum type_;
    LobbyState::Enum state_;

    MatchParameters params_;

    bool isHost_;
    bool finishedLoading_; // loaded the map yet slut?
    SystemAddress hostAddress_;

    LobbyUserArr users_;
    LobbyPeerArr peers_;
};

class Session : public ISession
{
public:
    Session(IPeer* pPeer, core::MemoryArenaBase* arena);


    void finishedLoading(void); // tell the session we finished loading the map.

    void quitToMenu(void);
    void createPartyLobby(const MatchParameters& parms);
    void createMatch(const MatchParameters& parms);
    void startMatch(void);


    void runUpdate(void) X_FINAL;

    // if we are a peer, we send user cmds.
    void sendUserCmd(const UserCmd& snap);
    // if we are a host and have peers we send snaps.
    void sendSnapShot(const SnapShot& snap);

    bool handleState(void);

    void sendChatMsg(const char* pMsg);

    X_INLINE IPeer* getPeer(void) const;

    X_INLINE SessionState::Enum getState(void) const;
    SessionStatus::Enum getStatus(void) const;

private:
    void setState(SessionState::Enum state);

private:

    bool stateIdle(void);
    bool stateConnecting(void);
    bool stateCreateAndMoveToPartyLobby(void);
    bool stateCreateAndMoveToGameLobby(void);
    bool statePartyLobbyHost(void);
    bool statePartyLobbyPeer(void);
    bool stateGameLobbyHost(void);
    bool stateGameLobbyPeer(void);
    bool stateLoading(void);
    bool stateInGame(void);

    bool hasLobbyCreateCompleted(Lobby& lobby);

    void startLoading(void);

    bool readPackets(void);


private:
    IPeer* pPeer_;
    core::MemoryArenaBase* arena_;

    SessionState::Enum state_;

    SystemHandle pleb_;


    Lobby partyLobby_;
    Lobby gameLobby_;
};


X_NAMESPACE_END

#include "Session.inl"