#pragma once


X_NAMESPACE_BEGIN(net)

class SystemAdd : public ISystemAdd
{
public:
	static const char* IP_LOOPBACK[IpVersion::ENUM_COUNT];
	static const char PORT_DELINEATOR = '|';

public:
	SystemAdd();
	SystemAdd(const char* pAddressStr);
	SystemAdd(const char* pAddressStr, uint16_t port);
	~SystemAdd() X_FINAL;

	void setFromSocket(SocketHandle socket);
	void setFromAddInfo(platform::addrinfo* pAddInfo);
	void setFromAddStorage(const platform::sockaddr_storage& addStorage);

	void setToLoopback(void);
	void setToLoopback(IpVersion::Enum ipVersion);

	void setPortFromHostByteOrder(uint16_t port);
	void setPortFromNetworkByteOrder(uint16_t port);

	X_INLINE const platform::sockaddr& getSocketAdd(void) const;
	X_INLINE int32_t getSocketAddSize(void) const;

	// ISystemAdd
	X_INLINE uint16_t getPort(void) const X_FINAL;
	X_INLINE IpVersion::Enum getIPVersion(void) const X_FINAL;
	X_INLINE bool IsLoopBack(void) const X_FINAL;
	X_INLINE bool IsLanAddress(void) const X_FINAL;

	const char* toString(IPStr& strBuf, bool incPort = true) const X_FINAL;
	// ~ISystemAdd

	// comparrision operators.
	X_INLINE SystemAdd& operator=(const SystemAdd& oth);
	X_INLINE bool operator==(const SystemAdd& rhs) const;
	X_INLINE bool operator!=(const SystemAdd& rhs) const;
	X_INLINE bool operator > (const SystemAdd& rhs) const;
	X_INLINE bool operator < (const SystemAdd& rhs) const;

	bool equalExcludingPort(const SystemAdd& oth) const;
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

#if X_DEBUG
	uint16_t portPeekVal_; // in host byte order just for debugging.
#endif // !X_DEBUG
	SystemIndex systemIndex_;
};


X_NAMESPACE_END

#include "SystemAdd.inl"