
X_NAMESPACE_BEGIN(net)

uint16_t SystemAdd::getPort(void) const
{
	return 0;
}

IpVersion::Enum SystemAdd::getIPVersion(void)
{
	if (address_.addr4.sin_family == AF_INET) {
		return IpVersion::Ipv4;
	}
	return IpVersion::Ipv6;
}

bool SystemAdd::IsLoopBack(void) const
{
	return false;
}

bool SystemAdd::IsLanAddress(void) const
{
	return (address_.addr4.sin_addr.s_addr >> 24) == 10 || (address_.addr4.sin_addr.s_addr >> 24) == 192;
}

// comparrision operators.
X_INLINE SystemAdd& SystemAdd::operator=(const SystemAdd& oth)
{
	memcpy(&address_, &oth.address_, sizeof(address_));
	systemIndex_ = oth.systemIndex_;
}

X_INLINE bool SystemAdd::operator==(const SystemAdd& rhs) const
{
	return address_.addr4.sin_port == rhs.address_.addr4.sin_port && equalExcludingPort(rhs);
}

X_INLINE bool SystemAdd::operator!=(const SystemAdd& rhs) const
{
	return !(*this == rhs);
}

X_INLINE bool SystemAdd::operator > (const SystemAdd& rhs) const
{
	if (address_.addr4.sin_port == rhs.address_.addr4.sin_port)
	{
#if NET_IPv6_SUPPORT
		if (address_.addr4.sin_family == AF_INET) {
			return address_.addr4.sin_addr.s_addr > rhs.address_.addr4.sin_addr.s_addr;
		}

		return std::memcmp(address_.addr6.sin6_addr.s6_addr, rhs.address_.addr6.sin6_addr.s6_addr,
			sizeof(address_.addr6.sin6_addr.s6_addr)) > 0;

#else
		return address_.addr4.sin_addr.s_addr > rhs.address_.addr4.sin_addr.s_addr;
#endif // !NET_IPv6_SUPPORT

	}

	return address_.addr4.sin_port > rhs.address_.addr4.sin_port;
}

X_INLINE bool SystemAdd::operator < (const SystemAdd& rhs) const
{
	if (address_.addr4.sin_port == rhs.address_.addr4.sin_port)
	{
#if NET_IPv6_SUPPORT
		if (address_.addr4.sin_family == AF_INET) {
			return address_.addr4.sin_addr.s_addr < rhs.address_.addr4.sin_addr.s_addr;
		}

		return std::memcmp(address_.addr6.sin6_addr.s6_addr, rhs.address_.addr6.sin6_addr.s6_addr,
			sizeof(address_.addr6.sin6_addr.s6_addr)) < 0;

#else
		return address_.addr4.sin_addr.s_addr < rhs.address_.addr4.sin_addr.s_addr;
#endif // !NET_IPv6_SUPPORT

	}

	return address_.addr4.sin_port < rhs.address_.addr4.sin_port;
}


X_NAMESPACE_END
