#include "stdafx.h"

namespace
{
    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

    const char* DEFAULT_PORT = "8001";
    const size_t RECV_BUF_LEN = 1024;

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

    bool listen(void)
    {
        struct platform::addrinfo hints;
        struct platform::addrinfo *result = nullptr;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = platform::IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the server address and port
        auto res = platform::getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
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

            // Receive until the peer shuts down the connection
            int sendResult;
            char recvbuf[RECV_BUF_LEN];
            int recvbuflen = RECV_BUF_LEN;

            do
            {
                res = platform::recv(clientSocket, recvbuf, recvbuflen, 0);
                if (res > 0)
                {
                    printf("Bytes received: %d\n", res);

#if 0
                    // Echo the buffer back to the sender
                    auto sendResult = platform::send(clientSocket, recvbuf, res, 0);
                    if (sendResult == SOCKET_ERROR) {
                        printf("send failed with error: %d\n", platform::WSAGetLastError());
                        platform::closesocket(clientSocket);
                        return false;
                    }

                    printf("Bytes sent: %d\n", sendResult);
#else
                    X_UNUSED(sendResult);
#endif
                }
                else if (res == 0)
                {
                    printf("Connection closing...\n");
                }
                else
                {
                    printf("recv failed with error: %d\n", platform::WSAGetLastError());
                    platform::closesocket(clientSocket);
                    return false;
                }

            } while (res > 0);

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
