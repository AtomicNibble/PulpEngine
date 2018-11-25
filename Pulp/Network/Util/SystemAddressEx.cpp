#include "stdafx.h"
#include "SystemAddressEx.h"

#include "Util\LastErrorWSA.h"

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
    if (address_.addr4.port == rhs.address_.addr4.port) {
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
    if (address_.addr4.port == rhs.address_.addr4.port) {
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

    if (platform::getsockname(socket, (struct platform::sockaddr*)&ss, &slen) != 0) {
        lastErrorWSA::Description Dsc;
        X_FATAL("Net", "Failed to get socket name for socket: %" PRIu32 " Error: \"%s\"", lastErrorWSA::ToString(Dsc));
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

    if (ss.ss_family == AF_INET) {
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
    if (pAddInfo->ai_family == AF_INET) {
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
    if (addStorage.ss_family == AF_INET) {
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

    if (address_.addr4.family == AddressFamily::INet) {
        ret = platform::getnameinfo((struct platform::sockaddr*)&address_.addr4,
            sizeof(struct platform::sockaddr_in),
            tmpBuf,
            sizeof(tmpBuf),
            nullptr,
            0,
            NI_NUMERICHOST);
    }
    else
#if NET_IPv6_SUPPORT
    {
        ret = platform::getnameinfo((struct platform::sockaddr*)&address_.addr6,
            sizeof(struct platform::sockaddr_in6),
            tmpBuf,
            sizeof(tmpBuf), // INET6_ADDRSTRLEN,
            nullptr,
            0,
            NI_NUMERICHOST);
    }
#else
    {
        X_ASSERT_UNREACHABLE();
    }
#endif // !NET_IPv6_SUPPORT

    if (ret != 0) {
        lastErrorWSA::Description Dsc;
        X_ERROR("Net", "Failed to get name info. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
        strBuf.clear();
        return strBuf.c_str();
    }

    strBuf.set(tmpBuf);

    if (incPort) {
        strBuf.appendFmt("%c%" PRIu16, PORT_DELIMITER, platform::ntohs(address_.addr4.port));
    }
     
    return strBuf.c_str();
}

bool SystemAddressEx::fromIP(const IPStr& ip, char portDelineator, IpVersion::Enum ipVersion)
{
    return fromString(ip.begin(), ip.end(), false, portDelineator, ipVersion);
}

bool SystemAddressEx::fromIP(const IPStr& ip, Port port, IpVersion::Enum ipVersion)
{
    bool res = fromString(ip.begin(), ip.end(), false, PORT_DELIMITER, ipVersion);
    setPortFromHostByteOrder(port);
    return res;
}

bool SystemAddressEx::fromHost(const HostStr& host, char portDelineator, IpVersion::Enum ipVersion)
{
    return fromString(host.begin(), host.end(), true, portDelineator, ipVersion);
}

bool SystemAddressEx::fromHost(const HostStr& host, Port port, IpVersion::Enum ipVersion)
{
    bool res = fromString(host.begin(), host.end(), true, PORT_DELIMITER, ipVersion);
    setPortFromHostByteOrder(port);
    return res;
}

bool SystemAddressEx::fromString(const char* pBegin, const char* pEnd, bool isHost, char portDelineator, IpVersion::Enum ipVersion)
{
    HostStr hostStr;
    core::StackString<32, char> portStr;

    const char* pPort = core::strUtil::Find(pBegin, pEnd, portDelineator);
    if (pPort) {
        hostStr.set(pBegin, pPort);
        portStr.set(pPort + 1, pEnd);
    }
    else {
        hostStr.set(pBegin, pEnd);
    }

    const uint16_t oldPort = address_.addr4.port;

    AddressArr address;
    if (!resolve(hostStr, isHost, address, ipVersion)) {
        return false;
    }

    X_ASSERT(address.isNotEmpty(), "Empty address list")();

    // pick a random one.
    size_t idx = 0;
    if (address.size() > 1) {
        idx = gEnv->xorShift.randIndex(address.size());
    }

    const auto& newAddress = address[idx];

    address_ = newAddress.address_;

    // port?
    if (portStr.isNotEmpty()) {
        X_ASSERT(core::strUtil::IsNumeric(portStr.begin(), portStr.end()), "PortStr is not numeric")(portStr.c_str());

        uint16_t port = core::strUtil::StringToInt<uint16_t>(portStr.c_str());
        address_.addr4.port = platform::htons(port);

#if X_DEBUG
        portPeekVal_ = platform::ntohs(address_.addr4.port);
#endif // !X_DEBUG
    }
    else {
        address_.addr4.port = oldPort;
    }

    return true;
}

bool SystemAddressEx::resolve(const HostStr& hostStr, bool isHost, AddressArr& address, IpVersion::Enum ipVersion)
{
    // resolve
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
    else if (ipVersion == IpVersion::Any) {
        hints.ai_family = AF_UNSPEC;
    }
    else {
        X_ASSERT_UNREACHABLE();
    }

    platform::getaddrinfo(hostStr.c_str(), "", &hints, &servinfo);
    if (servinfo == 0) {
        // if ipv6 try it as ipv4.
#if NET_IPv6_SUPPORT
        if (ipVersion == IpVersion::Ipv6) {
            ipVersion = IpVersion::Ipv4;
            hints.ai_family = AF_INET;

            platform::getaddrinfo(hostStr.c_str(), "", &hints, &servinfo);
            if (servinfo == 0) {
                // failed still
                lastErrorWSA::Description Dsc;
                X_ERROR("Net", "Failed to get addres info. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
                return false;
            }
            else {
                X_WARNING("Net", "Address passed as Ipv6, but is a valid Ipv4 address. add: \"%s\"", hostStr.c_str());
            }
        }
        else
#endif // !NET_IPv6_SUPPORT
        {
            lastErrorWSA::Description Dsc;
            X_ERROR("Net", "Failed to get address info for: \"%s\" Error: \"%s\"", hostStr.c_str(), lastErrorWSA::ToString(Dsc));
            return false;
        }
    }

    X_ASSERT(servinfo != 0, "ServerInfo not valid")(servinfo);

    auto* pCurAddr = servinfo;

    do
    {
        SystemAddressEx addr;

        if (pCurAddr->ai_family == AF_INET) {
            static_assert(sizeof(address_.addr4) == sizeof(struct platform::sockaddr_in), "Potentiall buffer overrun.");

            // offset lignup checks,
            static_assert(X_OFFSETOF(addr4_in, family) == X_OFFSETOF(platform::sockaddr_in, sin_family), "offset mismatch");
            static_assert(X_OFFSETOF(addr4_in, port) == X_OFFSETOF(platform::sockaddr_in, sin_port), "offset mismatch");
            static_assert(X_OFFSETOF(addr4_in, addr) == X_OFFSETOF(platform::sockaddr_in, sin_addr), "offset mismatch");

            addr.address_.addr4.family = AddressFamily::INet;
            std::memcpy(&addr.address_.addr4, (struct platform::sockaddr_in*)pCurAddr->ai_addr, sizeof(struct platform::sockaddr_in));
        }
#if NET_IPv6_SUPPORT
        else {
            X_ASSERT(pCurAddr->ai_family == AF_INET6, "Unexpected familey")(pCurAddr->ai_family);
            X_ASSERT(pCurAddr->ai_addrlen == sizeof(struct platform::sockaddr_in6), "Address length is diffrent than expected")(servinfo->ai_addrlen, sizeof(struct platform::sockaddr_in6));

            static_assert(sizeof(address_.addr6) == sizeof(struct platform::sockaddr_in6), "Potentiall buffer overrun.");

            // offset lignup checks,
            static_assert(X_OFFSETOF(addr6_in, family) == X_OFFSETOF(platform::sockaddr_in6, sin6_family), "offset mismatch");
            static_assert(X_OFFSETOF(addr6_in, port) == X_OFFSETOF(platform::sockaddr_in6, sin6_port), "offset mismatch");
            static_assert(X_OFFSETOF(addr6_in, flowInfo) == X_OFFSETOF(platform::sockaddr_in6, sin6_flowinfo), "offset mismatch");
            static_assert(X_OFFSETOF(addr6_in, addr) == X_OFFSETOF(platform::sockaddr_in6, sin6_addr), "offset mismatch");
            static_assert(X_OFFSETOF(addr6_in, scope_id) == X_OFFSETOF(platform::sockaddr_in6, sin6_scope_id), "offset mismatch");

            addr.address_.addr4.family = AddressFamily::INet6;
            std::memcpy(&addr.address_.addr6, (struct platform::sockaddr_in6*)pCurAddr->ai_addr, sizeof(struct platform::sockaddr_in6));
        }
#else
        else {
            X_ASSERT_UNREACHABLE();
        }
#endif // !NET_IPv6_SUPPORT

        address.emplace_back(std::move(addr));

        pCurAddr = pCurAddr->ai_next;

    } while (pCurAddr && address.size() < address.capacity());

    platform::freeaddrinfo(servinfo); // free the linked list
    return true;
}

bool SystemAddressEx::isValidIP(const IPStr& ip, IpVersion::Enum ipVersion)
{
    if (ipVersion == IpVersion::Any || ipVersion == IpVersion::Ipv4) {
        platform::in_addr add;

        if (platform::inet_pton(AF_INET, ip.c_str(), &add) == 1) {
            return true;
        }
    }

    if (ipVersion == IpVersion::Any || ipVersion == IpVersion::Ipv6) {
        platform::in6_addr add6;

        if (platform::inet_pton(AF_INET, ip.c_str(), &add6) == 1) {
            return true;
        }
    }

    return false;
}

X_NAMESPACE_END