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
        size_t bufLen;

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

} // namespace


    bool ttInit(void)
    {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);

        platform::WSADATA winsockInfo;

        if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
            return false;
        }

        return true;
    }

    void ttShutDown(void)
    {
        if (platform::WSACleanup() != 0) {
            // rip
            return;
        }

    }

    bool ttInitializeContext(TraceContexHandle& out, void* pBuf, size_t bufLen)
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

    void ttShutdownContext(TraceContexHandle ctx)
    {
        X_UNUSED(ctx);

    }

    TtError ttOpen(TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pServerAddress,
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

        return TtError::Ok;
    }

    bool ttClose(TraceContexHandle ctx)
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

    bool ttTick(TraceContexHandle ctx)
    {
        X_UNUSED(ctx);
        return true;
    }

    bool ttFlush(TraceContexHandle ctx)
    {
        X_UNUSED(ctx);
        return true;
    }

    void ttUpdateSymbolData(TraceContexHandle ctx)
    {
        X_UNUSED(ctx);
    }

    void ttPause(TraceContexHandle ctx, bool pause)
    {
        if (!isValidContext(ctx)) {
            return;
        }

        handleToContext(ctx)->isEnabled = pause;
    }

    bool ttIsPaused(TraceContexHandle ctx)
    {
        if (!isValidContext(ctx)) {
            return true;
        }

        return handleToContext(ctx)->isEnabled;
    }


