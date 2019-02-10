#include "stdafx.h"
#include "TelemetryLib.h"

#include <cstdio>

#include <../TelemetryCommon/TelemetryCommonLib.h>

X_LINK_LIB("engine_TelemetryCommonLib.lib");

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
    tt_uint64 gTicksPerMicro;

    X_INLINE tt_uint64 TicksToMicro(tt_uint64 tsc) 
    {
        return (tsc / gTicksPerMicro);
    }

    X_INLINE tt_uint32 getThreadID(void)
    {
        return ::GetCurrentThreadId();
    }

    X_INLINE tt_uint64 getTicks(void)
    {
        return __rdtsc();
    }

    X_INLINE void* AlignTop(void* ptr, tt_size alignment)
    {
        union
        {
            void* as_void;
            tt_uintptr as_uintptr_t;
        };

        const tt_size mask = alignment - 1;
        as_void = ptr;
        as_uintptr_t += mask;
        as_uintptr_t &= ~mask;
        return as_void;
    }

    template<typename T>
    X_INLINE bool IsAligned(T value, unsigned int alignment, unsigned int offset)
    {
        return ((value + offset) % alignment) == 0;
    }


    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);


    struct TraceZone
    {
        tt_uint64 start;
        tt_uint64 end;

        const char* pZoneName;
        const TtSourceInfo* pSourceInfo;
    };

    X_DISABLE_WARNING(4324) //  structure was padded due to alignment specifier

    // some data for each thread!
    X_ALIGNED_SYMBOL(struct TraceThread, 64)
    {
        TraceThread() {
            id = getThreadID();
            stackDepth = 0;

            // Debug only?
            zero_object(zones);
        }

        TtthreadId id;
        tt_int32 stackDepth;

        TraceZone zones[MAX_ZONE_DEPTH];
    };

    thread_local TraceThread* gThreadData = nullptr;

    // This is padded to 64bit to make placing TraceThread data on it's own boundy more simple.
    X_ALIGNED_SYMBOL(struct TraceContext, 64)
    {
        bool isEnabled;
        bool _pad[3];

        tt_uint64 ticksPerMicro;

        TraceThread* pThreadData;
        tt_int32 numThreadData;

        StringTable strTable;

        platform::SOCKET socket;
    };

    X_ENABLE_WARNING(4324)


    TraceThread* getThreadData(TraceContext* pCtx)
    {
        auto* pThreadData = gThreadData;
        if (!pThreadData) {

            if (pCtx->numThreadData == MAX_ZONE_THREADS) {
                return nullptr;
            }

            pThreadData = new (&pCtx->pThreadData[pCtx->numThreadData]) TraceThread();
            ++pCtx->numThreadData;

            gThreadData = pThreadData;
        }

        return pThreadData;
    }


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
                return false;
        }
    }

    void writeStringPacket(TraceContext* pCtx, const char* pStr)
    {
        tt_size strLen = strlen(pStr);
        if (strLen > MAX_STRING_LEN) {
            strLen = MAX_STRING_LEN;
        }

        tt_size packetLen = sizeof(StringTableAddData) + strLen;

        tt_uint8 strDataBuf[sizeof(StringTableAddData) + MAX_STRING_LEN];
        auto* pHeader = reinterpret_cast<StringTableAddData*>(strDataBuf);
        pHeader->type = DataStreamType::StringTableAdd;
        pHeader->length = static_cast<tt_uint16>(strLen);

        memcpy(strDataBuf + sizeof(StringTableAddData), pStr, strLen);

        addDataPacket(pCtx, &strDataBuf, packetLen);
    }


    void queueZone(TraceContext* pCtx, TraceThread* pThread, TraceZone& zone)
    {
        // TODO: sort this out.

        ZoneData packet;
        packet.type = DataStreamType::Zone;
        packet.stackDepth = static_cast<tt_uint8>(pThread->stackDepth);
        packet.threadID = pThread->id;
        packet.start = zone.start;
        packet.end = zone.end;
        packet.strIdxFile = StringTableGetIndex(pCtx->strTable, zone.pSourceInfo->pFile_);
        packet.strIdxFunction = StringTableGetIndex(pCtx->strTable, zone.pSourceInfo->pFunction_);
        packet.strIdxZone = StringTableGetIndex(pCtx->strTable, zone.pZoneName);

        // i want to write the string data.
        if (packet.strIdxFile.inserted) {
            writeStringPacket(pCtx, zone.pSourceInfo->pFile_);
        }
        if (packet.strIdxFunction.inserted) {
            writeStringPacket(pCtx, zone.pSourceInfo->pFunction_);
        }
        if (packet.strIdxZone.inserted) {
            writeStringPacket(pCtx, zone.pZoneName);
        }

        sizeof(ConnectionRequestRejectedData);
        addDataPacket(pCtx, &packet, sizeof(packet));
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

    SysTimer gSysTimer;

} // namespace


bool TelemInit(void)
{
    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    platform::WSADATA winsockInfo;

    if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
        return false;
    }

    if (!gSysTimer.StartUp()) {
        return false;
    }

    // want to work out ticks per micro.
    const auto micro_start = gSysTimer.GetMicro();
    const auto tsc_start = getTicks();

    tt_uint64 micro_end;
    tt_uint64 tsc_end;

    for (;;) {
        tsc_end = getTicks();
        micro_end = gSysTimer.GetMicro();
        if ((micro_end - micro_start) > 100000) {
            break;
        }
    }

    gTicksPerMicro = (tsc_end - tsc_start) / (micro_end - micro_start);
    return true;
}

void TelemShutDown(void)
{
    if (platform::WSACleanup() != 0) {
        // rip
        return;
    }
}

bool TelemInitializeContext(TraceContexHandle& out, void* pBuf_, tt_size bufLen)
{
    out = INVALID_TRACE_CONTEX;

    // need to align upto 64bytes.
    auto pBuf = AlignTop(pBuf_, 64);
    const tt_uintptr alignmentSize = reinterpret_cast<tt_uintptr>(pBuf) - reinterpret_cast<tt_uintptr>(pBuf_);

    constexpr tt_size contexSize = sizeof(TraceContext);
    constexpr tt_size threadDataSize = sizeof(TraceThread) * MAX_ZONE_THREADS;
    constexpr tt_size strTableSize = sizeof(void*) * 1024;

    constexpr tt_size minSize = contexSize + strTableSize + threadDataSize;
    if (bufLen < minSize + alignmentSize) {
        return false;
    }

    tt_uint8* pBufU8 = reinterpret_cast<tt_uint8*>(pBuf);

    tt_uint8* pThreadDataBuf = pBufU8 + contexSize;
    tt_uint8* pStrTableBuf = pThreadDataBuf + threadDataSize;


    TraceContext* pCtx = reinterpret_cast<TraceContext*>(pBuf);
    pCtx->isEnabled = true;
    pCtx->socket = INV_SOCKET;
    pCtx->pThreadData = reinterpret_cast<TraceThread*>(pThreadDataBuf);
    pCtx->numThreadData = 0;
    pCtx->strTable = CreateStringTable(pStrTableBuf, strTableSize);

  //  pCtx->pScratchBuf = pBufU8 + contexSize;
  //  pCtx->bufLen = bufLen - contexSize;

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

    X_UNUSED(timeoutMS);

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

    auto* pThreadData = getThreadData(pCtx);
    if (!pThreadData) {
        return;
    }

    auto depth = pThreadData->stackDepth;
    ++pThreadData->stackDepth;

    auto& zone = pThreadData->zones[depth];
    zone.start = getTicks();
    zone.pZoneName = pZoneName;
    zone.pSourceInfo = &sourceInfo;
}

void TelemLeave(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    auto* pThreadData = getThreadData(pCtx);
    if (!pThreadData) {
        return;
    }

    auto depth = --pThreadData->stackDepth;

    auto& zone = pThreadData->zones[depth];
    zone.end = getTicks();

    queueZone(pCtx, pThreadData, zone);
}

void TelemEnterEx(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pZoneName)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    auto* pThreadData = getThreadData(pCtx);
    if (!pThreadData) {
        return;
    }

    auto depth = pThreadData->stackDepth;
    ++pThreadData->stackDepth;

    auto& zone = pThreadData->zones[depth];
    zone.start = getTicks();
    zone.pZoneName = pZoneName;
    zone.pSourceInfo = &sourceInfo;

    // we can just copy it?
    matchIdOut = minMicroSec;
}


void TelemLeaveEx(TraceContexHandle ctx, tt_uint64 matchId)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    auto* pThreadData = getThreadData(pCtx);
    if (!pThreadData) {
        return;
    }

    auto depth = --pThreadData->stackDepth;

    auto& zone = pThreadData->zones[depth];
    zone.end = getTicks();

    // work out if we send it.
    auto minMicroSec = matchId;
    auto elpased = zone.end - zone.start;
    auto elapsedMicro = TicksToMicro(elpased);

    if (elapsedMicro > minMicroSec) {
        return;
    }

    queueZone(pCtx, pThreadData, zone);
}
