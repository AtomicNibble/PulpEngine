#include "stdafx.h"
#include "XPeer.h"
#include "XNet.h"

#include <Memory\VirtualMem.h>
#include <ITimer.h>

#include "Sockets\Socket.h"

X_NAMESPACE_BEGIN(net)

namespace
{

	template<typename T>
	BitSizeT bytesToBits(T bytes)
	{
		return safe_static_cast<BitSizeT>(core::bitUtil::bytesToBits(bytes));
	}

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

	static const size_t POOL_ALLOCATION_SIZE = core::Max(sizeof(BufferdCommand), sizeof(Packet));
	static const size_t POOL_ALLOCATION_ALIGN =  core::Max(X_ALIGN_OF(BufferdCommand), X_ALIGN_OF(Packet));

} // namespace 

PingAndClockDifferential::PingAndClockDifferential()
{
	pingTime = UNDEFINED_PING;
}

// -----------------------------------

RemoteSystem::RemoteSystem(NetVars& vars, core::MemoryArenaBase* arena, core::MemoryArenaBase* packetPool) :
	relLayer(vars, arena, packetPool)
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

void RemoteSystem::disconnect(void)
{
	isActive = false;
	weStartedconnection = false;

	connectState = ConnectState::NoAction;
	lowestPing = UNDEFINED_PING;
	MTUSize = MAX_MTU_SIZE;

	pNetSocket = nullptr;

	guid = UNASSIGNED_NET_GUID;
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
			PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE) * MAX_POOL_ALLOC,
			core::VirtualMem::GetPageSize()
		)
	),
	poolAllocator_(poolHeap_.start(), poolHeap_.end(),
		PoolArena::getMemoryRequirement(POOL_ALLOCATION_SIZE),
		PoolArena::getMemoryAlignmentRequirement(POOL_ALLOCATION_ALIGN),
		PoolArena::getMemoryOffsetRequirement()
	),
	poolArena_(&poolAllocator_, "PoolArena"),
	blockArena_(&blockAlloc_, "blockArena")
{
	remoteSystems_.getAllocator().setBaseAlignment(16);
	remoteSystems_.setGranularity(1);

	sockets_.setGranularity(4);

	guid_ = XNet::generateGUID();

	connectionRateLimitTime_ = core::TimeVal::fromMS(500);
	defaultTimeOut_ = core::TimeVal::fromMS(1000);
	unreliableTimeOut_ = core::TimeVal::fromMS(1000 * 10);

	defaultMTU_ = MAX_MTU_SIZE;
	maxIncommingConnections_ = 0;
	maxPeers_ = 0;

}

XPeer::~XPeer()
{

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
		core::MemoryArenaBase* packetPool = arena_;

		remoteSystems_.resize(maxPeers_, RemoteSystem(vars_, arena_, packetPool));
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
		
	// X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived messageId: \"%s\"", MessageID::ToString(msgId));

	if (vars_.debugEnabled() > 1)
	{
		NetGuidStr guidStr;
		X_LOG0("Net", "ProtoVersion: ^5%" PRIu8 ".%" PRIu8, PROTO_VERSION_MAJOR, PROTO_VERSION_MINOR);
		X_LOG0("Net", "Max peers: ^5%" PRIi32, maxPeers_);
		X_LOG0("Net", "Max incomming connections: ^5%" PRIi32, maxIncommingConnections_);
		X_LOG0("Net", "GUID: ^5%s", guid_.toString(guidStr));
		X_LOG0("Net", "Listening on ^5%" PRIuS " endpoints", sockets_.size());
		X_LOG_BULLET;

		for (auto& s : sockets_)
		{
			IPStr boundAddStr;
			s.getBoundAdd().toString(boundAddStr);

			X_LOG0("Net", "bound address: \"%s\"", boundAddStr.c_str());

		}
	}


	return StartupResult::Started;
}

void XPeer::shutdown(core::TimeVal blockDuration, uint8_t orderingChannel,
	PacketPriority::Enum disconnectionNotificationPriority)
{

}


// connection api
ConnectionAttemptResult::Enum XPeer::connect(const char* pHost, Port remotePort, uint32_t retryCount,
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

	RequestConnection* pConReq = X_NEW(RequestConnection, arena_, "ConRequest");
	pConReq->systemAddress = systemAddress;
	pConReq->nextRequestTime = gEnv->pTimer->GetTimeNowReal();
	pConReq->timeoutTime = timeoutTime;
	pConReq->retryDelay = retryDelay;
	pConReq->numRequestsMade = 0;
	pConReq->retryCount = retryCount;
	pConReq->socketIdx = socketIdx;
	pConReq->MTUIdxShift = 0;

	// only push if not trying to connect already.
	auto matchSysAddFunc = [&systemAddress](const RequestConnection* pOth) {
		return pOth->systemAddress == systemAddress;
	};

	core::CriticalSection::ScopedLock lock(connectionReqsCS_);

	if (std::find_if(connectionReqs_.begin(), connectionReqs_.end(), matchSysAddFunc) != connectionReqs_.end()) {
		X_DELETE(pConReq, arena_);
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

	if (!pRemoteSys->isActive) {
		return ConnectionState::Disconnected;
	}

	static_assert(ConnectState::ENUM_COUNT == 8, "Additional states? this logic needs updating");
	switch (pRemoteSys->connectState)
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
			X_ASSERT_NOT_IMPLEMENTED();
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
		// TODO
		return false;
	}
	
	if (!pRemoteSystem->canSend()) {
		// TODO
		return false;
	}

	if (broadcast) {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	pRemoteSystem->relLayer.send(
		pData,
		numberOfBitsToSend,
		currentTime,
		pRemoteSystem->MTUSize,
		priority,
		reliability,
		orderingChannel,
		receipt
	);

	return true;
}

void XPeer::closeConnectionInternal(const AddressOrGUID& systemIdentifier, bool sendDisconnectionNotification,
	bool performImmediate, uint8_t orderingChannel, PacketPriority::Enum notificationPriority)
{

	if (sendDisconnectionNotification)
	{

		return;
	}

	if (performImmediate)
	{
		BufferdCommand* pCmd = allocBufferdCmd(BufferdCommand::Cmd::CloseConnection, 0);
		pCmd->priority = notificationPriority;
		pCmd->orderingChannel = orderingChannel;
		pCmd->systemIdentifier = systemIdentifier;
		bufferdCmds_.push(pCmd);
		return;
	}

	RemoteSystem* pRemoteSys = getRemoteSystem(systemIdentifier, true);
	if (!pRemoteSys) {

		return;
	}

	pRemoteSys->disconnect();
}

// -------------------------------------------

void XPeer::processBufferdCommand(BufferdCommand& cmd)
{
	RemoteSystem* pRemoteSystem = getRemoteSystem(cmd.systemIdentifier, true);
	if (!pRemoteSystem) {
		return;
	}

	if (!pRemoteSystem->canSend()) {
		return;
	}

	if (cmd.broadcast) {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	pRemoteSystem->relLayer.send(
		cmd.pData,
		cmd.numberOfBitsToSend,
		core::TimeVal(),
		pRemoteSystem->MTUSize,
		cmd.priority,
		cmd.reliability,
		cmd.orderingChannel,
		cmd.receipt
	);
}


// -------------------------------------------

void XPeer::sendLoopback(const uint8_t* pData, size_t lengthBytes)
{
	Packet* pPacket = allocPacket(bytesToBits(lengthBytes));
	std::memcpy(pPacket->pData, pData, lengthBytes);
	pPacket->guid = getMyGUID();

	pushBackPacket(pPacket, false);
}

Packet* XPeer::receive(void)
{
	processRecvData();
	processConnectionRequests();
	processBufferdCommands();
	peerReliabilityTick();

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


void XPeer::pushBackPacket(Packet* pPacket, bool pushAtHead)
{
	X_ASSERT_NOT_NULL(pPacket);

	if (pushAtHead) {
		X_ASSERT_NOT_IMPLEMENTED();
	}

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

void XPeer::freebufferdCmd(BufferdCommand* pBufCmd)
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

uint32_t XPeer::nextSendReceipt(void)
{
	return sendReceiptSerial_;
}

uint32_t XPeer::incrementNextSendReceipt(void)
{
	return ++sendReceiptSerial_;
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
		X_DELETE(*it, arena_);
		connectionReqs_.erase(it);
	}
}


// connection limits
void XPeer::setMaximumIncomingConnections(uint16_t numberAllowed)
{
	maxIncommingConnections_ = numberAllowed;

	X_LOG0_IF(vars_.debugEnabled() > 1, "Net", "Set maxIncomingconnections to: ^5%" PRIu16, numberAllowed);
}

uint16_t XPeer::getMaximumIncomingConnections(void) const
{
	return maxIncommingConnections_;
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

uint32_t XPeer::getMaximunNumberOfPeers(void) const
{
	return maxPeers_;
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

	socket.send(sp);
	return false;
}


void XPeer::sendPing(const SystemAdd& sysAdd, PacketReliability::Enum rel, bool imediate)
{
	core::FixedBitStream<core::FixedBitStreamStackPolicy<64>> bsOut;

	core::FixedStreamBase* pPlz = &bsOut;
	
	core::TimeVal now = gEnv->pTimer->GetTimeNowReal();
	bsOut.write(MessageID::ConnectedPing);
	bsOut.write(now.GetValue());

	if (imediate) {
		sendImmediate(
			bsOut.data(),
			safe_static_cast<BitSizeT>(bsOut.size()),
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
		send(
			bsOut.data(),
			bsOut.sizeInBytes(),
			PacketPriority::Immediate,
			rel,
			0,
			AddressOrGUID(&sysAdd),
			false
		);
	}
}


// bans at connection level.
void XPeer::addToBanList(const char* pIP, core::TimeVal timeout)
{
	IPStr ip(pIP);

	if (ip.isEmpty()) {
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

	for (auto& ban : bans_) {
		if (ipWildMatch(ban.ip, ip)) {
			return true;
		}
	}
	return false;
}

void XPeer::clearBanList(void)
{
	bans_.clear();
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


const NetGUID& XPeer::getMyGUID(void) const
{
	return guid_;
}


void XPeer::setTimeoutTime(core::TimeVal time, const ISystemAdd* pTarget)
{
	X_ASSERT_NOT_NULL(pTarget);

}

core::TimeVal XPeer::getTimeoutTime(const ISystemAdd* pTarget)
{
	if (pTarget) {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return defaultTimeOut_;
}

void XPeer::setUnreliableTimeout(core::TimeVal timeout)
{
	unreliableTimeOut_ = timeout;
}

void XPeer::setConnectionRateLimit(core::TimeVal time)
{
	connectionRateLimitTime_ = time;
}

bool XPeer::accpetingIncomingConnections(void) const
{
	return getNumRemoteInitiatedConnections() < getMaximumIncomingConnections();
}

size_t XPeer::getNumRemoteInitiatedConnections(void) const
{
	size_t num = 0;

	for (auto rc : remoteSystems_)
	{
		if (rc.connectState == ConnectState::Connected && rc.isActive && !rc.weStartedconnection)
		{
			++num;
		}
	}

	return num;
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

	return false;
}


// ~IPeer

void XPeer::processRecvData(void)
{
	RecvData* pRecvData = nullptr;
	while (recvDataQue_.tryPop(pRecvData))
	{
		// OFFLINE_MSG_ID

		processRecvData(pRecvData, 0);

		freeRecvData(pRecvData);
	}
}


void XPeer::processConnectionRequests(void)
{
	// are we wanting to connect to some slutty peers?
	if (connectionReqs_.isNotEmpty())
	{
		core::CriticalSection::ScopedLock lock(connectionReqsCS_);

		core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();

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

			uint8_t buf[MAX_MTU_SIZE];
			FixedBitStream bsOut(buf, buf + sizeof(buf), false);

			bsOut.write(MessageID::OpenConnectionRequest);
			bsOut.write(OFFLINE_MSG_ID);
			bsOut.write<uint8_t>(PROTO_VERSION_MAJOR);
			bsOut.write<uint8_t>(PROTO_VERSION_MINOR);

			// devide the mtu array by retry count, so that we try mtu index 0 for 1/3 of request if mtusizes is 3.
			size_t mtuIdx = cr.numRequestsMade / (cr.retryCount / MTUSizesArr.size());
			mtuIdx = core::Min(mtuIdx + cr.MTUIdxShift, MTUSizesArr.size() - 1);
			
			size_t MTUSize = MTUSizesArr[mtuIdx] - UDP_HEADER_SIZE;
			bsOut.zeroPadToLength(MTUSize);

			core::TimeVal timeSend = gEnv->pTimer->GetTimeNowReal();

			NetSocket& socket = sockets_[cr.socketIdx];
			SendParameters sp;
			sp.setData(bsOut);
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

void XPeer::processBufferdCommands(void)
{
	if (bufferdCmds_.isEmpty()) {
		return;
	}

	BufferdCommand* pBufCmd;
	while (bufferdCmds_.tryPop(pBufCmd))
	{
		X_ASSERT_NOT_NULL(pBufCmd); // no null ref plz!
		processBufferdCommand(*pBufCmd);

		freebufferdCmd(pBufCmd);
	}

}

void XPeer::peerReliabilityTick(void)
{
	uint8_t buffer[MAX_MTU_SIZE];
	FixedBitStream bs(buffer, buffer + MAX_MTU_SIZE, false);

	for (auto& rs : remoteSystems_)
	{
		if (!rs.isActive) {
			continue;
		}

		core::TimeVal time = gEnv->pTimer->GetTimeNowReal();

		rs.relLayer.update(bs, *rs.pNetSocket, rs.systemAddress, rs.MTUSize, time, 1000);

		if (rs.connectState == ConnectState::Connected && time > rs.nextPingTime)
		{
			rs.nextPingTime = time + core::TimeVal::fromMS(5000);
			sendPing(rs.systemAddress, PacketReliability::UnReliable, true);

		}

		// do we have any packets?
		ReliabilityLayer::PacketData data;
		while (rs.relLayer.recive(data))
		{
			// okay we got a packet :D!
			// the first byte should be msgId.
			if (data.numBits < 8) {
				continue;
			}

			RecvBitStream stream(data.pData, data.pData + core::bitUtil::bitsToBytes(data.numBits), true);
			MessageID::Enum msgId = stream.read<MessageID::Enum>();

			if (msgId >= MessageID::ENUM_COUNT) {
				X_ERROR("Net", "Message contains invalid msgId: %" PRIi32, static_cast<int32_t>(msgId));
				continue;
			}

			X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived reliable messageId: \"%s\"", MessageID::ToString(msgId));

			// okay process the msg!
			uint8_t buffer1[MAX_MTU_SIZE];
			FixedBitStream tmpBs(buffer1, buffer1 + MAX_MTU_SIZE, false);

			switch (msgId)
			{
				case MessageID::ConnectionRequest:
					handleConnectionRequest(tmpBs, stream, rs);
					break;
				case MessageID::ConnectionRequestAccepted:
					handleConnectionRequestAccepted(tmpBs, stream, rs);
					break;
				case MessageID::ConnectionRequestHandShake:
					handleConnectionRequestHandShake(tmpBs, stream, rs);
					break;

				case MessageID::ConnectedPing:
					handleConnectedPing(tmpBs, stream, rs);
					break;
				case MessageID::ConnectedPong:
					handleConnectedPong(tmpBs, stream, rs);
					break;


				default:
					X_ERROR("Net", "Unhandled reliable message: \"%s\"", MessageID::ToString(msgId));
					break;
			}


		}
	}
}


void XPeer::processRecvData(RecvData* pData, int32_t byteOffset)
{
	X_ASSERT_NOT_NULL(pData);
	X_ASSERT_NOT_NULL(pData->pSrcSocket);
	X_ASSERT(pData->bytesRead > 0, "RecvData with no data should not reach here")(pData, pData->bytesRead);

	SystemAdd& sysAdd = pData->systemAdd;
	NetSocket& socket = *pData->pSrcSocket;

	IPStr strBuf;
	pData->systemAdd.toString(strBuf, false);

	uint8_t buf[MAX_MTU_SIZE];
	FixedBitStream bsOut(buf, buf + sizeof(buf), false);

	if (isBanned(strBuf))
	{
		// you fucking twat!
		bsOut.write(MessageID::ConnectionBanned);
		bsOut.write(OFFLINE_MSG_ID);
		bsOut.write(guid_);

		SendParameters sp;
		sp.setData(bsOut);
		sp.systemAddress = pData->systemAdd;
		pData->pSrcSocket->send(sp);
		return;
	}

	// create a fixed bitstrem around the recvData to make it easy to read the data.
	RecvBitStream stream(pData->data + byteOffset, pData->data + pData->bytesRead, true);

	X_ASSERT(stream.size() > 0, "Stream is empty")(stream.size(), stream.capacity());

	MessageID::Enum msgId = stream.read<MessageID::Enum>();
	// external data, check enum in range.
	if (msgId >= MessageID::ENUM_COUNT) {
		X_ERROR("Net", "Message contains invalid msgId: %" PRIi32, static_cast<int32_t>(msgId));
		return;
	}

	if (msgId == MessageID::SendTest) {
		return;
	}

	bool offlineMsg = false;
	// is this a offline msg.
	if (stream.sizeInBytes() >= OFFLINE_MSG_ID.size())
	{
		if (std::memcmp(pData->data + byteOffset + 1, OFFLINE_MSG_ID.data(), OFFLINE_MSG_ID.size()) == 0)
		{
			offlineMsg = true;
		}
	}

	if (!offlineMsg) {
		RemoteSystem* pRemoteSys = getRemoteSystem(pData->systemAdd, true);
		if (!pRemoteSys) {

			return;
		}

		pRemoteSys->relLayer.recv(
			pData->data + byteOffset, 
			pData->bytesRead, 
			*pData->pSrcSocket, 
			pData->systemAdd,
			pData->timeRead, 
			pRemoteSys->MTUSize
		);
		return;
	}

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived offline messageId: \"%s\"", MessageID::ToString(msgId));

	stream.skipBytes(OFFLINE_MSG_ID.size());

	switch(msgId)
	{
		case MessageID::ProtcolVersionIncompatible:
		case MessageID::ConnectionRequestFailed:
		case MessageID::ConnectionBanned:
		case MessageID::ConnectionNoFreeSlots:
		case MessageID::ConnectionRateLimited:
			handleConnectionFailure(bsOut, pData, stream, msgId);
			break;

		case MessageID::OpenConnectionRequest:
			handleOpenConnectionRequest(bsOut, pData, stream);
			break;
		case MessageID::OpenConnectionRequestStage2:
			handleOpenConnectionRequestStage2(bsOut, pData, stream);
			break;
		case MessageID::OpenConnectionResponse:
			handleOpenConnectionResponse(bsOut, pData, stream);
			break;
		case MessageID::OpenConnectionResponseStage2:
			handleOpenConnectionResponseStage2(bsOut, pData, stream);
			break;

			// hello.
		case MessageID::UnConnectedPing:
		case MessageID::UnConnectedPingOpenConnections:
			handleUnConnectedPing(bsOut, pData, stream, msgId == MessageID::UnConnectedPingOpenConnections);
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


void XPeer::handleConnectionFailure(FixedBitStream& bsBuf, RecvData* pData, RecvBitStream& bs, MessageID::Enum failureType)
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

void XPeer::handleOpenConnectionRequest(FixedBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
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

void XPeer::handleOpenConnectionResponse(FixedBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
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

void XPeer::handleOpenConnectionRequestStage2(FixedBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
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
			if (isIpConnectSpamming(pData->systemAdd, &lastConnectDelta))
			{
				// you noob, can you even coun to 10?
				// NO!
				IPStr addStr;
				X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived connection req from \"%s\" %gms ago, rate limiting", 
					pData->systemAdd.toString(addStr), lastConnectDelta.GetMilliSeconds());


				// i don't care if this ratelimit is allmost over and you could technically connect again sooner.
				uint32_t timeMs = safe_static_cast<uint32_t>(connectionRateLimitTime_.GetMilliSecondsAsInt64());

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
			}
		}
	}

	SendParameters sp;
	sp.setData(bsOut);
	sp.systemAddress = pData->systemAdd;
	pData->pSrcSocket->send(sp);
}

void XPeer::handleOpenConnectionResponseStage2(FixedBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
{
	// meow.
	// if we here the response was valid annd == OpenConnectionResponseStage2
	SystemAdd bindingAdd;
	NetGUID clientGuid;
	uint16_t mtu;

	bs.read(clientGuid);
	bs.read(bindingAdd);
	bs.read(mtu);

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
					// password?
					// : 123456

					// send the request to the remote.
					sendImmediate(
						bsOut.data(), 
						safe_static_cast<BitSizeT>(bsOut.size()),
						PacketPriority::Immediate, 
						PacketReliability::Reliable,
						0, 
						AddressOrGUID(&pData->systemAdd), 
						false, 
						timeNow, 
						0
					);
				}
				else
				{
					// out connection to remote failed.
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


void XPeer::handleUnConnectedPing(FixedBitStream& bsOut, RecvData* pData, RecvBitStream& bs, bool openConnectionsRequired)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived unConnectedPing");

	if (openConnectionsRequired) {

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

void XPeer::handleUnConnectedPong(FixedBitStream& bsOut, RecvData* pData, RecvBitStream& bs)
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

void XPeer::handleConnectionRequest(FixedBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs)
{
	NetGUID clientGuid;
	int64_t timeStamp;

	// must be a unverified sender trying to connect.
	if (rs.connectState != ConnectState::UnverifiedSender)
	{
		X_ERROR("Net", "Recived unexpected connection request from client, ignoring.");
		// ban the slut :D !

		return;
	}

	bs.read(clientGuid);
	bs.read(timeStamp);

	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived connection request. timeStamp: %" PRId64, timeStamp);

	// hey hey!
	rs.connectState = ConnectState::HandlingConnectionRequest;

	core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();

	bsOut.write(MessageID::ConnectionRequestAccepted);
	bsOut.write(rs.systemAddress);
	bsOut.write<uint8_t>(safe_static_cast<uint8_t>(ipList_.size()));
	for (auto& ip : ipList_) {
		bsOut.write(ip);
	}
	bsOut.write(timeStamp);
	bsOut.write(timeNow.GetValue());

	sendImmediate(
		bsOut.data(),
		safe_static_cast<BitSizeT>(bsOut.size()),
		PacketPriority::Immediate,
		PacketReliability::Reliable,
		0,
		AddressOrGUID(&rs.systemAddress),
		false,
		timeNow,
		0
	);
}


void XPeer::handleConnectionRequestAccepted(FixedBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs)
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
	for (size_t i = 0; i < numInternal; i++) {
		bs.read(localIps.AddOne());
	}
	bs.read(sendPingTime);
	bs.read(sendPongTime);

	// welcome to the club.
	rs.onConnected(externalSysId, localIps, core::TimeVal(sendPingTime), core::TimeVal(sendPongTime));

	// --------- Lets shake on it.. -------------

	core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();

	bsOut.write(MessageID::ConnectionRequestHandShake);
	bsOut.write(rs.systemAddress);
	bsOut.write<uint8_t>(safe_static_cast<uint8_t>(ipList_.size()));
	for (auto& ip : ipList_) {
		bsOut.write(ip);
	}
	bsOut.write(sendPongTime);
	bsOut.write(timeNow.GetValue());

	sendImmediate(
		bsOut.data(),
		safe_static_cast<BitSizeT>(bsOut.size()),
		PacketPriority::Immediate,
		PacketReliability::Reliable,
		0,
		AddressOrGUID(&rs.systemAddress),
		false,
		timeNow,
		0
	);

	sendPing(rs.systemAddress, PacketReliability::UnReliable, true);

	// tell the game.
	Packet* pPacket = allocPacket(8);
	pPacket->pData[0] = MessageID::ConnectionRequestAccepted;
	pPacket->pSystemAddress = &rs.systemAddress; // fuck
	pPacket->guid = rs.guid;
	pushBackPacket(pPacket);
}

void XPeer::handleConnectionRequestHandShake(FixedBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs)
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
	for (size_t i = 0; i < numInternal; i++) {
		bs.read(localIps.AddOne());
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

void XPeer::handleConnectedPing(FixedBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived ping");

	core::TimeVal timeNow = gEnv->pTimer->GetTimeNowReal();

	int64_t timeStamp;

	bs.read(timeStamp);

	// respond.
	bsOut.write(MessageID::ConnectedPong);
	bsOut.write(timeStamp);
	bsOut.write(timeNow.GetValue());

	sendImmediate(
		bsOut.data(),
		safe_static_cast<BitSizeT>(bsOut.size()),
		PacketPriority::Immediate,
		PacketReliability::Reliable,
		0,
		AddressOrGUID(&rs.systemAddress),
		false,
		timeNow,
		0
	);
}


void XPeer::handleConnectedPong(FixedBitStream& bsOut, RecvBitStream& bs, RemoteSystem& rs)
{
	X_LOG0_IF(vars_.debugEnabled(), "Net", "Recived pong");

	int64_t sendPingTime;
	int64_t sendPongTime;

	bs.read(sendPingTime);
	bs.read(sendPongTime);

	rs.onPong(core::TimeVal(sendPingTime), core::TimeVal(sendPongTime));
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

			if (connectionAttemptDelta < connectionRateLimitTime_)
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


void XPeer::onSocketRecv(RecvData* pData)
{
	// we own this pointer
	X_ASSERT_NOT_NULL(pData);

	recvDataQue_.push(pData);
}


core::Thread::ReturnValue XPeer::socketRecvThreadProc(const core::Thread& thread)
{
	NetSocket* pSocket = reinterpret_cast<NetSocket*>(thread.getData());
	X_ASSERT_NOT_NULL(pSocket);

	RecvData* pData = allocRecvData();

	while (thread.ShouldRun())
	{
		pSocket->recv(*pData);

		if (pData->bytesRead > 0)
		{
			onSocketRecv(pData);

			// get new data block.
			pData = allocRecvData();
		}
		else
		{
			core::Thread::Sleep(0);
		}
	}

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

