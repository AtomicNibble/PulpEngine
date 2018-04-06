#include "EngineCommon.h"
#include "SystemAddress.h"

#include <Containers\FixedBitStream.h>

X_NAMESPACE_BEGIN(net)

// ------------------------------------------

SystemAddress::SystemAddress()
{
#if NET_IPv6_SUPPORT
    // check we can use the ipv4 family / port to lookup the value for ipv4 and ipv6.
    static_assert(X_OFFSETOF(SystemAddress, address_.addr4.port) == X_OFFSETOF(SystemAddress, address_.addr6.port),
        "ports offsets not match");

    static_assert(X_OFFSETOF(SystemAddress, address_.addr4.family) == X_OFFSETOF(SystemAddress, address_.addr6.family),
        "ports offsets not match");
#endif // !NET_IPv6_SUPPORT

    core::zero_object(address_);
    address_.addr4.family = AddressFamily::INet;

#if X_DEBUG
    portPeekVal_ = 0;
#endif // !X_DEBUG
}

SystemAddress& SystemAddress::operator=(const SystemAddress& oth)
{
    address_ = oth.address_;

#if X_DEBUG
    portPeekVal_ = oth.portPeekVal_;
#endif // !X_DEBUG
    return *this;
}

void SystemAddress::writeToBitStream(core::FixedBitStreamBase& bs) const
{
    bs.writeAligned(address_);
}

void SystemAddress::fromBitStream(core::FixedBitStreamBase& bs)
{
    bs.readAligned(address_);

#if X_DEBUG
    portPeekVal_ = ntoh(address_.addr4.port);
#endif // !X_DEBUG
}

bool SystemAddress::IsLoopBack(void) const
{
    if (getIPVersion() == IpVersion::Ipv4) {
        // check for 1.
        if (address_.addr4.addr.as_int == X_TAG(127, 0, 0, 1)) {
            return true;
        }
        if (address_.addr4.addr.as_int == 0) {
            return true;
        }
    }
#if NET_IPv6_SUPPORT
    else {
        // meow meow meow.
        const static char localhost[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        if (std::memcmp(&address_.addr6.addr, localhost, sizeof(address_.addr6.addr)) == 0) {
            return true;
        }
    }
#endif // !NET_IPv6_SUPPORT

    return false;
}

bool SystemAddress::IsLanAddress(void) const
{
    return (address_.addr4.addr.as_int >> 24) == 10 || (address_.addr4.addr.as_int >> 24) == 192;
}

void SystemAddress::setToLoopback(IpVersion::Enum ipVersion)
{
    if (ipVersion == IpVersion::Ipv4) {
        address_.addr4.addr.bytes[0] = 127;
        address_.addr4.addr.bytes[1] = 0;
        address_.addr4.addr.bytes[2] = 0;
        address_.addr4.addr.bytes[3] = 1;
    }
#if NET_IPv6_SUPPORT
    else {
        // meow meow meow.
        const static char localhost[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        std::memcpy(&address_.addr6.addr, localhost, sizeof(address_.addr6.addr));
    }
#endif // !NET_IPv6_SUPPORT

    X_ASSERT(IsLoopBack(), "Failed to set to loopback")();
}

bool SystemAddress::operator==(const SystemAddress& rhs) const
{
    return address_.addr4.port == rhs.address_.addr4.port && equalExcludingPort(rhs);
}

bool SystemAddress::operator!=(const SystemAddress& rhs) const
{
    return !(*this == rhs);
}

bool SystemAddress::equalExcludingPort(const SystemAddress& oth) const
{
    bool ipv4Equal = (address_.addr4.family == AddressFamily::INet && address_.addr4.addr.as_int == oth.address_.addr4.addr.as_int);
#if NET_IPv6_SUPPORT
    bool ipv6Equal = (address_.addr4.family == AddressFamily::INet6 && std::memcmp(&address_.addr6.addr, &oth.address_.addr6.addr, sizeof(address_.addr6.addr)) == 0);

    return ipv4Equal || ipv6Equal;

#else
    return ipv4Equal;
#endif // !NET_IPv6_SUPPORT
}

X_NAMESPACE_END
