
X_NAMESPACE_BEGIN(net)

X_INLINE constexpr size_t SystemAddress::serializedSize(void)
{
    return sizeof(address_);
}

X_INLINE uint16_t SystemAddress::getPort(void) const
{
    return ntoh(address_.addr4.port);
}

X_INLINE IpVersion::Enum SystemAddress::getIPVersion(void) const
{
    if (address_.addr4.family == AddressFamily::INet) {
        return IpVersion::Ipv4;
    }
#if NET_IPv6_SUPPORT
    return IpVersion::Ipv6;
#else
    X_ASSERT_UNREACHABLE();
    return IpVersion::Ipv4;
#endif // !NET_IPv6_SUPPORT
}

X_INLINE void SystemAddress::setToLoopback(void)
{
    setToLoopback(getIPVersion());
}

X_INLINE void SystemAddress::setPortFromHostByteOrder(uint16_t port)
{
    address_.addr4.port = hton(port);
#if X_DEBUG
    portPeekVal_ = port;
#endif // !X_DEBUG
}

X_INLINE void SystemAddress::setPortFromNetworkByteOrder(uint16_t port)
{
    address_.addr4.port = port;
#if X_DEBUG
    portPeekVal_ = ntoh(port);
#endif // !X_DEBUG
}

X_NAMESPACE_END
