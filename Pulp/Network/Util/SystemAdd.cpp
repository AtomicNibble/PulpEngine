#include "stdafx.h"
#include "SystemAdd.h"
#include "LibaryStartup.h"


X_NAMESPACE_BEGIN(net)

const char* SystemAdd::IP_LOOPBACK[IpVersion::ENUM_COUNT] = {
	"127.0.0.1",
	"::1"
};

SystemAdd::SystemAdd()
{
	address_.addr4.sin_family = AF_INET;
	core::zero_object(address_);
	systemIndex_ = std::numeric_limits<SystemIndex>::max();

#if X_DEBUG
	portPeekVal_ = 0;
#endif // !X_DEBUG
}

SystemAdd::SystemAdd(const char* pAddressStr)
{
	address_.addr4.sin_family = AF_INET;
	fromString(pAddressStr);
	systemIndex_ = std::numeric_limits<SystemIndex>::max();
}

SystemAdd::SystemAdd(const char* pAddressStr, uint16_t port)
{
	address_.addr4.sin_family = AF_INET;
	fromStringExplicitPort(pAddressStr, port);
	systemIndex_ = std::numeric_limits<SystemIndex>::max();
}


SystemAdd::~SystemAdd()
{

}

void SystemAdd::setFromSocket(SocketHandle socket)
{
	platform::socklen_t slen;
	platform::sockaddr_storage ss;
	slen = sizeof(ss);

	if (platform::getsockname(socket, (struct platform::sockaddr*)&ss, &slen) != 0)
	{
		lastError::Description Dsc;
		X_FATAL("Net", "Failed to get socket name for socket: %" PRIu32 " Error: \"%s\"", lastError::ToString(Dsc));
		return;
	}

	const size_t ipv4AddSize = sizeof(address_.addr4.sin_addr);
	const size_t ipv6AddSize = sizeof(address_.addr6.sin6_addr);

	uint8_t zeroBuf[core::Max(ipv4AddSize, ipv6AddSize)];
	core::zero_object(zeroBuf);

	if (ss.ss_family == AF_INET)
	{
		std::memcpy(&address_.addr4, &ss, sizeof(platform::sockaddr_in));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr4.sin_port);
#endif // !X_DEBUG

		if (std::memcmp(&address_.addr4.sin_addr.s_addr, zeroBuf, ipv4AddSize) == 0) {
			setToLoopback(IpVersion::Ipv4);
		}
	}
	else
	{
		std::memcpy(&address_.addr6, &ss, sizeof(platform::sockaddr_in6));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr6.sin6_port);
#endif // !X_DEBUG

		if (std::memcmp(&address_.addr6.sin6_addr, zeroBuf, ipv6AddSize) == 0) {
			setToLoopback(IpVersion::Ipv6);
		}
	}
}

void SystemAdd::setFromAddInfo(platform::addrinfo* pAddInfo)
{
	if (pAddInfo->ai_family == AF_INET)
	{
		std::memcpy(&address_.addr4, pAddInfo->ai_addr, sizeof(address_.addr4));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr4.sin_port);
#endif // !X_DEBUG
	}
	else
	{
		std::memcpy(&address_.addr6, pAddInfo->ai_addr, sizeof(address_.addr6));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr6.sin6_port);
#endif // !X_DEBUG
	}
}

void SystemAdd::setFromAddStorage(const platform::sockaddr_storage& addStorage)
{
	if (addStorage.ss_family == AF_INET)
	{
		std::memcpy(&address_.addr4, &addStorage, sizeof(address_.addr4));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr4.sin_port);
#endif // !X_DEBUG
	}
	else
	{
		std::memcpy(&address_.addr6, &addStorage, sizeof(address_.addr6));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr6.sin6_port);
#endif // !X_DEBUG
	}
}


void SystemAdd::setToLoopback(void)
{
	setToLoopback(getIPVersion());
}

void SystemAdd::setToLoopback(IpVersion::Enum ipVersion)
{
	fromString(IP_LOOPBACK[ipVersion], '\0', ipVersion);
}


void SystemAdd::setPortFromHostByteOrder(uint16_t port)
{
	address_.addr4.sin_port = platform::htons(port);
#if X_DEBUG
	portPeekVal_ = port;
#endif // !X_DEBUG
}

void SystemAdd::setPortFromNetworkByteOrder(uint16_t port)
{
	address_.addr4.sin_port = port;
#if X_DEBUG
	portPeekVal_ = platform::ntohs(port);
#endif // !X_DEBUG
}

void SystemAdd::writeToBitStream(core::FixedBitStreamBase& bs) const
{
	bs.writeAligned(address_);
	bs.write(systemIndex_);
}

void SystemAdd::fromBitStream(core::FixedBitStreamBase& bs)
{
	bs.readAligned(address_);
	bs.read(systemIndex_);

#if X_DEBUG
	portPeekVal_ = platform::ntohs(address_.addr4.sin_port);
#endif // !X_DEBUG
}

bool SystemAdd::equalExcludingPort(const SystemAdd& oth) const
{
	bool ipv4Equal = (address_.addr4.sin_family == AF_INET && address_.addr4.sin_addr.s_addr == oth.address_.addr4.sin_addr.s_addr);

#if NET_IPv6_SUPPORT
	bool ipv6Equal = (address_.addr4.sin_family == AF_INET6 && std::memcmp(address_.addr6.sin6_addr.s6_addr,
		oth.address_.addr6.sin6_addr.s6_addr, sizeof(address_.addr6.sin6_addr.s6_addr)) == 0);

	return ipv4Equal || ipv6Equal;
#endif // !NET_IPv6_SUPPORT
		
	return ipv4Equal;
}


const char* SystemAdd::toString(IPStr& strBuf, bool incPort) const
{
	int32_t ret;

	char tmpBuf[IPStr::BUF_SIZE];

	if (address_.addr4.sin_family == AF_INET)
	{
		ret = platform::getnameinfo((struct platform::sockaddr*)&address_.addr4, 
			sizeof(struct platform::sockaddr_in),
			tmpBuf,
			sizeof(tmpBuf),
			nullptr, 
			0, 
			NI_NUMERICHOST
		);
	}
	else
	{
		ret = platform::getnameinfo((struct platform::sockaddr*)
			&address_.addr6, 
			sizeof(struct platform::sockaddr_in6), 
			tmpBuf,
			sizeof(tmpBuf), // INET6_ADDRSTRLEN, 
			nullptr,
			0, 
			NI_NUMERICHOST
		);
	}

	if (ret != 0) {
		lastError::Description Dsc;
		X_ERROR("Net", "Failed to get name info. Error: \"%s\"", lastError::ToString(Dsc));
		strBuf.clear();
		return strBuf.c_str();
	}

	strBuf.set(tmpBuf);

	if (incPort) {
		strBuf.appendFmt(" %" PRIu16, platform::ntohs(address_.addr4.sin_port));
	}

	return strBuf.c_str();
}


bool SystemAdd::fromString(const char* pAddressStr, char portDelineator, IpVersion::Enum ipVersion)
{
	X_ASSERT_NOT_NULL(pAddressStr);

	core::StackString<INET6_ADDRSTRLEN, char> ipPart;
	core::StackString<32, char> portPart;

	const char* pPort = core::strUtil::Find(pAddressStr, portDelineator);
	if (pPort)
	{
		ipPart.set(pAddressStr, pPort);
		portPart.set(pPort);
	}
	else
	{
		ipPart.set(pAddressStr);
	}

	PlatLib::ScopedRef libRef;

	platform::addrinfo hints, *servinfo = nullptr;
	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_DGRAM;

	if (ipVersion == IpVersion::Ipv4) {
		hints.ai_family = AF_INET;
	}
	else if (ipVersion == IpVersion::Ipv6) {
		hints.ai_family = AF_INET6;
	}
	else {
		hints.ai_family = AF_UNSPEC;
	}

	platform::getaddrinfo(ipPart.c_str(), "", &hints, &servinfo);
	if (servinfo == 0)
	{
		// if ipv6 try it as ipv4.
		if (ipVersion == IpVersion::Ipv6) 
		{
			ipVersion = IpVersion::Ipv4;
			hints.ai_family = AF_INET;

			platform::getaddrinfo(ipPart.c_str(), "", &hints, &servinfo);
			if (servinfo == 0)
			{
				// failed still
				lastError::Description Dsc;
				X_ERROR("Net", "Failed to get addres info. Error: \"%s\"", lastError::ToString(Dsc));
				return false;
			}
			else
			{
				X_WARNING("Net", "Address passed as Ipv6, but is a valid Ipv4 address. add: \"%s\"", pAddressStr);
			}
		}
		else
		{
			lastError::Description Dsc;
			X_ERROR("Net", "Failed to get addres info. Error: \"%s\"", lastError::ToString(Dsc));
			return false;
		}
	}

	X_ASSERT(servinfo != 0, "ServerInfo not valid")(servinfo);

	uint16_t oldPort = address_.addr4.sin_port;

	if (servinfo->ai_family == AF_INET)
	{
		address_.addr4.sin_family = AF_INET;
		std::memcpy(&address_.addr4, (struct platform::sockaddr_in *)servinfo->ai_addr, sizeof(struct platform::sockaddr_in));
	}
	else
	{
		address_.addr4.sin_family = AF_INET6;
		std::memcpy(&address_.addr6, (struct platform::sockaddr_in6 *)servinfo->ai_addr, sizeof(struct platform::sockaddr_in6));
	}

	platform::freeaddrinfo(servinfo); // free the linked list

	if (portPart.isNotEmpty())
	{
		uint16_t port = core::strUtil::StringToInt<uint16_t>(portPart.c_str());
		address_.addr4.sin_port = platform::htons(port);

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr4.sin_port);
#endif // !X_DEBUG
	}
	else
	{
		address_.addr4.sin_port = oldPort;
	}

	return true;
}

bool SystemAdd::fromStringExplicitPort(const char* pAddressStr, uint16_t port, IpVersion::Enum ipVersion)
{
	X_ASSERT_NOT_NULL(pAddressStr);
	
	if (!fromString(pAddressStr, PORT_DELINEATOR, ipVersion)) {
		return false;
	}

	address_.addr4.sin_port = platform::htons(port);

#if X_DEBUG
	portPeekVal_ = platform::ntohs(address_.addr4.sin_port);
#endif // !X_DEBUG
	return true;
}

X_NAMESPACE_END