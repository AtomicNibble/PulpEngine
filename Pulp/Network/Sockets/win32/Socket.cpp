#include "stdafx.h"
#include "Socket.h"


X_NAMESPACE_BEGIN(net)

namespace
{
	int32_t getRawSocketType(SocketType::Enum type)
	{
		switch (type)
		{
			case SocketType::Stream:
				return SOCK_DGRAM;
			case SocketType::Dgram:
				return SOCK_DGRAM;
			case SocketType::Raw:
				return SOCK_DGRAM;
			case SocketType::Rdm:
				return SOCK_DGRAM;
			case SocketType::SeqPacket:
				return SOCK_DGRAM;
			
			case SocketType::Invalid:
			default:
				X_ASSERT_UNREACHABLE();
				break;
		}

		return SOCK_DGRAM;
	}

} // namespace

BindParameters::BindParameters()
{
	port = 0;

	nonBlockingSocket = false;
	IPHdrIncl = false;
	broadCast = false;
}

// -----------------------

NetSocket::NetSocket()
{
	socketType_ = SocketType::Invalid;
	socket_ = INVALID_SOCKET;
}

NetSocket::~NetSocket()
{
	if (socket_ != INVALID_SOCKET)
	{
		platform::closesocket(socket_);
	}
}

BindResult::Enum NetSocket::bind(BindParameters& bindParameters)
{
	struct platform::addrinfo* pResult = nullptr;
	struct platform::addrinfo* pPtr = nullptr;
	struct platform::addrinfo hints;
	
	socketType_ = bindParameters.socketType;

	core::zero_object(hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = getRawSocketType(socketType_);
	hints.ai_flags = AI_PASSIVE;


	core::StackString<32, char> portStr(bindParameters.port);
	lastError::Description Dsc;

	int32_t res = getaddrinfo(0, portStr.c_str(), &hints, &pResult);
	if (res != 0)
	{
		X_ERROR("Net", "Failed to get address info for binding. Error: \"%s\"", lastError::ToString(Dsc));
		return BindResult::FailedToBind;
	}

	for (pPtr = pResult; pPtr != nullptr; pPtr = pPtr->ai_next)
	{

		socket_ = platform::socket(pPtr->ai_family, pPtr->ai_socktype, pPtr->ai_protocol);
		if (socket_ == INVALID_SOCKET)
		{
			X_ERROR("Net", "Failed to open socket. Error: \"%s\"", lastError::ToString(Dsc));
			platform::freeaddrinfo(pResult);
			return BindResult::SendTestFailed;
		}

		// bind it like it's hot.
		int32_t ret = platform::bind(socket_, pPtr->ai_addr, safe_static_cast<int32_t>(pPtr->ai_addrlen));
		if (ret == 0)
		{
			boundAdd_.setFromSocket(socket_);

			setSocketOptions();
			setNonBlockingSocket(bindParameters.nonBlockingSocket);
			setBroadcastSocket(bindParameters.broadCast);

			if (socketType_ == SocketType::Raw) {
				setIPHdrIncl(bindParameters.IPHdrIncl);
			}

			platform::freeaddrinfo(pResult);
			return BindResult::SendTestFailed;
		}
		else
		{
			// we failed to bind.
			// try next one.
			X_WARNING("Net", "Failed to bind socket. Error: \"%s\"", lastError::ToString(Dsc));
			platform::closesocket(socket_);
		}
	}

	// Shieeeeeeeeeeet.
	platform::freeaddrinfo(pResult);
	return BindResult::FailedToBind;
}

SendResult::Enum NetSocket::send(SendParameters& sendParameters)
{


	return SendResult::Success;
}


void NetSocket::setNonBlockingSocket(bool nonblocking)
{
	using platform::u_long;

	u_long noneBlockingLng = nonblocking ? 1 : 0;

	int32_t res = platform::ioctlsocket(socket_, FIONBIO, &noneBlockingLng);
	if (res != 0)
	{
		lastError::Description Dsc;
		X_ERROR("Net", "Failed to set nonblocking mode on socket. Error: \"%s\"", lastError::ToString(Dsc));
	}
}

void NetSocket::setSocketOptions(void)
{
	int32_t res;
	int32_t sock_opt;
	lastError::Description Dsc;

	// set the recive buffer to decent size
	sock_opt = 1024 * 256;
	res = platform::setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, (char*)&sock_opt, sizeof(sock_opt));
	if (res != 0)
	{
		X_ERROR("Net", "Failed to set max rcvbuf on socket. Error: \"%s\"", lastError::ToString(Dsc));
	}

	// decent size buf for send.
	sock_opt = 1024 * 16;
	res = platform::setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, (char*)&sock_opt, sizeof(sock_opt));
	if (res != 0)
	{
		X_ERROR("Net", "Failed to set max sndbuf on socket. Error: \"%s\"", lastError::ToString(Dsc));
	}

	// don't linger on close if un-sent data present.
	sock_opt = 0;
	res = platform::setsockopt(socket_, SOL_SOCKET, SO_LINGER, (char*)&sock_opt, sizeof(sock_opt));
	if (res != 0)
	{
		X_WARNING("Net", "Failed to set linger mode on socket. Error: \"%s\"", lastError::ToString(Dsc));
	}
}

void NetSocket::setBroadcastSocket(bool broadcast)
{
	int32_t res;
	int32_t val = broadcast ? 1 : 0;

	// enable / disable broadcast.
	res = platform::setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof(val));
	if (res != 0)
	{
		lastError::Description Dsc;
		X_ERROR("Net", "Failed to set broadcast mode on socket. Error: \"%s\"", lastError::ToString(Dsc));
	}
}

void NetSocket::setIPHdrIncl(bool ipHdrIncl)
{
	X_ASSERT(socketType_ == SocketType::Raw, "Must be a raw socket")(socketType_);

	int32_t res;
	int32_t val = ipHdrIncl ? 1 : 0;

/*
	When set to TRUE, indicates the application provides the IP header. 
	Applies only to SOCK_RAW sockets. The TCP/IP service provider may set the ID field, 
	if the value supplied by the application is zero.

	The IP_HDRINCL option is applied only to the SOCK_RAW type of protocol.
*/

	res = platform::setsockopt(socket_, IPPROTO_IP, IP_HDRINCL, (char*)&val, sizeof(val));
	if (res != 0)
	{
		lastError::Description Dsc;
		X_ERROR("Net", "Failed to set hdrincl on socket. Error: \"%s\"", lastError::ToString(Dsc));
	}
}

X_NAMESPACE_END
