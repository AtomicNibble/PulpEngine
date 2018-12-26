#pragma once


#include <Containers\Array.h>
#include <Containers\FixedFifo.h>

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

class UserCmdMan;
class SnapShot;

struct MatchParameters;

class SessionVars;

class Session : public ISession, ISessionCallbacks
{
    X_NO_COPY_MOVE_ALL(Session);

    using LobbyArr = std::array<Lobby,LobbyType::ENUM_COUNT>;


    struct PendingPeer
    {
        PendingPeer(net::SystemHandle sysHandle, net::NetGUID guid, core::TimeVal connectTime);

        net::SystemHandle sysHandle;
        net::NetGUID guid;

        core::TimeVal connectTime;
    };

    struct PendingConnection
    {
        PendingConnection(LobbyType::Enum type, net::SystemAddress address);

        LobbyType::Enum type;
        net::SystemAddress address;
    };

    struct ConnectedPeer
    {
        ConnectedPeer(net::SystemHandle sysHandle, net::NetGUID guid, LobbyFlags initalLobbys);
        ConnectedPeer(net::SystemHandle sysHandle, net::NetGUID guid, LobbyType::Enum initialLobbyType);

        static LobbyFlag::Enum typeToFlag(LobbyType::Enum type);

        net::SystemHandle sysHandle;
        net::NetGUID guid;

        LobbyFlags flags;
    };
    
    struct SnapTime
    {
        SnapTime() :
            currentMS(0),
            previousMS(0),
            deltaMS(0)
        {
        }

        void rotate(int32_t time) {
            previousMS = currentMS;
            currentMS = time;
            deltaMS = currentMS - previousMS;
        }

        int32_t currentMS;
        int32_t previousMS;
        int32_t deltaMS; // store for easy debug.
    };

    struct SnapBufferTimeInfo
    {
        SnapBufferTimeInfo(int32_t timeLeftMS, int32_t totalTimeMS, int32_t totalRecvTimeMS) :
            timeLeftMS(timeLeftMS),
            totalTimeMS(totalTimeMS),
            totalRecvTimeMS(totalRecvTimeMS)
        {}

        int32_t timeLeftMS;
        int32_t totalTimeMS;
        int32_t totalRecvTimeMS;
    };

    using PendingPeerArr = core::Array<PendingPeer>;
    using PendingConnectionArr = core::Array<PendingConnection>;
    using ConnectedPeerArr = core::Array<ConnectedPeer>;
    using SnapShotRingBuffer = core::FixedFifo<SnapShot, MAX_RECEIVE_SNAPSHOT_BUFFER_SIZE>;

public:
    Session(SessionVars& vars, IPeer* pPeer, IGameCallbacks* pGameCallbacks, core::MemoryArenaBase* arena);

    void update(void) X_FINAL;
    void handleSnapShots(core::FrameTimeData& timeInfo) X_FINAL;
    SnapBufferTimeInfo calculateSnapShotBufferTime(void) const;
    void processSnapShot(void);

    void connect(SystemAddress address) X_FINAL;
    void disconnect(void) X_FINAL; // basically quitToMenu()

    SessionStatus::Enum getBackStatus(void) const;
    void cancel(void) X_FINAL;

    void finishedLoading(void) X_FINAL; // tell the session we finished loading the map.
    bool hasFinishedLoading(void) const X_FINAL;

    void quitToMenu(void) X_FINAL;
    void quitMatch(void) X_FINAL;        // gracefull quit of match, will tell peers we left etc.
    void createPartyLobby(const MatchParameters& parms) X_FINAL;
    void createMatch(const MatchParameters& parms) X_FINAL;
    void startMatch(void) X_FINAL;

    // if we are a peer, we send user cmds.
    void sendUserCmd(const UserCmdMan& userCmdMan, int32_t localIdx, core::FrameTimeData& timeInfo) X_FINAL;
    // if we are a host and have peers we send snaps.
    bool shouldSendSnapShot(core::FrameTimeData& timeInfo) X_FINAL;
    void sendSnapShot(const SnapShot& snap) X_FINAL;

    ILobby* getLobby(LobbyType::Enum type) X_FINAL;

    bool handleState(void);

    X_INLINE IPeer* getPeer(void) const;

    X_INLINE SessionState::Enum getState(void) const;
    
    bool isHost(void) const X_FINAL;
    SessionStatus::Enum getStatus(void) const X_FINAL;
    const MatchParameters& getMatchParams(void) const X_FINAL;

    void drawDebug(engine::IPrimativeContext* pPrim) const X_FINAL;

private:
    ConnectionAttemptResult::Enum connectToPeer(LobbyType::Enum type, SystemAddress sa) X_FINAL;
    void closeConnection(LobbyType::Enum type, SystemHandle systemHandle) X_FINAL;
    void onLostConnectionToHost(LobbyType::Enum type) X_FINAL;
    void onReciveSnapShot(SnapShot&& snap) X_FINAL;
    void connectAndMoveToLobby(LobbyType::Enum type, SystemAddress sa) X_FINAL;
    void peerJoinedLobby(LobbyType::Enum type, SystemHandle handle) X_FINAL;
    void leaveGameLobby(void) X_FINAL;
    void endGame(bool early) X_FINAL;

    void processPendingPeers(void);

private:
    void handleTransportConnectionTermPacket(Packet* pPacket);
    void handleTransportConnectionResponse(Packet* pPacket);
    void handleTransportConnectionHandShake(Packet* pPacket);
    void sendPacketToLobbyIfGame(Packet* pPacket);
    void sendPacketToDesiredLobby(Packet* pPacket);



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
    IGameCallbacks* pGameCallbacks_;
    core::MemoryArenaBase* arena_;

    SessionState::Enum state_;

    LobbyArr lobbys_;
    PendingPeerArr pendingJoins_;
    PendingConnectionArr pendingConnections_;
    ConnectedPeerArr peers_;

    core::TimeVal nextUserCmdSendTime_;
    core::TimeVal nextSnapshotSendTime_;

    // Snapshot - reciving
    int32_t numSnapsReceived_;
    SnapShotRingBuffer recivedSnaps_;

    SnapTime snapTime_;
    SnapTime snapRecvTime_;
    int32_t snapInterpolationTimeMS_;   // TODO: better name? this is basically snap interpolation fraction in MS.
    float snapInterpolationResidual_;
};


X_NAMESPACE_END

#include "Session.inl"