#include "stdafx.h"
#include "XPeer.h"
#include "XNet.h"

#include "Sockets\Socket.h"

X_NAMESPACE_BEGIN(net)

XPeer::XPeer(core::MemoryArenaBase* arena) :
	sockets_(arena)
{
	guid_ = XNet::generateGUID();

	defaultTimeOut_ = core::TimeVal::fromMS(1000);
	unreliableTimeOut_ = core::TimeVal::fromMS(1000 * 10);

	maxIncommingConnections_ = 0;
	maxPeers_ = 0;


}

XPeer::~XPeer()
{

}

StartupResult::Enum XPeer::init(uint32_t maxConnections, SocketDescriptor* pSocketDescriptors,
	uint32_t socketDescriptorCount)
{
	if (maxConnections < 1) {
		return StartupResult::InvalidMaxCon;
	}
	if (!pSocketDescriptors || socketDescriptorCount < 1) {
		return StartupResult::InvalidSocketDescriptors;
	}
	
	BindParameters bindParam;
	bindParam.nonBlockingSocket = false;
	bindParam.IPHdrIncl = false;
	bindParam.broadCast = true;


	for (uint32_t i = 0; i < socketDescriptorCount; i++)
	{
		SocketDescriptor& socketDiscriptor = pSocketDescriptors[i];

		NetSocket socket;
		bindParam.hostAdd = socketDiscriptor.getHostAdd();
		bindParam.port = socketDiscriptor.getPort();

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

	return StartupResult::Error;
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



uint32_t XPeer::send(const char* pData, const size_t length, PacketPriority::Enum priority,
	PacketReliability::Enum reliability, uint8_t orderingChannel, 
	const AddressOrGUID systemIdentifier, bool broadcast,
	uint32_t forceReceiptNumber)
{


	return 0;
}


// connection limits
void XPeer::setMaximumIncomingConnections(uint16_t numberAllowed)
{

}

uint16_t XPeer::getMaximumIncomingConnections(void) const
{
	return 0;

}

uint16_t XPeer::numberOfConnections(void) const
{

	return 0;
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

void XPeer::populateIpList(void)
{


}


X_NAMESPACE_END

