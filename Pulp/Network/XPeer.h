#pragma once

#include <Containers\Array.h>
#include <Threading\ThreadQue.h>

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\AllocationPolicies\GrowingBlockAllocator.h>
#include <Memory\HeapArea.h>

#include <Time\TimeStamp.h>

#include "Sockets\Socket.h"

X_NAMESPACE_BEGIN(net)

X_DECLARE_ENUM(ConnectMode)(
	NoAction,
	DisconnectAsap,
	DisconnectAsapSilent,
	DisconnectOnNoAck,
	RequestedConnection,
	HandlingConnectionRequest,
	UnverifiedSender,
	Connected
);

struct BufferdCommand
{
	uint8_t* pData;

	// 4
	BitSizeT numberOfBitsToSend;

	// 4
	PacketPriority::Enum priority;
	PacketReliability::Enum reliability;
	uint8_t orderingChannel;
	bool broadcast;

	// ?
	AddressOrGUID systemIdentifier;
	
	// 4
	uint32_t receipt;
};

struct RemoteSystem
{
	typedef core::FixedArray<SystemAdd, MAX_INTERNAL_IDS> SystemAddArr;

public:
	RemoteSystem();

	bool isActive;

	SystemAdd systemAddress;
	SystemAdd myExternalSystemAddress;
	SystemAddArr thierInternalSystemAddress;
	
	core::TimeVal nextPingTime;
	core::TimeVal lastReliableSend;
	core::TimeVal connectionTime;

	NetGUID guid;

	uint16_t lowestPing;
	uint16_t MTUSize;
	ConnectMode::Enum connectMode;
};

X_ENSURE_SIZE(BufferdCommand, 56) // just to keep track of it's size for memory bandwidth consierations

class XPeer : public IPeer
{
	typedef core::FixedArray<SystemAdd, MAX_INTERNAL_IDS> SystemAddArr;
	typedef core::Array<NetSocket> SocketsArr;	
	typedef core::Array<RemoteSystem, core::ArrayAlignedAllocator<RemoteSystem>> RemoteSystemArr;
	typedef core::ThreadQue<BufferdCommand*, core::CriticalSection> BufferdCommandQue;

	typedef core::MemoryArena<
		core::PoolAllocator,
		core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
		core::SimpleBoundsChecking,
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> PoolArena;

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
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> BlockAlocArena;


	static const size_t MAX_POOL_ALLOC = 2048; // packets and buffered commands

public:
	XPeer(core::MemoryArenaBase* arena);
	~XPeer() X_FINAL;

	// IPeer

	StartupResult::Enum init(int32_t maxConnections, SocketDescriptor* pSocketDescriptors,
		size_t socketDescriptorCount) X_FINAL;
	void shutdown(core::TimeVal blockDuration, uint8_t orderingChannel = 0,
		PacketPriority::Enum disconnectionNotificationPriority = PacketPriority::Low) X_FINAL;

	// connection api
	ConnectionAttemptResult::Enum connect(const char* pHost, Port remotePort,
		uint32_t connectionSocketIndex = 0, uint32_t sendConnectionAttemptCount = 12, uint32_t
		timeBetweenSendConnectionAttemptsMS = 500, core::TimeVal timeoutTime = core::TimeVal()) X_FINAL;
	void closeConnection(const AddressOrGUID target, bool sendDisconnectionNotification,
		uint8_t orderingChannel = 0, PacketPriority::Enum disconnectionNotificationPriority = PacketPriority::Low) X_FINAL;

	// connection util
	ConnectionState::Enum getConnectionState(const AddressOrGUID systemIdentifier) X_FINAL;
	void cancelConnectionAttempt(const ISystemAdd* pTarget) X_FINAL;


	uint32_t send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceiptNumber = 0) X_FINAL;

	void sendLoopback(const uint8_t* pData, size_t lengthBytes) X_FINAL;

	Packet* receive(void) X_FINAL;


	// connection limits
	void setMaximumIncomingConnections(uint16_t numberAllowed) X_FINAL;
	uint16_t getMaximumIncomingConnections(void) const X_FINAL;
	uint16_t numberOfConnections(void) const X_FINAL;

	uint32_t getMaximunNumberOfPeers(void) const X_FINAL;

	// Ping 
	void ping(const ISystemAdd* pTarget) X_FINAL;
	bool ping(const char* pHost, uint16_t remotePort, bool onlyReplyOnAcceptingConnections,
		uint32_t connectionSocketIndex = 0) X_FINAL;


	int getAveragePing(const AddressOrGUID systemIdentifier) X_FINAL;
	int getLastPing(const AddressOrGUID systemIdentifier) const X_FINAL;
	int getLowestPing(const AddressOrGUID systemIdentifier) const X_FINAL;


	const NetGUID& getMyGUID(void) const X_FINAL;

	void setTimeoutTime(core::TimeVal time, const ISystemAdd* pTarget) X_FINAL;
	core::TimeVal getTimeoutTime(const ISystemAdd* pTarget = nullptr) X_FINAL;

	// MTU for a given system
	int getMTUSize(const ISystemAdd* pTarget = nullptr) X_FINAL;

	bool getStatistics(const ISystemAdd* pTarget, NetStatistics& stats) X_FINAL;

	// ~IPeer

	void setUnreliableTimeout(core::TimeVal timeout);


private:
	void sendBuffered(const uint8_t* pData, BitSizeT numberOfBitsToSend, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t receipt);
	bool sendImmediate(const uint8_t* pData, BitSizeT numberOfBitsToSend, PacketPriority::Enum priority, 
		PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, 
		bool useCallerDataAllocation, core::TimeStamp currentTime, uint32_t receipt);

	bool isLoopbackAddress(const AddressOrGUID& systemIdentifier, bool matchPort) const;

	// Remote Sys
	RemoteSystem* getRemoteSystem(const AddressOrGUID systemIdentifier, bool onlyActive);
	RemoteSystem* getRemoteSystemFromSystemAddress(const SystemAdd& systemAddress, bool onlyActive);
	RemoteSystem* getRemoteSystemFromGUID(const NetGUID guid, bool onlyActive);
	RemoteSystem* getRemoteSystem(const SystemAdd& systemAddress);
	size_t getRemoteSystemIndex(const SystemAdd& systemAddress) const;
	size_t getRemoteSystemIndex(const NetGUID& guid) const;
	size_t getRemoteSystemIndex(const AddressOrGUID& systemIdentifier) const;

	void parseConnectionRequestPacket(RemoteSystem* pRemoteSystem, const SystemAdd& systemAddress, const uint8_t* pData, size_t byteSize);
	void onConnectionRequest(RemoteSystem* pRemoteSystem, core::TimeStamp incomingTimestamp);


	// adds packet to back of receive qeue
	void pushBackPacket(Packet* pPacket, bool pushAtHead = false);

	Packet* allocPacket(size_t lengthBytes);
	void freePacket(Packet* pPacket) X_FINAL;

	BufferdCommand* allocBufferdCmd(size_t lengthBytes);
	void freebufferdCmd(BufferdCommand* pBufCmd);


	uint8_t* allocPacketData(size_t lengthBytes);
	void freePacketData(uint8_t* pPacketData);

	uint32_t nextSendReceipt(void);
	uint32_t incrementNextSendReceipt(void);

private:
	bool populateIpList(void);

private:
	NetGUID guid_;

	core::TimeVal defaultTimeOut_;
	core::TimeVal unreliableTimeOut_;
	int32_t defaultMTU_;
	int32_t maxIncommingConnections_;
	int32_t maxPeers_;
	core::AtomicInt sendReceiptSerial_;

	SystemAddArr ipList_;
	SocketsArr sockets_;

	// ques.
	BufferdCommandQue bufferdCmds_;

	// rmeote systems
	RemoteSystemArr		remoteSystems_;

	// allocators
	core::HeapArea      poolHeap_;
	core::PoolAllocator poolAllocator_;
	PoolArena			poolArena_;

	core::GrowingBlockAllocator blockAlloc_;
	BlockAlocArena		blockArena_;
};



X_NAMESPACE_END

