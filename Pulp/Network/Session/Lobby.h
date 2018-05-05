#pragma once

#include <Containers\Array.h>
#include <Time\TimeVal.h>

X_NAMESPACE_BEGIN(net)

struct ISessionCallbacks;

struct UserCmd;
class SnapShot;

X_DECLARE_ENUM(LobbyType)(
    Party,
    Game
);

X_DECLARE_ENUM(LobbyState)(
    Idle,
    Creating,
    Connecting,
    Error
);

struct LobbyUser
{
    NetGUID guid;
    SystemAddress address;

};

struct LobbyPeer
{
    X_DECLARE_ENUM8(ConnectionState)(
        Pending,
        Established    
    );

    LobbyPeer() {
        connectionState = ConnectionState::Pending;

        loaded = false;
        inGame = false;
        snapHz = 0.f;
        systemHandle = INVALID_SYSTEM_HANDLE;
    }

    bool loaded;
    bool inGame;
    ConnectionState::Enum connectionState;

    core::TimeVal lastSnap;

    float snapHz;

    SystemHandle systemHandle;
    SystemAddress systemAddr;
    NetGUID guid;
};

class Lobby
{
    typedef core::Array<LobbyUser> LobbyUserArr;
    typedef core::Array<LobbyPeer> LobbyPeerArr;

public:
    Lobby(ISessionCallbacks* pCallbacks, IPeer* pPeer, LobbyType::Enum type, core::MemoryArenaBase* arena);

    void connectTo(SystemAddress address);

    bool handlePacket(Packet* pPacket);

    // if we are a peer, we send user cmds.
    void sendUserCmd(const UserCmd& snap);
    // if we are a host and have peers we send snaps.
    void sendSnapShot(const SnapShot& snap);

    void sendToHost(MessageID::Enum id);
    void sendToHost(const uint8_t* pData, size_t lengthInBytes);

    void sendToPeers(MessageID::Enum id);
    void sendToPeers(const uint8_t* pData, size_t lengthInBytes);

    bool handleState(void);

    void startHosting(const MatchParameters& params);
    //  void sendMembersToLobby(Lobby& destLobby);
    void finishedLoading(void);

    void shutdown(void);


    bool allPeersLoaded(void) const;
    X_INLINE LobbyState::Enum getState(void) const;
    X_INLINE LobbyType::Enum getType(void) const;
    X_INLINE bool isHost(void) const;
    X_INLINE bool isPeer(void) const;
    X_INLINE bool hasFinishedLoading(void) const;
    X_INLINE MatchFlags getMatchFlags(void) const;
    X_INLINE const MatchParameters& getMatchParams(void) const;

    X_INLINE int32_t numUsers(void) const;
    X_INLINE int32_t numFreeSlots(void) const;
    X_INLINE bool isFull(void) const;

private:
    const LobbyPeer* findPeer(SystemHandle handle) const;
    LobbyPeer& addPeer(SystemHandle handle, NetGUID guid);

private:
    void setState(LobbyState::Enum state);

    void handleConnectionAccepted(SystemHandle handle);
    void handleConnectionHandShake(Packet* pPacket);
    void handleConnectionAttemptFailed(MessageID::Enum id);
    void handleConnectionLost(Packet* pPacket);


private:
    bool stateIdle(void);
    bool stateCreating(void);
    bool stateConnecting(void);

private:
    void initStateLobbyHost(void);
    void clearUsers(void);

private:
    ISessionCallbacks* pCallbacks_;
    IPeer* pPeer_;
    const LobbyType::Enum type_;
    LobbyState::Enum state_;

    MatchParameters params_;

    bool isHost_;
    bool finishedLoading_; // loaded the map yet slut?
    SystemAddress hostAddress_;

    int32_t hostIdx_;

    LobbyUserArr users_;
    LobbyPeerArr peers_;
};


X_NAMESPACE_END

#include "Lobby.inl"