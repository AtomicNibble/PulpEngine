#pragma once

#include <Time\TimeVal.h>
#include <Time\DateStamp.h>

X_NAMESPACE_BEGIN(net)

static const uint32_t MAX_ORDERED_STREAMS = 16; // can bump this but it increases memory per connection.
static const uint32_t MAX_SUPPORTED_PEERS = 1 << 10; // go nuts.
static const uint32_t MAX_BAN_ENTRIES = 256; // max bans entries you can add, wildcards supported.


X_DECLARE_ENUM(ConnectionState)(
	Pending,
	Connecting,
	Connected,
	Disconnecting,
	DisconnectingSilently,
	Disconnected,
	NotConnected
);

X_DECLARE_ENUM(ConnectionAttemptResult)(
	Started,
	InvalidParam,
	FailedToResolve,
	AlreadyConnected,
	AlreadyInProgress,
	SecurityInitalizationFailed
);

X_DECLARE_ENUM(StartupResult)(
	Started,
	AlreadyStarted,
	InvalidSocketDescriptors,
	InvalidMaxCon,
	InvalidPort,
	SocketFamilyNotSupported,
	SocketPortInUse,
	SocketFailedToBind,
	SocketFailedToTestSend,
	Error
);


X_DECLARE_ENUM(PacketPriority)(
	Immediate, // send immediate, no buffering  or aggregating with other packets.
	High,
	Medium,
	Low
);

X_DECLARE_ENUM(PacketReliability)(
	UnReliable,
	UnReliableSequenced,
	Reliable,
	ReliableOrdered,
	ReliableSequenced,
	UnReliableWithAck,
	ReliableWithAck,
	ReliableOrderedWithAck
);

X_DECLARE_ENUM(IpVersion)(
	Ipv4,
	Ipv6
);

X_DECLARE_ENUM(SocketFamily)(
	INet,
	INet6
);


typedef uint16_t SystemIndex;
typedef uint16_t Port;
// typedef uint8_t MessageID;
typedef uint32_t BitSizeT;

typedef core::StackString<512, char> HostAddStr;
typedef core::StackString<45 + 11, char> IPStr; // 11 for port, making sizeof() 64 bytes for x64.
typedef core::StackString<46, char> NetGuidStr;

// ---------------------------------

struct NetStatistics
{
	X_DECLARE_ENUM(Metric)(
		BytesPushed,
		BytesSent,
		BytesResent,
		BytesRecivedProcessed,
		BytesRecivedIgnored,
		// includes any overhead 
		ActualBytesSent,
		ActualBytesReceived
	);

public:
	core::DateStamp connectionStartTime;

	uint64_t lastSecondMetrics[Metric::ENUM_COUNT];
	uint64_t runningMetrics[Metric::ENUM_COUNT];

	uint32_t msgInSendBuffers[PacketPriority::ENUM_COUNT];
	uint64_t bytesInSendBuffers[PacketPriority::ENUM_COUNT];

	float packetLossLastSecond;
	float packetLossTotal;
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
	X_INLINE SocketDescriptor(uint16_t port) : 
		port_(port),
		socketFamily_(SocketFamily::INet)
	{
	}
	X_INLINE SocketDescriptor(uint16_t port, HostAddStr hostAddress) : 
		port_(port), 
		socketFamily_(SocketFamily::INet),
		hostAddress_(hostAddress)
	{
	}

	X_INLINE void setSocketFamiley(SocketFamily::Enum socketFamily) {
		socketFamily_ = socketFamily;
	}


	X_INLINE uint16_t getPort(void) const {
		return port_;
	}
	X_INLINE SocketFamily::Enum getSocketFamiley(void) const {
		return socketFamily_;
	}
	X_INLINE const HostAddStr& getHostAdd(void) const {
		return hostAddress_;
	}


private:
	uint16_t port_;
	SocketFamily::Enum socketFamily_;
	HostAddStr hostAddress_;
};

// ---------------------------------

class NetGUID 
{
public:
	typedef NetGuidStr StrBuf;

public:
	X_INLINE NetGUID() : val_(0), sysIdx_(0) {}
	X_INLINE explicit NetGUID(uint64_t d) : val_(d), sysIdx_(0) {}


	X_INLINE bool operator==(const NetGUID& rhs) const
	{
		return val_ == rhs.val_;
	}
	X_INLINE bool operator!=(const NetGUID& rhs) const
	{
		return val_ != rhs.val_;
	}
	X_INLINE bool operator > (const NetGUID& rhs) const
	{
		return val_ > rhs.val_;
	}
	X_INLINE bool operator < (const NetGUID& rhs) const
	{
		return val_ < rhs.val_;
	}

	X_INLINE const char* toString(StrBuf& buf) {
		buf.appendFmt("%" PRIu64, val_);
		return buf.c_str();
	}

private:
	uint64_t val_;
	SystemIndex sysIdx_;
};

// ---------------------------------

// and address, not necissarialy a unique identifier.
struct ISystemAdd
{

	virtual ~ISystemAdd() {}

	virtual uint16_t getPort(void) const X_ABSTRACT;

	virtual IpVersion::Enum getIPVersion(void) const X_ABSTRACT;
	virtual bool IsLoopBack(void) const X_ABSTRACT;
	virtual bool IsLanAddress(void) const X_ABSTRACT;

	virtual const char* toString(IPStr& strBuf, bool incPort = true) const X_ABSTRACT;
};

// ---------------------------------

struct AddressOrGUID
{
	X_INLINE AddressOrGUID() : 
		pSystemAddress(nullptr) 
	{
	}
	X_INLINE AddressOrGUID(const AddressOrGUID& oth) :
		netGuid(oth.netGuid), 
		pSystemAddress(oth.pSystemAddress) 
	{		
	}
	X_INLINE AddressOrGUID(const NetGUID& guid) : netGuid(guid), pSystemAddress(nullptr)
	{
	}
	X_INLINE AddressOrGUID(const ISystemAdd* pSysAdd) : pSystemAddress(pSysAdd)
	{

	}
	X_INLINE bool operator==(const AddressOrGUID& oth) {
		return netGuid == oth.netGuid && pSystemAddress == oth.pSystemAddress;
	}
	X_INLINE AddressOrGUID& operator=(const AddressOrGUID& rhs) {
		netGuid = rhs.netGuid;
		pSystemAddress = rhs.pSystemAddress;
		return *this;
	}

	X_INLINE bool isAddressValid(void) const {
		return pSystemAddress != 0;
	}


	NetGUID netGuid;
	const ISystemAdd* pSystemAddress;
};

// ---------------------------------


struct Packet
{
	ISystemAdd* pSystemAddress; // sender add.
	NetGUID guid;

	uint32_t length;
	BitSizeT bitLength; // length in bits.
	uint8_t* pData; // data from sender.
};

X_ENSURE_SIZE(Packet, 40)

// ---------------------------------

struct IPeer
{
	virtual ~IPeer() {}

	virtual StartupResult::Enum init(int32_t maxConnections, SocketDescriptor* pSocketDescriptors,
		size_t socketDescriptorCount) X_ABSTRACT;
	virtual void shutdown(core::TimeVal blockDuration, uint8_t orderingChannel = 0,
		PacketPriority::Enum disconnectionNotificationPriority = PacketPriority::Low) X_ABSTRACT;

	// connection api
	virtual ConnectionAttemptResult::Enum connect(const char* pHost, Port remotePort, uint32_t retryCount = 12, 
		core::TimeVal retryDelay = core::TimeVal(0.5f), core::TimeVal timeoutTime = core::TimeVal()) X_ABSTRACT;
	virtual void closeConnection(const AddressOrGUID target, bool sendDisconnectionNotification, 
		uint8_t orderingChannel = 0, PacketPriority::Enum notificationPriority = PacketPriority::Low) X_ABSTRACT;

	// connection util
	virtual ConnectionState::Enum getConnectionState(const AddressOrGUID systemIdentifier) X_ABSTRACT;
	virtual void cancelConnectionAttempt(const ISystemAdd* pTarget) X_ABSTRACT;

	// send some data :)
	virtual uint32_t send(const uint8_t* pData, const size_t length, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceiptNumber = 0) X_ABSTRACT;

	// send to self.
	virtual void sendLoopback(const uint8_t* pData, size_t lengthBytes) X_ABSTRACT;

	virtual Packet* receive(void) X_ABSTRACT;
	virtual void freePacket(Packet* pPacket) X_ABSTRACT;


	// connection limits
	virtual void setMaximumIncomingConnections(uint16_t numberAllowed) X_ABSTRACT;
	virtual uint16_t getMaximumIncomingConnections(void) const X_ABSTRACT;
	virtual uint16_t numberOfConnections(void) const X_ABSTRACT; // current number of connected peers.

	virtual uint32_t getMaximunNumberOfPeers(void) const X_ABSTRACT;

	// Ping 
	virtual void ping(const ISystemAdd* pTarget) X_ABSTRACT;
	virtual bool ping(const char* pHost, uint16_t remotePort, bool onlyReplyOnAcceptingConnections,
		uint32_t connectionSocketIndex = 0) X_ABSTRACT;

	// bans at connection level.
	virtual void addToBanList(const char* pIP, core::TimeVal timeout = core::TimeVal()) X_ABSTRACT;
	virtual void removeFromBanList(const char* pIP) X_ABSTRACT;
	virtual bool isBanned(const char* pIP) X_ABSTRACT;
	virtual void clearBanList(void) X_ABSTRACT;


	virtual int32_t getAveragePing(const AddressOrGUID systemIdentifier) const X_ABSTRACT;
	virtual int32_t getLastPing(const AddressOrGUID systemIdentifier) const X_ABSTRACT;
	virtual int32_t getLowestPing(const AddressOrGUID systemIdentifier) const X_ABSTRACT;


	virtual const NetGUID& getMyGUID(void) const X_ABSTRACT;

	virtual void setTimeoutTime(core::TimeVal time, const ISystemAdd* pTarget) X_ABSTRACT;
	virtual core::TimeVal getTimeoutTime(const ISystemAdd* pTarget = nullptr) X_ABSTRACT;

	// MTU for a given system
	virtual int getMTUSize(const ISystemAdd* pTarget = nullptr) X_ABSTRACT;

	virtual bool getStatistics(const ISystemAdd* pTarget, NetStatistics& stats) X_ABSTRACT;


};

// ---------------------------------

struct INet
{
	virtual ~INet() {}


	virtual void registerVars(void) X_ABSTRACT;
	virtual void registerCmds(void) X_ABSTRACT;

	virtual bool init(void) X_ABSTRACT;
	virtual void shutDown(void) X_ABSTRACT;
	virtual void release(void) X_ABSTRACT;

	virtual IPeer* createPeer(void) X_ABSTRACT;
	virtual void deletePeer(IPeer* pPeer) X_ABSTRACT;

	virtual ISystemAdd* createSysAddress(const char* pAddressStr) X_ABSTRACT;
	virtual ISystemAdd* createSysAddress(const char* pAddressStr, uint16_t port) X_ABSTRACT;


};

X_NAMESPACE_END