#include "stdafx.h"


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
            zero_this(this);
            socket = INV_SOCKET;
        }

        VersionInfo clientVer;

        // TODO: not need nullterm.
        char appName[MAX_APP_NAME_LEN + 1];
        char buildInfo[MAX_BUILD_INFO_LEN + 1];
        char cmdLine[MAX_CMDLINE_LEN + 1];

        platform::SOCKET socket;

        FILE* pFile;
    };

    void sendPacketToClient(Client& client, const void* pData, tt_size len)
    {
        // send some data...
        int res = platform::send(client.socket, reinterpret_cast<const char*>(pData), static_cast<int>(len), 0);
        if (res == SOCKET_ERROR) {
            printf("send failed with error: %d\n", platform::WSAGetLastError());
            return;
        }
    }

    void sendConnectionRejected(Client& client, const char* pReason)
    {
        printf("ConnectionRejected:\n");

        tt_size msgLen = strlen(pReason);
        tt_size datalen = sizeof(ConnectionRequestRejectedHdr) + msgLen;

        if (msgLen > MAX_ERR_MSG_LEN) {
            msgLen = MAX_ERR_MSG_LEN;
        }

        char buf[sizeof(ConnectionRequestRejectedHdr) + MAX_ERR_MSG_LEN];

        auto* pCr = reinterpret_cast<ConnectionRequestRejectedHdr*>(buf);
        pCr->dataSize = static_cast<tt_uint16>(datalen);
        pCr->type = PacketType::ConnectionRequestRejected;

        memcpy(pCr + 1, pReason, msgLen);
        sendPacketToClient(client, buf, datalen);
    }

    void handleConnectionRequest(Client& client, tt_uint8* pData)
    {
        auto* pConReq = reinterpret_cast<const ConnectionRequestHdr*>(pData);
        if (pConReq->type != PacketType::ConnectionRequest) {
            sendConnectionRejected(client, "Packet type is invalid");
            return;
        }

        VersionInfo serverVer;
        serverVer.major = X_TELEMETRY_VERSION_MAJOR;
        serverVer.minor = X_TELEMETRY_VERSION_MINOR;
        serverVer.patch = X_TELEMETRY_VERSION_PATCH;
        serverVer.build = X_TELEMETRY_VERSION_BUILD;

        if (pConReq->clientVer != serverVer) {
            sendConnectionRejected(client, "Client server version incompatible");
            return;
        }

        client.clientVer = pConReq->clientVer;

        // now need to read the strings.

        zero_object(client.appName);
        zero_object(client.buildInfo);
        zero_object(client.cmdLine);

        auto* pStrData = reinterpret_cast<const tt_uint8*>(pConReq + 1);
        memcpy(client.appName, pStrData, pConReq->appNameLen);
        pStrData += pConReq->appNameLen;
        memcpy(client.buildInfo, pStrData, pConReq->buildInfoLen);
        pStrData += pConReq->buildInfoLen;
        memcpy(client.cmdLine, pStrData, pConReq->cmdLineLen);

        // Meow...
        printf("ConnectionAccepted:\n");
        printf("> AppName: %s\n", client.appName);
        printf("> BuildInfo: %s\n", client.buildInfo);
        printf("> CmdLine: %s\n", client.cmdLine);

        // send a packet back!
        ConnectionRequestAcceptedHdr cra;
        zero_object(cra);
        cra.dataSize = sizeof(cra);
        cra.type = PacketType::ConnectionRequestAccepted;
        cra.serverVer = serverVer;

        sendPacketToClient(client, &cra, sizeof(cra));
    }

    void handleDataSream(Client& client, tt_uint8* pData)
    {
        X_UNUSED(client);
        X_UNUSED(pData);
        

    }

    bool processPacket(Client& client, tt_uint8* pData)
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
                return false;
        }

        return true;
    }

    bool readPacket(Client& client, char* pBuffer, int& bufLength)
    {
        // this should return complete packets or error.
        int bytesRead = 0;

        while (1)
        {
            int maxReadSize = bufLength - bytesRead;
            int res = platform::recv(client.socket, &pBuffer[bytesRead], maxReadSize, 0);

            if (res == 0)
            {
                printf("Connection closing...\n");
                return false;
            }
            else if(res < 0)
            {
                printf("recv failed with error: %d\n", platform::WSAGetLastError());
                return false;
            }

            bytesRead += res;

            if (bytesRead >= sizeof(PacketBase))
            {
                auto* pHdr = reinterpret_cast<const PacketBase*>(pBuffer);
                if (pHdr->dataSize == 0) {
                    printf("Client sent packet with length zero...\n");
                    return false;
                }

                bufLength = pHdr->dataSize;

                if (bytesRead == bufLength) {
                    return true;
                }
                else if(bytesRead >= bufLength) {
                    printf("Overread packet bytesRead: %d recvbuflen: %d\n", bytesRead, bufLength);
                    return false;
                }
            }

            printf("got: %d bytes\n", res);
        }
    }

    void handleClient(Client& client)
    {
        char recvbuf[MAX_PACKET_SIZE];

        while(1)
        {
            int recvbuflen = sizeof(recvbuf);
            if (!readPacket(client, recvbuf, recvbuflen)) {
                return;
            }

            printf("Bytes received: %d\n", recvbuflen);

            if (client.pFile) {
                fwrite(recvbuf, recvbuflen, 1, client.pFile);
            }

            if (!processPacket(client, reinterpret_cast<tt_uint8*>(recvbuf))) {
                return;
            }
        }
    }

    bool listen(void)
    {
        struct platform::addrinfo hints;
        struct platform::addrinfo *result = nullptr;

        zero_object(hints);
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
            printf("Failed to set rcvbuf on socket. Error: %d\n", platform::WSAGetLastError());
        }

        // Setup the TCP listening socket
        res = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (res == SOCKET_ERROR) {
            printf("bind failed with error: %d\n", platform::WSAGetLastError());
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
            printf("Waiting for client on port: %s\n", DEFAULT_PORT);

            // Accept a client socket
            clientSocket = platform::accept(listenSocket, NULL, NULL);
            if (clientSocket == INV_SOCKET) {
                platform::closesocket(listenSocket);
                return false;
            }

            printf("Client connected\n");

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

} // namespace

#if 1 // SubSystem /console
int main() 
{
#else
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    X_UNUSED(hInstance);
    X_UNUSED(hPrevInstance);
    X_UNUSED(lpCmdLine);
    X_UNUSED(nCmdShow);
#endif

    if (!winSockInit()) {
        return 1;
    }

    // have the server listen...
    if (!listen()) {
        getchar();
    }

    winSockShutDown();
    return 0;
}


