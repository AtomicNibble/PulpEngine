#pragma once

#include <Containers\Array.h>
#include <Threading\ThreadQue.h>

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingPoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\HeapArea.h>
#include <Hashing\sha1.h>
#include <Time\TimeStamp.h>
#include <Random\CryptRand.h>

#include "Sockets\Socket.h"
#include "Reliability\ReliabilityLayer.h"

X_NAMESPACE_BEGIN(net)

class NetVars;

// We hash the password with a nonce on the client so you can't simply snif it from the traffic.
// The server also creates a per connection nonce to prevent replay.
typedef core::Hash::SHA1 NonceHash;
typedef core::Hash::SHA1 PasswdHash;

X_DECLARE_ENUM(ConnectState)
(
    NoAction,
    DisconnectAsap,         // Disconnect when we have sent all pending data / and reliable data has been ack'd
    DisconnectAsapSilent,   // Disconnect with no notification.
    DisconnectOnNoAck,      // Disconnect when have no more acks to send.
    RequestedConnection,
    HandlingConnectionRequest,
    UnverifiedSender,
    Connected);

struct BufferdCommand
{
    X_DECLARE_ENUM(Cmd)
    (
        Send,
        CloseConnection);

    uint8_t* pData;

    // 4
    Cmd::Enum cmd;

    // 4
    BitSizeT numberOfBitsToSend;

    // 4
    PacketPriority::Enum priority;
    PacketReliability::Enum reliability;
    OrderingChannel::Enum orderingChannel;
    bool broadcast;

    // ?
    SystemAddressEx systemAddress;
    SystemHandle systemHandle;

    // 4
    union
    {
        SendReceipt receipt;
        bool sendDisconnectionNotification;
    };
};

struct PingAndClockDifferential
{
    PingAndClockDifferential();

    X_INLINE bool isValid(void) const;

    uint16_t pingTime;
    // this is a rough guess about how to translate a remotes systems relative time to our relative time
    // this may be positive or negative.
    core::TimeVal clockDifferential;
};

X_ALIGNED_SYMBOL(struct RemoteSystem, 64) // each remote can be updated on different thread, prevent any false sharing.
{
    static const size_t PING_HISTORY_COUNT = 3;

    typedef core::FixedArray<SystemAddressEx, MAX_INTERNAL_IDS> SystemAddArr;
    typedef std::array<PingAndClockDifferential, PING_HISTORY_COUNT> PingArr;

    X_NO_COPY(RemoteSystem);
    X_NO_ASSIGN(RemoteSystem);

public:
    RemoteSystem(NetVars& vars, core::MemoryArenaBase* arena, core::MemoryArenaBase* packetDataArena, core::MemoryArenaBase* packetPool);
    RemoteSystem(RemoteSystem&& oth) = default;

    RemoteSystem& operator=(RemoteSystem&& rhs) = default;

    void free(void);
    void closeConnection(void);

    X_NO_DISCARD bool canSend(void) const;
    ConnectionState::Enum getConnectionState(void) const;
    int32_t getAveragePing(void) const;
    int32_t getLastPing(void) const;

    SystemHandle getHandle(void) const;
    void setHandle(SystemHandle handle);

    void onConnected(const SystemAddressEx& externalSysId, const SystemAddArr& localIps,
        core::TimeVal sendPingTime, core::TimeVal sendPongTime);
    void onPong(core::TimeVal sendPingTime, core::TimeVal sendPongTime);

    X_NO_DISCARD X_INLINE bool sendReliable(const uint8_t* pData, BitSizeT numberOfBitsToSend, bool ownData, PacketPriority::Enum priority,
        PacketReliability::Enum reliability, OrderingChannel::Enum orderingChannel, core::TimeVal currentTime, SendReceipt receipt = INVALID_SEND_RECEIPT);
    X_NO_DISCARD X_INLINE bool sendReliable(const core::FixedBitStreamBase& bs, PacketPriority::Enum priority,
        PacketReliability::Enum reliability, OrderingChannel::Enum orderingChannel, core::TimeVal currentTime, SendReceipt receipt = INVALID_SEND_RECEIPT);

private:
    X_INLINE void onSend(PacketReliability::Enum reliability, core::TimeVal sendTime);

public:
    bool isActive;
    bool weStartedConnection;
    SystemHandle systemHandle;

    SystemAddressEx systemAddress;           // add remote system
    SystemAddressEx myExternalSystemAddress; // my add from the point of view of remote system
    SystemAddArr theirInternalSystemAddress; // copy of the peers internal local sys add.

    core::TimeVal nextPingTime;
    core::TimeVal lastReliableSend;
    core::TimeVal connectionTime;

    PingArr pings;
    uint32_t lastPingIdx;

    NetGUID guid;

    uint16_t lowestPing;
    uint16_t MTUSize;
    ConnectState::Enum connectState;

    NetSocket* pNetSocket;

    ReliabilityLayer relLayer;

    NonceHash::Digest nonce;
};

struct RequestConnection
{
    SystemAddressEx systemAddress;

    core::TimeVal nextRequestTime;
    core::TimeVal timeoutTime; // TODO: this is not even used currently.
    core::TimeVal retryDelay;

    uint8_t numRequestsMade;
    uint8_t retryCount;
    uint8_t socketIdx;
    uint8_t MTUIdxShift;

    PasswordStr password;
};

struct Ban
{
    SystemAddressEx sysAdd;
    core::TimeVal timeOut; // 0 = never.
};

// just to keep track of it's size for memory bandwidth considerations
// X_ENSURE_SIZE(BufferdCommand, 40)
// X_ENSURE_SIZE(RemoteSystem, 520 + sizeof(ReliabilityLayer))
// X_ENSURE_SIZE(RequestConnection, 72)

struct RemoteSystemLookup
{
    SystemAddress systemAddress;
    NetGUID guid;

    RemoteSystem* pRemoteSystem;
};

class XPeer : public IPeer
{
    typedef core::FixedArray<SystemAddressEx, MAX_INTERNAL_IDS> SystemAddArr;
    typedef core::Array<NetSocket> SocketsArr;
    typedef core::Array<RemoteSystem, core::ArrayAlignedAllocator<RemoteSystem>> RemoteSystemArr;
    typedef core::Array<RemoteSystem*> RemoteSystemPtrArr;
    typedef core::Array<RemoteSystemLookup> RemoteSystemLookupArr;

    // thead que's
    typedef core::ThreadQue<BufferdCommand*, core::CriticalSection> BufferdCommandQue;
    typedef core::ThreadQue<Packet*, core::CriticalSection> PacketQue;
    typedef core::ThreadQue<RecvData*, core::CriticalSection> RecvDataQue;

    typedef core::Array<RequestConnection*> RequestConnectionArr;
    typedef core::Array<Ban> BanArr;
    typedef core::Array<core::ThreadMember<XPeer>> ThreadArr;

    // a bit stream that don't own the memory.
    // we just read directly off the received buffer.
    typedef core::FixedBitStreamBase RecvBitStream;
    typedef core::FixedBitStreamBase UpdateBitStream;
    typedef core::FixedBitStreamStack<MAX_MTU_SIZE> BitStreamMTU;

    typedef core::MemoryArena<
        core::GrowingPoolAllocator,
        core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // X_ENABLE_MEMORY_SIMPLE_TRACKING
        >
        PoolArena;

    typedef core::MemoryArena<
        core::GrowingBlockAllocator,
        core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // X_ENABLE_MEMORY_SIMPLE_TRACKING
        >
        BlockAlocArena;

    static const std::array<uint32_t, 3> MTUSizesArr;

public:
    XPeer(NetVars& vars, const SystemAddArr& localAddress, core::MemoryArenaBase* arena);
    ~XPeer() X_FINAL;

    // IPeer

    X_NO_DISCARD StartupResult::Enum init(int32_t maxConnections, core::span<const SocketDescriptor> socketDescriptors) X_FINAL;
    void shutdown(core::TimeVal blockDuration, OrderingChannel::Enum orderingChannel,
        PacketPriority::Enum disconnectionNotificationPriority) X_FINAL;

    void runUpdate(void) X_FINAL;

    void setPassword(const PasswordStr& pass) X_FINAL;

    // connection api
    X_NO_DISCARD ConnectionAttemptResult::Enum connect(const HostStr& host, Port remotePort, const PasswordStr& password, uint32_t retryCount = 12,
        core::TimeVal retryDelay = core::TimeVal(0.5f), core::TimeVal timeoutTime = core::TimeVal()) X_FINAL;
    X_NO_DISCARD ConnectionAttemptResult::Enum connect(const IPStr& ip, Port remotePort, const PasswordStr& password, uint32_t retryCount = 12,
        core::TimeVal retryDelay = core::TimeVal(0.5f), core::TimeVal timeoutTime = core::TimeVal()) X_FINAL;
    X_NO_DISCARD ConnectionAttemptResult::Enum connect(const SystemAddress& systemAddress, const PasswordStr& password, uint32_t retryCount = 12,
        core::TimeVal retryDelay = core::TimeVal(0.5f), core::TimeVal timeoutTime = core::TimeVal()) X_FINAL;

    void closeConnection(SystemHandle systemHandle, bool sendDisconnectionNotification,
        OrderingChannel::Enum orderingChannel, PacketPriority::Enum notificationPriority) X_FINAL;

    // connection util
    X_NO_DISCARD SystemHandle getSystemHandleForAddress(const SystemAddress& systemAddress) const X_FINAL;
    X_NO_DISCARD ConnectionState::Enum getConnectionState(SystemHandle systemHandle) const X_FINAL;
    X_NO_DISCARD ConnectionState::Enum getConnectionState(const SystemAddress& systemAddress) X_FINAL;
    void cancelConnectionAttempt(const SystemAddress& target) X_FINAL;

    X_NO_DISCARD SendReceipt send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority,
        PacketReliability::Enum reliability, SystemHandle systemHandle, OrderingChannel::Enum orderingChannel,
        bool broadcast, SendReceipt forceReceiptNumber = INVALID_SEND_RECEIPT) X_FINAL;

    void sendLoopback(const uint8_t* pData, size_t lengthBytes) X_FINAL;

    X_NO_DISCARD Packet* receive(void) X_FINAL;
    void clearPackets(void) X_FINAL; // free's any packets in the receive que.

    // drain me
    void setDrainSockets(bool drainSocket) X_FINAL;

    // connection limits
    void setMaximumIncomingConnections(uint16_t numberAllowed) X_FINAL;
    X_INLINE uint16_t getMaximumIncomingConnections(void) const X_FINAL;
    uint16_t numberOfConnections(void) const X_FINAL;

    X_INLINE uint32_t getMaximunNumberOfPeers(void) const X_FINAL;

    // Ping
    void ping(SystemHandle handle) X_FINAL;
    bool ping(const HostStr& host, Port remotePort, bool onlyReplyOnAcceptingConnections,
        uint32_t connectionSocketIndex = 0) X_FINAL;

    // bans at connection level.
    void addToBanList(const IPStr& ip, core::TimeVal timeout = core::TimeVal()) X_FINAL;
    void addToBanList(const SystemAddressEx& sysAdd, core::TimeVal timeout = core::TimeVal());
    void removeFromBanList(const IPStr& ip) X_FINAL;
    bool isBanned(const IPStr& ip) X_FINAL;
    bool isBanned(const SystemAddressEx& sysAdd);
    void clearBanList(void) X_FINAL;
    void listBans(void) const;
    void listLocalAddress(void) const;

    X_NO_DISCARD bool getPingInfo(SystemHandle systemHandle, PingInfo& info) const X_FINAL;

    X_INLINE const NetGUID& getMyGUID(void) const X_FINAL;
    SystemAddress getMyBoundAddress(void) const X_FINAL;

    // MTU for a given system
    int32_t getMTUSize(SystemHandle systemHandle = INVALID_SYSTEM_HANDLE) const X_FINAL;
    SystemAddress getAddressForHandle(SystemHandle systemHandle) const X_FINAL;
    NetGUID getGuidForHandle(SystemHandle systemHandle) const X_FINAL;

    X_NO_DISCARD bool getStatistics(const NetGUID guid, NetStatistics& stats) X_FINAL;

    NetBandwidthStatistics getBandwidthStatistics(void) const X_FINAL;

    // ~IPeer

    X_INLINE void setUnreliableTimeout(core::TimeVal timeout);

    X_INLINE bool accpetingIncomingConnections(void) const;
    // the number of remote connections to us.
    // excludes connections made by us.
    size_t getNumRemoteInitiatedConnections(void) const;

    void listRemoteSystems(bool verbose) const;

private:
    void sendBuffered(const uint8_t* pData, BitSizeT numberOfBitsToSend, PacketPriority::Enum priority,
        PacketReliability::Enum reliability, OrderingChannel::Enum orderingChannel, SystemHandle systemHandle, bool broadcast, SendReceipt receipt);

    X_INLINE void sendBuffered(const core::FixedBitStreamBase& bs, PacketPriority::Enum priority,
        PacketReliability::Enum reliability, OrderingChannel::Enum orderingChannel, SystemHandle systemHandle, bool broadcast, SendReceipt receipt);

    void sendPing(RemoteSystem& rs, PacketReliability::Enum rel);
    void notifyAndFlagForShutdown(RemoteSystem& rs, OrderingChannel::Enum orderingChannel, PacketPriority::Enum notificationPriority);

    X_INLINE static bool isLoopbackHandle(SystemHandle systemHandle);

    // Remote Sys
    const RemoteSystem* getRemoteSystem(SystemHandle handle, bool onlyActive) const;
    const RemoteSystem* getRemoteSystem(const SystemAddressEx& systemAddress, bool onlyActive) const;
    const RemoteSystem* getRemoteSystem(const NetGUID guid, bool onlyActive) const;
    RemoteSystem* getRemoteSystem(SystemHandle handle, bool onlyActive);
    RemoteSystem* getRemoteSystem(const SystemAddressEx& systemAddress, bool onlyActive);
    RemoteSystem* getRemoteSystem(const NetGUID guid, bool onlyActive);

    // adds packet to back of receive qeue
    void pushBackPacket(const RemoteSystem& rs, ReliabilityLayer::PacketData& data);
    X_INLINE void pushBackPacket(Packet* pPacket);

    // helpers that alloc packet and push it.
    X_INLINE void pushPacket(MessageID::Enum msgId, const SystemAddressEx& sysAdd);
    X_INLINE void pushPacket(MessageID::Enum msgId, const SystemAddressEx& sysAdd, const NetGUID& guid);
    X_INLINE void pushPacket(MessageID::Enum msgId, const RemoteSystem& rs);

    X_NO_DISCARD Packet* allocPacket(size_t lengthBits);
    void freePacket(Packet* pPacket) X_FINAL;

    X_NO_DISCARD BufferdCommand* allocBufferedCmd(BufferdCommand::Cmd::Enum type, size_t lengthBits);
    void freeBufferedCmd(BufferdCommand* pBufCmd);

    X_NO_DISCARD uint8_t* allocPacketData(size_t lengthBytes);
    void freePacketData(uint8_t* pPacketData);

    X_NO_DISCARD RecvData* allocRecvData(void);
    void freeRecvData(RecvData* pRecvData);

    X_NO_DISCARD RequestConnection* allocConnectionRequest(void);
    void freeConnectionRequest(RequestConnection* pConReq);

    X_INLINE SendReceipt nextSendReceipt(void);
    X_INLINE SendReceipt incrementNextSendReceipt(void);

    void removeConnectionRequest(const SystemAddressEx& sysAdd);

private:
    void Job_remoteReliabilityTick(RemoteSystem** pRemoteSystems, uint32_t count);

private:
    void processRecvData(UpdateBitStream& updateBS, core::TimeVal timeNow);
    void processConnectionRequests(UpdateBitStream& updateBS, core::TimeVal timeNow);
    void processBufferedCommands(UpdateBitStream& updateBS, core::TimeVal timeNow);
    void remoteReliabilityTick(UpdateBitStream& updateBS, core::TimeVal timeNow);
    void remoteReliabilityTick(RemoteSystem& rs, UpdateBitStream& updateBS, core::TimeVal timeNow);

    void processRecvData(UpdateBitStream& updateBS, RecvData* pData, int32_t byteOffset);
    void processOfflineMsg(UpdateBitStream& updateBS, RecvData* pData, uint8_t* pBegin, uint8_t* pEnd);

    // some msg handlers.
    void handleConnectionFailure(UpdateBitStream& bsBuf, RecvData* pData, RecvBitStream& bs, MessageID::Enum failureType);
    // connection open msg's
    void handleOpenConnectionRequest(UpdateBitStream& bsBuf, RecvData* pData, RecvBitStream& bs);
    void handleOpenConnectionResponse(UpdateBitStream& bsBuf, RecvData* pData, RecvBitStream& bs);
    void handleOpenConnectionRequestStage2(UpdateBitStream& bsBuf, RecvData* pData, RecvBitStream& bs);
    void handleOpenConnectionResponseStage2(UpdateBitStream& bsBuf, RecvData* pData, RecvBitStream& bs);
    // ------
    void handleUnConnectedPing(UpdateBitStream& bsBuf, RecvData* pData, RecvBitStream& bs, bool openConnectionsRequired);
    void handleUnConnectedPong(UpdateBitStream& bsBuf, RecvData* pData, RecvBitStream& bs);

    // conencted handlers
    void handleConnectionRequest(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs, core::TimeVal timeNow);
    void handleConnectionRequestAccepted(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs, core::TimeVal timeNow);
    void handleConnectionRequestHandShake(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs);

    void handleConnectedPing(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs, core::TimeVal timeNow);
    void handleConnectedPong(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs);

    void handleDisconnectNotification(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs);
    void handleInvalidPassword(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs);

    // ------

    X_NO_DISCARD RemoteSystem* addRemoteSystem(const SystemAddressEx& sysAdd, NetGUID guid, int32_t remoteMTU,
        NetSocket* pSrcSocket, const SystemAddressEx& bindingAdd, ConnectState::Enum state);
    void disconnectRemote(RemoteSystem& rs);
    bool isIpConnectSpamming(const SystemAddressEx& sysAdd, core::TimeVal* pDeltaOut = nullptr);

    // ------

    X_INLINE void onSocketRecv(RecvData* pData);
    X_NO_DISCARD core::Thread::ReturnValue socketRecvThreadProc(const core::Thread& thread);

    // ------

    X_NO_DISCARD NonceHash::Digest generateNonce(void);

private:
    NetVars& vars_;
    core::V2::JobSystem* pJobSys_;

    NetGUID guid_;

    core::TimeVal unreliableTimeOut_;
    int32_t defaultMTU_;
    int32_t maxIncomingConnections_;
    int32_t maxPeers_;
    bool drainSockets_;
    core::AtomicInt sendReceiptSerial_;

    SystemAddArr ipList_;
    SocketsArr sockets_;
    ThreadArr socketThreads_;

    // ques.
    BufferdCommandQue bufferedCmds_;
    PacketQue packetQue_;
    RecvDataQue recvDataQue_;

    core::CriticalSection connectionReqsCS_;
    RequestConnectionArr connectionReqs_;

    BanArr bans_;

    // remote systems
    RemoteSystemArr remoteSystems_;
    RemoteSystemPtrArr activeRemoteSystems_;
    RemoteSystemLookupArr remoteSystemLookup_;

    // allocators
    core::MemoryArenaBase* arena_; // gen purpose.

    PoolArena::AllocationPolicy poolAllocator_;
    PoolArena poolArena_;

    PoolArena::AllocationPolicy pool2Allocator_;
    PoolArena pool2Arena_;

    BlockAlocArena::AllocationPolicy blockAlloc_;
    BlockAlocArena blockArena_;

    PasswordStr password_; // 12345

    core::random::CryptRand cryptRnd_;
};

X_NAMESPACE_END

#include "XPeer.inl"
