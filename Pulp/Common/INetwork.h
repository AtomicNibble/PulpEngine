#pragma once

#include <Time\TimeVal.h>
#include <Time\DateStamp.h>
#include <Util\EndianUtil.h>
#include <Util\Span.h>

#include <NetMsgIds.h>

X_NAMESPACE_BEGIN(net)

#define NET_IPv6_SUPPORT 1

class SystemAddress;

static const uint32_t MAX_ORDERED_STREAMS = 16; // can bump this but it increases memory per connection.
static const uint32_t MAX_PASSWORD_LEN = 128;
static const uint32_t MAX_PEERS = 4; // a server only needs 1 peer.


static const uint32_t MAX_REPLICATION_WORLDS = 4;
static const uint32_t MAX_ENTS = 1 << 10; // the max of networked ents.
static const uint32_t MAX_ENTS_FIELDS = 128; // the max fields a ent can sync

static const uint32_t MAX_RESOLVE_ADDR = 6; // the max address resolve will return.

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
    AlreadyStarted,
    InvalidSocketDescriptors,
    InvalidMaxCon,
    InvalidPort,
    SocketFamilyNotSupported,
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
        INet6 = 23
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

    X_INLINE const char* toString(StrBuf& buf)
    {
        buf.appendFmt("0x%" PRIx64, val_);
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

struct IPeer
{
    virtual ~IPeer() = default;

    virtual StartupResult::Enum init(int32_t maxConnections, core::span<const SocketDescriptor> socketDescriptors) X_ABSTRACT;
    X_INLINE StartupResult::Enum init(int32_t maxConnections, const  SocketDescriptor& socketDescriptors);

    virtual void shutdown(core::TimeVal blockDuration, uint8_t orderingChannel = 0,
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
        uint8_t orderingChannel = 0, PacketPriority::Enum notificationPriority = PacketPriority::Low) X_ABSTRACT;

    // connection util
    virtual ConnectionState::Enum getConnectionState(SystemHandle systemHandle) X_ABSTRACT;
    virtual ConnectionState::Enum getConnectionState(const SystemAddress& systemAddress) X_ABSTRACT;
    virtual void cancelConnectionAttempt(const SystemAddress& address) X_ABSTRACT;

    // send some data :)
    virtual SendReceipt send(const uint8_t* pData, const size_t length, PacketPriority::Enum priority,
        PacketReliability::Enum reliability, SystemHandle systemHandle, uint8_t orderingChannel, bool broadcast = false, SendReceipt forceReceiptNumber = INVALID_SEND_RECEIPT) X_ABSTRACT;
    X_INLINE SendReceipt send(const uint8_t* pData, const size_t length, PacketPriority::Enum priority,
        PacketReliability::Enum reliability, SystemHandle systemHandle, size_t orderingChannel);
    X_INLINE SendReceipt send(const uint8_t* pData, const size_t length, PacketPriority::Enum priority,
        PacketReliability::Enum reliability, SystemHandle systemHandle);

    // send to self.
    virtual void sendLoopback(const uint8_t* pData, size_t lengthBytes) X_ABSTRACT;

    virtual Packet* receive(void) X_ABSTRACT;
    virtual void clearPackets(void) X_ABSTRACT; // free's any packets in the receive que.
    virtual void freePacket(Packet* pPacket) X_ABSTRACT;

    // connection limits
    virtual void setMaximumIncomingConnections(uint16_t numberAllowed) X_ABSTRACT;
    virtual uint16_t getMaximumIncomingConnections(void) const X_ABSTRACT;
    virtual uint16_t numberOfConnections(void) const X_ABSTRACT; // current number of connected peers.

    virtual uint32_t getMaximunNumberOfPeers(void) const X_ABSTRACT;

    // Ping
    virtual void ping(SystemHandle systemHandle) X_ABSTRACT;
    virtual bool ping(const HostStr& host, Port remotePort, bool onlyReplyOnAcceptingConnections,
        uint32_t connectionSocketIndex = 0) X_ABSTRACT;

    // bans at connection level.
    virtual void addToBanList(const IPStr& ip, core::TimeVal timeout = core::TimeVal()) X_ABSTRACT;
    virtual void removeFromBanList(const IPStr& ip) X_ABSTRACT;
    virtual bool isBanned(const IPStr& ip) X_ABSTRACT;
    virtual void clearBanList(void) X_ABSTRACT;

    virtual int32_t getAveragePing(SystemHandle systemHandle) const X_ABSTRACT;
    virtual int32_t getLastPing(SystemHandle systemHandle) const X_ABSTRACT;
    virtual int32_t getLowestPing(SystemHandle systemHandle) const X_ABSTRACT;

    virtual const NetGUID& getMyGUID(void) const X_ABSTRACT;

    // MTU for a given system
    virtual int getMTUSize(SystemHandle systemHandle = INVALID_SYSTEM_HANDLE) const X_ABSTRACT;

    virtual bool getStatistics(const NetGUID guid, NetStatistics& stats) X_ABSTRACT;

    virtual NetBandwidthStatistics getBandwidthStatistics(void) const X_ABSTRACT;
};

X_INLINE StartupResult::Enum IPeer::init(int32_t maxConnections, const SocketDescriptor& socketDescriptors)
{
    return init(maxConnections, core::make_span(&socketDescriptors, 1));
}

X_INLINE SendReceipt IPeer::send(const uint8_t* pData, const size_t length, PacketPriority::Enum priority,
    PacketReliability::Enum reliability, SystemHandle systemHandle, size_t orderingChannel)
{
    return send(pData, length, priority, reliability, systemHandle, safe_static_cast<uint8_t>(orderingChannel), false);
}

X_INLINE SendReceipt IPeer::send(const uint8_t* pData, const size_t length, PacketPriority::Enum priority,
    PacketReliability::Enum reliability, SystemHandle systemHandle)
{
    return send(pData, length, priority, reliability, systemHandle, 0, false);
}

// ---------------------------------

// A session is like a server state, that uses a IPeer for connections and transport.
// It's split up like this so that IPeer can be used as like a TCP replacement for misc tasks. (also makes it easy to UnitTest)
// A session is what's used for game state and snapshots.
struct ISession
{
    virtual ~ISession() = default;

    virtual void runUpdate(void) X_ABSTRACT;

};

// ---------------------------------

struct INet : public core::IEngineSysBase
{
    virtual ~INet() = default;

    virtual bool asyncInitFinalize(void) X_ABSTRACT;

    virtual IPeer* createPeer(void) X_ABSTRACT;
    virtual void deletePeer(IPeer* pPeer) X_ABSTRACT;

    // Creates the session, with the given peer for transport.
    virtual bool createSession(IPeer* pPeer) X_ABSTRACT;
    virtual ISession* getSession(void) X_ABSTRACT;

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
    virtual const char* systemAddressToString(const SystemAddress& systemAddress, IPStr& strBuf, bool incPort = true) const X_ABSTRACT;
};


X_NAMESPACE_END
