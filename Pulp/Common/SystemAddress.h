#pragma once

#include <INetwork.h>

X_NAMESPACE_DECLARE(core,
    class FixedBitStreamBase)

X_NAMESPACE_BEGIN(net)

class SystemAddress
{
public:
    static const char PORT_DELIMITER = ':';

public:
    SystemAddress();

    SystemAddress& operator=(const SystemAddress& oth);

    void writeToBitStream(core::FixedBitStreamBase& bs) const;
    void fromBitStream(core::FixedBitStreamBase& bs);
    static constexpr size_t serializedSize(void);

    X_INLINE uint16_t getPort(void) const;
    X_INLINE IpVersion::Enum getIPVersion(void) const;
    bool IsLoopBack(void) const;
    bool IsLanAddress(void) const;

    X_INLINE void setToLoopback(void);
    void setToLoopback(IpVersion::Enum ipVersion);

    X_INLINE void setPortFromHostByteOrder(uint16_t port);
    X_INLINE void setPortFromNetworkByteOrder(uint16_t port);

    bool operator==(const SystemAddress& rhs) const;
    bool operator!=(const SystemAddress& rhs) const;

    bool equalExcludingPort(const SystemAddress& oth) const;

protected:
    struct addr4_in
    {
        AddressFamily::Enum family;
        uint16_t port;

        union
        {
            uint8_t bytes[4];
            uint16 shorts[2];
            uint32_t as_int;
        } addr;

        uint8_t _zero_pad[8];
    };

#if NET_IPv6_SUPPORT
    struct addr6_in
    {
        AddressFamily::Enum family;
        uint16_t port;
        uint32_t flowInfo; // fuck you.

        union
        {
            uint8_t bytes[16];
            uint16 shorts[8];
        } addr;

        uint32_t scope_id;
    };
#endif // !NET_IPv6_SUPPORT

protected:
#if X_DEBUG
    uint16_t portPeekVal_; // host order.
#endif                     // !X_DEBUG \
                           // add both ip4 and ipv6.
    union
    {
        addr4_in addr4;

#if NET_IPv6_SUPPORT
        addr6_in addr6;
#endif // !NET_IPv6_SUPPORT
    } address_;
};

X_NAMESPACE_END

#include "SystemAddress.inl"
