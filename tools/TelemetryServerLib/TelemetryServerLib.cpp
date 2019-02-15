#include "stdafx.h"
#include "TelemetryServerLib.h"

X_LINK_LIB("engine_TelemetryCommonLib.lib");


#include <../TelemetryCommon/TelemetryCommonLib.h>
// #include <winsock2.h>

namespace
{
    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

    const char* DEFAULT_PORT = "8001";

    bool winSockInit(void)
    {
        platform::WSADATA winsockInfo;

        if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
            return false;
        }

        return true;
    }

    void winSockShutDown(void)
    {
        if (platform::WSACleanup() != 0) {
            // rip
            return;
        }
    }

    struct Client
    {
        Client() {
            core::zero_this(this);
            socket = INV_SOCKET;
        }

        VersionInfo clientVer;

        // TODO: not need nullterm.
        char appName[MAX_STRING_LEN + 1];
        char buildInfo[MAX_STRING_LEN + 1];
        char cmdLine[MAX_CMDLINE_LEN + 1];

        platform::SOCKET socket;

        FILE* pFile;
    };

    void sendPacketToClient(Client& client, const void* pData, size_t len)
    {
        // send some data...
        int res = platform::send(client.socket, reinterpret_cast<const char*>(pData), static_cast<int>(len), 0);
        if (res == SOCKET_ERROR) {
            X_LOG0("TelemSrv", "send failed with error: %d", platform::WSAGetLastError());
            return;
        }
    }

    void sendConnectionRejected(Client& client, const char* pReason)
    {
        X_LOG0("TelemSrv", "ConnectionRejected:");

        size_t msgLen = strlen(pReason);
        size_t datalen = sizeof(ConnectionRequestRejectedHdr) + msgLen;

        if (msgLen > MAX_STRING_LEN) {
            msgLen = MAX_STRING_LEN;
        }

        char buf[sizeof(ConnectionRequestRejectedHdr) + MAX_STRING_LEN];

        auto* pCr = reinterpret_cast<ConnectionRequestRejectedHdr*>(buf);
        pCr->dataSize = static_cast<tt_uint16>(datalen);
        pCr->type = PacketType::ConnectionRequestRejected;

        memcpy(pCr + 1, pReason, msgLen);
        sendPacketToClient(client, buf, datalen);
    }

    void handleConnectionRequest(Client& client, uint8_t* pData)
    {
        auto* pConReq = reinterpret_cast<const ConnectionRequestHdr*>(pData);
        if (pConReq->type != PacketType::ConnectionRequest) {
            sendConnectionRejected(client, "Packet type is invalid");
            return;
        }

        VersionInfo serverVer;
        serverVer.major = TELEM_VERSION_MAJOR;
        serverVer.minor = TELEM_VERSION_MINOR;
        serverVer.patch = TELEM_VERSION_PATCH;
        serverVer.build = TELEM_VERSION_BUILD;

        if (pConReq->clientVer != serverVer) {
            sendConnectionRejected(client, "Client server version incompatible");
            return;
        }

        client.clientVer = pConReq->clientVer;

        // now need to read the strings.

        core::zero_object(client.appName);
        core::zero_object(client.buildInfo);
        core::zero_object(client.cmdLine);

        auto* pStrData = reinterpret_cast<const uint8_t*>(pConReq + 1);
        memcpy(client.appName, pStrData, pConReq->appNameLen);
        pStrData += pConReq->appNameLen;
        memcpy(client.buildInfo, pStrData, pConReq->buildInfoLen);
        pStrData += pConReq->buildInfoLen;
        memcpy(client.cmdLine, pStrData, pConReq->cmdLineLen);

        // Meow...
        X_LOG0("TelemSrv", "ConnectionAccepted:");
        X_LOG0("TelemSrv", "> AppName: %s", client.appName);
        X_LOG0("TelemSrv", "> BuildInfo: %s", client.buildInfo);
        X_LOG0("TelemSrv", "> CmdLine: %s", client.cmdLine);

        // send a packet back!
        ConnectionRequestAcceptedHdr cra;
        core::zero_object(cra);
        cra.dataSize = sizeof(cra);
        cra.type = PacketType::ConnectionRequestAccepted;
        cra.serverVer = serverVer;

        sendPacketToClient(client, &cra, sizeof(cra));
    }

    void handleDataSream(Client& client, uint8_t* pData)
    {
        X_UNUSED(client);
        X_UNUSED(pData);
        

    }

    bool processPacket(Client& client, uint8_t* pData)
    {
        auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);

        switch (pPacketHdr->type)
        {
            case PacketType::ConnectionRequest:
                handleConnectionRequest(client, pData);
                break;
            case PacketType::DataStream:
                handleDataSream(client, pData);
                break;
            default:
                X_ERROR("TelemSrv", "Unknown packet type %" PRIi32, static_cast<int>(pPacketHdr->type));
                return false;
        }

        return true;
    }

    void handleClient(Client& client)
    {
        char recvbuf[MAX_PACKET_SIZE];

        while(1)
        {
            int recvbuflen = sizeof(recvbuf);
            if (!readPacket(client.socket, recvbuf, recvbuflen)) {
                X_LOG0("TelemSrv", "Error reading packet");
                return;
            }

            X_LOG0("TelemSrv", "Bytes received: %" PRIi32, recvbuflen);

            if (client.pFile) {
                fwrite(recvbuf, recvbuflen, 1, client.pFile);
            }

            if (!processPacket(client, reinterpret_cast<uint8_t*>(recvbuf))) {
                X_LOG0("TelemSrv", "Failed to process packet");
                return;
            }
        }
    }


} // namespace



X_NAMESPACE_BEGIN(telemetry)

Server::Server(core::MemoryArenaBase* arena) :
    arena_(arena)
{

}

Server::~Server()
{

}

bool Server::run()
{
    if (!winSockInit()) {
        return false;
    }

    // have the server listen...
    if (!listen()) {
        return false;
    }

    winSockShutDown();
    return true;
}


bool Server::listen(void)
{
    // completetion port.
    // auto hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    struct platform::addrinfo hints;
    struct platform::addrinfo *result = nullptr;

    core::zero_object(hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = platform::IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    int res = platform::getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (res != 0) {
        return false;
    }

    platform::SOCKET listenSocket = INV_SOCKET;
    platform::SOCKET clientSocket = INV_SOCKET;

    // Create a SOCKET for connecting to server
    listenSocket = platform::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INV_SOCKET) {
        platform::freeaddrinfo(result);
        return false;
    }

    tt_int32 sock_opt = 1024 * 256;
    res = platform::setsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, (char*)&sock_opt, sizeof(sock_opt));
    if (res != 0) {
        X_ERROR("TelemSrv", "Failed to set rcvbuf on socket. Error: %d", platform::WSAGetLastError());
    }

    // Setup the TCP listening socket
    res = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (res == SOCKET_ERROR) {
        X_ERROR("TelemSrv", "bind failed with error: %d", platform::WSAGetLastError());
        platform::freeaddrinfo(result);
        platform::closesocket(listenSocket);
        return false;
    }

    platform::freeaddrinfo(result);

    res = platform::listen(listenSocket, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        platform::closesocket(listenSocket);
        return false;
    }

    while (true)
    {
        X_LOG0("TelemSrv", "Waiting for client on port: %s", DEFAULT_PORT);

        // Accept a client socket
        clientSocket = platform::accept(listenSocket, NULL, NULL);
        if (clientSocket == INV_SOCKET) {
            platform::closesocket(listenSocket);
            return false;
        }

        X_LOG0("TelemSrv", "Client connected");

        Client client;
        client.socket = clientSocket;
        client.pFile = fopen("stream.dump", "wb");

        handleClient(client);

        if (client.pFile) {
            fclose(client.pFile);
        }

        platform::closesocket(clientSocket);
    }

    // clean up socket.
    platform::closesocket(listenSocket);

    return true;

}

X_NAMESPACE_END