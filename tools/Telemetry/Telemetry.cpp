#include "stdafx.h"
#include "TelemetryLib.h"

// TODO: get rid of
#include <cstdio>
#include <stddef.h> // for offsetof rip

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
    X_PACK_PUSH(8)
    struct THREADNAME_INFO
    {
        DWORD dwType;     // Must be 0x1000.
        LPCSTR szName;    // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags;    // Reserved for future use, must be zero.
    };
    X_PACK_POP;

    void setThreadName(DWORD dwThreadID, const char* pThreadName)
    {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = pThreadName;
        info.dwThreadID = dwThreadID;
        info.dwFlags = 0;

        constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

        __try {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

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
    X_INLINE constexpr bool IsAligned(T value, unsigned int alignment, unsigned int offset)
    {
        return ((value + offset) % alignment) == 0;
    }

    template<typename T>
    X_INLINE constexpr T RoundUpToMultiple(T numToRound, T multipleOf)
    {
        return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
    }

    template<typename T>
    X_INLINE constexpr T RoundDownToMultiple(T numToRound, T multipleOf)
    {
        return numToRound & ~(multipleOf - 1);
    }

    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);


    struct TraceZone
    {
        tt_uint64 start;
        tt_uint64 end;

        const char* pZoneName;
        TtSourceInfo sourceInfo;
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

        tt_uint64 lastTick;

        TraceThread* pThreadData;
        tt_int32 numThreadData;

        StringTable strTable;

        // zone buffers.
        tt_uint8* pActiveTickBuf;
        tt_int32 tickBufCapacity;
        volatile tt_int32 tickBufOffset;

        // -- Cace lane boundry --

        tt_uint8* pTickBufs[2];
        tt_int32 tickBufOffsets[2];

        // used by background thread
        tt_uint8* pPacketBuffer;
        tt_int32 packetBufSize;
        tt_int32 packetBufCapacity;

        DWORD threadId_;
        HANDLE hThread_;
        HANDLE hSignal_;
        platform::SOCKET socket;
    };

    static_assert(X_OFFSETOF(TraceContext, tickBufOffset) < 64, "Cold fields not on firstcache lane");
    static_assert(X_OFFSETOF(TraceContext, pTickBufs) == 64, "Cold fields not on next cache lane");

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

    bool handleConnectionResponse(tt_uint8* pData, tt_size len)
    {
        // TODO: this can't actually happen.
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

    void sendDataToServer(TraceContext* pCtx, const void* pData, tt_size len)
    {
        // send some data...
        // TODO: none blocking?
        int res = platform::send(pCtx->socket, reinterpret_cast<const char*>(pData), static_cast<int>(len), 0);
        if (res == SOCKET_ERROR) {
            printf("send failed with error: %d\n", platform::WSAGetLastError());
            return;
        }
    }

    void flushPacketBuffer(TraceContext* pCtx)
    {
        if (pCtx->packetBufSize == sizeof(DataStreamData)) {
            return;
        }

        // patch the length
        auto* pHdr = reinterpret_cast<DataStreamData*>(pCtx->pPacketBuffer);
        pHdr->dataSize = pCtx->packetBufSize;

        // flush to socket.
        sendDataToServer(pCtx, pCtx->pPacketBuffer, pCtx->packetBufSize);
        pCtx->packetBufSize = sizeof(DataStreamData);
    }

    void addDataPacket(TraceContext* pCtx, const void* pData, tt_size len)
    {
        // even fit in a packet?
        if (len > pCtx->packetBufCapacity - sizeof(DataStreamData)) {
            ::DebugBreak();
        }

        // can we fit this data?
        const auto space = pCtx->packetBufCapacity - pCtx->packetBufSize;
        if (space < len) {
            flushPacketBuffer(pCtx);
        }

        memcpy(&pCtx->pPacketBuffer[pCtx->packetBufSize], pData, len);
        pCtx->packetBufSize += static_cast<tt_int32>(len);
    }

    void writeStringPacket(TraceContext* pCtx, const char* pStr)
    {
        tt_size strLen = strlen(pStr);
        if (strLen > MAX_STRING_LEN) {
            strLen = MAX_STRING_LEN;
        }

        // TODO: skip the copy and write this directly to packet buffer?
        tt_size packetLen = sizeof(StringTableAddData) + strLen;

        tt_uint8 strDataBuf[sizeof(StringTableAddData) + MAX_STRING_LEN];
        auto* pHeader = reinterpret_cast<StringTableAddData*>(strDataBuf);
        pHeader->type = DataStreamType::StringTableAdd;
        pHeader->length = static_cast<tt_uint16>(strLen);

        memcpy(strDataBuf + sizeof(StringTableAddData), pStr, strLen);

        addDataPacket(pCtx, &strDataBuf, packetLen);
    }

    struct ZoneRawData
    {
        tt_int8 stackDepth;
        TtthreadId threadID;
        
        TraceZone zone;
    };

    void queueZone(TraceContext* pCtx, TraceThread* pThread, TraceZone& zone)
    {
        ZoneRawData data;
        data.stackDepth = static_cast<tt_uint8>(pThread->stackDepth);
        data.threadID = pThread->id;
        data.zone = zone;

        // TODO: make thread safe.
        auto* pBuf = pCtx->pActiveTickBuf;
        auto offset = pCtx->tickBufOffset;
        pCtx->tickBufOffset += sizeof(data);

        memcpy(pBuf + offset, &data, sizeof(data));
    }

    void FLipBuffer(TraceContext* pCtx)
    {
        // flush it baby.
        // basically want to flip the pointers and tell the background thread.
        X_UNUSED(pCtx);

        const tt_int32 curIdx = pCtx->pActiveTickBuf == pCtx->pTickBufs[0] ? 0 : 1;

        // TODO: this needs to be atomic so if something try to write when we doing this it's okay.
        // o also need to know if the background thread finished with the old buffer.
        pCtx->tickBufOffsets[curIdx] = pCtx->tickBufOffset;
        pCtx->pActiveTickBuf = pCtx->pTickBufs[curIdx ^ 1];
        pCtx->tickBufOffset = 0;

        // tell the background thread we are HOT!

        ::SetEvent(pCtx->hSignal_);
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

DWORD __stdcall WorkerThread(LPVOID pParam)
{
    setThreadName(::GetCurrentThreadId(), "Telemetry");

    auto* pCtx = reinterpret_cast<TraceContext*>(pParam);
    
    const bool alertable = false;

    for (;;)
    {
        DWORD result = WaitForSingleObjectEx(pCtx->hSignal_, INFINITE, alertable);
        if (result != WAIT_OBJECT_0) {
            // rip.
            break;
        }

        auto start = gSysTimer.GetMicro();

        // process the bufffer.
        // we only have zone info currently.
        const tt_int32 curIdx = pCtx->pActiveTickBuf == pCtx->pTickBufs[0] ? 1 : 0;

        const auto* pBuf = pCtx->pTickBufs[curIdx];
        const auto size = pCtx->tickBufOffsets[curIdx];

        if (size == 0) {
            ::DebugBreak();
        }

        if (size % sizeof(ZoneRawData) != 0) {
            ::DebugBreak();
        }

        // right you dirty whore.
        auto* pZones = reinterpret_cast<const ZoneRawData*>(pBuf);
        const tt_int32 numZones = size / sizeof(ZoneRawData);

        for (tt_int32 i = 0; i < numZones; i++)
        {
            auto& zoneInfo = pZones[i];
            auto& zone = zoneInfo.zone;

            ZoneData packet;
            packet.type = DataStreamType::Zone;
            packet.stackDepth = static_cast<tt_uint8>(zoneInfo.stackDepth);
            packet.threadID = zoneInfo.threadID;
            packet.start = zone.start;
            packet.end = zone.end;
            packet.strIdxFile = StringTableGetIndex(pCtx->strTable, zone.sourceInfo.pFile_);
            packet.strIdxFunction = StringTableGetIndex(pCtx->strTable, zone.sourceInfo.pFunction_);
            packet.strIdxZone = StringTableGetIndex(pCtx->strTable, zone.pZoneName);

            // i want to write the string data.
            if (packet.strIdxFile.inserted) {
                writeStringPacket(pCtx, zone.sourceInfo.pFile_);
            }
            if (packet.strIdxFunction.inserted) {
                writeStringPacket(pCtx, zone.sourceInfo.pFunction_);
            }
            if (packet.strIdxZone.inserted) {
                writeStringPacket(pCtx, zone.pZoneName);
            }

            addDataPacket(pCtx, &packet, sizeof(packet));
        }

        // flush anything left over.
        flushPacketBuffer(pCtx);

        pCtx->tickBufOffsets[curIdx] = 0;

        auto end = gSysTimer.GetMicro();
        auto ellapsed = end - start;
        
        printf("processed: %d in: %lld\n", numZones, ellapsed);
    }
    
    
    return 0;
}


bool TelemInit(void)
{
#if 1 // TODO: temp
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
#endif

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

TtError TelemInitializeContext(TraceContexHandle& out, void* pArena, tt_size bufLen)
{
    out = INVALID_TRACE_CONTEX;

    const auto* pEnd = reinterpret_cast<tt_uint8*>(pArena) + bufLen;

    // need to align upto 64bytes.
    auto* pBuf = AlignTop(pArena, 64);
    const tt_uintptr alignmentSize = reinterpret_cast<tt_uintptr>(pBuf) - reinterpret_cast<tt_uintptr>(pArena);

    bufLen -= alignmentSize;

    // send packets this size?
    constexpr tt_size packetBufSize = MAX_PACKET_SIZE;
    constexpr tt_size contexSize = sizeof(TraceContext);
    constexpr tt_size threadDataSize = sizeof(TraceThread) * MAX_ZONE_THREADS;
    constexpr tt_size strTableSize = sizeof(void*) * 1024;
    constexpr tt_size minBufferSize = 1024 * 10; // 10kb.. enougth?

    constexpr tt_size internalSize = packetBufSize + contexSize + threadDataSize + strTableSize;
    if (bufLen < internalSize + minBufferSize) {
        return TtError::ArenaTooSmall;
    }

    // i want to split this into two buffers both starting on 64bit boundry.
    // and both multiple of 64.
    // so if we have a number need to round down till it's a multiple of 128?
    const tt_size internalEndAligned = RoundUpToMultiple<tt_size>(internalSize, 64);
    const tt_size bytesLeft = bufLen - internalEndAligned;
    if (bytesLeft < minBufferSize) {
        ::DebugBreak(); // should not happen.
        return TtError::ArenaTooSmall;
    }

    const tt_size tickBufferSize = RoundDownToMultiple<tt_size>(bytesLeft, 128) / 2;
    

    tt_uint8* pBufU8 = reinterpret_cast<tt_uint8*>(pBuf);
    tt_uint8* pThreadDataBuf = pBufU8 + contexSize;
    tt_uint8* pStrTableBuf = pThreadDataBuf + threadDataSize;
    tt_uint8* pPacketBuffer = pStrTableBuf + strTableSize;
    tt_uint8* pTickBuffer0 = reinterpret_cast<tt_uint8*>(AlignTop(pPacketBuffer + packetBufSize, 64));
    tt_uint8* pTickBuffer1 = pTickBuffer0 + tickBufferSize;

    // retard check.
    const tt_ptrdiff trailingBytes = pEnd - (pTickBuffer1 + tickBufferSize);
    if (trailingBytes < 0) {
        ::DebugBreak(); // should not happen, we would write out of bounds.
        return TtError::Error;
    }
    if (trailingBytes > 128) {
        ::DebugBreak(); // should not happen, we are underusing the buffer.
        return TtError::Error;
    }

    TraceContext* pCtx = reinterpret_cast<TraceContext*>(pBuf);
    pCtx->lastTick = gSysTimer.GetMicro();
    pCtx->isEnabled = true;
    pCtx->socket = INV_SOCKET;
    pCtx->pThreadData = reinterpret_cast<TraceThread*>(pThreadDataBuf);
    pCtx->numThreadData = 0;
    pCtx->strTable = CreateStringTable(pStrTableBuf, strTableSize);
    pCtx->pPacketBuffer = pPacketBuffer;
    pCtx->packetBufSize = sizeof(DataStreamData);
    pCtx->packetBufCapacity = packetBufSize;

    // pre fill the header.
    auto* pDataHeader = reinterpret_cast<DataStreamData*>(pCtx->pPacketBuffer);
    pDataHeader->type = PacketType::DataStream;
    pDataHeader->dataSize = 0;

    pCtx->pTickBufs[0] = pTickBuffer0;
    pCtx->pTickBufs[1] = pTickBuffer1;
    pCtx->tickBufOffsets[0] = 0;
    pCtx->tickBufOffsets[1] = 0;
    pCtx->pActiveTickBuf = pCtx->pTickBufs[0];
    pCtx->tickBufCapacity = static_cast<tt_uint32>(tickBufferSize);
    pCtx->tickBufOffset = 0;

    pCtx->hThread_ = ::CreateThread(nullptr, BACKGROUND_THREAD_STACK_SIZE, WorkerThread, pCtx, 0, &pCtx->threadId_);
    if (!pCtx->hThread_) {
        return TtError::Error;
    }

    // make sure we don't get starved, since the host program might make use of all cores
    if (!SetThreadPriority(pCtx->hThread_, THREAD_PRIORITY_ABOVE_NORMAL)) {
        // not fatal.
    }

    pCtx->hSignal_ = CreateEventW(nullptr, false, false, nullptr);


    out = contextToHandle(pCtx);
    return TtError::Ok;
}

void TelemShutdownContext(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);


    if (::WaitForSingleObject(pCtx->hThread_, INFINITE) == WAIT_FAILED) {
        // rip
        return;
    }

    if (pCtx->hSignal_) {
        ::CloseHandle(pCtx->hSignal_);
    }
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

    sendDataToServer(pCtx, &cr, sizeof(cr));

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

void TelemTick(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);

    // TODO: maybe not return
    if (!pCtx->isEnabled) {
        return;
    }

    auto curTime = gSysTimer.GetMicro();
    auto sinceLast = curTime - pCtx->lastTick;

    // if we are been called at a very high freq don't bother sending unless needed.
    if (sinceLast < 1000000) {
        // if the buffer is half full send it!
        auto halfBufferCap = pCtx->tickBufCapacity / 2;
        if (pCtx->tickBufOffset < halfBufferCap) {
            return;
        }
    }

    pCtx->lastTick = curTime;

    FLipBuffer(pCtx);
    return;
}

void TelemFlush(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);

    // TODO: maybe not return
    if (!pCtx->isEnabled) {
        return;
    }

    if (pCtx->tickBufOffset == 0) {
        return;
    }

    FLipBuffer(pCtx);
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
    zone.sourceInfo = sourceInfo;
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
    zone.sourceInfo = sourceInfo;

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
