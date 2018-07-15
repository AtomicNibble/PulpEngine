#pragma once

#include <Containers\Array.h>
#include <Containers\PriorityQueue.h>
#include <Containers\Fifo.h>

#include <Time\TimeVal.h>
#include <Util\UniquePointer.h>

#include "SnapshotManager.h"

X_NAMESPACE_BEGIN(net)

struct ISessionCallbacks;

struct UserCmd;
class SnapShot;

class SessionVars;
class SnapshotManager;


X_DECLARE_ENUM(LobbyState)(
    Idle,
    Creating,
    Connecting,
    Joining,
    Error
);

// So we have 'users' and 'peers'
// everyone should have a copy of the user list, it's basically the meta for each player.
// you only have a peer for a user if you need to talk to it.
// so a server will have peers for all users.
// while everyone else will just have peer for host.


struct LobbyUser
{
    LobbyUser();

    X_INLINE bool hasPeer(void) const;

    void writeToBitStream(core::FixedBitStreamBase& bs) const;
    void fromBitStream(core::FixedBitStreamBase& bs);

public:
    NetGUID guid;
    SystemAddress address;
    int32_t peerIdx;

    core::StackString<MAX_USERNAME_LEN> username;
};

struct LobbyPeer
{
    X_DECLARE_ENUM8(ConnectionState)(
        Free,
        Pending,
        Established    
    );
    
    LobbyPeer();

    void reset(void);
    bool isConnected(void) const;
    ConnectionState::Enum getConnectionState(void) const;
    void setConnectionState(ConnectionState::Enum state);

public:
    bool loaded;
    bool inGame;
    bool pauseSnapShots; // stop sending snaps :D
private:
    ConnectionState::Enum connectionState;
public:
    core::TimeVal lastSnap;
    core::TimeVal stateChangeTime;
    float snapHz;
    int32_t numSnapsSent;
    core::UniquePointer<SnapshotManager> pSnapMan;

    SystemHandle systemHandle;
    SystemAddress systemAddr;
    NetGUID guid;
};

class Lobby : public ILobby
{
    typedef core::Array<LobbyUser> LobbyUserArr;
    typedef core::Array<LobbyPeer> LobbyPeerArr;
    typedef core::PriorityQueue<ChatMsg, core::ArrayGrowMultiply<ChatMsg>, core::greater<ChatMsg>> ChatMsgPriortyQueue;
    typedef core::Fifo<ChatMsg> ChatMsgFifo;

    typedef core::FixedBitStreamStack<0x8> MsgIdBs;
    typedef core::FixedBitStreamStack<0x400> UserInfoBs;
    typedef core::FixedBitStreamStack<0x20 + (sizeof(NetGUID) * MAX_PLAYERS)> NetGUIDBs;
    typedef core::FixedBitStreamStack<MAX_CHAT_MSG_LENGTH + sizeof(ChatMsg) + 0x20> ChatMsgBs;

    using NetGUIDArr = core::FixedArray<NetGUID, MAX_PLAYERS>;

    // TODO: use..
    // static const OrderingChannelIdx CHAT_CHANNEL = 1;

    X_NO_COPY_MOVE_ALL(Lobby);

public:
    Lobby(SessionVars& vars, ISessionCallbacks* pCallbacks, IPeer* pPeer, IGameCallbacks* pGameCallbacks, 
        LobbyType::Enum type, core::MemoryArenaBase* arena);

    void reset(void);

    bool handleState(void);
    bool handlePacket(Packet* pPacket);

    void connectTo(SystemAddress address);
    void startHosting(const MatchParameters& params);
    void finishedLoading(void);

    void sendPingsToPeers(void) const;
    void sendMembersToLobby(Lobby& destLobby) const;
    void sendMembersToLobby(LobbyType::Enum type) const;
    void sendPeerToLobby(int32_t peerIdx, LobbyType::Enum type) const;

    void notifyPeersLeavingGameLobby(void);

    void sendChatMsg(core::span<const char> msg) X_FINAL;
    // if we are a peer, we send user cmds.
    void sendUserCmd(const core::FixedBitStreamBase& bs);
    // if we are a host and have peers we send snaps.
    void sendSnapShot(const SnapShot& snap);

    void sendToHost(MessageID::Enum id);
    void sendToHost(const uint8_t* pData, size_t lengthInBytes);

    void sendToPeers(MessageID::Enum id) const;
    void sendToPeers(const uint8_t* pData, size_t lengthInBytes) const;

    void sendToAll(const uint8_t* pData, size_t lengthInBytes);

    void sendToPeer(int32_t peerIdx, MessageID::Enum id) const;
    void sendToPeer(int32_t peerIdx, const uint8_t* pData, size_t lengthInBytes) const;

    // Peers
    bool hasActivePeers(void) const X_FINAL;
    bool allPeersLoaded(void) const X_FINAL;
    bool allPeersInGame(void) const X_FINAL;
    int32_t getNumConnectedPeers(void) const X_FINAL;
    int32_t getNumConnectedPeersInGame(void) const X_FINAL;
    X_INLINE int32_t getHostPeerIdx(void) const X_FINAL;

    // Users
    X_INLINE int32_t getNumUsers(void) const X_FINAL;
    X_INLINE int32_t getNumFreeUserSlots(void) const X_FINAL;
    X_INLINE LobbyUserHandle getUserHandleForIdx(int32_t idx) const X_FINAL;

    const char* getUserName(LobbyUserHandle handle) const X_FINAL;
    bool getUserInfoForIdx(int32_t idx, UserInfo& info) const X_FINAL;
    bool getUserInfo(LobbyUserHandle handle, UserInfo& info) const X_FINAL;

    // Misc
    X_INLINE bool isActive(void) const X_FINAL;
    X_INLINE bool isHost(void) const X_FINAL;
    X_INLINE bool isPeer(void) const X_FINAL;
    X_INLINE LobbyType::Enum getType(void) const X_FINAL;
    X_INLINE const MatchParameters& getMatchParams(void) const X_FINAL;
    X_INLINE MatchFlags getMatchFlags(void) const;

    X_INLINE LobbyState::Enum getState(void) const;
    X_INLINE bool shouldStartLoading(void) const;
    X_INLINE void beganLoading(void);
    X_INLINE bool hasFinishedLoading(void) const;

    bool tryPopChatMsg(ChatMsg& msg) X_FINAL;

    Vec2f drawDebug(Vec2f base, engine::IPrimativeContext* pPrim) const;

private:
    void setState(LobbyState::Enum state);

    // Peers
    const LobbyPeer* findPeer(SystemHandle handle) const;
    int32_t findPeerIdx(SystemHandle handle) const;
    int32_t addPeer(SystemAddress address);
    void disconnectPeer(int32_t peerIdx);
    void setPeerConnectionState(int32_t peerIdx, LobbyPeer::ConnectionState::Enum state);
    void setPeerConnectionState(LobbyPeer& peer, LobbyPeer::ConnectionState::Enum state);

    // Users
    void addUsersFromBs(core::FixedBitStreamBase& bs, int32_t peerIdx);
    void addUsersToBs(core::FixedBitStreamBase& bs) const;
    void sendNewUsersToPeers(int32_t skipPeer, int32_t startIdx, int32_t num) const;
    void addLocalUsers(void);
    void clearUsers(void);
    size_t removeUsersWithDisconnectedPeers(void);
    void removeUsersByGuid(const NetGUIDArr& ids);

    // Chat
    void pushChatMsg(ChatMsg&& msg);
    void sendChatMsgToPeers(const ChatMsg& msg);
    void sendChatHistoryToPeer(int32_t peerIdx);

private:
    void handleSnapShot(Packet* pPacket);
    void handleUserCmd(Packet* pPacket);
    void handleConnectionAccepted(Packet* pPacket);
    void handleConnectionHandShake(Packet* pPacket);
    void handleConnectionAttemptFailed(MessageID::Enum id);
    void handleConnectionLost(Packet* pPacket);
    void handleDisconnectNotification(Packet* pPacket);
    void handleLobbyJoinRequest(Packet* pPacket);
    void handleLobbyJoinAccepted(Packet* pPacket);
    void handleLobbyUsersConnected(Packet* pPacket);
    void handleLobbyUsersDiconnected(Packet* pPacket);
    void handleLobbyGameParams(Packet* pPacket);
    void handleLobbyConnectAndMove(Packet* pPacket);
    void handleLobbyLeaveGameLobby(Packet* pPacket);
    void handleLoadingStart(Packet* pPacket);
    void handleLoadingDone(Packet* pPacket);
    void handleInGame(Packet* pPacket);
    void handleEndGame(Packet* pPacket);
    void handleLobbyChatMsg(Packet* pPacket);

private:
    void sendJoinRequestToHost(void);

private:
    bool stateIdle(void);
    bool stateCreating(void);
    bool stateConnecting(void);
    bool stateJoining(void);

private:
    void initStateLobbyHost(void);


private:
    SessionVars& vars_;
    core::MemoryArenaBase* arena_;
    ISessionCallbacks* pCallbacks_;
    IPeer* pPeer_;
    IGameCallbacks* pGameCallbacks_;
    const LobbyType::Enum type_;
    LobbyState::Enum state_;

    MatchParameters params_;

    bool isHost_;
    bool startLoading_;
    bool finishedLoading_; // loaded the map yet slut?
    SystemAddress hostAddress_;

    int32_t hostIdx_;

    LobbyUserArr users_;
    LobbyPeerArr peers_;

    ChatMsgPriortyQueue chatMsgs_;
    ChatMsgFifo chatHistory_;
};


X_NAMESPACE_END

#include "Lobby.inl"