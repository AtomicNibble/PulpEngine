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
	T bitsToBytes(T bits)
	{
		return (bits + 7) >> 3;
	}

	template<typename T>
	BitSizeT bytesToBits(T bits)
	{
		return safe_static_cast<BitSizeT>(bits << 3);
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

	static const size_t POOL_ALLOCATION_SIZE = core::Max(sizeof(BufferdCommand), sizeof(Packet));
	static const size_t POOL_ALLOCATION_ALIGN =  core::Max(X_ALIGN_OF(BufferdCommand), X_ALIGN_OF(Packet));

} // namespace 

PingAndClockDifferential::PingAndClockDifferential()
{
	pingTime = UNDEFINED_PING;
}

// -----------------------------------

RemoteSystem::RemoteSystem()
{
	isActive = false;

	connectState = ConnectState::NoAction;
	MTUSize = MAX_MTU_SIZE;

	pNetSocket = nullptr;
}


// -----------------------------------

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

		maxPeers_ = maxIncommingConnections_;

		remoteSystems_.resize(maxPeers_);
	}
	
	BindParameters bindParam;
	bindParam.nonBlockingSocket = false;
	bindParam.IPHdrIncl = false;
	bindParam.broadCast = true;

	for (size_t i = 0; i < socketDescriptorCount; i++)
	{
		SocketDescriptor& socketDiscriptor = pSocketDescriptors[i];

		NetSocket socket;
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

	// only push if not trying to connect already.
	auto matchSysAddFunc = [&systemAddress](const RequestConnection* pOth) {
		return pOth->systemAddress == systemAddress;
	};

	core::CriticalSection::ScopedLock lock(connectionReqsCS_);

	if (std::find_if(connectionReqs_.begin(), connectionReqs_.end(), matchSysAddFunc) != connectionReqs_.end()) {
		X_DELETE(pConReq, arena_);
		return ConnectionAttemptResult::AlreadyInProgress;
	}

	connectionReqs_.emplace_back(pConReq);
	
	return ConnectionAttemptResult::Started;
}

void XPeer::closeConnection(const AddressOrGUID target, bool sendDisconnectionNotification,
	uint8_t orderingChannel, PacketPriority::Enum notificationPriority)
{
	X_ASSERT_NOT_IMPLEMENTED();
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

	BufferdCommand* pCmd = allocBufferdCmd(numberOfBitsToSend);
	std::memcpy(pCmd->pData, pData, bitsToBytes(numberOfBitsToSend));
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
	bool useCallerDataAllocation, core::TimeStamp currentTime, uint32_t receipt)
{

	size_t remoteIdx = getRemoteSystemIndex(systemIdentifier);

	if (!broadcast)
	{

	}
	else
	{


	}

	X_ASSERT_NOT_IMPLEMENTED();

	return true;
}

void XPeer::sendLoopback(const uint8_t* pData, size_t lengthBytes)
{
	Packet* pPacket = allocPacket(bytesToBits(lengthBytes));
	std::memcpy(pPacket->pData, pData, lengthBytes);
	pPacket->guid = getMyGUID();

	pushBackPacket(pPacket, false);
}

Packet* XPeer::receive(void)
{
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

void XPeer::parseConnectionRequestPacket(RemoteSystem* pRemoteSystem, const SystemAdd& systemAddress, const uint8_t* pData, size_t byteSize)
{
	X_ASSERT_NOT_IMPLEMENTED();
}

void XPeer::onConnectionRequest(RemoteSystem* pRemoteSystem, core::TimeStamp incomingTimestamp)
{
	X_ASSERT_NOT_IMPLEMENTED();
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
	pPacket->pData = allocPacketData(bitsToBytes(lengthBits));
	pPacket->length = safe_static_cast<uint32_t>(bitsToBytes(lengthBits));
	pPacket->bitLength = safe_static_cast<uint32_t>(lengthBits);
	return pPacket;
}

void XPeer::freePacket(Packet* pPacket)
{
	freePacketData(pPacket->pData);
	X_DELETE(pPacket, &poolArena_);
}


BufferdCommand* XPeer::allocBufferdCmd(size_t lengthBits)
{
	BufferdCommand* pCmd = X_NEW(BufferdCommand, &poolArena_, "BufferdCmd");
	pCmd->pData = allocPacketData(bitsToBytes(lengthBits));
	pCmd->numberOfBitsToSend = safe_static_cast<BitSizeT>(lengthBits);
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
}

uint16_t XPeer::getMaximumIncomingConnections(void) const
{
	return maxIncommingConnections_;

}

uint16_t XPeer::numberOfConnections(void) const
{
	// number of open connections.
	X_ASSERT_NOT_IMPLEMENTED();
	return 0;
}

uint32_t XPeer::getMaximunNumberOfPeers(void) const
{
	return maxPeers_;
}


// Ping 
void XPeer::ping(const ISystemAdd* pTarget)
{
	X_ASSERT_NOT_NULL(pTarget);

}

bool XPeer::ping(const char* pHost, uint16_t remotePort, bool onlyReplyOnAcceptingConnections,
	uint32_t connectionSocketIndex)
{
	X_ASSERT_NOT_IMPLEMENTED();

	return false;
}

// bans at connection level.
void XPeer::addToBanList(const char* pIP, core::TimeVal timeout)
{
	IPStr ip(pIP);

	if (ip.isEmpty()) {
		return;
	}

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
	
	auto findBanIP = [&ip](const Ban& oth) {
		return oth.ip == ip;
	};

	auto it = std::find_if(bans_.begin(), bans_.end(), findBanIP);
	if (it != bans_.end())
	{
		bans_.erase(it);
	}
}

bool XPeer::isBanned(const char* pIP)
{
	if (bans_.isEmpty()) {
		return false;
	}

	IPStr ip(pIP);

	for (auto& ban : bans_) {
		if (ipWildMatch(ban.ip, ip)) {
			return true;
		}
	}
	return false;
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
// MTU for a given system
int XPeer::getMTUSize(const ISystemAdd* pTarget)
{
	if (pTarget) {
		const SystemAdd* pSysAdd = static_cast<const SystemAdd*>(pTarget);
		auto* pRemoteSys = getRemoteSystem(*pSysAdd, false);
	
		if (pRemoteSys) {
			return pRemoteSys->MTUSize;
		}

	//	return -1;
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
		processRecvData(pRecvData);

		freeRecvData(pRecvData);
	}
}


void XPeer::processRecvData(RecvData* pData)
{
	X_ASSERT_NOT_NULL(pData);
	X_ASSERT_NOT_NULL(pData->pSrcSocket);

	SystemAdd& sysAdd = pData->systemAdd;
	NetSocket& socket = *pData->pSrcSocket;

	IPStr strBuf;
	pData->systemAdd.toString(strBuf, false);

	core::BitStream bs(arena_, MAX_MTU_SIZE);

	if (isBanned(strBuf))
	{
		// you fucking twat!



		SendParameters sp;
		sp.setData(bs);
		sp.systemAddress = pData->systemAdd;

		pData->pSrcSocket->send(sp);
		return;
	}


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

