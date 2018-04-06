#include "stdafx.h"
#include "Socket.h"

#include "Vars\NetVars.h"

#include "Util\LastErrorWSA.h"
#include <ITimer.h>

X_NAMESPACE_BEGIN(net)

namespace
{
    int32_t getRawIpProto(IpVersion::Enum ipVer)
    {
        switch (ipVer) {
            case IpVersion::Ipv4:
                return IPPROTO_IP;
#if NET_IPv6_SUPPORT
            case IpVersion::Ipv6:
                return platform::IPPROTO_IPV6;
#endif // !NET_IPv6_SUPPORT

            default:
                X_ASSERT_UNREACHABLE();
                break;
        }

        return IPPROTO_IP;
    }

    int32_t getRawSocketFamily(SocketFamily::Enum family)
    {
        static_assert(AddressFamily::INet == AF_INET, "AddressFamily enum don't match platform value");

#if NET_IPv6_SUPPORT
        static_assert(AddressFamily::INet6 == AF_INET6, "AddressFamily enum don't match platform value");
#endif // !NET_IPv6_SUPPORT

        return family;
    }

    int32_t getRawSocketType(SocketType::Enum type)
    {
        switch (type) {
            case SocketType::Stream:
                return SOCK_DGRAM;
            case SocketType::Dgram:
                return SOCK_DGRAM;
            case SocketType::Raw:
                return SOCK_DGRAM;
            case SocketType::Rdm:
                return SOCK_DGRAM;
            case SocketType::SeqPacket:
                return SOCK_DGRAM;

            case SocketType::Invalid:
            default:
                X_ASSERT_UNREACHABLE();
                break;
        }

        return SOCK_DGRAM;
    }

} // namespace

BindParameters::BindParameters()
{
    port = 0;

    nonBlockingSocket = false;
    IPHdrIncl = false;
    broadCast = false;
}

// -----------------------

NetSocket::NetSocket(NetVars& vars) :
    vars_(vars)
{
    socketType_ = SocketType::Invalid;
    socket_ = INVALID_SOCKET;
}

NetSocket::NetSocket(NetSocket&& oth) :
    vars_(oth.vars_)
{
    socketType_ = oth.socketType_;
    socket_ = oth.socket_;
    boundAdd_ = oth.boundAdd_;

    oth.socket_ = INVALID_SOCKET;
}

NetSocket::~NetSocket()
{
    if (socket_ != INVALID_SOCKET) {
        platform::closesocket(socket_);
    }
}

NetSocket& NetSocket::operator=(NetSocket&& oth)
{
    if (this != &oth) {
        vars_ = oth.vars_;
        socketType_ = oth.socketType_;
        socket_ = oth.socket_;
        boundAdd_ = oth.boundAdd_;

        oth.socket_ = INVALID_SOCKET;
    }
    return *this;
}

BindResult::Enum NetSocket::bind(BindParameters& bindParameters)
{
    struct platform::addrinfo* pResult = nullptr;
    struct platform::addrinfo* pPtr = nullptr;
    struct platform::addrinfo hints;

    socketType_ = bindParameters.socketType;

    core::zero_object(hints);
    hints.ai_family = getRawSocketFamily(bindParameters.socketFamily);
    hints.ai_socktype = getRawSocketType(socketType_);
    hints.ai_flags = AI_PASSIVE;

    core::StackString<32, char> portStr(bindParameters.port);
    lastErrorWSA::Description Dsc;

    int32_t res = getaddrinfo(0, portStr.c_str(), &hints, &pResult);
    if (res != 0) {
        X_ERROR("Net", "Failed to get address info for binding. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
        return BindResult::FailedToBind;
    }

    for (pPtr = pResult; pPtr != nullptr; pPtr = pPtr->ai_next) {
        socket_ = platform::socket(pPtr->ai_family, pPtr->ai_socktype, pPtr->ai_protocol);
        if (socket_ == INVALID_SOCKET) {
            X_ERROR("Net", "Failed to open socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
            platform::freeaddrinfo(pResult);
            return BindResult::FailedToBind;
        }

        // bind it like it's hot.
        int32_t ret = platform::bind(socket_, pPtr->ai_addr, safe_static_cast<int32_t>(pPtr->ai_addrlen));
        if (ret == 0) {
            boundAdd_.setFromSocket(socket_);

            setSocketOptions();
            setNonBlockingSocket(bindParameters.nonBlockingSocket);
            setBroadcastSocket(bindParameters.broadCast);

            if (socketType_ == SocketType::Raw) {
                setIPHdrIncl(bindParameters.IPHdrIncl);
            }

            platform::freeaddrinfo(pResult);

            // send a test.
            if (!sendSendTest()) {
                return BindResult::SendTestFailed;
            }

            return BindResult::Success;
        }
        else {
            // we failed to bind.
            // try next one.
            X_WARNING("Net", "Failed to bind socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
            platform::closesocket(socket_);
        }
    }

    // Shieeeeeeeeeeet.
    platform::freeaddrinfo(pResult);
    return BindResult::FailedToBind;
}

bool NetSocket::sendSendTest(void)
{
    // send a test.
    MessageID::Enum msgId = MessageID::SendTest;

    SendParameters sendParam;
    sendParam.pData = reinterpret_cast<uint8_t*>(&msgId);
    sendParam.length = sizeof(msgId);
    sendParam.systemAddress = boundAdd_;
    sendParam.ttl = 0;

    return send(sendParam) == sizeof(msgId);
}

SendResult NetSocket::send(SendParameters& sendParameters)
{
    X_ASSERT(sendParameters.length > 0, "Can't send empty buffer")
    ();

    // eat it you slag!
    int32_t oldTtl = -1;

    X_LOG0_IF(vars_.debugSocketsEnabled(), "Net", "^2socket::^3send:^7 pData ^5%p^7 length: ^5%" PRIi32 "^7 ttl: ^5%" PRIi32,
        sendParameters.pData, sendParameters.length, sendParameters.ttl);

#if X_ENABLE_NET_STATS
    ++stats_.numPacketsSent;
    stats_.numBytesSent += sendParameters.length;
#endif // !X_ENABLE_NET_STATS

    const auto ipVer = sendParameters.systemAddress.getIPVersion();
    if (sendParameters.ttl > 0) {
        if (getTTL(ipVer, oldTtl)) {
            setTTL(ipVer, sendParameters.ttl);
        }
    }

    int32_t len;
    do {
        len = platform::sendto(socket_,
            reinterpret_cast<const char*>(sendParameters.pData),
            sendParameters.length,
            0,
            &sendParameters.systemAddress.getSocketAdd(),
            sendParameters.systemAddress.getSocketAddSize());

        if (len < 0) {
            lastErrorWSA::Description Dsc;
            int32_t lastErr = lastErrorWSA::Get();
            len = -lastErr; // pass back the last err but negative
            X_ERROR("Net", "Failed to sendto, length: %" PRIi32 ". Error: \"%s\"", sendParameters.length, lastErrorWSA::ToString(lastErr, Dsc));
        }

    } while (len == 0); // keep trying, while not sent anything and not had a error.

    if (oldTtl != -1) {
        setTTL(ipVer, oldTtl);
    }

    return len;
}

RecvResult::Enum NetSocket::recv(RecvData& dataOut)
{
    platform::sockaddr_storage senderAddr;
    platform::socklen_t senderAddLen;
    core::zero_object(senderAddr);

    const int32_t flags = 0;
    senderAddLen = sizeof(senderAddr);

    int32_t bytesRead = platform::recvfrom(
        socket_,
        reinterpret_cast<char*>(dataOut.data),
        sizeof(dataOut.data),
        flags,
        reinterpret_cast<platform::sockaddr*>(&senderAddr),
        &senderAddLen);

    dataOut.bytesRead = bytesRead;

    X_LOG0_IF(vars_.debugSocketsEnabled(), "Net", "^2socket::^1recv:^7 length: ^5%" PRIi32, bytesRead);

    if (bytesRead <= 0) {
        // this is not error, just making it so single branch if above zero.
        if (bytesRead == 0) {
            return RecvResult::Success;
        }

        // error.
        const auto err = lastErrorWSA::Get();
        lastErrorWSA::Description Dsc;

        if (err == WSAECONNRESET) {
            dataOut.systemAddress.setFromAddStorage(senderAddr);

            IPStr ipStr;
            X_WARNING("Net", "Failed to recvfrom. \"%s\" Add: \"%s\"",
                lastErrorWSA::ToString(err, Dsc), dataOut.systemAddress.toString(ipStr));

            return RecvResult::ConnectionReset;
        }

        X_ERROR("Net", "Failed to recvfrom. Error: \"%s\"", lastErrorWSA::ToString(err, Dsc));
        return RecvResult::Error;
    }

#if X_ENABLE_NET_STATS
    ++stats_.numPacketsRecived;
    stats_.numBytesRecived += bytesRead;
#endif // !X_ENABLE_NET_STATS

    dataOut.timeRead = gEnv->pTimer->GetTimeNowReal();
    dataOut.systemAddress.setFromAddStorage(senderAddr);
    dataOut.pSrcSocket = this;
    return RecvResult::Success;
}

bool NetSocket::getMyIPs(SystemAddArr& addresses)
{
    addresses.clear();

    int32_t res;
    char hostName[256];
    res = platform::gethostname(hostName, sizeof(hostName));
    if (res != 0) {
        lastErrorWSA::Description Dsc;
        X_ERROR("Net", "Failed to get hostname. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
        return false;
    }

    struct platform::addrinfo* pResult = nullptr;
    struct platform::addrinfo* pPtr = nullptr;
    struct platform::addrinfo hints;

    core::zero_object(hints);
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    res = getaddrinfo(hostName, "", &hints, &pResult);
    if (res != 0) {
        lastErrorWSA::Description Dsc;
        X_ERROR("Net", "Failed to get address info for local hostname. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
        return false;
    }

    for (pPtr = pResult; pPtr != nullptr; pPtr = pPtr->ai_next) {
        if (addresses.size() == addresses.capacity()) {
            X_WARNING("Net", "Found more local ips that supported ignoring remaning addresses");
            break;
        }

        auto& sysAdd = addresses.AddOne();
        sysAdd.setFromAddInfo(pPtr);
    }

    platform::freeaddrinfo(pResult);
    return true;
}

void NetSocket::setNonBlockingSocket(bool nonblocking)
{
    using platform::u_long;

    u_long noneBlockingLng = nonblocking ? 1 : 0;

    int32_t res = platform::ioctlsocket(socket_, FIONBIO, &noneBlockingLng);
    if (res != 0) {
        lastErrorWSA::Description Dsc;
        X_ERROR("Net", "Failed to set nonblocking mode on socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
    }
}

void NetSocket::setSocketOptions(void)
{
    int32_t res;
    int32_t sock_opt;
    lastErrorWSA::Description Dsc;

    // set the recive buffer to decent size
    sock_opt = 1024 * 256;
    res = platform::setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, (char*)&sock_opt, sizeof(sock_opt));
    if (res != 0) {
        X_ERROR("Net", "Failed to set max rcvbuf on socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
    }

    // decent size buf for send.
    sock_opt = 1024 * 16;
    res = platform::setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, (char*)&sock_opt, sizeof(sock_opt));
    if (res != 0) {
        X_ERROR("Net", "Failed to set max sndbuf on socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
    }

#if 0
	// don't linger on close if un-sent data present.
	sock_opt = 0;
	res = platform::setsockopt(socket_, SOL_SOCKET, SO_LINGER, (char*)&sock_opt, sizeof(sock_opt));
	if (res != 0)
	{
		X_WARNING("Net", "Failed to set linger mode on socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
	}
#endif
}

void NetSocket::setBroadcastSocket(bool broadcast)
{
    int32_t res;
    int32_t val = broadcast ? 1 : 0;

    // enable / disable broadcast.
    res = platform::setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, (char*)&val, sizeof(val));
    if (res != 0) {
        lastErrorWSA::Description Dsc;
        X_ERROR("Net", "Failed to set broadcast mode on socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
    }
}

void NetSocket::setIPHdrIncl(bool ipHdrIncl)
{
    X_ASSERT(socketType_ == SocketType::Raw, "Must be a raw socket")
    (socketType_);

    int32_t res;
    int32_t val = ipHdrIncl ? 1 : 0;

    /*
	When set to TRUE, indicates the application provides the IP header. 
	Applies only to SOCK_RAW sockets. The TCP/IP service provider may set the ID field, 
	if the value supplied by the application is zero.

	The IP_HDRINCL option is applied only to the SOCK_RAW type of protocol.
*/

    res = platform::setsockopt(socket_, IPPROTO_IP, IP_HDRINCL, (char*)&val, sizeof(val));
    if (res != 0) {
        lastErrorWSA::Description Dsc;
        X_ERROR("Net", "Failed to set hdrincl on socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
    }
}

void NetSocket::setTTL(IpVersion::Enum ipVer, int32_t ttl)
{
    int32_t res;

    res = platform::setsockopt(socket_, getRawIpProto(ipVer), IP_TTL, (char*)&ttl, sizeof(ttl));
    if (res != 0) {
        lastErrorWSA::Description Dsc;
        X_ERROR("Net", "Failed to set TTL on socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
    }
}

bool NetSocket::getTTL(IpVersion::Enum ipVer, int32_t& ttl)
{
    int32_t res;
    int32_t len = sizeof(ttl);

    res = platform::getsockopt(socket_, getRawIpProto(ipVer), IP_TTL, (char*)&ttl, &len);
    if (res != 0) {
        lastErrorWSA::Description Dsc;
        X_ERROR("Net", "Failed to get TTL on socket. Error: \"%s\"", lastErrorWSA::ToString(Dsc));
        return false;
    }

    return true;
}

#if X_ENABLE_NET_STATS

NetBandwidthStatistics NetSocket::getStats(void) const
{
    return stats_;
}

#endif // !X_ENABLE_NET_STATS

X_NAMESPACE_END
