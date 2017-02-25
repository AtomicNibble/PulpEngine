
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
	return address_.addr4.sin_port == rhs.address_.addr4.sin_port;
}

X_INLINE bool SystemAdd::operator!=(const SystemAdd& rhs) const
{
	return !(*this == rhs);
}

X_INLINE bool SystemAdd::operator > (const SystemAdd& rhs) const
{

	return false;
}

X_INLINE bool SystemAdd::operator < (const SystemAdd& rhs) const
{

	return false;
}


X_NAMESPACE_END
