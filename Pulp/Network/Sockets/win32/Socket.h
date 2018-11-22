#pragma once

X_NAMESPACE_BEGIN(net)

class NetVars;

X_DECLARE_ENUM(BindResult)
(
    Success,
    FailedToBind,
    SendTestFailed,
    AddrInUse);

X_DECLARE_ENUM(SocketType)
(
    Stream,    // stream
    Dgram,     // datagram
    Raw,       // raw-protocol
    Rdm,       // reliably-deliverd
    SeqPacket, // sequenced packet stream
    Invalid);

// anyhing negative is error.
typedef int32_t SendResult;

X_DECLARE_ENUM(RecvResult)
(
    Success,
    ConnectionReset,
    Error);

struct BindParameters
{
    BindParameters();

    HostStr hostAddr;
    Port port;
    SocketFamily::Enum socketFamily;
    SocketType::Enum socketType;

    bool nonBlockingSocket;
    bool IPHdrIncl;
    bool broadCast;
    bool _pad;
};

struct SendParameters
{
    X_INLINE SendParameters() :
        pData(nullptr),
        length(0),
        ttl(0)
    {
    }

    X_INLINE void setData(core::BitStream& bs)
    {
        pData = bs.data();
        length = safe_static_cast<int32_t>(bs.sizeInBytes());
    }
    X_INLINE void setData(core::FixedBitStreamBase& bs)
    {
        pData = bs.data();
        length = safe_static_cast<int32_t>(bs.sizeInBytes());
    }

    uint8_t* pData;
    int32_t length;
    int32_t ttl;
    SystemAddressEx systemAddress;
};

class NetSocket;

struct RecvData
{
    uint8_t data[MAX_MTU_SIZE];
    int32_t bytesRead;
    core::TimeVal timeRead;
    SystemAddressEx systemAddress;
    NetSocket* pSrcSocket;
};

class NetSocket
{
public:
    typedef core::FixedArray<SystemAddressEx, MAX_INTERNAL_IDS> SystemAddArr;

public:
    NetSocket(NetVars& vars);
    NetSocket(NetSocket&& oth);
    ~NetSocket();

    NetSocket& operator=(NetSocket&& oth);

    void drainRecv(void);

    BindResult::Enum bind(BindParameters& bindParameters);
    bool sendSendTest(void);
    SendResult send(SendParameters& sendParameters);
    RecvResult::Enum recv(RecvData& dataOut);

    X_INLINE const SystemAddressEx& getBoundAdd(void) const;

#if X_ENABLE_NET_STATS
    NetBandwidthStatistics getStats(void) const;
#endif // !X_ENABLE_NET_STATS

public:
    static bool getMyIPs(SystemAddArr& addresses);

private:
    void setNonBlockingSocket(bool nonblocking);
    void setSocketOptions(void);
    void setBroadcastSocket(bool broadcast);
    void setIPHdrIncl(bool ipHdrIncl);

    int32_t getPendingBytes(void) const;

    void setTTL(IpVersion::Enum ipVer, int32_t ttl);
    bool getTTL(IpVersion::Enum ipVer, int32_t& ttl);

private:
    X_NO_COPY(NetSocket);
    X_NO_ASSIGN(NetSocket);

private:
    NetVars& vars_;

    SocketType::Enum socketType_;
    SocketHandle socket_;
    SystemAddressEx boundAdd_;

    // False sharing is not much of a issue here.
    core::AtomicInt waiting_;

#if X_ENABLE_NET_STATS
    NetBandwidthStatistics stats_;
#endif // !X_ENABLE_NET_STATS
};

X_NAMESPACE_END

#include "Socket.inl"