#include "stdafx.h"
#include "XPeer.h"
#include "XNet.h"

#include <Memory\VirtualMem.h>

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

	static const size_t POOL_ALLOCATION_SIZE = core::Max(sizeof(BufferdCommand), sizeof(Packet));
	static const size_t POOL_ALLOCATION_ALIGN =  core::Max(X_ALIGN_OF(BufferdCommand), X_ALIGN_OF(Packet));

} // namespace 


RemoteSystem::RemoteSystem()
{
	isActive = false;

	connectMode = ConnectMode::NoAction;
	MTUSize = MAX_MTU_SIZE;

}


// -----------------------------------

XPeer::XPeer(core::MemoryArenaBase* arena) :
	sockets_(arena),
	remoteSystems_(arena),
	bufferdCmds_(arena),
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

		sockets_.emplace_back(socket);
	}

	return StartupResult::Started;
}

void XPeer::shutdown(core::TimeVal blockDuration, uint8_t orderingChannel,
	PacketPriority::Enum disconnectionNotificationPriority)
{

}


// connection api
ConnectionAttemptResult::Enum XPeer::connect(const char* pHost, Port remotePort,
	uint32_t connectionSocketIndex, uint32_t sendConnectionAttemptCount, 
	uint32_t timeBetweenSendConnectionAttemptsMS, core::TimeVal timeoutTime)
{


	return ConnectionAttemptResult::FailedToResolve;
}

void XPeer::closeConnection(const AddressOrGUID target, bool sendDisconnectionNotification,
	uint8_t orderingChannel, PacketPriority::Enum disconnectionNotificationPriority)
{

}


// connection util
ConnectionState::Enum XPeer::getConnectionState(const AddressOrGUID systemIdentifier)
{


	return ConnectionState::Disconnected;
}

void XPeer::cancelConnectionAttempt(const ISystemAdd* pTarget)
{
	X_ASSERT_NOT_NULL(pTarget);


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

		if (deadConIdx && !onlyActive)
		{
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
		if (rs.guid == guid && (!onlyActive || rs.isActive))
		{
			return &rs;
		}
	}

	return nullptr;
}

RemoteSystem* XPeer::getRemoteSystem(const SystemAdd& systemAddress)
{
	size_t remoteSystemIndex = getRemoteSystemIndex(systemAddress);
	if (remoteSystemIndex == std::numeric_limits<size_t>::max()) {
		return nullptr;
	}

	return &remoteSystems_[remoteSystemIndex];
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

}

void XPeer::onConnectionRequest(RemoteSystem* pRemoteSystem, core::TimeStamp incomingTimestamp)
{

}


void XPeer::pushBackPacket(Packet* pPacket, bool pushAtHead)
{


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


uint32_t XPeer::nextSendReceipt(void)
{
	return sendReceiptSerial_;
}

uint32_t XPeer::incrementNextSendReceipt(void)
{
	return ++sendReceiptSerial_;
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


	return false;
}



int XPeer::getAveragePing(const AddressOrGUID systemIdentifier)
{
	return 0;
}

int XPeer::getLastPing(const AddressOrGUID systemIdentifier) const
{
	return 0;

}

int XPeer::getLowestPing(const AddressOrGUID systemIdentifier) const
{
	return 0;

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

	}

	return defaultTimeOut_;
}

void XPeer::setUnreliableTimeout(core::TimeVal timeout)
{
	unreliableTimeOut_ = timeout;
}

// MTU for a given system
int XPeer::getMTUSize(const ISystemAdd* pTarget)
{
	if (pTarget) {

	}


	return defaultMTU_;
}


bool XPeer::getStatistics(const ISystemAdd* pTarget, NetStatistics& stats)
{
	X_ASSERT_NOT_NULL(pTarget);

	return false;
}


// ~IPeer

bool XPeer::populateIpList(void)
{
	if (!NetSocket::getMyIPs(ipList_)) {
		return false;
	}

	return true;
}


X_NAMESPACE_END

