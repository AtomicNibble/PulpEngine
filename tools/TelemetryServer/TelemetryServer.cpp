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

        char appName[MAX_APP_NAME_LEN];
        char buildInfo[MAX_BUILD_INFO_LEN];

        platform::SOCKET socket;
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

    void onConnectionRejected(Client& client, const char* pReason)
    {
        ConnectionRequestRejectedData cra;
        zero_object(cra);
        cra.type = PacketType::ConnectionRequestRejected;
        strcpy_s(cra.reason, pReason);

        sendPacketToClient(client, &cra, sizeof(cra));
    }

    void handleConnectionRequest(Client& client, tt_uint8* pData, tt_size len)
    {
        if (len != sizeof(ConnectionRequestData)) {
            onConnectionRejected(client, "Invalid connection packet size");
            return;
        }

        auto* pConReq = reinterpret_cast<const ConnectionRequestData*>(pData);
        if (pConReq->type != PacketType::ConnectionRequest) {
            onConnectionRejected(client, "Packet type is invalid");
            return;
        }

        VersionInfo serverVer;
        serverVer.major = X_TELEMETRY_VERSION_MAJOR;
        serverVer.minor = X_TELEMETRY_VERSION_MINOR;
        serverVer.patch = X_TELEMETRY_VERSION_PATCH;
        serverVer.build = X_TELEMETRY_VERSION_BUILD;

        if (pConReq->clientVer != serverVer) {
            onConnectionRejected(client, "Client server version incompatible");
            return;
        }

        client.clientVer = pConReq->clientVer;
        strcpy_s(client.appName, pConReq->appName);
        strcpy_s(client.buildInfo, pConReq->buildInfo);

        // Meow...
        printf("ConnectionRequest:\n");
        printf("AppName: %s\n", pConReq->appName);
        printf("BuildInfo: %s\n", pConReq->buildInfo);

        // send a packet back!
        ConnectionRequestAcceptedData cra;
        zero_object(cra);
        cra.type = PacketType::ConnectionRequestAccepted;
        cra.serverVer = serverVer;

        sendPacketToClient(client, &cra, sizeof(cra));
    }

    bool processPacket(Client& client, tt_uint8* pData, tt_size len)
    {
        X_UNUSED(pData);
        X_UNUSED(len);

        if (len < 1) {
            return false;
        }

        tt_uint8 val = pData[0];
        if (val >= PacketType::Num) {
            return false;
        }

        switch (val)
        {
            case PacketType::ConnectionRequest:
                handleConnectionRequest(client, pData, len);
                break;

            default:
                return false;
        }

        return true;
    }

    void handleClient(Client& client)
    {
        int res;
        
        char recvbuf[MAX_PACKET_SIZE];
        int recvbuflen = sizeof(recvbuf);

        do
        {
            res = platform::recv(client.socket, recvbuf, recvbuflen, 0);
            if (res > 0)
            {
                printf("Bytes received: %d\n", res);

                processPacket(client, reinterpret_cast<tt_uint8*>(recvbuf), static_cast<tt_size>(res));
            }
            else if (res == 0)
            {
                printf("Connection closing...\n");
            }
            else
            {
                printf("recv failed with error: %d\n", platform::WSAGetLastError());
                return;
            }

        } while (res > 0);
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

            handleClient(client);

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
