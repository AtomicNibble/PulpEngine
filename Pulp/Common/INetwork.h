#pragma once

#include <Time\TimeVal.h>
#include <Time\DateStamp.h>

X_NAMESPACE_BEGIN(net)


X_DECLARE_ENUM(ConnectionState)(
	Pending,
	Connecting,
	Connected,
	Disconnecting,
	SilentlyDisconnecting,
	Disconnected,
	NotConnected
);

X_DECLARE_ENUM(ConnectionAttemptResult)(
	Started,
	InvalidPAram,
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

typedef uint16_t SystemIndex;
typedef uint16_t Port;
typedef uint8_t MessageID;
typedef uint32_t BitSizeT;

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
	typedef core::StackString<32, char> HostAddStr;

public:
	X_INLINE SocketDescriptor();
	X_INLINE SocketDescriptor(uint16_t port, HostAddStr hostAddress);


private:
	uint16_t port_;
	uint16_t socketFamily_;
	HostAddStr hostAddress_;
};

// ---------------------------------

class NetGUID 
{
public:
	NetGUID();
	explicit NetGUID(uint64_t d);


	X_INLINE bool operator==(const NetGUID& rhs) const;
	X_INLINE bool operator!=(const NetGUID& rhs) const;
	X_INLINE bool operator > (const NetGUID& rhs) const;
	X_INLINE bool operator < (const NetGUID& rhs) const;


private:
	uint64_t val_;
	SystemIndex sysIdx_;
};

// ---------------------------------

// and address, not necissarialy a unique identifier.
struct ISystemAdd
{
	typedef char AddressStr[256];

	virtual ~ISystemAdd() {}

	virtual uint16_t getPort(void) const X_ABSTRACT;

	virtual IpVersion::Enum getIPVersion(void) X_ABSTRACT;
	virtual bool IsLoopBack(void) const X_ABSTRACT;
	virtual bool IsLanAddress(void) const X_ABSTRACT;

	virtual const char* toString(AddressStr& strBuf, bool incPort = true) X_ABSTRACT;
};

// ---------------------------------

struct AddressOrGUID
{
	NetGUID netGuid;
	ISystemAdd* pSystemAddress;
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


// ---------------------------------

struct IPeer
{
	virtual ~IPeer() {}

	virtual StartupResult::Enum init(uint32_t maxConnections, SocketDescriptor* pSocketDescriptors, 
		uint32_t socketDescriptorCount) X_ABSTRACT;
	virtual void shutdown(core::TimeVal blockDuration, uint8_t orderingChannel = 0,
		PacketPriority::Enum disconnectionNotificationPriority = PacketPriority::Low) X_ABSTRACT;

	// connection api
	virtual ConnectionAttemptResult::Enum connect(const char* pHost, Port remotePort, 
		uint32_t connectionSocketIndex = 0, uint32_t sendConnectionAttemptCount = 12, uint32_t 
		timeBetweenSendConnectionAttemptsMS = 500, core::TimeVal timeoutTime = core::TimeVal()) X_ABSTRACT;
	virtual void closeConnection(const AddressOrGUID target, bool sendDisconnectionNotification, 
		uint8_t orderingChannel = 0, PacketPriority::Enum disconnectionNotificationPriority = PacketPriority::Low) X_ABSTRACT;

	// connection util
	virtual ConnectionState::Enum getConnectionState(const AddressOrGUID systemIdentifier) X_ABSTRACT;
	virtual void cancelConnectionAttempt(const ISystemAdd* pTarget) X_ABSTRACT;


	virtual uint32_t send(const char* pData, const size_t length, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceiptNumber = 0) X_ABSTRACT;

	// connection limits
	virtual void setMaximumIncomingConnections(uint16_t numberAllowed) X_ABSTRACT;
	virtual uint16_t getMaximumIncomingConnections(void) const X_ABSTRACT;
	virtual uint16_t numberOfConnections(void) const X_ABSTRACT;

	// Ping 
	virtual void ping(const ISystemAdd* pTarget) X_ABSTRACT;
	virtual bool ping(const char* pHost, uint16_t remotePort, bool onlyReplyOnAcceptingConnections,
		uint32_t connectionSocketIndex = 0) X_ABSTRACT;


	virtual int getAveragePing(const AddressOrGUID systemIdentifier) X_ABSTRACT;
	virtual int getLastPing(const AddressOrGUID systemIdentifier) const X_ABSTRACT;
	virtual int getLowestPing(const AddressOrGUID systemIdentifier) const X_ABSTRACT;


	virtual const NetGUID& getMyGUID(void) const X_ABSTRACT;

	virtual void setTimeoutTime(core::TimeVal time, const ISystemAdd* pTarget) X_ABSTRACT;
	virtual core::TimeVal getTimeoutTime(const ISystemAdd* pTarget) X_ABSTRACT;

	// MTU for a given system
	virtual int getMTUSize(const ISystemAdd* pTarget) X_ABSTRACT;

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