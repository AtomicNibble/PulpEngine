#pragma once


X_NAMESPACE_BEGIN(net)


class SystemAdd : public ISystemAdd
{
public:

	static const char* IPV6_LOOPBACK;
	static const char* IPV4_LOOPBACK;
	static const char PORT_DELINEATOR = '|';

public:
	SystemAdd();
	SystemAdd(const char* pAddressStr);
	SystemAdd(const char* pAddressStr, uint16_t port);
	~SystemAdd() X_FINAL;

	X_INLINE uint16_t getPort(void) const X_FINAL;
	
	X_INLINE IpVersion::Enum getIPVersion(void) X_FINAL;
	X_INLINE bool IsLoopBack(void) const X_FINAL;
	X_INLINE bool IsLanAddress(void) const X_FINAL;

	const char* toString(AddressStr& strBuf, bool incPort = true) X_FINAL;

	// comparrision operators.
	X_INLINE SystemAdd& operator=(const SystemAdd& oth);
	X_INLINE bool operator==(const SystemAdd& rhs) const;
	X_INLINE bool operator!=(const SystemAdd& rhs) const;
	X_INLINE bool operator > (const SystemAdd& rhs) const;
	X_INLINE bool operator < (const SystemAdd& rhs) const;

private:

	bool fromString(const char* pAddressStr, char portDelineator = PORT_DELINEATOR, IpVersion::Enum ipVersion = IpVersion::Ipv4);
	bool fromStringExplicitPort(const char* pAddressStr, uint16_t port, IpVersion::Enum ipVersion = IpVersion::Ipv4);

private:
	union// In6OrIn4
	{
#if NET_IPv6_SUPPORT 
		platform::sockaddr_in6 addr6;
#endif // !NET_IPv6_SUPPORT 
		platform::sockaddr_in addr4;
	} address_;


	SystemIndex systemIndex_;
};


X_NAMESPACE_END

#include "SystemAdd.inl"