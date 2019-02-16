#include "stdafx.h"
#include "TelemetryServerLib.h"

#include <Compression/LZ4.h>

#include <../TelemetryCommon/TelemetryCommonLib.h>
// #include <winsock2.h>

X_LINK_LIB("engine_TelemetryCommonLib.lib");
X_LINK_ENGINE_LIB("SqLite")

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


    bool readPacket(platform::SOCKET& socket, char* pBuffer, int& bufLengthInOut)
    {
        // this should return complete packets or error.
        int bytesRead = 0;
        int bufLength = sizeof(PacketBase);

        while (1) {
            int maxReadSize = bufLength - bytesRead;
            int res = platform::recv(socket, &pBuffer[bytesRead], maxReadSize, 0);

            if (res == 0) {
                X_ERROR("TelemSrv", "Connection closing...");
                return false;
            }
            else if (res < 0) {
                X_ERROR("TelemSrv", "recv failed with error: %d", platform::WSAGetLastError());
                return false;
            }

            bytesRead += res;

            X_LOG0("TelemSrv", "got: %d bytes", res);

            if (bytesRead == sizeof(PacketBase))
            {
                auto* pHdr = reinterpret_cast<const PacketBase*>(pBuffer);
                if (pHdr->dataSize == 0) {
                    X_ERROR("TelemSrv", "Client sent packet with length zero...");
                    return false;
                }

                if (pHdr->dataSize > bufLengthInOut) {
                    X_ERROR("TelemSrv", "Client sent oversied packet of size %i...", static_cast<tt_int32>(pHdr->dataSize));
                    return false;
                }

                bufLength = pHdr->dataSize;
            }

            if (bytesRead == bufLength) {
                bufLengthInOut = bytesRead;
                return true;
            }
            else if (bytesRead > bufLength) {
                X_ERROR("TelemSrv", "Overread packet bytesRead: %d recvbuflen: %d", bytesRead, bufLength);
                return false;
            }
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

        int32_t cmpBufferOffset;
        int8_t srcRingBuf[COMPRESSION_RING_BUFFER_SIZE];

        core::Compression::LZ4StreamDecode lz4Stream;

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

    bool handleConnectionRequest(Client& client, uint8_t* pData)
    {
        auto* pConReq = reinterpret_cast<const ConnectionRequestHdr*>(pData);
        if (pConReq->type != PacketType::ConnectionRequest) {
            X_ASSERT_UNREACHABLE();
        }

        VersionInfo serverVer;
        serverVer.major = TELEM_VERSION_MAJOR;
        serverVer.minor = TELEM_VERSION_MINOR;
        serverVer.patch = TELEM_VERSION_PATCH;
        serverVer.build = TELEM_VERSION_BUILD;

        if (pConReq->clientVer != serverVer) {
            sendConnectionRejected(client, "Client server version incompatible");
            return false;
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
        return true;
    }

    bool handleDataSream(Client& client, uint8_t* pData)
    {
        auto* pHdr = reinterpret_cast<const DataStreamHdr*>(pData);
        if (pHdr->type != PacketType::DataStream) {
            X_ASSERT_UNREACHABLE();
        }

        // the data is compressed.
        // decompress it..
        int32_t cmpLen = pHdr->dataSize - sizeof(DataStreamHdr);
        int32_t origLen = pHdr->origSize - sizeof(DataStreamHdr);
        X_UNUSED(cmpLen);

        auto* pDst = &client.srcRingBuf[client.cmpBufferOffset];

        auto res = client.lz4Stream.decompressContinue(pHdr + 1, pDst, origLen);
        if (res != cmpLen) {
            // TODO: ..
            return false;
        }

        client.cmpBufferOffset += origLen;

        if (client.cmpBufferOffset >= (COMPRESSION_RING_BUFFER_SIZE - COMPRESSION_MAX_INPUT_SIZE)) {
            client.cmpBufferOffset = 0;
        }

        if (client.pFile) {
        //    fwrite(pDst, origLen, 1, client.pFile);
        }

        // process this data?
        for (int32 i = 0; i < origLen; )
        {
            // packet me baby!
            auto* pPacket = reinterpret_cast<const DataPacketBase*>(&pDst[i]);

            switch (pPacket->type)
            {
                case DataStreamType::StringTableAdd:
                {
                    auto* pStr = reinterpret_cast<const DataPacketStringTableAdd*>(&pDst[i]);
                    i += sizeof(DataPacketStringTableAdd);
                    i += pStr->length;
                }
                    break;
                case DataStreamType::Zone:
                {
                    auto* pZone = reinterpret_cast<const DataPacketZone*>(&pDst[i]);
                    i += sizeof(DataPacketZone);

                    fprintf(client.pFile, "TID: %" PRIu32 " s: %" PRIu64 " e: %" PRIu64 "\n", pZone->threadID, pZone->start, pZone->end);
                }
                    break;
                case DataStreamType::TickInfo:
                    i += sizeof(DataPacketTickInfo);
                    break;
                case DataStreamType::ThreadSetName:
                    i += sizeof(DataPacketThreadSetName);
                    break;
                case DataStreamType::CallStack:
                    i += sizeof(DataPacketCallStack);
                    break;
                case DataStreamType::LockSetName:
                    i += sizeof(DataPacketLockSetName);
                    break;
                case DataStreamType::LockTry:
                    i += sizeof(DataPacketLockTry);
                    break;
                case DataStreamType::LockState:
                    i += sizeof(DataPacketLockState);
                    break;
                case DataStreamType::LockCount:
                    i += sizeof(DataPacketLockCount);
                    break;
                case DataStreamType::MemAlloc:
                    i += sizeof(DataPacketMemAlloc);
                    break;
                case DataStreamType::MemFree:
                    i += sizeof(DataPacketMemFree);
                    break;
                    break;

                default:
                    X_NO_SWITCH_DEFAULT_ASSERT;
            }
        }

        return true;
    }

    bool processPacket(Client& client, uint8_t* pData)
    {
        auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);

        switch (pPacketHdr->type)
        {
            case PacketType::ConnectionRequest:
                return handleConnectionRequest(client, pData);
                break;
            case PacketType::DataStream:
                return handleDataSream(client, pData);
                break;
            default:
                X_ERROR("TelemSrv", "Unknown packet type %" PRIi32, static_cast<int>(pPacketHdr->type));
                return false;
        }
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
            //    fwrite(recvbuf, recvbuflen, 1, client.pFile);
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