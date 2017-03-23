#include "stdafx.h"
#include "SystemAddressEx.h"


X_NAMESPACE_BEGIN(net)

namespace
{

	// just match the paltform values, if the platform valeus are diffrent i'll abstract the values.
	static_assert(AddressFamily::INet == AF_INET, "AddressFamily enum don't match platform value");
	
#if NET_IPv6_SUPPORT
	static_assert(AddressFamily::INet6 == AF_INET6, "AddressFamily enum don't match platform value");
#endif // !NET_IPv6_SUPPORT

} // namespace

SystemAddressEx::SystemAddressEx()
{

}

bool SystemAddressEx::operator>(const SystemAddressEx& rhs) const
{
	if (address_.addr4.port == rhs.address_.addr4.port)
	{
#if NET_IPv6_SUPPORT
		if (address_.addr4.family == AF_INET) {
			return address_.addr4.addr.as_int > rhs.address_.addr4.addr.as_int;
		}
		return std::memcmp(&address_.addr6.addr, &rhs.address_.addr6.addr, sizeof(address_.addr6.addr)) > 0;
#else
		return address_.addr4.addr.as_int > rhs.address_.addr4.addr.as_int;
#endif // !NET_IPv6_SUPPORT
	}
	return address_.addr4.port > rhs.address_.addr4.port;
}

bool SystemAddressEx::operator<(const SystemAddressEx& rhs) const
{
	if (address_.addr4.port == rhs.address_.addr4.port)
	{
#if NET_IPv6_SUPPORT
		if (address_.addr4.family == AF_INET) {
			return address_.addr4.addr.as_int < rhs.address_.addr4.addr.as_int;
		}

		return std::memcmp(&address_.addr6.addr, &rhs.address_.addr6.addr, sizeof(address_.addr6.addr)) < 0;
#else
		return address_.addr4.addr.as_int < rhs.address_.addr4.addr.as_int;
#endif // !NET_IPv6_SUPPORT
	}
	return address_.addr4.port < rhs.address_.addr4.port;
}


void SystemAddressEx::setFromSocket(SocketHandle socket)
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

	const size_t ipv4AddSize = sizeof(address_.addr4.addr);

#if NET_IPv6_SUPPORT
	const size_t ipv6AddSize = sizeof(address_.addr6.addr);
	uint8_t zeroBuf[core::Max(ipv4AddSize, ipv6AddSize)];
#else
	uint8_t zeroBuf[ipv4AddSize];
#endif // !NET_IPv6_SUPPORT

	core::zero_object(zeroBuf);

	if (ss.ss_family == AF_INET)
	{
		std::memcpy(&address_.addr4, &ss, sizeof(platform::sockaddr_in));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr4.port);
#endif // !X_DEBUG

		if (std::memcmp(&address_.addr4.addr, zeroBuf, ipv4AddSize) == 0) {
			setToLoopback(IpVersion::Ipv4);
		}
	}
	else
#if NET_IPv6_SUPPORT
	{
		std::memcpy(&address_.addr6, &ss, sizeof(platform::sockaddr_in6));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr6.port);
#endif // !X_DEBUG

		if (std::memcmp(&address_.addr6.addr, zeroBuf, ipv6AddSize) == 0) {
			setToLoopback(IpVersion::Ipv6);
		}
	}
#else
	{
		X_ASSERT_UNREACHABLE();
	}
#endif // !NET_IPv6_SUPPORT
}

void SystemAddressEx::setFromAddInfo(platform::addrinfo* pAddInfo)
{
	if (pAddInfo->ai_family == AF_INET)
	{
		std::memcpy(&address_.addr4, pAddInfo->ai_addr, sizeof(address_.addr4));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr4.port);
#endif // !X_DEBUG
	}
	else
#if NET_IPv6_SUPPORT
	{
		std::memcpy(&address_.addr6, pAddInfo->ai_addr, sizeof(address_.addr6));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr6.port);
#endif // !X_DEBUG
	}
#else
	{
		X_ASSERT_UNREACHABLE();
	}
#endif // !NET_IPv6_SUPPORT
}

void SystemAddressEx::setFromAddStorage(const platform::sockaddr_storage& addStorage)
{
	if (addStorage.ss_family == AF_INET)
	{
		std::memcpy(&address_.addr4, &addStorage, sizeof(address_.addr4));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr4.port);
#endif // !X_DEBUG
	}
	else
#if NET_IPv6_SUPPORT
	{
		std::memcpy(&address_.addr6, &addStorage, sizeof(address_.addr6));

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr6.port);
#endif // !X_DEBUG
	}
#else
	{
		X_ASSERT_UNREACHABLE();
	}
#endif // !NET_IPv6_SUPPORT
}


const char* SystemAddressEx::toString(IPStr& strBuf, bool incPort) const
{
	int32_t ret;

	char tmpBuf[IPStr::BUF_SIZE];

	if (address_.addr4.family == AddressFamily::INet)
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
#if NET_IPv6_SUPPORT
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
#else
	{
		X_ASSERT_UNREACHABLE();
	}
#endif // !NET_IPv6_SUPPORT

	if (ret != 0) {
		lastError::Description Dsc;
		X_ERROR("Net", "Failed to get name info. Error: \"%s\"", lastError::ToString(Dsc));
		strBuf.clear();
		return strBuf.c_str();
	}

	strBuf.set(tmpBuf);

	if (incPort) {
		strBuf.appendFmt(" %" PRIu16, platform::ntohs(address_.addr4.port));
	}

	return strBuf.c_str();
}


bool SystemAddressEx::fromString(const char* pAddressStr, char portDelineator, IpVersion::Enum ipVersion)
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

	platform::addrinfo hints, *servinfo = nullptr;
	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_DGRAM;

	if (ipVersion == IpVersion::Ipv4) {
		hints.ai_family = AF_INET;
	}
#if NET_IPv6_SUPPORT
	else if (ipVersion == IpVersion::Ipv6) {
		hints.ai_family = AF_INET6;
	}
#endif // !NET_IPv6_SUPPORT
	else {
		hints.ai_family = AF_UNSPEC;
	}

	platform::getaddrinfo(ipPart.c_str(), "", &hints, &servinfo);
	if (servinfo == 0)
	{
		// if ipv6 try it as ipv4.
#if NET_IPv6_SUPPORT
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
#endif // !NET_IPv6_SUPPORT
		{
			lastError::Description Dsc;
			X_ERROR("Net", "Failed to get addres info. Error: \"%s\"", lastError::ToString(Dsc));
			return false;
		}
	}

	X_ASSERT(servinfo != 0, "ServerInfo not valid")(servinfo);

	uint16_t oldPort = address_.addr4.port;

	if (servinfo->ai_family == AF_INET)
	{
		address_.addr4.family = AddressFamily::INet;
		std::memcpy(&address_.addr4, (struct platform::sockaddr_in *)servinfo->ai_addr, sizeof(struct platform::sockaddr_in));
	}
#if NET_IPv6_SUPPORT
	else
	{
		address_.addr4.family = AddressFamily::INet6;
		std::memcpy(&address_.addr6, (struct platform::sockaddr_in6 *)servinfo->ai_addr, sizeof(struct platform::sockaddr_in6));
	}
#else
	else
	{
		X_ASSERT_UNREACHABLE();
	}
#endif // !NET_IPv6_SUPPORT

	platform::freeaddrinfo(servinfo); // free the linked list

	if (portPart.isNotEmpty())
	{
		uint16_t port = core::strUtil::StringToInt<uint16_t>(portPart.c_str());
		address_.addr4.port = platform::htons(port);

#if X_DEBUG
		portPeekVal_ = platform::ntohs(address_.addr4.port);
#endif // !X_DEBUG
	}
	else
	{
		address_.addr4.port = oldPort;
	}

	return true;
}

bool SystemAddressEx::fromStringExplicitPort(const char* pAddressStr, uint16_t port, IpVersion::Enum ipVersion)
{
	X_ASSERT_NOT_NULL(pAddressStr);
	
	if (!fromString(pAddressStr, PORT_DELINEATOR, ipVersion)) {
		return false;
	}

	address_.addr4.port = platform::htons(port);

#if X_DEBUG
	portPeekVal_ = platform::ntohs(address_.addr4.port);
#endif // !X_DEBUG
	return true;
}

X_NAMESPACE_END