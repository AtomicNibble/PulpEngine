#include "stdafx.h"
#include "XPeer.h"
#include "XNet.h"

#include <Hashing\sha1.h>
#include <Memory\VirtualMem.h>
#include <String\HumanDuration.h>
#include <String\HumanSize.h>
#include <Random\MultiplyWithCarry.h>
#include <ITimer.h>

#include "Sockets\Socket.h"

X_NAMESPACE_BEGIN(net)

namespace
{



	// returns if ip is matched by the pattern.
	bool ipWildMatch(const IPStr& pattern, const IPStr& ip)
	{
		// do lame matching?
		// 127.1.*.*
		// 127.1.25.255
		// so make sure we match untill we reach a *, then just check we only have '.' && '*' left.

		// simple case where match
		if (pattern == ip) {
			return true;
		}

		// do wild matching.
		size_t idx = 0;
		while (idx < pattern.length() && idx < ip.length())
		{
			if (pattern[idx] == ip[idx])
			{
				++idx;
			}
			else if (pattern[idx] == '*')
			{
				// check the rest of the pattern is not digits.
				while (idx < pattern.length()) {
					if (pattern[idx] != '.' && pattern[idx] != '*') {
						X_WARNING("Net", "Potential ip matching error: \"%s\" - \"%s\"", pattern.c_str(), ip.c_str());
					}
				}

				return true;
			}
		}

		return false;
	}

	// start of packet to mark offline msg.
	static std::array<uint8_t, 8> OFFLINE_MSG_ID = {
		0x00, 0x35, 0x33, 0x24, 0xbb, 0xa5, 0x38, 0x85
	};

	static const size_t POOL2_ALLOCATION_SIZE = sizeof(ReliablePacket);
	static const size_t POOL2_ALLOCATION_ALIGN = X_ALIGN_OF(ReliablePacket);
	static const size_t POOL2_ALLOC_MAX = 8096;

	static const size_t POOL_ALLOC_MAX = 2048; // packets and buffered commands
	static const size_t POOL_ALLOCATION_SIZE = core::Max(sizeof(BufferdCommand), sizeof(Packet));
	static const size_t POOL_ALLOCATION_ALIGN =  core::Max(X_ALIGN_OF(BufferdCommand), X_ALIGN_OF(Packet));

} // namespace 

PingAndClockDifferential::PingAndClockDifferential()
{
	pingTime = UNDEFINED_PING;
}

// -----------------------------------

RemoteSystem::RemoteSystem(NetVars& vars, core::MemoryArenaBase* arena, 
	core::MemoryArenaBase* packetDataArena, core::MemoryArenaBase* packetPool) :
	relLayer(vars, arena, packetDataArena, packetPool)
{
	isActive = false;
	weStartedconnection = false;

	connectState = ConnectState::NoAction;
	lowestPing = UNDEFINED_PING;
	MTUSize = MAX_MTU_SIZE;

	pNetSocket = nullptr;
}


bool RemoteSystem::canSend(void) const
{
	if (!isActive) {
		return false;
	}

	// in a mode we want to send to?
	switch (connectState)
	{
		case ConnectState::DisconnectAsap:
		case ConnectState::DisconnectAsapSilent:
		case ConnectState::DisconnectOnNoAck:
			return false;
		default:
			break;
	}

	return true;
}

ConnectionState::Enum RemoteSystem::getConnectionState(void) const
{
	if (!isActive) {
		return ConnectionState::Disconnected;
	}

	static_assert(ConnectState::ENUM_COUNT == 8, "Additional states? this logic needs updating");
	switch (connectState)
	{
		case ConnectState::DisconnectAsap:
		case ConnectState::DisconnectOnNoAck:
			return ConnectionState::Disconnecting;
		case ConnectState::DisconnectAsapSilent:
			return ConnectionState::DisconnectingSilently;

		case ConnectState::RequestedConnection:
		case ConnectState::HandlingConnectionRequest:
		case ConnectState::UnverifiedSender:
			return ConnectionState::Connecting;

		case ConnectState::Connected:
			return ConnectionState::Connected;
	}

	return ConnectionState::Disconnected;
}

void RemoteSystem::disconnect(void)
{
	isActive = false;
	weStartedconnection = false;

	connectState = ConnectState::NoAction;
	lowestPing = UNDEFINED_PING;
	MTUSize = MAX_MTU_SIZE;

	pNetSocket = nullptr;

	guid = UNASSIGNED_NET_GUID;

	relLayer.reset(MTUSize);
}

void RemoteSystem::onConnected(const SystemAdd& externalSysId, const SystemAddArr& localIps,
	core::TimeVal sendPingTime, core::TimeVal sendPongTime)
{
	myExternalSystemAddress = externalSysId;
	thierInternalSystemAddress = localIps;
	connectState = ConnectState::Connected;

	onPong(sendPingTime, sendPongTime);

	IPStr ipStr;

	X_LOG0("Net", "RemoteSystem ^2Connected^7: myExternalIp: \"%s\"", externalSysId.toString(ipStr));
	X_LOG_BULLET;
	for (auto ip : localIps)
	{
		X_LOG0("Net", "Remote LocalIp: \"%s\"", ip.toString(ipStr));
	}
}

void RemoteSystem::onPong(core::TimeVal sendPingTime, core::TimeVal sendPongTime)
{
	core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();
	core::TimeVal ping = timeNow - sendPingTime;

	uint16_t pingMS = safe_static_cast<uint16_t>(ping.GetMilliSecondsAsInt64());
	int64_t diff = sendPongTime.GetMilliSecondsAsInt64() - (timeNow.GetMilliSecondsAsInt64() / 2 + sendPingTime.GetMilliSecondsAsInt64() / 2);

	pings[lastPingIdx].pingTime = pingMS;
	pings[lastPingIdx].clockDifferential = core::TimeVal(diff);
	lastPingIdx = (++lastPingIdx % PING_HISTORY_COUNT);

	if (lowestPing == UNDEFINED_PING || pingMS < lowestPing) {
		lowestPing = pingMS;
	}
}


// -----------------------------------

std::array<uint32_t, 3> XPeer::MTUSizesArr = { MAX_MTU_SIZE, 1200, 576 };

XPeer::XPeer(NetVars& vars, core::MemoryArenaBase* arena) :
	vars_(vars),
	sockets_(arena),
	socketThreads_(arena),
	remoteSystems_(arena),
	bufferdCmds_(arena),
	packetQue_(arena),
	recvDataQue_(arena),
	connectionReqs_(arena),
	bans_(arena),
	arena_(arena),
	poolHeap_(
		core::bitUtil::RoundUpToMultiple<size_t>(
			PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE) * POOL_ALLOC_MAX,
			core::VirtualMem::GetPageSize()
		)
	),
	poolAllocator_(poolHeap_.start(), poolHeap_.end(),
		PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE),
		PoolArena::getMemoryAlignmentRequirement(POOL_ALLOCATION_ALIGN),
		PoolArena::getMemoryOffsetRequirement()
	),
	poolArena_(&poolAllocator_, "PoolArena"),
	pool2Heap_(
		core::bitUtil::RoundUpToMultiple<size_t>(
			PoolArena::getMemoryRequirement(POOL2_ALLOCATION_SIZE) * POOL2_ALLOC_MAX,
			core::VirtualMem::GetPageSize()
		)
	),
	pool2Allocator_(pool2Heap_.start(), pool2Heap_.end(),
		PoolArena::getMemoryRequirement(POOL2_ALLOCATION_SIZE),
		PoolArena::getMemoryAlignmentRequirement(POOL2_ALLOCATION_ALIGN),
		PoolArena::getMemoryOffsetRequirement()
	),
	pool2Arena_(&pool2Allocator_, "PoolArena"),
	blockArena_(&blockAlloc_, "blockArena")
{
	remoteSystems_.getAllocator().setBaseAlignment(16);
	remoteSystems_.setGranularity(1);

	sockets_.setGranularity(4);

	guid_ = XNet::generateGUID();

	unreliableTimeOut_ = core::TimeVal::fromMS(1000 * 10);

	defaultMTU_ = MAX_MTU_SIZE;
	maxIncommingConnections_ = 0;
	maxPeers_ = 0;

}

XPeer::~XPeer()
{
	shutdown(core::TimeVal(0ll));
}

StartupResult::Enum XPeer::init(int32_t maxConnections, SocketDescriptor* pSocketDescriptors,
	size_t socketDescriptorCount)
{
	bufferdCmds_.reserve(256);
	packetQue_.reserve(256);
	recvDataQue_.reserve(256);
	
	connectionReqs_.setGranularity(8);

	if (maxConnections < 1) {
		return StartupResult::InvalidMaxCon;
	}
	if (!pSocketDescriptors || socketDescriptorCount < 1) {
		return StartupResult::InvalidSocketDescriptors;
	}

	if (!populateIpList()) {
		return StartupResult::Error;
	}

	if (maxPeers_ == 0)
	{
		if (maxIncommingConnections_ > maxConnections) {
			maxIncommingConnections_ = maxConnections;
		}

		maxPeers_ = maxConnections;

		// todo create a pool for packets to share between rel layers.
		core::MemoryArenaBase* packetPool = &pool2Arena_;

		remoteSystems_.reserve(maxPeers_);
		for (int32_t i = 0; i < maxPeers_; i++) {
			remoteSystems_.emplace_back(vars_, arena_, &blockArena_, packetPool);
		}
	}
	
	BindParameters bindParam;
	bindParam.nonBlockingSocket = false;
	bindParam.IPHdrIncl = false;
	bindParam.broadCast = true;

	for (size_t i = 0; i < socketDescriptorCount; i++)
	{
		SocketDescriptor& socketDiscriptor = pSocketDescriptors[i];

		NetSocket socket(vars_);
		bindParam.hostAdd = socketDiscriptor.getHostAdd();
		bindParam.port = socketDiscriptor.getPort();
		bindParam.socketFamily = socketDiscriptor.getSocketFamiley();
		bindParam.socketType = SocketType::Dgram;

		BindResult::Enum res = socket.bind(bindParam);
		if (res != BindResult::Success)
		{
			if (res != BindResult::FailedToBind)
			{
				return StartupResult::SocketFailedToBind;
			}
			if (res != BindResult::SendTestFailed)
			{
				return StartupResult::SocketFailedToTestSend;
			}

			X_ASSERT_UNREACHABLE();
		}

		sockets_.emplace_back(std::move(socket));
	}

	socketThreads_.resize(sockets_.size());
	for (size_t i=0; i<socketThreads_.size(); i++)
	{
		auto& socket = sockets_[i];
		auto& socketThread = socketThreads_[i];

		core::StackString256 threadName;
		threadName.appendFmt("netSocketRecv_%i", i);

		ThreadArr::Type::FunctionDelagate del;
		del.Bind<XPeer, &XPeer::socketRecvThreadProc>(this);

		socketThread.Create(threadName.c_str(), 1024 * 4);
		socketThread.setData(&socket);
		socketThread.Start(del);
	}
		
	if (vars_.debugEnabled() > 1)
	{
		NetGuidStr guidStr;
		X_LOG0("Net", "ProtoVersion: ^5%" PRIu8 ".%" PRIu8, PROTO_VERSION_MAJOR, PROTO_VERSION_MINOR);
		X_LOG0("Net", "Max peers: ^5%" PRIi32, maxPeers_);
		X_LOG0("Net", "Max incomming connections: ^5%" PRIi32, maxIncommingConnections_);
		X_LOG0("Net", "GUID: ^5%s", guid_.toString(guidStr));
		{
			X_LOG0("Net", "Listening on ^5%" PRIuS " endpoints", sockets_.size());
			X_LOG_BULLET;
			for (auto& s : sockets_)
			{
				IPStr boundAddStr;
				s.getBoundAdd().toString(boundAddStr);
				X_LOG0("Net", "bound address: \"%s\"", boundAddStr.c_str());
			}
		}

		{
			X_LOG0("Net", "LocalAdd ^5%" PRIuS, ipList_.size());
			X_LOG_BULLET;
			for (auto& ip : ipList_)
			{
				IPStr addStr;
				X_LOG0("Net", "local address: \"%s\"", ip.toString(addStr));
			}
		}
	}


	return StartupResult::Started;
}

void XPeer::shutdown(core::TimeVal blockDuration, uint8_t orderingChannel,
	PacketPriority::Enum disconnectionNotificationPriority)
{
	X_LOG0("Net", "Shutting down peer");

	if (blockDuration.GetValue() > 0)
	{
		bool anyActive = false;

		for (auto& rs : remoteSystems_)
		{
			if (rs.isActive) {
				notifyAndFlagForShutdown(rs, true, orderingChannel, disconnectionNotificationPriority);
				anyActive |= true;
			}
		}

		if (anyActive)
		{
			auto timeNow = gEnv->pTimer->GetTimeNowReal();
			auto startTime = timeNow;

			// send out the packets.
			core::FixedBitStreamStack<MAX_MTU_SIZE> updateBS;
			peerReliabilityTick(updateBS, timeNow);

			// scale so if block is low we don't sleep much
			auto sleepTime = safe_static_cast<uint32_t>(core::Max(blockDuration.GetMilliSecondsAsInt64() / 100, 1ll));
			sleepTime = core::Min(50u, sleepTime);

			// spin till all closed or timeout.
			while (blockDuration > (timeNow - startTime))
			{
				anyActive = false;
				for (auto& rs : remoteSystems_) {
					anyActive |= rs.isActive;
				}

				if (!anyActive) {
					break;
				}

				core::Thread::Sleep(sleepTime);

				timeNow = gEnv->pTimer->GetTimeNowReal();

				peerReliabilityTick(updateBS, timeNow);
			}

			if (anyActive) {
				X_WARNING("Net", "Timed out waiting for disconnect notification dispatch.");
			}
			else {
				X_LOG0_IF(vars_.debugEnabled(), "Net", "All disconnect notifications, dispatched.");
			}
		}
	}


	for (auto& socketThread : socketThreads_) {
		socketThread.Stop();
	}

	for (size_t i = 0; i < socketThreads_.size(); i++)
	{
		auto& socket = sockets_[i];
		auto& socketThread = socketThreads_[i];

		if (!socketThread.HasFinished())
		{
			if (socket.sendSendTest()) {
				socketThread.Join();
			}
			else {
				// if the send test failed potentially the thread won't join.

			}
		}
	}

	sockets_.clear();
	socketThreads_.clear();

	{
		core::CriticalSection::ScopedLock lock(connectionReqsCS_);
		for (auto* pConReq : connectionReqs_) {
			freeConnectionRequest(pConReq);
		}
	}
	
	for (auto& rs : remoteSystems_) {
		rs.disconnect();
		rs.relLayer.free();
	}

	bufferdCmds_.tryPopAll([&](BufferdCommand* pCmd) {
		freeBufferdCmd(pCmd);
	});

	packetQue_.tryPopAll([&](Packet* pPacket) {
		freePacket(pPacket);
	});

	recvDataQue_.tryPopAll([&](RecvData* pData) {
		freeRecvData(pData);
	});

	ipList_.clear();
	sendReceiptSerial_ = 0;
}


void XPeer::setPassword(const PasswordStr& pass)
{
	password_ = pass;
}

// connection api
ConnectionAttemptResult::Enum XPeer::connect(const char* pHost, Port remotePort, const PasswordStr& password, uint32_t retryCount,
	core::TimeVal retryDelay, core::TimeVal timeoutTime)
{
	uint8_t socketIdx = 0; // hard coded socket idx for now

	if (socketIdx >= sockets_.size()) {
		return ConnectionAttemptResult::InvalidParam;
	}

	// work out what ip version this socket is, if the address is ipv4 and socket is ipv6 it's okay.
	auto ipVer = sockets_[socketIdx].getBoundAdd().getIPVersion();

	// need to work out the address.
	SystemAdd systemAddress;
	if (!systemAddress.fromStringExplicitPort(pHost, remotePort, ipVer)) {
		return ConnectionAttemptResult::FailedToResolve;
	}

	// are we already connected?
	if (getRemoteSystem(systemAddress, true)) {
		return ConnectionAttemptResult::AlreadyConnected;
	}

	RequestConnection* pConReq = allocConnectionRequest();
	pConReq->systemAddress = systemAddress;
	pConReq->nextRequestTime = gEnv->pTimer->GetTimeNowReal();
	pConReq->timeoutTime = timeoutTime;
	pConReq->retryDelay = retryDelay;
	pConReq->numRequestsMade = 0;
	pConReq->retryCount = retryCount;
	pConReq->socketIdx = socketIdx;
	pConReq->MTUIdxShift = 0;
	pConReq->password = password;

	// only push if not trying to connect already.
	auto matchSysAddFunc = [&systemAddress](const RequestConnection* pOth) {
		return pOth->systemAddress == systemAddress;
	};

	core::CriticalSection::ScopedLock lock(connectionReqsCS_);

	if (std::find_if(connectionReqs_.begin(), connectionReqs_.end(), matchSysAddFunc) != connectionReqs_.end()) {
		freeConnectionRequest(pConReq);
		return ConnectionAttemptResult::AlreadyInProgress;
	}

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Started Connection request to host: \"%s\" port: ^5%" PRIu16, pHost, remotePort);

	connectionReqs_.emplace_back(pConReq);
	
	return ConnectionAttemptResult::Started;
}

void XPeer::closeConnection(const AddressOrGUID target, bool sendDisconnectionNotification,
	uint8_t orderingChannel, PacketPriority::Enum notificationPriority)
{
	closeConnectionInternal(target, sendDisconnectionNotification, false, orderingChannel, notificationPriority);

	if (!sendDisconnectionNotification && getConnectionState(target) == ConnectionState::Connected)
	{
		NetGUID guid = target.netGuid;

		if (guid == UNASSIGNED_NET_GUID) {
			X_ASSERT_NOT_IMPLEMENTED();
		}

		// if not notification we need to tell game here.
		Packet* pPacket = allocPacket(8);
		pPacket->pData[0] = MessageID::ConnectionLost;
		pPacket->pSystemAddress = const_cast<ISystemAdd*>(target.pSystemAddress); // fuck
		pPacket->guid = guid;
		pushBackPacket(pPacket);
	}
}


// connection util
ConnectionState::Enum XPeer::getConnectionState(const AddressOrGUID systemIdentifier)
{
	if (systemIdentifier.isAddressValid())
	{
		// pending?
		const SystemAdd& sysAdd = *static_cast<const SystemAdd*>(systemIdentifier.pSystemAddress);

		auto matchSysAddFunc = [&sysAdd](const RequestConnection* pOth) {
			return pOth->systemAddress == sysAdd;
		};

		core::CriticalSection::ScopedLock lock(connectionReqsCS_);

		if (std::find_if(connectionReqs_.begin(), connectionReqs_.end(), matchSysAddFunc) != connectionReqs_.end()) {
			return ConnectionState::Pending;
		}
	}

	const RemoteSystem* pRemoteSys = getRemoteSystem(systemIdentifier, false);
	if (!pRemoteSys) {
		return ConnectionState::NotConnected;
	}

	return pRemoteSys->getConnectionState();
}

void XPeer::cancelConnectionAttempt(const ISystemAdd* pTarget)
{
	X_ASSERT_NOT_NULL(pTarget);
	const SystemAdd& sysAdd = *static_cast<const SystemAdd*>(pTarget);

	IPStr addStr;
	sysAdd.toString(addStr);

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Canceling Connection request to host: \"%s\"", addStr);

	removeConnectionRequest(sysAdd);
}


uint32_t XPeer::send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority,
	PacketReliability::Enum reliability, uint8_t orderingChannel, 
	const AddressOrGUID systemIdentifier, bool broadcast,
	uint32_t forceReceiptNumber)
{
	if (!lengthBytes) {
		return 0;
	}

	X_ASSERT_NOT_NULL(pData);

	uint32_t usedSendReceipt;

	if (forceReceiptNumber) {
		usedSendReceipt = forceReceiptNumber;
	}
	else {
		usedSendReceipt = incrementNextSendReceipt();
	}

	if (!broadcast && isLoopbackAddress(systemIdentifier, true))
	{
		sendLoopback(pData, lengthBytes);

		if (reliability == PacketReliability::UnReliableWithAck)
		{
			uint8_t tmpBuf[5];
			tmpBuf[0] = MessageID::SndReceiptAcked;
			std::memcpy(tmpBuf + 1, &usedSendReceipt, sizeof(usedSendReceipt));
			sendLoopback(tmpBuf, sizeof(tmpBuf));
		}

		return usedSendReceipt;
	}

	sendBuffered(
		pData, 
		safe_static_cast<BitSizeT>(lengthBytes * 8),
		priority, 
		reliability,
		orderingChannel,
		systemIdentifier, 
		broadcast, 
		usedSendReceipt
	);

	return usedSendReceipt;
}

uint32_t XPeer::send(const uint8_t* pData, const size_t lengthBytes, PacketPriority::Enum priority,
	PacketReliability::Enum reliability, const AddressOrGUID systemIdentifier)
{
	if (!lengthBytes) {
		return 0;
	}

	X_ASSERT_NOT_NULL(pData);

	uint32_t usedSendReceipt = incrementNextSendReceipt();

	if (isLoopbackAddress(systemIdentifier, true))
	{
		sendLoopback(pData, lengthBytes);

		if (reliability == PacketReliability::UnReliableWithAck)
		{
			uint8_t tmpBuf[5];
			tmpBuf[0] = MessageID::SndReceiptAcked;
			std::memcpy(tmpBuf + 1, &usedSendReceipt, sizeof(usedSendReceipt));
			sendLoopback(tmpBuf, sizeof(tmpBuf));
		}

		return usedSendReceipt;
	}

	sendBuffered(
		pData,
		safe_static_cast<BitSizeT>(lengthBytes * 8),
		priority,
		reliability,
		0,
		systemIdentifier,
		false,
		usedSendReceipt
	);

	return usedSendReceipt;
}

void XPeer::sendBuffered(const uint8_t* pData, BitSizeT numberOfBitsToSend, PacketPriority::Enum priority,
	PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t receipt)
{
	X_ASSERT(numberOfBitsToSend > 0, "Null request should not reach here")(numberOfBitsToSend);

	BufferdCommand* pCmd = allocBufferdCmd(BufferdCommand::Cmd::Send, numberOfBitsToSend);
	std::memcpy(pCmd->pData, pData, core::bitUtil::bitsToBytes(numberOfBitsToSend));
	pCmd->priority = priority;
	pCmd->reliability = reliability;
	pCmd->orderingChannel = orderingChannel;
	pCmd->broadcast = broadcast;
	pCmd->systemIdentifier = systemIdentifier;
	pCmd->receipt = receipt;

	bufferdCmds_.push(pCmd);
}

bool XPeer::sendImmediate(const uint8_t* pData, BitSizeT numberOfBitsToSend, PacketPriority::Enum priority,
	PacketReliability::Enum reliability, uint8_t orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast,
	core::TimeVal currentTime, uint32_t receipt)
{
	RemoteSystem* pRemoteSystem = getRemoteSystem(systemIdentifier, true);
	if (!pRemoteSystem) {
		X_ERROR("Net", "Failed to find remote system for send");
		return false;
	}
	
	if (!pRemoteSystem->canSend()) {
		X_WARNING_IF(vars_.debugEnabled(), "Net", "Tried to send data to remote, where sending is currently disabled");
		return false;
	}

	if (broadcast) {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	bool res = pRemoteSystem->sendReliabile(
		pData,
		numberOfBitsToSend,
		priority,
		reliability,
		orderingChannel,
		currentTime,
		receipt
	);

	return res;
}


void XPeer::closeConnectionInternal(const AddressOrGUID& systemIdentifier, bool sendDisconnectionNotification,
	bool performImmediate, uint8_t orderingChannel, PacketPriority::Enum notificationPriority)
{
	RemoteSystem* pRemoteSys = getRemoteSystem(systemIdentifier, true);
	if (!pRemoteSys) {
		X_ERROR("Net", "Failed to find remote system for connection close.");
		return;
	}

	if (sendDisconnectionNotification)
	{
		notifyAndFlagForShutdown(*pRemoteSys, performImmediate, orderingChannel, notificationPriority);
		return;
	}

	if (!performImmediate)
	{
		BufferdCommand* pCmd = allocBufferdCmd(BufferdCommand::Cmd::CloseConnection, 0);
		pCmd->priority = notificationPriority;
		pCmd->orderingChannel = orderingChannel;
		pCmd->systemIdentifier = systemIdentifier;
		bufferdCmds_.push(pCmd);
		return;
	}

	pRemoteSys->disconnect();
}

void XPeer::notifyAndFlagForShutdown(RemoteSystem& rs, bool imediate,
	uint8_t orderingChannel, PacketPriority::Enum notificationPriority)
{
	IPStr ipStr;
	X_LOG0_IF(vars_.debugEnabled(), "Net", "sending disconnectNotification to remoteSystem: \"%s\"", rs.systemAddress.toString(ipStr));

	core::FixedBitStream<core::FixedBitStreamStackPolicy<16>> bsOut;
	bsOut.write(MessageID::DisconnectNotification);

	core::TimeVal now = gEnv->pTimer->GetTimeNowReal();

	if (imediate) {
		rs.sendReliabile(
			bsOut,
			PacketPriority::Immediate,
			PacketReliability::ReliableOrdered,
			0,
			now
		);

		rs.connectState = ConnectState::DisconnectAsap;
	}
	else {
		send(
			bsOut.data(),
			bsOut.sizeInBytes(),
			PacketPriority::Immediate,
			PacketReliability::ReliableOrdered,
			0,
			AddressOrGUID(&rs.systemAddress),
			false
		);
	}
}



// -------------------------------------------

void XPeer::sendLoopback(const uint8_t* pData, size_t lengthBytes)
{
	Packet* pPacket = allocPacket(core::bitUtil::bytesToBits(lengthBytes));
	std::memcpy(pPacket->pData, pData, lengthBytes);
	pPacket->guid = getMyGUID();

	pushBackPacket(pPacket);
}

Packet* XPeer::receive(void)
{
	core::FixedBitStreamStack<MAX_MTU_SIZE> updateBS;
	core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();

	processRecvData(updateBS, timeNow);
	processConnectionRequests(updateBS, timeNow);
	processBufferdCommands(updateBS, timeNow);
	peerReliabilityTick(updateBS, timeNow);

	if (packetQue_.isEmpty()) {
		return nullptr;
	}

	Packet* pPacket = nullptr;
	if (packetQue_.tryPop(pPacket))
	{
		X_ASSERT_NOT_NULL(pPacket->pData);
		return pPacket;
	}

	return nullptr;
}

bool XPeer::isLoopbackAddress(const AddressOrGUID& systemIdentifier, bool matchPort) const
{
	if (systemIdentifier.netGuid == getMyGUID()) {
		X_ASSERT(getMyGUID() != UNASSIGNED_NET_GUID, "My guid should not be invalid")();
		return true;
	}

	if (!systemIdentifier.isAddressValid()) {
		return false;
	}

	const SystemAdd& sysAdd = *static_cast<const SystemAdd*>(systemIdentifier.pSystemAddress);

	if (matchPort)
	{
		for (const auto& local : ipList_) {
			if (local == sysAdd) {
				return true;
			}
		}
	}
	else
	{
		for (const auto& local : ipList_) {
			if (local.equalExcludingPort(sysAdd)) {
				return true;
			}
		}
	}

	// nope.
	return false;
}

const RemoteSystem* XPeer::getRemoteSystem(const AddressOrGUID systemIdentifier, bool onlyActive) const
{
	if (systemIdentifier.netGuid != UNASSIGNED_NET_GUID) {
		return getRemoteSystem(systemIdentifier.netGuid, onlyActive);
	}

	const SystemAdd* pSysAdd = static_cast<const SystemAdd*>(systemIdentifier.pSystemAddress);
	X_ASSERT_NOT_NULL(pSysAdd);
	return getRemoteSystem(*pSysAdd, onlyActive);
}

const RemoteSystem* XPeer::getRemoteSystem(const SystemAdd& systemAddress, bool onlyActive) const
{
	if (systemAddress == UNASSIGNED_SYSTEM_ADDRESS) {
		X_WARNING("Net", "Tried to get remote for unassigned address");
		return nullptr;
	}

	int32_t deadConIdx = -1;

	for (size_t i = 0; i < remoteSystems_.size(); i++)
	{
		auto& rs = remoteSystems_[i];
		if (rs.systemAddress == systemAddress)
		{
			if (rs.isActive)
			{
				return &rs;
			}
			else
			{
				// see if any active ones in list before returning this.
				deadConIdx = safe_static_cast<int32_t>(i);
			}
		}

		if (deadConIdx && !onlyActive) {
			return &remoteSystems_[deadConIdx];
		}
	}

	return nullptr;
}

const RemoteSystem* XPeer::getRemoteSystem(const NetGUID guid, bool onlyActive) const
{
	if (guid == UNASSIGNED_NET_GUID) {
		X_WARNING("Net", "Tried to get remote for unassigned guid");
		return nullptr;
	}

	for (auto& rs : remoteSystems_)
	{
		if (rs.guid == guid && (!onlyActive || rs.isActive)) {
			return &rs;
		}
	}

	return nullptr;
}



RemoteSystem* XPeer::getRemoteSystem(const AddressOrGUID systemIdentifier, bool onlyActive)
{
	if (systemIdentifier.netGuid != UNASSIGNED_NET_GUID) {
		return getRemoteSystem(systemIdentifier.netGuid, onlyActive);
	}

	const SystemAdd* pSysAdd = static_cast<const SystemAdd*>(systemIdentifier.pSystemAddress);
	X_ASSERT_NOT_NULL(pSysAdd);
	return getRemoteSystem(*pSysAdd, onlyActive);
}

RemoteSystem* XPeer::getRemoteSystem(const SystemAdd& systemAddress, bool onlyActive)
{
	if (systemAddress == UNASSIGNED_SYSTEM_ADDRESS) {
		X_WARNING("Net", "Tried to get remote for unassigned address");
		return nullptr;
	}

	int32_t deadConIdx = -1;

	for (size_t i = 0; i < remoteSystems_.size(); i++)
	{
		auto& rs = remoteSystems_[i];
		if (rs.systemAddress == systemAddress)
		{
			if (rs.isActive)
			{
				return &rs;
			}
			else
			{
				// see if any active ones in list before returning this.
				deadConIdx = safe_static_cast<int32_t>(i);
			}
		}

		if (deadConIdx && !onlyActive) {
			return &remoteSystems_[deadConIdx];
		}
	}

	return nullptr;
}

RemoteSystem* XPeer::getRemoteSystem(const NetGUID guid, bool onlyActive)
{
	if (guid == UNASSIGNED_NET_GUID) {
		X_WARNING("Net", "Tried to get remote for unassigned guid");
		return nullptr;
	}

	for (auto& rs : remoteSystems_)
	{
		if (rs.guid == guid && (!onlyActive || rs.isActive)) {
			return &rs;
		}
	}

	return nullptr;
}



size_t XPeer::getRemoteSystemIndex(const SystemAdd& systemAddress) const
{
	for (size_t i = 0; i < remoteSystems_.size(); i++)
	{
		auto& rs = remoteSystems_[i];

		if (rs.systemAddress == systemAddress)
		{
			return i;
		}
	}

	return std::numeric_limits<size_t>::max();
}

size_t XPeer::getRemoteSystemIndex(const NetGUID& guid) const
{
	for (size_t i = 0; i < remoteSystems_.size(); i++)
	{
		auto& rs = remoteSystems_[i];

		if (rs.guid == guid)
		{
			return i;
		}
	}

	return std::numeric_limits<size_t>::max();
}


size_t XPeer::getRemoteSystemIndex(const AddressOrGUID& systemIdentifier) const
{
	if (systemIdentifier.isAddressValid()) {
		return getRemoteSystemIndex(systemIdentifier.pSystemAddress);
	}

	return getRemoteSystemIndex(systemIdentifier.netGuid);
}


void XPeer::pushBackPacket(const RemoteSystem& rs, ReliabilityLayer::PacketData& data)
{
	// need to decide if i want to pass this back as up.
	auto up = std::move(data.getUP());

	// currently we only allow data to de returned that was allocated from the blockArena.
	// this is so i don't need to pass arena back.
	// and we know which arena to delete the data with when we get the packet back.
	X_ASSERT(up.getArena() == &blockArena_, "Should be block arena")(up.getArena());

	// want to take ownership of the data.
	Packet* pPacket = X_NEW(Packet, &poolArena_, "Packet");
	pPacket->pData = up.release();
	pPacket->bitLength = data.getNumbBits();
	pPacket->length = safe_static_cast<uint32_t>(core::bitUtil::bitsToBytes(data.getNumbBits()));
	pPacket->guid = rs.guid;

	packetQue_.push(pPacket);
}


Packet* XPeer::allocPacket(size_t lengthBits)
{
	Packet* pPacket = X_NEW(Packet, &poolArena_, "Packet");
	pPacket->pData = allocPacketData(core::bitUtil::bitsToBytes(lengthBits));
	pPacket->length = safe_static_cast<uint32_t>(core::bitUtil::bitsToBytes(lengthBits));
	pPacket->bitLength = safe_static_cast<uint32_t>(lengthBits);
	return pPacket;
}

void XPeer::freePacket(Packet* pPacket)
{
	freePacketData(pPacket->pData);
	X_DELETE(pPacket, &poolArena_);
}


BufferdCommand* XPeer::allocBufferdCmd(BufferdCommand::Cmd::Enum type, size_t lengthBits)
{
	BufferdCommand* pCmd = X_NEW(BufferdCommand, &poolArena_, "BufferdCmd");
	pCmd->cmd = type;
	pCmd->numberOfBitsToSend = safe_static_cast<BitSizeT>(lengthBits);

	if (lengthBits) {
		pCmd->pData = allocPacketData(core::bitUtil::bitsToBytes(lengthBits));
	}
	else {
		pCmd->pData = nullptr;
	}
	return pCmd;
}

void XPeer::freeBufferdCmd(BufferdCommand* pBufCmd)
{
	freePacketData(pBufCmd->pData);
	X_DELETE(pBufCmd, &poolArena_);
}

uint8_t* XPeer::allocPacketData(size_t lengthBytes)
{
	return X_NEW_ARRAY_ALIGNED(uint8_t, lengthBytes, &blockArena_, "PacketData", 16);
}

void XPeer::freePacketData(uint8_t* pPacketData)
{
	X_DELETE_ARRAY(pPacketData, &blockArena_);
}


RecvData* XPeer::allocRecvData(void)
{
	return X_NEW(RecvData, arena_, "RecvData");
}

void XPeer::freeRecvData(RecvData* pRecvData)
{
	X_DELETE(pRecvData, arena_);
}


RequestConnection* XPeer::allocConnectionRequest(void)
{
	return X_NEW(RequestConnection, arena_, "connectionReq");
}

void XPeer::freeConnectionRequest(RequestConnection* pConReq)
{
	X_DELETE(pConReq, arena_);
}



void XPeer::removeConnectionRequest(const SystemAdd& sysAdd)
{
	core::CriticalSection::ScopedLock lock(connectionReqsCS_);

	auto matchSysAddFunc = [&sysAdd](const RequestConnection* pOth) {
		return pOth->systemAddress == sysAdd;
	};

	auto it = std::find_if(connectionReqs_.begin(), connectionReqs_.end(), matchSysAddFunc);
	if (it != connectionReqs_.end())
	{
		freeConnectionRequest(*it);
		connectionReqs_.erase(it);
	}
	else
	{
		X_WARNING("Net", "Failed to find connection request for removal");
	}
}


// connection limits
void XPeer::setMaximumIncomingConnections(uint16_t numberAllowed)
{
	maxIncommingConnections_ = numberAllowed;

	X_LOG0_IF(vars_.debugEnabled() > 1, "Net", "Set maxIncomingconnections to: ^5%" PRIu16, numberAllowed);
}

uint16_t XPeer::numberOfConnections(void) const
{
	// number of open connections.
	uint16_t num = 0;

	for (const auto& rs : remoteSystems_)
	{
		if (rs.isActive && rs.connectState == ConnectState::Connected) {
			++num;
		}
	}

	return num;
}

// Ping 
void XPeer::ping(const ISystemAdd* pTarget)
{
	X_ASSERT_NOT_NULL(pTarget);
	const SystemAdd& sysAdd = *static_cast<const SystemAdd*>(pTarget);

	sendPing(sysAdd, PacketReliability::UnReliable, false);
}

bool XPeer::ping(const char* pHost, uint16_t remotePort, bool onlyReplyOnAcceptingConnections,
	uint32_t connectionSocketIndex)
{
	X_ASSERT(connectionSocketIndex < sockets_.size(), "Socket index out of range")(connectionSocketIndex, sockets_.size());

	core::TimeVal now = gEnv->pTimer->GetTimeNowReal();

	core::FixedBitStream<core::FixedBitStreamStackPolicy<64>> bsOut;
	if (onlyReplyOnAcceptingConnections) {
		bsOut.write(MessageID::UnConnectedPingOpenConnections);
	}
	else {
		bsOut.write(MessageID::UnConnectedPing);
	}
	bsOut.write(OFFLINE_MSG_ID);
	bsOut.write(now.GetValue());
	bsOut.write(guid_);

	NetSocket& socket = sockets_[connectionSocketIndex];
	SendParameters sp;
	sp.setData(bsOut);
	if (!sp.systemAddress.fromStringExplicitPort(pHost, remotePort, socket.getBoundAdd().getIPVersion())) {
		return false;
	}

	auto result = socket.send(sp);
	return result > 0;
}


void XPeer::sendPing(const SystemAdd& sysAdd, PacketReliability::Enum rel, bool imediate)
{
	core::FixedBitStream<core::FixedBitStreamStackPolicy<64>> bsOut;

	core::TimeVal now = gEnv->pTimer->GetTimeNowReal();
	bsOut.write(MessageID::ConnectedPing);
	bsOut.write(now.GetValue());

	if (imediate) {
		sendImmediate(
			bsOut,
			PacketPriority::Immediate,
			rel,
			0,
			AddressOrGUID(&sysAdd),
			false,
			now,
			0
		);
	}
	else {
		sendBuffered(
			bsOut,
			PacketPriority::Immediate,
			rel,
			0,
			AddressOrGUID(&sysAdd),
			false,
			0
		);
	}
}


// bans at connection level.
void XPeer::addToBanList(const IPStr& ip, core::TimeVal timeout)
{
	if (ip.isEmpty()) {
		X_WARNING("Net", "Passed empty ip to ban list");
		return;
	}

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Adding ban: \"%s\" timeout: %" PRIi64 "ms", ip, timeout.GetMilliSecondsAsInt64());

	core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();

	auto assignBanTime = [timeNow](Ban& ban, core::TimeVal timeout) {
		if (timeout.GetValue() == 0ll) {
			ban.timeOut.SetValue(0ll);
		}
		else {
			ban.timeOut = timeNow + timeout;
		}
	};

	for (auto& ban : bans_)
	{
		if (ban.ip == ip)
		{
			assignBanTime(ban, timeout);
			return;
		}
	}

	auto& ban = bans_.AddOne();
	ban.ip = ip;
	assignBanTime(ban, timeout);
}

void XPeer::removeFromBanList(const char* pIP)
{
	IPStr ip(pIP);

	if (ip.isEmpty()) {
		return;
	}

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Removing ban: \"%s\"", ip);
	
	auto findBanIP = [&ip](const Ban& oth) {
		return oth.ip == ip;
	};

	auto it = std::find_if(bans_.begin(), bans_.end(), findBanIP);
	if (it != bans_.end())
	{
		bans_.erase(it);
	} 
	else
	{
		X_LOG0_IF(vars_.debugEnabled(), "Net", "Failed to remove ban, no entry for \"%s\" found", ip);
	}
}

bool XPeer::isBanned(const char* pIP)
{
	return isBanned(IPStr(pIP));
}

bool XPeer::isBanned(const IPStr& ip)
{
	if (bans_.isEmpty()) {
		return false;
	}

	for (auto it = bans_.begin(); it != bans_.end(); /* ++it */)
	{
		auto& ban = (*it);

		if (ipWildMatch(ban.ip, ip)) 
		{
			// expired?
			if (ban.timeOut.GetValue() != 0)
			{
				core::TimeVal time = gEnv->pTimer->GetTimeNowReal();
				if (time > ban.timeOut)
				{
					it = bans_.erase(it);
					continue;
				}
			}

			return true;
		}

		++it;
	}

	return false;
}

void XPeer::clearBanList(void)
{
	bans_.clear();
}

void XPeer::listBans(void) const
{
	core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();

	for (const auto& ban : bans_)
	{
		int64_t msLeft = -1;

		if (ban.timeOut.GetValue() != 0)
		{
			msLeft = (ban.timeOut - timeNow).GetMilliSecondsAsInt64();
		}

		X_LOG0("Net", "Ban: \"%s\" timeLeftMS: ^5%" PRIi64, ban.ip.c_str(), msLeft);
	}
}

int32_t XPeer::getAveragePing(const AddressOrGUID systemIdentifier) const
{
	const RemoteSystem* pRemoteSys = getRemoteSystem(systemIdentifier, false);
	if (!pRemoteSys) {
		return -1;
	}

	int32_t sum = 0;
	int32_t num = 0;

	for (auto& ping : pRemoteSys->pings)
	{
		if (ping.isValid())
		{
			sum += ping.pingTime;
			++num;
		}
	}

	if (num) {
		return sum / num;
	}

	return -1;
}

int32_t XPeer::getLastPing(const AddressOrGUID systemIdentifier) const
{
	const RemoteSystem* pRemoteSys = getRemoteSystem(systemIdentifier, false);
	if (!pRemoteSys) {
		return -1;
	}

	return pRemoteSys->lowestPing;

}

int32_t XPeer::getLowestPing(const AddressOrGUID systemIdentifier) const
{
	const RemoteSystem* pRemoteSys = getRemoteSystem(systemIdentifier, false);
	if (!pRemoteSys) {
		return -1;
	}

	return pRemoteSys->pings[pRemoteSys->lastPingIdx].pingTime;
}



// MTU for a given system
int32_t XPeer::getMTUSize(const ISystemAdd* pTarget)
{
	if (pTarget) {
		const SystemAdd* pSysAdd = static_cast<const SystemAdd*>(pTarget);
		auto* pRemoteSys = getRemoteSystem(*pSysAdd, false);
	
		if (pRemoteSys) {
			return pRemoteSys->MTUSize;
		}

		X_WARNING("Net", "Failed to find remote system for MTU size returning default");
	}

	return defaultMTU_;
}


bool XPeer::getStatistics(const ISystemAdd* pTarget, NetStatistics& stats)
{
	X_ASSERT_NOT_NULL(pTarget);

	const SystemAdd* pSysAdd = static_cast<const SystemAdd*>(pTarget);
	auto* pRemoteSys = getRemoteSystem(*pSysAdd, false);

	if (!pRemoteSys) {
		X_WARNING("Net", "Failed to find remote system for stat retrival");
		return false;
	}

	stats.connectionStartTime = pRemoteSys->connectionTime;

	pRemoteSys->relLayer.getStatistics(stats);
	return true;
}


// ~IPeer


size_t XPeer::getNumRemoteInitiatedConnections(void) const
{
	size_t num = 0;

	for (const auto& rc : remoteSystems_)
	{
		if (rc.connectState == ConnectState::Connected && rc.isActive && !rc.weStartedconnection)
		{
			++num;
		}
	}

	return num;
}

void XPeer::listRemoteSystems(bool verbose) const
{
	auto timeNow = gEnv->pTimer->GetTimeNowReal();

	for (const auto& rs : remoteSystems_)
	{
		if (!rs.isActive) {
			continue;
		}

		auto connectionElapsed = timeNow - rs.connectionTime;
		
		IPStr ipStr;
		core::HumanDuration::Str durStr;

		if (!verbose)
		{
			X_LOG0("Net", "Host: \"%s\" connectionTime: ^5%s^7 state: ^5%s",
				rs.systemAddress.toString(ipStr),
				core::HumanDuration::toString(durStr, connectionElapsed.GetSeconds()),
				ConnectionState::ToString(rs.getConnectionState()));
		}
		else
		{
			core::HumanSize::Str sizeStr;

			NetStatistics stats;
			rs.relLayer.getStatistics(stats);

			X_LOG0("Net", "Host: \"%s\" connectionTime: ^5%s^7 state: ^5%s^7 memUsage: ^5%s",
				rs.systemAddress.toString(ipStr),
				core::HumanDuration::toString(durStr, connectionElapsed.GetSeconds()),
				ConnectionState::ToString(rs.getConnectionState()),
				core::HumanSize::toString(sizeStr, stats.internalMemUsage));
		}
	}
}


void XPeer::processConnectionRequests(UpdateBitStream& updateBS, core::TimeVal timeNow)
{
	// are we wanting to connect to some slutty peers?
	if (connectionReqs_.isNotEmpty())
	{
		core::CriticalSection::ScopedLock lock(connectionReqsCS_);

		for (auto it = connectionReqs_.begin(); it != connectionReqs_.end(); /* ++it */)
		{
			RequestConnection& cr = *(*it);

			// time for a reuest?
			if (cr.nextRequestTime > timeNow) {
				++it;
				continue;
			}

			IPStr addStr;

			// give up?
			if (cr.numRequestsMade == cr.retryCount)
			{
				X_LOG0_IF(vars_.debugEnabled(), "Net", "Reached max connection retry count for: \"%s\"", cr.systemAddress.toString(addStr));

				freeConnectionRequest(*it);
				it = connectionReqs_.erase(it);

				// send packet.
				Packet* pPacket = allocPacket(8);
				pPacket->pData[0] = MessageID::ConnectionRequestFailed;
				pPacket->pSystemAddress = nullptr; // fuck
				pushBackPacket(pPacket);
				continue;
			}

			X_LOG0_IF(vars_.debugEnabled(), "Net", "Dispatching open connection request(%" PRIu8 "): \"%s\"", 
				cr.numRequestsMade, cr.systemAddress.toString(addStr));

			++cr.numRequestsMade;
			cr.nextRequestTime = timeNow + cr.retryDelay;

			updateBS.reset();
			updateBS.write(MessageID::OpenConnectionRequest);
			updateBS.write(OFFLINE_MSG_ID);
			updateBS.write<uint8_t>(PROTO_VERSION_MAJOR);
			updateBS.write<uint8_t>(PROTO_VERSION_MINOR);

			// devide the mtu array by retry count, so that we try mtu index 0 for 1/3 of request if mtusizes is 3.
			size_t mtuIdx = cr.numRequestsMade / (cr.retryCount / MTUSizesArr.size());
			mtuIdx = core::Min(mtuIdx + cr.MTUIdxShift, MTUSizesArr.size() - 1);
			
			size_t MTUSize = MTUSizesArr[mtuIdx] - UDP_HEADER_SIZE;
			updateBS.zeroPadToLength(MTUSize);

			core::TimeVal timeSend = gEnv->pTimer->GetTimeNowReal();

			NetSocket& socket = sockets_[cr.socketIdx];
			SendParameters sp;
			sp.setData(updateBS);
			sp.systemAddress = cr.systemAddress;
			if (socket.send(sp) == -WSAEMSGSIZE) // A message sent on a datagram socket was larger than the internal message buffer 
			{
				// skip this MTU size.
				cr.MTUIdxShift++;
				cr.nextRequestTime = timeNow;

				X_LOG0_IF(vars_.debugEnabled(), "Net", "Moving to next MTU size for connection request");
			}
			else
			{
				core::TimeVal timeSendFin = gEnv->pTimer->GetTimeNowReal();
				core::TimeVal timeToSend = timeSendFin - timeSend;
				if (timeToSend > core::TimeVal::fromMS(100))
				{
					// if you took more than 100ms drop, to lowest MTU size.
					cr.MTUIdxShift = safe_static_cast<uint8_t>(MTUSizesArr.size() - 1);

					X_LOG0_IF(vars_.debugEnabled(), "Net", "Moving to last MTU size for connection request");
				}
			}

			++it;
		}
	}
}

void XPeer::processBufferdCommands(UpdateBitStream& updateBS, core::TimeVal timeNow)
{
	if (bufferdCmds_.isEmpty()) {
		return;
	}

	BufferdCommand* pBufCmd;
	while (bufferdCmds_.tryPop(pBufCmd))
	{
		X_ASSERT_NOT_NULL(pBufCmd); // no null ref plz!
		auto& cmd = *pBufCmd;

		if (cmd.cmd == BufferdCommand::Cmd::Send)
		{
			RemoteSystem* pRemoteSystem = getRemoteSystem(cmd.systemIdentifier, true);
			if (!pRemoteSystem) {
				return;
			}

			sendImmediate(
				cmd.pData,
				cmd.numberOfBitsToSend,
				cmd.priority,
				cmd.reliability,
				cmd.orderingChannel,
				cmd.systemIdentifier,
				cmd.broadcast,
				timeNow,
				cmd.receipt
			);
		}
		else if (cmd.cmd == BufferdCommand::Cmd::CloseConnection)
		{
			closeConnectionInternal(cmd.systemIdentifier, false, true, cmd.orderingChannel, cmd.priority);
		}
		else
		{
			X_ASSERT_UNREACHABLE();
		}


		freeBufferdCmd(pBufCmd);
	}
}

void XPeer::peerReliabilityTick(UpdateBitStream& updateBS, core::TimeVal timeNow)
{
	for (auto& rs : remoteSystems_)
	{
		if (!rs.isActive) {
			continue;
		}

		rs.relLayer.update(updateBS, *rs.pNetSocket, rs.systemAddress, rs.MTUSize, timeNow);

		const bool deadConnection = rs.relLayer.isConnectionDead();
		const bool disconnecting = rs.connectState == ConnectState::DisconnectAsap || rs.connectState == ConnectState::DisconnectAsapSilent;
		const bool disconnectingAfterAck = rs.connectState == ConnectState::DisconnectOnNoAck;
		const bool waitingforConnection = rs.connectState == ConnectState::RequestedConnection || 
										  rs.connectState == ConnectState::HandlingConnectionRequest ||
										  rs.connectState == ConnectState::UnverifiedSender;

		// has this connection not completed yet?
		const core::TimeVal dropCon = core::TimeVal::fromMS(vars_.dropPartialConnectionsMS());
		const bool connectionOpenTimeout = (waitingforConnection && timeNow > (rs.connectionTime + dropCon));
		const bool dissconectAckTimedOut = (disconnectingAfterAck && !rs.relLayer.isWaitingForAcks());
		const bool disconnectingNoData = disconnecting && !rs.relLayer.pendingOutgoingData();
		const bool socketClosed = false;

		if (deadConnection || disconnectingNoData || connectionOpenTimeout || dissconectAckTimedOut || socketClosed)
		{
			if (vars_.debugEnabled())
			{
				const char* pCloseReason = "<ukn>";

				if (deadConnection) {
					pCloseReason = "Connection timeout";
				}
				else if (disconnectingNoData) {
					pCloseReason = "Disconnection request";
				}
				else if (connectionOpenTimeout) {
					pCloseReason = "Partial connection timeout";
				}
				else if (dissconectAckTimedOut) {
					pCloseReason = "Discconect ack timeout";
				}
				else if (dissconectAckTimedOut) {
					pCloseReason = "Socket Closed";
				}

				IPStr ipStr;
				X_LOG0("Net", "Closing connection for remote system: \"%s\" reason: \"%s\"", rs.systemAddress.toString(ipStr), pCloseReason);
			}

			// ya cunt!
			Packet* pPacket = allocPacket(8);
			if (rs.connectState == ConnectState::RequestedConnection) {
				pPacket->pData[0] = MessageID::ConnectionRequestFailed;
			}
			else if (rs.connectState == ConnectState::Connected) {
				pPacket->pData[0] = MessageID::ConnectionLost;
			}
			else  {
				pPacket->pData[0] = MessageID::DisconnectNotification;
			}

			pPacket->pSystemAddress = nullptr; // fuck
			pPacket->guid = rs.guid;
			pushBackPacket(pPacket);


			closeConnectionInternal(AddressOrGUID(&rs.systemAddress), false, true, 0, PacketPriority::Low);
			continue;
		}

		if (rs.connectState == ConnectState::Connected && timeNow > rs.nextPingTime)
		{
			rs.nextPingTime = timeNow + core::TimeVal::fromMS(vars_.pingTimeMS());
			sendPing(rs.systemAddress, PacketReliability::UnReliable, true);
		}


		// do we have any packets?
		ReliabilityLayer::PacketData data(arena_);
		while (rs.relLayer.recive(data))
		{
			// okay we got a packet :D!
			// the first byte should be msgId.
			if (data.getNumbBits() < 8) {
				continue;
			}

			core::FixedBitStreamNoneOwning stream(data.begin(), data.end(), true);
			MessageID::Enum msgId = stream.read<MessageID::Enum>();

			if (msgId >= MessageID::ENUM_COUNT) {
				X_ERROR("Net", "Message contains invalid msgId: %" PRIi32, static_cast<int32_t>(msgId));
				continue;
			}

			X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived reliable messageId: \"%s\"", MessageID::ToString(msgId));

			updateBS.reset();

			// okay process the msg!
			switch (msgId)
			{
				case MessageID::ConnectionRequest:
					handleConnectionRequest(updateBS, stream, rs, timeNow);
					break;
				case MessageID::ConnectionRequestAccepted:
					handleConnectionRequestAccepted(updateBS, stream, rs, timeNow);
					break;
				case MessageID::ConnectionRequestHandShake:
					handleConnectionRequestHandShake(updateBS, stream, rs);
					break;

				case MessageID::ConnectedPing:
					handleConnectedPing(updateBS, stream, rs, timeNow);
					break;
				case MessageID::ConnectedPong:
					handleConnectedPong(updateBS, stream, rs);
					break;

				case MessageID::DisconnectNotification:
					handleDisconnectNotification(updateBS, stream, rs);
					break;

				case MessageID::InvalidPassword:
					handleInvalidPassword(updateBS, stream, rs);
					break;
					
				default:
					// we send this out.
					pushBackPacket(rs, data);
					break;
			}
		}
	}
}


void XPeer::processRecvData(UpdateBitStream& updateBS, core::TimeVal timeNow)
{
	X_UNUSED(timeNow);

	// use a single BS instance for processing all the data, to try improve cache hits.
	RecvData* pRecvData = nullptr;
	while (recvDataQue_.tryPop(pRecvData))
	{
		updateBS.reset();
		processRecvData(updateBS, pRecvData, 0);

		freeRecvData(pRecvData);
	}
}


void XPeer::processRecvData(core::FixedBitStreamBase& updateBS, RecvData* pData, int32_t byteOffset)
{
	X_ASSERT_NOT_NULL(pData);
	X_ASSERT_NOT_NULL(pData->pSrcSocket);
	X_ASSERT(pData->bytesRead > 0, "RecvData with no data should not reach here")(pData, pData->bytesRead);

	IPStr ipStr;
	pData->systemAdd.toString(ipStr, false);

	if (isBanned(ipStr))
	{
		// you fucking twat!
		updateBS.write(MessageID::ConnectionBanned);
		updateBS.write(OFFLINE_MSG_ID);
		updateBS.write(guid_);

		SendParameters sp;
		sp.setData(updateBS);
		sp.systemAddress = pData->systemAdd;
		pData->pSrcSocket->send(sp);
		return;
	}


	uint8_t* pBegin = (pData->data + byteOffset);
	uint8_t* pEnd = (pData->data + pData->bytesRead);
	X_ASSERT(pBegin < pEnd, "Stream is empty")(pBegin, pEnd);

	size_t dataLength = union_cast<size_t>(pEnd - pBegin);

	// is this a offline msg.
	if (dataLength == 1)
	{
		if (static_cast<MessageID::Enum>(*pBegin) == MessageID::SendTest) {
			return;
		}
	}
	else if (dataLength >= OFFLINE_MSG_ID.size())
	{
		if (std::memcmp(pBegin + 1, OFFLINE_MSG_ID.data(), OFFLINE_MSG_ID.size()) == 0)
		{
			processOfflineMsg(updateBS, pData, pBegin, pEnd);
			return;
		}
		// fall through
	}


	RemoteSystem* pRemoteSys = getRemoteSystem(pData->systemAdd, true);
	if (!pRemoteSys) {
		X_ERROR("Net", "Recived reliabile message for none connected client: \"%s\"", ipStr);

		// temp ban them.
		addToBanList(ipStr, core::TimeVal::fromMS(vars_.unexpectedMsgBanTime()));
		return;
	}

	bool res = pRemoteSys->relLayer.recv(
		pData->data + byteOffset,
		pData->bytesRead,
		*pData->pSrcSocket,
		pData->systemAdd,
		pData->timeRead,
		pRemoteSys->MTUSize
	);

	if (!res) {
		X_ERROR("Net", "Error processign recived packet");
	}
}

void XPeer::processOfflineMsg(UpdateBitStream& updateBS, RecvData* pData,
	uint8_t* pBegin, uint8_t* pEnd)
{
	core::FixedBitStreamNoneOwning stream(pBegin, pEnd, true);
	X_ASSERT(stream.sizeInBytes() > OFFLINE_MSG_ID.size(), "Called with too small buffer")(stream.sizeInBytes());

	MessageID::Enum msgId = stream.read<MessageID::Enum>();
	stream.skipBytes(OFFLINE_MSG_ID.size());
	X_ASSERT(msgId < MessageID::ENUM_COUNT, "Invalid msg id")(msgId);

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived offline messageId: \"%s\"", MessageID::ToString(msgId));

	switch (msgId)
	{
		case MessageID::ProtcolVersionIncompatible:
		case MessageID::ConnectionRequestFailed:
		case MessageID::ConnectionBanned:
		case MessageID::ConnectionNoFreeSlots:
		case MessageID::ConnectionRateLimited:
		case MessageID::InvalidPassword:
			handleConnectionFailure(updateBS, pData, stream, msgId);
			break;

		case MessageID::OpenConnectionRequest:
			handleOpenConnectionRequest(updateBS, pData, stream);
			break;
		case MessageID::OpenConnectionRequestStage2:
			handleOpenConnectionRequestStage2(updateBS, pData, stream);
			break;
		case MessageID::OpenConnectionResponse:
			handleOpenConnectionResponse(updateBS, pData, stream);
			break;
		case MessageID::OpenConnectionResponseStage2:
			handleOpenConnectionResponseStage2(updateBS, pData, stream);
			break;

			// hello.
		case MessageID::UnConnectedPing:
		case MessageID::UnConnectedPingOpenConnections:
			handleUnConnectedPing(updateBS, pData, stream, msgId == MessageID::UnConnectedPingOpenConnections);
			return;


			// hello.
		case MessageID::SendTest:
			return;

			// one day.
		case MessageID::StuNotFnd:
			return;

			break;
		default:
			X_ERROR("Net", "Unhandled message: \"%s\"", MessageID::ToString(msgId));
			break;
	}
}


void XPeer::handleConnectionFailure(UpdateBitStream& bsBuf, RecvData* pData, RecvBitStream& bs, MessageID::Enum failureType)
{
	NetGUID guid;
	bs.read(guid);

	Packet* pPacket = nullptr;

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived connection failure: \"%s\"", MessageID::ToString(failureType));

	// rip connection.
	if (failureType == MessageID::ConnectionRateLimited)
	{
		uint32_t waitMS = bs.read<uint32_t>();

		pPacket = allocPacket(8 + 32);
		std::memcpy(pPacket->pData + 1, &waitMS, sizeof(waitMS));	
	}
	else
	{
		pPacket = allocPacket(8);
	}

	// remove connection request.
	removeConnectionRequest(pData->systemAdd);

	pPacket->pData[0] = failureType;
	pPacket->pSystemAddress = nullptr; // fuck
	pPacket->guid = guid;
	pushBackPacket(pPacket);
}

void XPeer::handleOpenConnectionRequest(UpdateBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
{
	// hello, pleb.
	uint8_t protoVersionMajor = bs.read<uint8_t>();
	uint8_t protoVersionMinor = bs.read<uint8_t>();

	IPStr ipStr;
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived open connection request from \"%s\" with proto version: ^5%" PRIu8 ".%" PRIu8, 
		pData->systemAdd.toString(ipStr), protoVersionMinor, protoVersionMajor);


	if (protoVersionMajor != PROTO_VERSION_MAJOR || protoVersionMinor != PROTO_VERSION_MINOR)
	{
		// we don't support you.
		bsOut.write(MessageID::ProtcolVersionIncompatible);
		bsOut.write(OFFLINE_MSG_ID);
		bsOut.write<uint8_t>(PROTO_VERSION_MAJOR);
		bsOut.write<uint8_t>(PROTO_VERSION_MINOR);
		bsOut.write(guid_);

		SendParameters sp;
		sp.setData(bsOut);
		sp.systemAddress = pData->systemAdd;
		pData->pSrcSocket->send(sp);
		return;
	}
	
	// hey i like you!
	bsOut.write(MessageID::OpenConnectionResponse);
	bsOut.write(OFFLINE_MSG_ID);
	bsOut.write(guid_);
	bsOut.write<uint16_t>(MAX_MTU_SIZE); // i'll show you mine if you show me your's...

	SendParameters sp;
	sp.setData(bsOut);
	sp.systemAddress = pData->systemAdd;
	pData->pSrcSocket->send(sp);
}

void XPeer::handleOpenConnectionResponse(UpdateBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
{
	// hello almighty server.
	NetGUID serverGuid;
	uint16_t mtu;

	bs.read(serverGuid);
	bs.read(mtu);

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived open connection response");

	// find this fuck.
	core::CriticalSection::ScopedLock lock(connectionReqsCS_);

	for (auto& pReq : connectionReqs_)
	{
		if (pReq->systemAddress == pData->systemAdd)
		{
			// oh it was me, send stage2
			bsOut.write(MessageID::OpenConnectionRequestStage2);
			bsOut.write(OFFLINE_MSG_ID);
			bsOut.write(guid_);
			bsOut.write(pReq->systemAddress);
			bsOut.write<uint16_t>(MAX_MTU_SIZE); 

			SendParameters sp;
			sp.setData(bsOut);
			sp.systemAddress = pData->systemAdd;
			pData->pSrcSocket->send(sp);
			return;
		}
	}

	IPStr remoteStr;
	pData->systemAdd.toString(remoteStr);
	X_ERROR("Net", "Recived connection response for remote system we are not trying to connect to: \"%s\"", remoteStr);
}

void XPeer::handleOpenConnectionRequestStage2(UpdateBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
{
	// you stull here ? jesus christ.
	SystemAdd bindingAdd;
	NetGUID clientGuid;
	uint16_t mtu;

	bs.read(clientGuid);
	bs.read(bindingAdd);
	bs.read(mtu);

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived open connection request2");

	// right lets check the list.
	const RemoteSystem* pSys = getRemoteSystem(bindingAdd, true);
	const RemoteSystem* pSysGuid = getRemoteSystem(clientGuid, true);

	// response matrix:
	// IPAddrInUse GuidInUse	response
	// true		   true	 		already connected
	// false       true     	already connected.
	// true		   false	 	already connected
	// false	   false	 	allow connection

	if (pSys && pSysGuid)
	{
		// you stupid twat.
		bsOut.write(MessageID::AlreadyConnected);
		bsOut.write(OFFLINE_MSG_ID);
		bsOut.write(guid_);
	}
	else
	{
		if (!accpetingIncomingConnections())
		{
			// stuff like reserved slots needs to be handled outside network layer.
			bsOut.write(MessageID::ConnectionNoFreeSlots);
			bsOut.write(OFFLINE_MSG_ID);
			bsOut.write(guid_);
		}
		else
		{
			// is this peer a grade F twat?
			core::TimeVal lastConnectDelta;
			if (vars_.rlConnectionsPerIP() && isIpConnectSpamming(pData->systemAdd, &lastConnectDelta))
			{
				// you noob, can you even coun to 10?
				// NO!
				IPStr addStr;
				X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived connection req from \"%s\" %gms ago, rate limiting", 
					pData->systemAdd.toString(addStr), lastConnectDelta.GetMilliSeconds());


				// i don't care if this ratelimit is allmost over and you could technically connect again sooner.
				uint32_t timeMs = vars_.rlConnectionsPerIPBanTimeMS();

				bsOut.write(MessageID::ConnectionRateLimited);
				bsOut.write(OFFLINE_MSG_ID);
				bsOut.write(guid_);
				bsOut.write(timeMs);
			}
			else
			{
				auto* pRemoteSys = addRemoteSystem(pData->systemAdd, clientGuid, mtu, pData->pSrcSocket, bindingAdd, ConnectState::UnverifiedSender);
				X_UNUSED(pRemoteSys);

				bsOut.write(MessageID::OpenConnectionResponseStage2);
				bsOut.write(OFFLINE_MSG_ID);
				bsOut.write(guid_);
				bsOut.write(pData->systemAdd);
				bsOut.write<uint16_t>(mtu);

				// generate a nonce, for password if requried.
				core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();
				core::Hash::SHA1 hash;
				hash.update(timeNow);
				for (size_t i = 0; i < 16; i++) {
					hash.update(core::random::MultiplyWithCarry());
				}
				pRemoteSys->nonce = hash.finalize();

				bsOut.write(pRemoteSys->nonce);
			}
		}
	}

	SendParameters sp;
	sp.setData(bsOut);
	sp.systemAddress = pData->systemAdd;
	pData->pSrcSocket->send(sp);
}

void XPeer::handleOpenConnectionResponseStage2(UpdateBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
{
	// meow.
	// if we here the response was valid annd == OpenConnectionResponseStage2
	SystemAdd bindingAdd;
	NetGUID clientGuid;
	uint16_t mtu;
	core::Hash::SHA1Digest nonce;

	bs.read(clientGuid);
	bs.read(bindingAdd);
	bs.read(mtu);
	bs.read(nonce);

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived connection response2");


	// find it.
	{
		core::CriticalSection::ScopedLock lock(connectionReqsCS_);

		for (auto& pReq : connectionReqs_)
		{
			if (pReq->systemAddress == pData->systemAdd)
			{
				RemoteSystem* pSys = getRemoteSystem(bindingAdd, true);
				if (!pSys)
				{
					// add systen
					pSys = addRemoteSystem(pData->systemAdd, clientGuid, mtu, pData->pSrcSocket, bindingAdd, ConnectState::UnverifiedSender);
				}
				if (pSys)
				{
					pSys->connectState = ConnectState::RequestedConnection;
					pSys->weStartedconnection = true;

					core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();

					// send connectionRequest now.
					bsOut.write(MessageID::ConnectionRequest);
					bsOut.write(guid_);
					bsOut.write(timeNow.GetValue());
					bsOut.write(pReq->password.isNotEmpty());
					if (pReq->password.isNotEmpty()) 
					{
						core::Hash::SHA1 hash;
						hash.update(nonce);
						hash.update(pReq->password.c_str(), pReq->password.length());
						auto digest = hash.finalize();

						bsOut.writeAligned(digest);
					}

					// send the request to the remote.
					pSys->sendReliabile(
						bsOut, 
						PacketPriority::Immediate, 
						PacketReliability::Reliable,
						0, 
						timeNow
					);
				}
				else
				{
					// failed to add remote sys.
					X_ERROR("Net", "Failed to add new remote system to internal list");

					Packet* pPacket = allocPacket(8);
					pPacket->pData[0] = MessageID::ConnectionRequestFailed;
					pPacket->pSystemAddress = nullptr; // fuck
					pPacket->guid = clientGuid;
					pushBackPacket(pPacket);
				}
			}
		}
	}

	// remove the req.
	removeConnectionRequest(pData->systemAdd);
}


void XPeer::handleUnConnectedPing(UpdateBitStream& bsOut, RecvData* pData, RecvBitStream& bs, bool openConnectionsRequired)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived unConnectedPing");

	if (openConnectionsRequired && !accpetingIncomingConnections()) {
		X_LOG0_IF(vars_.debugEnabled(), "Net", "Ignoring ping, not accepting incoming connections");
		return;
	}

	int64_t timeStamp;
	NetGUID clientGuid;

	bs.read(timeStamp);
	bs.read(clientGuid);

	// respond.
	bsOut.write(MessageID::UnConnectedPong);
	bsOut.write(timeStamp);
	bsOut.write(guid_);

	SendParameters sp;
	sp.setData(bsOut);
	sp.systemAddress = pData->systemAdd;
	pData->pSrcSocket->send(sp);
}

void XPeer::handleUnConnectedPong(UpdateBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived unConnectedPong");

	int64_t timeStamp;
	NetGUID clientGuid;

	bs.read(timeStamp);
	bs.read(clientGuid);

	// tell the game.
	Packet* pPacket = allocPacket(8 + sizeof(int64_t));

	core::FixedBitStream<core::FixedBitStreamNoneOwningPolicy> packetBs(pPacket->pData, pPacket->pData + pPacket->length, false);
	packetBs.write(MessageID::UnConnectedPong);
	packetBs.write(timeStamp);

	pPacket->pSystemAddress = nullptr;
	pPacket->guid = clientGuid;
	pushBackPacket(pPacket);
}

// ----------------------------------------------------

void XPeer::handleConnectionRequest(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs, core::TimeVal timeNow)
{
	NetGUID clientGuid;
	int64_t timeStamp;
	bool passwordInc;

	// must be a unverified sender trying to connect.
	if (rs.connectState != ConnectState::UnverifiedSender)
	{
		X_ERROR("Net", "Recived unexpected connection request from client, ignoring.");
		// ban the slut :D !
		closeConnectionInternal(AddressOrGUID(&rs.systemAddress), false, true, 0, PacketPriority::Low);

		IPStr ipStr;
		rs.systemAddress.toString(ipStr);
		addToBanList(ipStr, core::TimeVal::fromMS(vars_.unexpectedMsgBanTime()));
		return;
	}

	bs.read(clientGuid);
	bs.read(timeStamp);
	bs.read(passwordInc);

	core::Hash::SHA1Digest passwordDisget;
	if (passwordInc) {
		bs.readAligned(passwordDisget);
	}

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived connection request. timeStamp: %" PRId64, timeStamp);

	// optional don't allow clients that give us a password when one not required.
	const bool clientMustNotSendPassword = !vars_.ignorePasswordFromClientIfNotRequired();
	// if this is true will force the hash check, which will fail.
	const bool invalidPassord = (passwordInc && clientMustNotSendPassword);


	if (password_.isNotEmpty() || invalidPassord)
	{
		core::Hash::SHA1 hash;
		hash.update(rs.nonce);
		hash.update(password_.c_str(), password_.length());
		auto serverPassDigest = hash.finalize();

		if (passwordDisget != serverPassDigest)
		{
			bsOut.write(MessageID::InvalidPassword);
			bsOut.write(guid_);

			rs.sendReliabile(
				bsOut,
				PacketPriority::Immediate,
				PacketReliability::Reliable,
				0,
				timeNow
			);

			rs.connectState = ConnectState::DisconnectAsapSilent;
			return;
		}
	}

	// hey hey!
	rs.connectState = ConnectState::HandlingConnectionRequest;

	bsOut.write(MessageID::ConnectionRequestAccepted);
	bsOut.write(rs.systemAddress);
	bsOut.write<uint8_t>(safe_static_cast<uint8_t>(ipList_.size()));
	for (auto& ip : ipList_) {
		ip.writeToBitStream(bsOut);
	}
	bsOut.write(timeStamp);
	bsOut.write(timeNow.GetValue());

	rs.sendReliabile(
		bsOut,
		PacketPriority::Immediate,
		PacketReliability::Reliable,
		0,
		timeNow
	);
}


void XPeer::handleConnectionRequestAccepted(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs, core::TimeVal timeNow)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived connection request accepted");

	if (rs.connectState != ConnectState::RequestedConnection)
	{
		X_ERROR("Net", "Recived unexpected connection request accept, ignoring.");
		return;
	}

	// mmmm
	SystemAdd externalSysId;
	uint8_t numInternal = 0;
	RemoteSystem::SystemAddArr localIps;
	int64_t sendPingTime;
	int64_t sendPongTime;

	bs.read(externalSysId);
	bs.read(numInternal);
	X_ASSERT(numInternal < localIps.capacity(), "Peer sent too many internal addresses")(numInternal, localIps.capacity());

	localIps.resize(numInternal);
	for (size_t i = 0; i < numInternal; i++) {
		localIps[i].fromBitStream(bs);
	}
	bs.read(sendPingTime);
	bs.read(sendPongTime);

	// welcome to the club.
	rs.onConnected(externalSysId, localIps, core::TimeVal(sendPingTime), core::TimeVal(sendPongTime));

	// --------- Lets shake on it.. -------------

	bsOut.write(MessageID::ConnectionRequestHandShake);
	bsOut.write(rs.systemAddress);
	bsOut.write<uint8_t>(safe_static_cast<uint8_t>(ipList_.size()));
	for (auto& ip : ipList_) {
		ip.writeToBitStream(bsOut);
	}
	bsOut.write(sendPongTime);
	bsOut.write(timeNow.GetValue());

	rs.sendReliabile(
		bsOut,
		PacketPriority::Immediate,
		PacketReliability::Reliable,
		0,
		timeNow
	);

	sendPing(rs.systemAddress, PacketReliability::UnReliable, true);

	// tell the game.
	Packet* pPacket = allocPacket(8);
	pPacket->pData[0] = MessageID::ConnectionRequestAccepted;
	pPacket->pSystemAddress = &rs.systemAddress; // fuck
	pPacket->guid = rs.guid;
	pushBackPacket(pPacket);
}

void XPeer::handleConnectionRequestHandShake(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived connection request handshake");

	if (rs.connectState != ConnectState::HandlingConnectionRequest)
	{
		X_ERROR("Net", "Recived unexpected connection request handshake, ignoring.");
		return;
	}

	// same shit as above, my face is twitching a little due to the repeating of logic :) (OCD)
	SystemAdd externalSysId;
	uint8_t numInternal = 0;
	RemoteSystem::SystemAddArr localIps;
	int64_t sendPingTime;
	int64_t sendPongTime;

	bs.read(externalSysId);
	bs.read(numInternal);
	X_ASSERT(numInternal < localIps.capacity(), "Peer sent too many internal addresses")(numInternal, localIps.capacity());
	localIps.resize(numInternal);

	for (size_t i = 0; i < numInternal; i++) {
		localIps[i].fromBitStream(bs);
	}
	bs.read(sendPingTime); 
	bs.read(sendPongTime); 

	rs.onConnected(externalSysId, localIps, core::TimeVal(sendPingTime), core::TimeVal(sendPongTime));

	sendPing(rs.systemAddress, PacketReliability::UnReliable, true);

	// tell the game.
	Packet* pPacket = allocPacket(8);
	pPacket->pData[0] = MessageID::ConnectionRequestHandShake;
	pPacket->pSystemAddress = &rs.systemAddress; // fuck
	pPacket->guid = rs.guid;
	pushBackPacket(pPacket);
}

// ----------------------------------

void XPeer::handleConnectedPing(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs, core::TimeVal timeNow)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived ping");

	int64_t timeStamp;
	bs.read(timeStamp);

	// respond.
	bsOut.write(MessageID::ConnectedPong);
	bsOut.write(timeStamp);
	bsOut.write(timeNow);

	rs.sendReliabile(
		bsOut,
		PacketPriority::Immediate,
		PacketReliability::UnReliable,
		0,
		timeNow
	);
}


void XPeer::handleConnectedPong(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived pong");

	int64_t sendPingTime;
	int64_t sendPongTime;

	bs.read(sendPingTime);
	bs.read(sendPongTime);

	rs.onPong(core::TimeVal(sendPingTime), core::TimeVal(sendPongTime));
}


void XPeer::handleDisconnectNotification(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived Desconnect notification");

	// if ack never sends disconnect.
	rs.connectState = ConnectState::DisconnectOnNoAck;
}


void XPeer::handleInvalidPassword(UpdateBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived invalid password notification");

	// tell the game.
	Packet* pPacket = allocPacket(8);
	pPacket->pData[0] = MessageID::InvalidPassword;
	pPacket->pSystemAddress = &rs.systemAddress; // fuck
	pPacket->guid = rs.guid;
	pushBackPacket(pPacket);

	rs.connectState = ConnectState::DisconnectAsapSilent;
}


// ----------------------------------



// -------------------------------------------

RemoteSystem* XPeer::addRemoteSystem(const SystemAdd& sysAdd, NetGUID guid, int32_t remoteMTU, 
	NetSocket* pSrcSocket, SystemAdd bindingAdd, ConnectState::Enum state)
{
	// hello hello.
	IPStr ipStr;
	NetGuidStr guidStr;

	X_LOG0_IF(vars_.debugEnabled(), "Net", "^6Adding remote system^7 sysAdd: \"%s\" guid: ^5%s",
		sysAdd.toString(ipStr), guid.toString(guidStr));

	for (auto& remoteSys : remoteSystems_)
	{
		if (!remoteSys.isActive)
		{
			core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();

			remoteSys.systemAddress = sysAdd;
			remoteSys.myExternalSystemAddress = UNASSIGNED_SYSTEM_ADDRESS;
			remoteSys.thierInternalSystemAddress.clear();

			remoteSys.nextPingTime = core::TimeVal(0ll);
			remoteSys.lastReliableSend = timeNow;
			remoteSys.connectionTime = timeNow;

			remoteSys.pings.fill(PingAndClockDifferential());
			remoteSys.lastPingIdx = 0;

			remoteSys.relLayer.reset(remoteMTU);

			remoteSys.guid = guid;
			remoteSys.lowestPing = UNDEFINED_PING;
			remoteSys.MTUSize = remoteMTU;
			remoteSys.connectState = state;
			remoteSys.pNetSocket = pSrcSocket;
			remoteSys.isActive = true; // diverts all packets to reliabilty layer.

			if (pSrcSocket->getBoundAdd() != bindingAdd)
			{
				// print warning.
				X_WARNING("Net", "Binding address is diffrent to source socket");
			}

			return &remoteSys;
		}
	}

	// warning?
	return nullptr;
}

bool XPeer::isIpConnectSpamming(const SystemAdd& sysAdd, core::TimeVal* pDeltaOut)
{
	for (auto& remoteSys : remoteSystems_)
	{
		if (remoteSys.isActive && remoteSys.systemAddress.equalExcludingPort(sysAdd))
		{
			core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();
			core::TimeVal connectionAttemptDelta = timeNow - remoteSys.connectionTime;
			core::TimeVal rateLimitThresh = core::TimeVal::fromMS(vars_.rlConnectionsPerIPThreshMS());

			if (connectionAttemptDelta < rateLimitThresh)
			{
				if (pDeltaOut) {
					*pDeltaOut = connectionAttemptDelta;
				}

				return true;
			}
		}
	}

	return false;
}

core::Thread::ReturnValue XPeer::socketRecvThreadProc(const core::Thread& thread)
{
	NetSocket* pSocket = reinterpret_cast<NetSocket*>(thread.getData());
	X_ASSERT_NOT_NULL(pSocket);

	RecvData* pData = allocRecvData();

	while (thread.ShouldRun())
	{
		auto res = pSocket->recv(*pData);

		if (res == RecvResult::Success)
		{
			if (pData->bytesRead > 0)
			{
				onSocketRecv(pData);
				pData = allocRecvData();
			}
			else
			{
				core::Thread::Sleep(0);
			}
		}
		else if (res == RecvResult::ConnectionReset)
		{
			// okay so we know a socket has been closed we don't need to wait for timeout.
			// we send buffered as we on diffrent thread.
			closeConnectionInternal(AddressOrGUID(&pData->systemAdd), false, false, 0);
		}
		else if (res == RecvResult::Error)
		{
			// ... 
			// for now ignore, have to see what errors we get and see if want to handle them.
			// they will get logged by socket code.
		}
	}

	freeRecvData(pData);
	return core::Thread::ReturnValue(0);
}

bool XPeer::populateIpList(void)
{
	if (!NetSocket::getMyIPs(ipList_)) {
		return false;
	}

	return true;
}


X_NAMESPACE_END

