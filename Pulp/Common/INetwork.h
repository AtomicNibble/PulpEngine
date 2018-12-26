#pragma once

#include <Time\TimeVal.h>
#include <Time\DateTimeStamp.h>
#include <Util\EndianUtil.h>
#include <Util\Span.h>

#include <NetMsgIds.h>
#include <IAssetDb.h>

X_NAMESPACE_DECLARE(engine,
    class IPrimativeContext);

X_NAMESPACE_DECLARE(core,
    struct FrameTimeData;
    class FixedBitStreamBase);

X_NAMESPACE_BEGIN(net)

#define NET_IPv6_SUPPORT 1

class UserCmdMan;
class SystemAddress;

static const uint32_t MAX_ORDERED_STREAMS = 8; // can bump this but it increases memory per connection.
static const uint32_t MAX_PASSWORD_LEN = 128;
static const uint32_t MAX_PEERS = 4; // a server only needs 1 peer.
static const uint32_t MAX_SESSION = 2;
static const uint32_t MAX_USERNAME_LEN = 64;

static const uint32_t MAX_PLAYERS = 8;

static const uint32_t MAX_SYNCED_ENTS = 1 << 8; // the max of networked ents.

static const uint32_t MAX_RESOLVE_ADDR = 6; // the max address resolve will return.


static const uint32_t MAX_CHAT_MSG_LENGTH = 256; // plz no life stories
static const uint32_t MAX_CHAT_MSGS = 16;

static const uint32_t MAX_CHAT_BUFFER_SIZE = 1024;

static const uint32_t MAX_USERCMD_SEND = 8;
static const uint32_t MAX_RECEIVE_SNAPSHOT_BUFFER_SIZE = 8; // the max recveive snapshots we can have buffered.

typedef core::FixedArray<SystemAddress, MAX_RESOLVE_ADDR> SystemAddressResolveArr;


X_DECLARE_ENUM8(ConnectionState)
(
    Pending,
    Connecting,
    Connected,
    Disconnecting,
    DisconnectingSilently,
    Disconnected,
    NotConnected);

X_DECLARE_ENUM(ConnectionAttemptResult)
(
    Started,
    InvalidParam,
    FailedToResolve,
    AlreadyConnected,
    AlreadyInProgress,
    SecurityInitalizationFailed);

X_DECLARE_ENUM(StartupResult)
(
    Started,
    InvalidSocketDescriptors,
    InvalidMaxCon,
    InvalidPort,
    SocketPortInUse,
    SocketFailedToBind,
    SocketFailedToTestSend,
    Error);

X_DECLARE_ENUM8(PacketPriority)
(
    Immediate, // send immediate, no buffering  or aggregating with other packets.
    High,
    Medium,
    Low);

// Sequenced - We ignore any packets that are older than last recived packet.
// Ordered - all packets come out the other side in order for a given channel.
X_DECLARE_ENUM8(PacketReliability)
(
    UnReliable,
    UnReliableSequenced,
    Reliable,
    ReliableOrdered,
    ReliableSequenced,
    UnReliableWithAck,
    ReliableWithAck,
    ReliableOrderedWithAck);

X_DECLARE_ENUM8(OrderingChannel)(
    Default,    
    SessionMsg,
    Channel2,
    Channel3,
    Channel4,
    Channel5,
    Channel6,
    Channel7
);

static_assert(OrderingChannel::ENUM_COUNT <= MAX_ORDERED_STREAMS, "Defined ordering channels exceeds MAX_ORDERED_STREAMS");

#if NET_IPv6_SUPPORT // can't if/def inside DECLARE_ENUM.

X_DECLARE_ENUM8(IpVersion)
(
    Ipv4,
    Ipv6,
    Any);

#else

X_DECLARE_ENUM8(IpVersion)
(
    Ipv4,
    Any // ipv4 only yo!
);

#endif // !NET_IPv6_SUPPORT

struct AddressFamily
{
    enum Enum : uint16_t
    {
        INet = 2,
#if NET_IPv6_SUPPORT
        INet6 = 23 // best be a static asset for this magic number :|
#endif // !NET_IPv6_SUPPORT
    };
};

typedef AddressFamily SocketFamily;

typedef uint16_t SystemHandle;
typedef uint16_t Port;
// typedef uint8_t MessageID;
typedef uint32_t SendReceipt;
typedef uint32_t BitSizeT;

typedef core::StackString<512, char> HostStr;
typedef core::StackString<45 + 11, char> IPStr; // 11 for port. and ipv6 is 39 / 45 for ipv4 mapped notation
typedef core::StackString<46, char> NetGuidStr;
typedef core::StackString<MAX_PASSWORD_LEN, char> PasswordStr;

static constexpr SendReceipt INVALID_SEND_RECEIPT = 0;
static constexpr SystemHandle INVALID_SYSTEM_HANDLE = std::numeric_limits<SystemHandle>::max();
static constexpr SystemHandle LOOPBACK_SYSTEM_HANDLE = std::numeric_limits<SystemHandle>::max() - 1;

// ---------------------------------

// network to host, i define this since we have no windows includes here.
template<typename T>
X_INLINE T hton(const T v)
{
    return core::Endian::swap(v);
}

template<typename T>
X_INLINE T ntoh(const T v)
{
    return core::Endian::swap(v);
}

// ---------------------------------

struct NetStatistics
{
    X_DECLARE_ENUM(Metric)
    (
        BytesPushed,
        BytesSent,
        BytesResent,
        BytesRecivedProcessed,
        BytesRecivedIgnored,
        // includes any overhead
        ActualBytesSent,
        ActualBytesReceived);

    typedef std::array<uint32_t, PacketPriority::ENUM_COUNT> PriorityMsgCountsArr;
    typedef std::array<uint64_t, PacketPriority::ENUM_COUNT> PriorityByteCountsArr;

public:
    core::TimeVal connectionStartTime;

    uint64_t internalMemUsage; // rougth memory usage for overhead. don't include packet data.
    uint64_t lastSecondMetrics[Metric::ENUM_COUNT];
    uint64_t runningMetrics[Metric::ENUM_COUNT];

    PriorityMsgCountsArr msgInSendBuffers;
    PriorityByteCountsArr bytesInSendBuffers;

    float packetLossLastSecond;
    float packetLossTotal;

    bool isLimitedByCongestionControl;
    bool isLimitedByOutgoingBadwidthLimit;
};

// global sats.
struct NetBandwidthStatistics
{
    X_INLINE NetBandwidthStatistics& operator+=(const NetBandwidthStatistics& oth)
    {
        numPacketsRecived += oth.numPacketsRecived;
        numBytesRecived += oth.numBytesRecived;
        numPacketsSent += oth.numPacketsSent;
        numBytesSent += oth.numBytesSent;
        return *this;
    }

    // leave signed.
    int64_t numPacketsRecived;
    int64_t numBytesRecived;

    int64_t numPacketsSent;
    int64_t numBytesSent;
};

// ---------------------------------

class SocketDescriptor
{
public:
    X_INLINE SocketDescriptor() :
        port_(0),
        socketFamily_(SocketFamily::INet)
    {
    }
    X_INLINE SocketDescriptor(Port port) :
        port_(port),
        socketFamily_(SocketFamily::INet)
    {
    }
    X_INLINE SocketDescriptor(Port port, HostStr hostAddress) :
        port_(port),
        socketFamily_(SocketFamily::INet),
        hostAddress_(hostAddress)
    {
    }

    X_INLINE void setPort(Port port)
    {
        port_ = port;
    }
    X_INLINE void setSocketFamiley(SocketFamily::Enum socketFamily)
    {
        socketFamily_ = socketFamily;
    }

    X_INLINE Port getPort(void) const
    {
        return port_;
    }
    X_INLINE SocketFamily::Enum getSocketFamiley(void) const
    {
        return socketFamily_;
    }
    X_INLINE const HostStr& getHostAdd(void) const
    {
        return hostAddress_;
    }

private:
    Port port_;
    SocketFamily::Enum socketFamily_;
    HostStr hostAddress_;
};

// ---------------------------------

class NetGUID
{
public:
    typedef NetGuidStr StrBuf;

public:
    X_INLINE NetGUID() :
        val_(0)
    {
    }
    X_INLINE explicit NetGUID(uint64_t d) :
        val_(d)
    {
    }

    X_INLINE bool isValid(void) const
    {
        return val_ != 0;
    }
    X_INLINE bool operator==(const NetGUID& rhs) const
    {
        return val_ == rhs.val_;
    }
    X_INLINE bool operator!=(const NetGUID& rhs) const
    {
        return val_ != rhs.val_;
    }
    X_INLINE bool operator>(const NetGUID& rhs) const
    {
        return val_ > rhs.val_;
    }
    X_INLINE bool operator<(const NetGUID& rhs) const
    {
        return val_ < rhs.val_;
    }

    X_INLINE const char* toString(StrBuf& buf) const
    {
        buf.setFmt("0x%" PRIx64, val_);
        return buf.c_str();
    }

private:
    uint64_t val_;
};

// ---------------------------------

struct Packet
{
    X_INLINE MessageID::Enum getID(void) const
    {
        X_ASSERT(length > 0, "Can't read id from empty message")(length, bitLength);
        return static_cast<MessageID::Enum>(pData[0]);
    }
    X_INLINE uint8_t* begin(void)
    {
        return pData + 1;
    }
    X_INLINE uint8_t* end(void)
    {
        return pData + core::bitUtil::bitsToBytes(bitLength);
    }

    SystemHandle systemHandle; // the handle
    NetGUID guid;

    uint32_t length;
    BitSizeT bitLength; // length in bits.
    uint8_t* pData;     // data from sender.
};

// Muh cache lanes!
X_ENSURE_SIZE(Packet, 32)

// ---------------------------------

struct PingInfo
{
    PingInfo() {
        cur = -1_i16;
        avg = -1_i16;
        min = -1_i16;
    }

    int16_t cur;
    int16_t avg;
    int16_t min;
};

X_ENSURE_SIZE(PingInfo, 6)

// ---------------------------------

struct IPeer
{
    virtual ~IPeer() = default;

    virtual StartupResult::Enum init(int32_t maxConnections, core::span<const SocketDescriptor> socketDescriptors) X_ABSTRACT;
    X_INLINE StartupResult::Enum init(int32_t maxConnections, const SocketDescriptor& socketDescriptors);

    virtual void shutdown(core::TimeVal blockDuration, OrderingChannel::Enum orderingChannel,
        PacketPriority::Enum disconnectionNotificationPriority = PacketPriority::Low) X_ABSTRACT;

    virtual void runUpdate(void) X_ABSTRACT;

    virtual void setPassword(const PasswordStr& pass) X_ABSTRACT;

    // connection api
    virtual ConnectionAttemptResult::Enum connect(const HostStr& host, Port remotePort, const PasswordStr& password = PasswordStr(), uint32_t retryCount = 12,
        core::TimeVal retryDelay = core::TimeVal(0.5f), core::TimeVal timeoutTime = core::TimeVal()) X_ABSTRACT;
    virtual ConnectionAttemptResult::Enum connect(const IPStr& ip, Port remotePort, const PasswordStr& password = PasswordStr(), uint32_t retryCount = 12,
        core::TimeVal retryDelay = core::TimeVal(0.5f), core::TimeVal timeoutTime = core::TimeVal()) X_ABSTRACT;
    virtual ConnectionAttemptResult::Enum connect(const SystemAddress& systemAddress, const PasswordStr& password = PasswordStr(), uint32_t retryCount = 12,
        core::TimeVal retryDelay = core::TimeVal(0.5f), core::TimeVal timeoutTime = core::TimeVal()) X_ABSTRACT;

    virtual void closeConnection(SystemHandle systemHandle, bool sendDisconnectionNotification,
        OrderingChannel::Enum orderingChannel, PacketPriority::Enum notificationPriority) X_ABSTRACT;

    // connection util
    virtual SystemHandle getSystemHandleForAddress(const SystemAddress& systemAddress) const X_ABSTRACT;
    virtual ConnectionState::Enum getConnectionState(SystemHandle systemHandle) const X_ABSTRACT;
    virtual ConnectionState::Enum getConnectionState(const SystemAddress& systemAddress) X_ABSTRACT;
    virtual void cancelConnectionAttempt(const SystemAddress& address) X_ABSTRACT;

    // send some data :)
    virtual SendReceipt send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority, PacketReliability::Enum reliability, SystemHandle systemHandle, OrderingChannel::Enum orderingChannel, bool broadcast, SendReceipt forceReceiptNumber) X_ABSTRACT;
    X_INLINE SendReceipt send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority, PacketReliability::Enum reliability, SystemHandle systemHandle, OrderingChannel::Enum orderingChannel);
    X_INLINE SendReceipt send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority, PacketReliability::Enum reliability, SystemHandle systemHandle, size_t orderingChannel);
    X_INLINE SendReceipt send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority, PacketReliability::Enum reliability, SystemHandle systemHandle);

    // send to self.
    virtual void sendLoopback(const uint8_t* pData, size_t lengthBytes) X_ABSTRACT;

    virtual Packet* receive(void) X_ABSTRACT;
    virtual void clearPackets(void) X_ABSTRACT; // free's any packets in the receive que.
    virtual void freePacket(Packet* pPacket) X_ABSTRACT;

    virtual void setDrainSockets(bool drainSocket) X_ABSTRACT;

    // connection limits
    virtual void setMaximumIncomingConnections(uint16_t numberAllowed) X_ABSTRACT;
    virtual uint16_t getMaximumIncomingConnections(void) const X_ABSTRACT;
    virtual uint16_t numberOfConnections(void) const X_ABSTRACT; // current number of connected peers.

    virtual uint32_t getMaximunNumberOfPeers(void) const X_ABSTRACT;

    // Ping
    virtual void ping(SystemHandle systemHandle) X_ABSTRACT;
    virtual bool ping(const HostStr& host, Port remotePort, bool onlyReplyOnAcceptingConnections,
        uint32_t connectionSocketIndex) X_ABSTRACT;

    // bans at connection level.
    virtual void addToBanList(const IPStr& ip, core::TimeVal timeout) X_ABSTRACT;
    virtual void removeFromBanList(const IPStr& ip) X_ABSTRACT;
    virtual bool isBanned(const IPStr& ip) X_ABSTRACT;
    virtual void clearBanList(void) X_ABSTRACT;

    virtual bool getPingInfo(SystemHandle systemHandle, PingInfo& info) const X_ABSTRACT;

    virtual const NetGUID& getMyGUID(void) const X_ABSTRACT;
    virtual SystemAddress getMyBoundAddress(void) const X_ABSTRACT;

    // MTU for a given system
    virtual int getMTUSize(SystemHandle systemHandle) const X_ABSTRACT;
    virtual SystemAddress getAddressForHandle(SystemHandle systemHandle) const X_ABSTRACT;
    virtual NetGUID getGuidForHandle(SystemHandle systemHandle) const X_ABSTRACT;

    virtual bool getStatistics(const NetGUID guid, NetStatistics& stats) X_ABSTRACT;

    virtual NetBandwidthStatistics getBandwidthStatistics(void) const X_ABSTRACT;
};

X_INLINE StartupResult::Enum IPeer::init(int32_t maxConnections, const SocketDescriptor& socketDescriptors)
{
    return init(maxConnections, core::make_span(&socketDescriptors, 1));
}

X_INLINE SendReceipt IPeer::send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority,
    PacketReliability::Enum reliability, SystemHandle systemHandle, OrderingChannel::Enum orderingChannel)
{
    return send(pData, lengthBytes, priority, reliability, systemHandle, orderingChannel, false, INVALID_SEND_RECEIPT);
}

X_INLINE SendReceipt IPeer::send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority,
    PacketReliability::Enum reliability, SystemHandle systemHandle, size_t orderingChannel)
{
    return send(pData, lengthBytes, priority, reliability, systemHandle, safe_static_cast<OrderingChannel::Enum>(orderingChannel), false, INVALID_SEND_RECEIPT);
}

X_INLINE SendReceipt IPeer::send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority,
    PacketReliability::Enum reliability, SystemHandle systemHandle)
{
    return send(pData, lengthBytes, priority, reliability, systemHandle, OrderingChannel::Default, false, INVALID_SEND_RECEIPT);
}

// ---------------------------------


X_DECLARE_FLAGS(MatchFlag)(
    Online,
    Private,
    JoinInviteOnly,
    JoinInProgress
);

X_DECLARE_ENUM(GameMode)(
    SinglePlayer,
    MultiPlayer,
    Cooperative
);

typedef Flags<MatchFlag> MatchFlags;

typedef core::StackString<assetDb::ASSET_NAME_MAX_LENGTH> MapNameStr;

struct MatchParameters
{
    MatchParameters() {
        numSlots = 0;
        mode = GameMode::SinglePlayer;
    }

    void writeToBitStream(core::FixedBitStreamBase& bs) const;
    void fromBitStream(core::FixedBitStreamBase& bs);

    int32_t numSlots;
    MatchFlags flags;
    GameMode::Enum mode;

    MapNameStr mapName;
};


// A session is like a server state, that uses a IPeer for connections and transport.
// It's split up like this so that IPeer can be used as like a TCP replacement for misc tasks. (also makes it easy to UnitTest)
// A session is what's used for game state and snapshots.
X_DECLARE_ENUM(SessionStatus)(
    Idle,
    Connecting,
    PartyLobby,
    GameLobby,
    Loading,
    InGame
);

static_assert(SessionStatus::GameLobby > SessionStatus::PartyLobby, "Incorrect enum order");
static_assert(SessionStatus::Loading > SessionStatus::GameLobby, "Incorrect enum order");
static_assert(SessionStatus::InGame > SessionStatus::Loading, "Incorrect enum order");
static_assert((SessionStatus::InGame + 1) == SessionStatus::ENUM_COUNT, "Incorrect enum order");


X_DECLARE_ENUM8(LobbyType)(
    Party, //  you'r not invited
    Game
);

X_DECLARE_FLAGS(LobbyFlag)(
    Party,
    Game
);

using LobbyFlags = Flags<LobbyFlag>;

class SnapShot;

typedef uintptr_t LobbyUserHandle;

struct UserInfo
{
    X_INLINE bool hasPeer(void) const {
        return peerIdx >= 0;
    }

public:
    const char* pName;
    NetGUID guid;
    // Not always valid.
    int32_t peerIdx;
    SystemHandle systemHandle;
};                                               

struct ChatMsg
{
    bool operator<(const ChatMsg& rhs) const {
        return dateTimeStamp < rhs.dateTimeStamp;
    }
    bool operator>(const ChatMsg& rhs) const {
        return dateTimeStamp > rhs.dateTimeStamp;
    }

    void writeToBitStream(core::FixedBitStreamBase& bs) const;
    void fromBitStream(core::FixedBitStreamBase& bs);

public:
    NetGUID userGuid;
    core::DateTimeStamp dateTimeStamp;
    core::string msg;
};

struct ILobby
{
    virtual ~ILobby() = default;

    // Peers
    virtual bool hasActivePeers(void) const X_ABSTRACT;
    virtual bool allPeersLoaded(void) const X_ABSTRACT;         // returns true if no peers
    virtual bool allPeersInGame(void) const X_ABSTRACT;         // returns true if no peers
    virtual int32_t getNumConnectedPeers(void) const X_ABSTRACT;
    virtual int32_t getNumConnectedPeersLoaded(void) const X_ABSTRACT;
    virtual int32_t getNumConnectedPeersInGame(void) const X_ABSTRACT;
    virtual int32_t getHostPeerIdx(void) const X_ABSTRACT;

    // Users
    virtual int32_t getNumUsers(void) const X_ABSTRACT;
    virtual int32_t getNumFreeUserSlots(void) const X_ABSTRACT;

    virtual void getUserInfoForIdx(int32_t idx, UserInfo& info) const X_ABSTRACT;

    // Misc
    virtual bool isActive(void) const X_ABSTRACT;
    virtual bool isHost(void) const X_ABSTRACT;
    virtual bool isPeer(void) const X_ABSTRACT;
    virtual LobbyType::Enum getType(void) const X_ABSTRACT;

    virtual const MatchParameters& getMatchParams(void) const X_ABSTRACT;

    // Chat
    virtual void sendChatMsg(core::span<const char> msg) X_ABSTRACT;
    virtual bool tryPopChatMsg(ChatMsg& msg) X_ABSTRACT;

};

struct IGameCallbacks
{
    virtual ~IGameCallbacks() = default;

    virtual void onUserCmdReceive(NetGUID guid, core::FixedBitStreamBase& bs) X_ABSTRACT;
    virtual void buildSnapShot(net::SnapShot& snap) X_ABSTRACT;
    virtual void applySnapShot(const net::SnapShot& snap) X_ABSTRACT;
    virtual void setInterpolation(float fraction, int32_t serverGameTimeMS, int32_t ssStartTimeMS, int32_t ssEndTimeMS) X_ABSTRACT;

};

struct ISession
{
    virtual ~ISession() = default;

    virtual void update(void) X_ABSTRACT;
    virtual void handleSnapShots(core::FrameTimeData& timeInfo) X_ABSTRACT;

    virtual void connect(SystemAddress address) X_ABSTRACT;
    virtual void disconnect(void) X_ABSTRACT; // basically quitToMenu()
    
    virtual SessionStatus::Enum getBackStatus(void) const X_ABSTRACT;
    virtual void cancel(void) X_ABSTRACT; 

    virtual void finishedLoading(void) X_ABSTRACT;
    virtual bool hasFinishedLoading(void) const X_ABSTRACT;

    virtual bool isHost(void) const X_ABSTRACT;
    virtual SessionStatus::Enum getStatus(void) const X_ABSTRACT;
    virtual const MatchParameters& getMatchParams(void) const X_ABSTRACT;
 
    virtual void quitToMenu(void) X_ABSTRACT;       // force move to menu, no telling peers etc.
    virtual void quitMatch(void) X_ABSTRACT;        // gracefull quit of match, will tell peers we left etc.
    virtual void createPartyLobby(const MatchParameters& parms) X_ABSTRACT;
    virtual void createMatch(const MatchParameters& parms) X_ABSTRACT;
    virtual void startMatch(void) X_ABSTRACT;

    virtual void sendUserCmd(const UserCmdMan& userCmdMan, int32_t localIdx, core::FrameTimeData& timeInfo) X_ABSTRACT;
    virtual void sendSnapShot(core::FrameTimeData& timeInfo) X_ABSTRACT;
    virtual void sendSnapShot(SnapShot&& snap) X_ABSTRACT; // used for testing
    
    virtual ILobby* getLobby(LobbyType::Enum type) X_ABSTRACT;

    virtual void drawDebug(engine::IPrimativeContext* pPrim) const X_ABSTRACT;
};

// ---------------------------------

struct INet : public core::IEngineSysBase
{
    virtual ~INet() = default;

    virtual bool asyncInitFinalize(void) X_ABSTRACT;

    virtual IPeer* createPeer(void) X_ABSTRACT;
    virtual void deletePeer(IPeer* pPeer) X_ABSTRACT;

    // Creates the session, with the given peer for transport.
    virtual ISession* createSession(IPeer* pPeer, IGameCallbacks* pGameCallbacks) X_ABSTRACT;
    virtual void deleteSession(ISession* pSession) X_ABSTRACT;

    // ipv4/6 address with optional trailing |port, or explicit port.
    // specify what ip version you want the returned address to be. aka asking for a ipv6 address of '127.0.0.1'
    virtual bool systemAddressFromIP(const IPStr& ip, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_ABSTRACT;
    virtual bool systemAddressFromIP(const IPStr& ip, Port port, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_ABSTRACT;

    // hostname to system address, supports: 'localhost', 'goat.com', 'dial-a-pickle.nz'
    // supports optional trailing port 'localhost|1234', or explict port using overload.
    // this can be used to pre-resolve a host before trying to connect to it, since resolving may take a while.
    virtual bool systemAddressFromHost(const HostStr& host, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_ABSTRACT;
    virtual bool systemAddressFromHost(const HostStr& host, Port port, SystemAddress& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_ABSTRACT;

    // Get all address
    virtual bool systemAddressFromHost(const HostStr& host, SystemAddressResolveArr& out, IpVersion::Enum ipVersion = IpVersion::Any) const X_ABSTRACT;

    // the two string logic is passed to platform abstract logic, so you do it via this interface.
    // since the platform lib is only loaded if you have created a instanced of something deriving this interface.
    virtual const char* systemAddressToString(const SystemAddress& systemAddress, IPStr& strBuf, bool incPort) const X_ABSTRACT;
};

X_NAMESPACE_END


X_DECLARE_ENUM_NUMERIC_LIMITS(X_NAMESPACE(net)::OrderingChannel);