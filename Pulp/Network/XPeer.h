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

#include "Sockets\Socket.h"
#include "Reliability\ReliabilityLayer.h"


X_NAMESPACE_BEGIN(net)

class NetVars;

X_DECLARE_ENUM(ConnectState)(
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
	X_DECLARE_ENUM(Cmd) (
		Send,
		CloseConnection
	);

	uint8_t* pData;

	// 4
	Cmd::Enum cmd;

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
	union {
		uint32_t receipt;
		bool sendDisconnectionNotification;
	};
};

struct PingAndClockDifferential
{
	PingAndClockDifferential();

	X_INLINE bool isValid(void) const;

	uint16_t pingTime;
	// this is a rougth guess about how to translate a remotes systems relative time to our relative time
	// this may be possitive or negative.
	core::TimeVal clockDifferential; 
};

X_ALIGNED_SYMBOL(struct RemoteSystem, 64) // each remote can be updated on diffrent thread, prevent any false sharing.
{
	static const size_t PING_HISTORY_COUNT = 3;

	typedef core::FixedArray<SystemAdd, MAX_INTERNAL_IDS> SystemAddArr;
	typedef std::array<PingAndClockDifferential, PING_HISTORY_COUNT> PingArr;

	X_NO_COPY(RemoteSystem);
	X_NO_ASSIGN(RemoteSystem);

public:
	RemoteSystem(NetVars& vars, core::MemoryArenaBase* arena, core::MemoryArenaBase* packetDataArena, core::MemoryArenaBase* packetPool);
	RemoteSystem(RemoteSystem&& oth) = default;

	RemoteSystem& operator=(RemoteSystem&& rhs) = default;

	bool canSend(void) const;
	ConnectionState::Enum getConnectionState(void) const;

	void closeConnection(void);

	void onConnected(const SystemAdd& externalSysId, const SystemAddArr& localIps,
		core::TimeVal sendPingTime, core::TimeVal sendPongTime);
	void onPong(core::TimeVal sendPingTime, core::TimeVal sendPongTime);

	X_INLINE bool sendReliabile(const uint8_t* pData, BitSizeT numberOfBitsToSend, bool ownData, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, core::TimeVal currentTime, uint32_t receipt = 0);
	X_INLINE bool sendReliabile(const core::FixedBitStreamBase& bs, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, core::TimeVal currentTime, uint32_t receipt = 0);

private:
	X_INLINE void onSend(PacketReliability::Enum reliability, core::TimeVal sendTime);

public:
	bool isActive;
	bool weStartedconnection;
	bool _pad[2];

	SystemAdd systemAddress;				// add remote system
	SystemAdd myExternalSystemAddress;		// my add from the point of view of remote system
	SystemAddArr thierInternalSystemAddress; // copy of the peers internal local sys add.
	
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

	core::Hash::SHA1Digest nonce;
};

struct RequestConnection
{
	SystemAdd systemAddress;

	core::TimeVal nextRequestTime;
	core::TimeVal timeoutTime;
	core::TimeVal retryDelay;

	uint8_t numRequestsMade;
	uint8_t retryCount;
	uint8_t socketIdx;
	uint8_t MTUIdxShift;

	PasswordStr password;
};

struct Ban
{
	IPStr ip;
	core::TimeVal timeOut; // 0 = never.
};

// just to keep track of it's size for memory bandwidth consierations
// X_ENSURE_SIZE(BufferdCommand, 40) 
// X_ENSURE_SIZE(RemoteSystem, 520 + sizeof(ReliabilityLayer))
// X_ENSURE_SIZE(RequestConnection, 72)


class XPeer : public IPeer
{
	typedef core::FixedArray<SystemAdd, MAX_INTERNAL_IDS> SystemAddArr;
	typedef core::Array<NetSocket> SocketsArr;	
	typedef core::Array<RemoteSystem, core::ArrayAlignedAllocator<RemoteSystem>> RemoteSystemArr;
	typedef core::Array<RemoteSystem*> RemoteSystemPtrArr;

	// thead que's
	typedef core::ThreadQue<BufferdCommand*, core::CriticalSection> BufferdCommandQue;
	typedef core::ThreadQue<Packet*, core::CriticalSection> PacketQue;
	typedef core::ThreadQue<RecvData*, core::CriticalSection> RecvDataQue;

	typedef core::Array<RequestConnection*> RequestConnectionArr;
	typedef core::Array<Ban> BanArr;
	typedef core::Array<core::ThreadMember<XPeer>> ThreadArr;

	// a bit stream that don't own the memory.
	// we just read directly off the recived buffer.
	typedef core::FixedBitStreamBase RecvBitStream;
	typedef core::FixedBitStreamBase UpdateBitStream;

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


	static std::array<uint32_t, 3> MTUSizesArr;

public:
	XPeer(NetVars& vars, core::MemoryArenaBase* arena);
	~XPeer() X_FINAL;

	// IPeer

	StartupResult::Enum init(int32_t maxConnections, SocketDescriptor* pSocketDescriptors,
		size_t socketDescriptorCount) X_FINAL;
	void shutdown(core::TimeVal blockDuration, uint8_t orderingChannel = 0,
		PacketPriority::Enum disconnectionNotificationPriority = PacketPriority::Low) X_FINAL;

	void runUpdate(void) X_FINAL;

	void setPassword(const PasswordStr& pass) X_FINAL;

	// connection api
	ConnectionAttemptResult::Enum connect(const char* pHost, Port remotePort, const PasswordStr& password, uint32_t retryCount = 12,
		core::TimeVal retryDelay = core::TimeVal(0.5f), core::TimeVal timeoutTime = core::TimeVal()) X_FINAL;
	void closeConnection(const AddressOrGUID target, bool sendDisconnectionNotification,
		uint8_t orderingChannel = 0, PacketPriority::Enum notificationPriority = PacketPriority::Low) X_FINAL;

	// connection util
	ConnectionState::Enum getConnectionState(const AddressOrGUID systemIdentifier) X_FINAL;
	void cancelConnectionAttempt(const ISystemAdd* pTarget) X_FINAL;

	uint32_t send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, 
		bool broadcast, uint32_t forceReceiptNumber = 0) X_FINAL;
	uint32_t send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, const AddressOrGUID systemIdentifier) X_FINAL;

	void sendLoopback(const uint8_t* pData, size_t lengthBytes) X_FINAL;

	Packet* receive(void) X_FINAL;
	void clearPackets(void) X_FINAL; // free's any packets in the receive que.

	// connection limits
	void setMaximumIncomingConnections(uint16_t numberAllowed) X_FINAL;
	X_INLINE uint16_t getMaximumIncomingConnections(void) const X_FINAL;
	uint16_t numberOfConnections(void) const X_FINAL;

	X_INLINE uint32_t getMaximunNumberOfPeers(void) const X_FINAL;

	// Ping 
	void ping(const ISystemAdd* pTarget) X_FINAL;
	bool ping(const char* pHost, uint16_t remotePort, bool onlyReplyOnAcceptingConnections,
		uint32_t connectionSocketIndex = 0) X_FINAL;

	// bans at connection level.
	void addToBanList(const IPStr& ip, core::TimeVal timeout = core::TimeVal()) X_FINAL;
	void removeFromBanList(const char* pIP) X_FINAL;
	bool isBanned(const char* pIP) X_FINAL;
	bool isBanned(const IPStr& ip);
	void clearBanList(void) X_FINAL;
	void listBans(void) const;

	int32_t getAveragePing(const AddressOrGUID systemIdentifier) const X_FINAL;
	int32_t getLastPing(const AddressOrGUID systemIdentifier) const X_FINAL;
	int32_t getLowestPing(const AddressOrGUID systemIdentifier) const X_FINAL;


	X_INLINE const NetGUID& getMyGUID(void) const X_FINAL;

	// MTU for a given system
	int32_t getMTUSize(const ISystemAdd* pTarget = nullptr) X_FINAL;

	bool getStatistics(const ISystemAdd* pTarget, NetStatistics& stats) X_FINAL;

	// ~IPeer

	X_INLINE void setUnreliableTimeout(core::TimeVal timeout);

	X_INLINE bool accpetingIncomingConnections(void) const;
	// the number of remote connections to us.
	// excludes connections made by us.
	size_t getNumRemoteInitiatedConnections(void) const;

	void listRemoteSystems(bool verbose) const;

private:
	void sendBuffered(const uint8_t* pData, BitSizeT numberOfBitsToSend, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t receipt);

	X_INLINE void sendBuffered(const core::FixedBitStreamBase& bs, PacketPriority::Enum priority,
		PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t receipt);


	void sendPing(RemoteSystem& rs, PacketReliability::Enum rel);
	void notifyAndFlagForShutdown(RemoteSystem& rs, uint8_t orderingChannel, PacketPriority::Enum notificationPriority);
	
	bool isLoopbackAddress(const AddressOrGUID& systemIdentifier, bool matchPort) const;

	// Remote Sys
	const RemoteSystem* getRemoteSystem(const AddressOrGUID systemIdentifier, bool onlyActive) const;
	const RemoteSystem* getRemoteSystem(const SystemAdd& systemAddress, bool onlyActive) const;
	const RemoteSystem* getRemoteSystem(const NetGUID guid, bool onlyActive) const;
	RemoteSystem* getRemoteSystem(const AddressOrGUID systemIdentifier, bool onlyActive);
	RemoteSystem* getRemoteSystem(const SystemAdd& systemAddress, bool onlyActive);
	RemoteSystem* getRemoteSystem(const NetGUID guid, bool onlyActive);
	size_t getRemoteSystemIndex(const SystemAdd& systemAddress) const;
	size_t getRemoteSystemIndex(const NetGUID& guid) const;
	size_t getRemoteSystemIndex(const AddressOrGUID& systemIdentifier) const;

	// adds packet to back of receive qeue
	void pushBackPacket(const RemoteSystem& rs, ReliabilityLayer::PacketData& data);
	X_INLINE void pushBackPacket(Packet* pPacket);

	Packet* allocPacket(size_t lengthBits);
	void freePacket(Packet* pPacket) X_FINAL;

	BufferdCommand* allocBufferdCmd(BufferdCommand::Cmd::Enum type, size_t lengthBits);
	void freeBufferdCmd(BufferdCommand* pBufCmd);

	uint8_t* allocPacketData(size_t lengthBytes);
	void freePacketData(uint8_t* pPacketData);

	RecvData* allocRecvData(void);
	void freeRecvData(RecvData* pRecvData);

	RequestConnection* allocConnectionRequest(void);
	void freeConnectionRequest(RequestConnection* pConReq);

	X_INLINE uint32_t nextSendReceipt(void);
	X_INLINE uint32_t incrementNextSendReceipt(void);

	void removeConnectionRequest(const SystemAdd& sysAdd);

private:
	void Job_remoteReliabilityTick(RemoteSystem** pRemoteSystems, uint32_t count);

private:
	void processRecvData(UpdateBitStream& updateBS, core::TimeVal timeNow);
	void processConnectionRequests(UpdateBitStream& updateBS, core::TimeVal timeNow);
	void processBufferdCommands(UpdateBitStream& updateBS, core::TimeVal timeNow);
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

	RemoteSystem* addRemoteSystem(const SystemAdd& sysAdd, NetGUID guid, int32_t remoteMTU, 
		NetSocket* pSrcSocket, SystemAdd bindingAdd, ConnectState::Enum state);
	void disconnectRemote(RemoteSystem& rs);
	bool isIpConnectSpamming(const SystemAdd& sysAdd, core::TimeVal* pDeltaOut = nullptr);

	// ------


	X_INLINE void onSocketRecv(RecvData* pData);
	core::Thread::ReturnValue socketRecvThreadProc(const core::Thread& thread);


private:
	bool populateIpList(void);

private:
	NetVars& vars_;
	core::V2::JobSystem* pJobSys_;

	NetGUID guid_;

	core::TimeVal unreliableTimeOut_;
	int32_t defaultMTU_;
	int32_t maxIncommingConnections_;
	int32_t maxPeers_;
	core::AtomicInt sendReceiptSerial_;

	SystemAddArr ipList_;
	SocketsArr sockets_;
	ThreadArr socketThreads_;

	// ques.
	BufferdCommandQue	bufferdCmds_;
	PacketQue			packetQue_;
	RecvDataQue			recvDataQue_;

	core::CriticalSection connectionReqsCS_;
	RequestConnectionArr connectionReqs_;

	BanArr				bans_;

	// rmeote systems
	RemoteSystemArr		remoteSystems_;
	RemoteSystemPtrArr  activeRemoteSystems_;

	// allocators
	core::MemoryArenaBase* arena_; // gen purpose.

	PoolArena::AllocationPolicy poolAllocator_;
	PoolArena			poolArena_;

	PoolArena::AllocationPolicy pool2Allocator_;
	PoolArena			pool2Arena_;

	BlockAlocArena::AllocationPolicy blockAlloc_;
	BlockAlocArena		blockArena_;

	PasswordStr password_; // 12345
};



X_NAMESPACE_END

#include "XPeer.inl"