#include "stdafx.h"
#include "TelemetryLib.h"

#include <cstdio>

#include <../TelemetryCommon/Version.h>
#include <../TelemetryCommon/PacketTypes.h>

namespace platform
{
    #ifndef NEAR
    #define NEAR
    #endif

    #ifndef FAR
    #define FAR
    #endif

    #include <WinSock2.h>
    #include <Ws2tcpip.h>

    X_LINK_LIB("Ws2_32.lib");

} // namespace platform


namespace
{
    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

    struct TraceContext
    {
        tt_uint8* pScratchBuf;
        tt_size bufLen;

        bool isEnabled;
        bool _pad[3];

        platform::SOCKET socket;
    };

    void sendPacketToServer(TraceContext* pCtx, const void* pData, tt_size len)
    {
        // send some data...
        int res = platform::send(pCtx->socket, reinterpret_cast<const char*>(pData), static_cast<int>(len), 0);
        if (res == SOCKET_ERROR) {
            printf("send failed with error: %d\n", platform::WSAGetLastError());
            return;
        }
    }

    void addDataPacket(TraceContext* pCtx, const void* pData, tt_size len)
    {
        // add packet to end of buffer, if we are full force a flush?
        DataStreamData ds;
        ds.type = PacketType::DataStream;
        ds.dataSize = static_cast<tt_uint32>(len);

        sendPacketToServer(pCtx, &ds, sizeof(ds));
        sendPacketToServer(pCtx, pData, len);
    }

    bool handleConnectionResponse(tt_uint8* pData, tt_size len)
    {
        if (len < 1) {
            return false;
        }

        tt_uint8 type = pData[0];
        if (type >= PacketType::Num) {
            return false;
        }

        switch (type)
        {
            case PacketType::ConnectionRequestAccepted:
                // don't care about response currently.
                return true;
            case PacketType::ConnectionRequestRejected: {
                if (len != sizeof(ConnectionRequestRejectedData)) {
                    printf("Recived invalid connection rejected packet\n");
                    return false;
                }
                
                auto* pConRej = reinterpret_cast<const ConnectionRequestRejectedData*>(pData);
                printf("Connection rejected: %s\n", pConRej->reason);
            }
            default:
                break;
        }

        return false;
    }

    TraceContexHandle contextToHandle(TraceContext* pCtx)
    {
        return reinterpret_cast<TraceContexHandle>(pCtx);
    }

    TraceContext* handleToContext(TraceContexHandle handle)
    {
        return reinterpret_cast<TraceContext*>(handle);
    }

    bool isValidContext(TraceContexHandle handle)
    {
        return handle != INVALID_TRACE_CONTEX;
    }

    X_INLINE tt_uint64 getTicks(void)
    {
        return __rdtsc();
    }


} // namespace


bool TelemInit(void)
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    platform::WSADATA winsockInfo;

    if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
        return false;
    }

    return true;
}

void TelemShutDown(void)
{
    if (platform::WSACleanup() != 0) {
        // rip
        return;
    }
}

bool TelemInitializeContext(TraceContexHandle& out, void* pBuf, tt_size bufLen)
{
    out = INVALID_TRACE_CONTEX;

    auto contexSize = sizeof(TraceContext);
    if (bufLen < contexSize) {
        return false;
    }

    tt_uint8* pBufU8 = reinterpret_cast<tt_uint8*>(pBuf);

    TraceContext* pCtx = reinterpret_cast<TraceContext*>(pBuf);
    pCtx->pScratchBuf = pBufU8 + contexSize;
    pCtx->bufLen = bufLen - contexSize;
    pCtx->isEnabled = true;
    pCtx->socket = INV_SOCKET;

    out = contextToHandle(pCtx);
    return true;
}

void TelemShutdownContext(TraceContexHandle ctx)
{
    X_UNUSED(ctx);
}

TtError TelemOpen(TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
    tt_uint16 serverPort, TtConnectionType conType, tt_int32 timeoutMS)
{
    if (!isValidContext(ctx)) {
        return TtError::InvalidContex;
    }

    if (conType != TtConnectionType::Tcp) {
        return TtError::InvalidParam;
    }

    X_UNUSED(pAppName);
    X_UNUSED(pBuildInfo);
    X_UNUSED(timeoutMS);

    // need to open a connection yo.
    // should i reuse all my networking stuff.
    // or just use TCP.
    // think it be good if it's all totaly seperate from engine tho.
    // so lets just write some raw winsock shit.
    X_UNUSED(pServerAddress);
    X_UNUSED(serverPort);

    // need to connect to the server :O
    struct platform::addrinfo hints, *servinfo = nullptr;
    zero_object(hints);
    hints.ai_family = AF_UNSPEC; // ipv4/6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = platform::IPPROTO_TCP;

    char portStr[32] = {};
    sprintf(portStr, "%d", serverPort);

    // Resolve the server address and port
    auto res = platform::getaddrinfo(pServerAddress, portStr, &hints, &servinfo);
    if (res != 0) {
        return TtError::Error;
    }

    platform::SOCKET connectSocket = INV_SOCKET;

    for (auto pPtr = servinfo; pPtr != nullptr; pPtr = pPtr->ai_next) {
        // Create a SOCKET for connecting to server
        connectSocket = platform::socket(pPtr->ai_family, pPtr->ai_socktype, pPtr->ai_protocol);
        if (connectSocket == INV_SOCKET) {
            return TtError::Error;
        }

        // Connect to server.
        res = connect(connectSocket, pPtr->ai_addr, static_cast<int>(pPtr->ai_addrlen));
        if (res == SOCKET_ERROR) {
            platform::closesocket(connectSocket);
            connectSocket = INV_SOCKET;
            continue;
        }

        break;
    }

    platform::freeaddrinfo(servinfo);

    if (connectSocket == INV_SOCKET) {
        return TtError::Error;
    }

    auto* pCtx = handleToContext(ctx);
    pCtx->socket = connectSocket;

    ConnectionRequestData cr;
    zero_object(cr);
    cr.type = PacketType::ConnectionRequest;
    cr.clientVer.major = X_TELEMETRY_VERSION_MAJOR;
    cr.clientVer.minor = X_TELEMETRY_VERSION_MINOR;
    cr.clientVer.patch = X_TELEMETRY_VERSION_PATCH;
    cr.clientVer.build = X_TELEMETRY_VERSION_BUILD;

    strcpy_s(cr.appName, pAppName);
    strcpy_s(cr.buildInfo, pBuildInfo);

    sendPacketToServer(pCtx, &cr, sizeof(cr));

    // wait for a response O.O
    char recvbuf[MAX_PACKET_SIZE];
    int recvbuflen = sizeof(recvbuf);

    // TODO: support timeout.
    res = platform::recv(connectSocket, recvbuf, recvbuflen, 0);

    // we should get a packet back like a hot slut.
    if (res == 0) {
        TtError::Error;
    }
    if (res < 0) {
        TtError::Error;
    }

    if (!handleConnectionResponse(reinterpret_cast<tt_uint8*>(recvbuf), static_cast<tt_size>(res))) {
        return TtError::Error;
    }

    return TtError::Ok;
}

bool TelemClose(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);

#if 0 // TODO: needed?
        int res = platform::shutdown(pCtx->socket, SD_BOTH);
        if (res == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", platform::WSAGetLastError());
        }
#endif

    if (pCtx->socket != INV_SOCKET) {
        platform::closesocket(pCtx->socket);
        pCtx->socket = INV_SOCKET;
    }

    return true;
}

bool TelemTick(TraceContexHandle ctx)
{
    X_UNUSED(ctx);

    // send some data to the server!

    return true;
}

bool TelemFlush(TraceContexHandle ctx)
{
    X_UNUSED(ctx);
    return true;
}

void TelemUpdateSymbolData(TraceContexHandle ctx)
{
    X_UNUSED(ctx);
}

void TelemPause(TraceContexHandle ctx, bool pause)
{
    if (!isValidContext(ctx)) {
        return;
    }

    handleToContext(ctx)->isEnabled = pause;
}

bool TelemIsPaused(TraceContexHandle ctx)
{
    if (!isValidContext(ctx)) {
        return true;
    }

    return handleToContext(ctx)->isEnabled;
}

// Zones
void TelemEnter(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pZoneName)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    X_UNUSED(ctx);
    X_UNUSED(sourceInfo);
    X_UNUSED(pZoneName);

    ZoneEnterData packet;
    packet.type = DataStreamType::ZoneEnter;
    packet.time = getTicks();

    addDataPacket(pCtx, &packet, sizeof(packet));
}

void TelemEnterEx(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pZoneName)
{
    X_UNUSED(ctx);
    X_UNUSED(sourceInfo);
    X_UNUSED(matchIdOut);
    X_UNUSED(minMicroSec);
    X_UNUSED(pZoneName);
}

void TelemLeave(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    ZoneLeaveData packet;
    packet.type = DataStreamType::ZoneLeave;
    packet.time = getTicks();

    addDataPacket(pCtx, &packet, sizeof(packet));
}

void TelemLeaveEx(TraceContexHandle ctx, tt_uint64 matchId)
{
    X_UNUSED(ctx);
    X_UNUSED(matchId);
}
