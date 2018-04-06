
X_NAMESPACE_BEGIN(net)

X_INLINE const platform::sockaddr& SystemAddressEx::getSocketAdd(void) const
{
    if (getIPVersion() == IpVersion::Ipv4) {
        return reinterpret_cast<const platform::sockaddr&>(address_.addr4);
    }

#if NET_IPv6_SUPPORT
    return reinterpret_cast<const platform::sockaddr&>(address_.addr6);
#else
    X_ASSERT_UNREACHABLE();
    return reinterpret_cast<const platform::sockaddr&>(address_.addr4);
#endif // !NET_IPv6_SUPPORT
}

X_INLINE int32_t SystemAddressEx::getSocketAddSize(void) const
{
    if (getIPVersion() == IpVersion::Ipv4) {
        return sizeof(platform::sockaddr_in);
    }

#if !NET_IPv6_SUPPORT
    X_ASSERT_UNREACHABLE();
#endif // !NET_IPv6_SUPPORT

    return sizeof(platform::sockaddr_in6);
}

X_NAMESPACE_END
