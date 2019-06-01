#include "stdafx.h"
#include "TraceImport.h"

#include <IFileSys.h>

#include <String/HumanDuration.h>
#include <String/HumanSize.h>

#include <Time/StopWatch.h>

X_NAMESPACE_BEGIN(telemetry)


TraceImport::TraceImport()
{

}


bool TraceImport::ingestTraceFile(core::Path<>& path)
{
    // we might have a bigggggg boy.
    // couple 100 GB etc.
    // so need to ingrest him.
    // but also sometimes i will want to load the whole file for benchmarking.

    core::FileFlags mode = core::FileFlag::READ | core::FileFlag::SHARE;

    core::XFileScoped file;
    if (!file.openFile(path, mode)) {
        return false;
    }

    TelemFileHdr hdr;
    if (file.readObj(hdr) != sizeof(hdr)) {
        X_ERROR("TelemSrv", "Failed to read trace file header");
        return false;
    }

    if (!hdr.isValid()) {
        X_ERROR("TelemSrv", "Trace file header is invalid");
        return false;
    }

    if (hdr.version != TRACE_FILE_VERSION) {
        X_ERROR("TelemSrv", "Trace file version mismatch");
        return false;
    }

    // read the connection request?
    constexpr int32_t MAX_STR_LEN = MAX_CMDLINE_LEN + MAX_STRING_LEN + MAX_STRING_LEN;

    struct {
        ConnectionRequestHdr cr;
        char stringData[MAX_STR_LEN];
    } data;

    core::zero_object(data);

    if (file.readObj(data.cr) != sizeof(data.cr)) {
        X_ERROR("TelemSrv", "Failed to read trace file meta");
        return false;
    }

    int32_t strDataLen = data.cr.appNameLen + data.cr.buildInfoLen + data.cr.cmdLineLen;

    if (file.read(data.stringData, strDataLen) != strDataLen) {
        X_ERROR("TelemSrv", "Failed to read trace file meta strings");
        return false;
    }

    struct platform::addrinfo hints, *servinfo = nullptr;
    core::zero_object(hints);
    hints.ai_family = AF_INET; // ipv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = platform::IPPROTO_TCP;

    // Resolve the server address and port
    int res = platform::getaddrinfo("localhost", telem::DEFAULT_PORT_STR, &hints, &servinfo);
    if (res != 0) {
        lastErrorWSA::Description Dsc;
        const auto err = lastErrorWSA::Get();
        X_ERROR("TelemServ", "Failed to getaddrinfo. Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
        return false;
    }

    platform::SOCKET connectSocket = platform::INV_SOCKET;

    for (auto pPtr = servinfo; pPtr != nullptr; pPtr = pPtr->ai_next) {
        // Create a SOCKET for connecting to server
        connectSocket = platform::socket(pPtr->ai_family, pPtr->ai_socktype, pPtr->ai_protocol);
        if (connectSocket == platform::INV_SOCKET) {
            return false;
        }

        // Connect to server.
        res = connect(connectSocket, pPtr->ai_addr, static_cast<int32_t>(pPtr->ai_addrlen));
        if (res == SOCKET_ERROR) {
            platform::closesocket(connectSocket);
            connectSocket = platform::INV_SOCKET;
            continue;
        }

        break;
    }

    platform::freeaddrinfo(servinfo);
    if (connectSocket == platform::INV_SOCKET) {
        X_ERROR("TelemSrv", "Failed to connect to server for ingest");
        return false;
    }

    // Give the socket a decent sized buffer.
    tt_int32 sock_opt = 1024 * 512;
    res = platform::setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sock_opt, sizeof(sock_opt));
    if (res != 0) {
        lastErrorWSA::Description Dsc;
        const auto err = lastErrorWSA::Get();
        X_ERROR("TelemSrv", "Failed to set sndbuf on socket. Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
        return false;
    }

    auto sendToServer = [&](const char* pBuffer, int32_t length) -> bool {
        int32_t res = platform::send(connectSocket, pBuffer, length, 0);
        if (res != SOCKET_ERROR) {
            return true;
        }

        lastErrorWSA::Description Dsc;
        const auto err = lastErrorWSA::Get();

        X_ERROR("TelemSrv", "Socket: send failed with Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
        return false;
    };

    if (!sendToServer(reinterpret_cast<const char*>(&data.cr), sizeof(data.cr))) {
        platform::closesocket(connectSocket);
        return false;
    }

    if (!sendToServer(reinterpret_cast<const char*>(&data.stringData), strDataLen)) {
        platform::closesocket(connectSocket);
        return false;
    }

    core::StopWatch timer;

    const uint64_t totalBytes = file.remainingBytes();
    uint64_t bytesLeft = totalBytes;
    uint8_t buffer[1024 * 64];

    while (bytesLeft)
    {
        auto toRead = safe_static_cast<int32_t>(core::Min(sizeof(buffer), bytesLeft));

        if (file.read(buffer, toRead) != toRead) {
            X_ERROR("TelemSrv", "Error reading trace file data");
            platform::closesocket(connectSocket);
            return false;
        }

        // we have some data!
        if (!sendToServer(reinterpret_cast<const char*>(buffer), toRead)) {
            platform::closesocket(connectSocket);
            return false;
        }

        bytesLeft -= toRead;
    }

    // flush
    res = platform::shutdown(connectSocket, SD_BOTH);
    if (res == SOCKET_ERROR) {
        lastErrorWSA::Description Dsc;
        const auto err = lastErrorWSA::Get();
        X_ERROR("TelemSrv", "socket shutdown failed with Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
    }

    auto ellapsedMs = timer.GetMilliSeconds();
    auto ellapsedSec = ellapsedMs / 1000;

    float MBs = static_cast<float>(totalBytes / 1024 / 1024);
    float MBPerSec = MBs / ellapsedSec;

    core::HumanDuration::Str durStr;
    core::HumanSize::Str sizeStr;

    X_LOG0("TelemSrv", "Ingested ^6%s^7 in ^6%s^7 speed ^6%.2fMB/s", core::HumanSize::toString(sizeStr, totalBytes), core::HumanDuration::toString(durStr, ellapsedMs), MBPerSec);

    platform::closesocket(connectSocket);

    if (file.remainingBytes() != 0) {
        X_ERROR("TelemSrv", "Error reading trace file, did not consume all bytes");
    }

    return true;
}

X_NAMESPACE_END
