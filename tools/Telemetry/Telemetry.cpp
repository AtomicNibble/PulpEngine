#include "stdafx.h"
#include "TelemetryLib.h"

// TODO: get rid of
#include <memory>
#include <cstdio>
#include <stddef.h> // for offsetof rip

#include <intrin.h>

#include <../../3rdparty/source/lz4-1.8.3/lz4_lib.h>

TELEM_LINK_LIB("engine_TelemetryCommonLib.lib");

TELEM_DISABLE_WARNING(4324) //  structure was padded due to alignment specifier


namespace
{
    TELEM_PACK_PUSH(8)
    struct THREADNAME_INFO
    {
        DWORD dwType;     // Must be 0x1000.
        LPCSTR szName;    // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags;    // Reserved for future use, must be zero.
    };
    TELEM_PACK_POP;

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

    SysTimer gSysTimer;
    tt_uint64 gTicksPerMicro;

    TELEM_INLINE tt_uint32 getThreadID(void)
    {
        // TODO: remove function call.
        return ::GetCurrentThreadId();
    }

    TELEM_INLINE tt_uint64 getTicks(void)
    {
        return __rdtsc();
    }

    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

    struct ArgDataBuilder
    {
        constexpr static tt_int32 BUF_SIZE = 255;

        tt_uint8 numArgs;
        tt_uint8 data[BUF_SIZE];
    };

    TELEM_PACK_PUSH(1)

    struct TraceLock
    {
        tt_uint64 start;
        tt_uint64 end;

        const char* pFmtStr;
        TtLockResult result;
        tt_uint16 depth;
        TtSourceInfo sourceInfo;

        static_assert(std::numeric_limits<decltype(depth)>::max() >= MAX_ZONE_DEPTH, "Can't store max zone depth");
    };

    TELEM_PACK_POP;

    struct TraceLockBuilder
    {
        TraceLock lock;
        tt_int32 argDataSize;
        ArgDataBuilder argData;
    };

    struct TraceLocks
    {
        const void* pLockPtr[MAX_LOCKS_HELD_PER_THREAD];
        TraceLockBuilder locks[MAX_LOCKS_HELD_PER_THREAD];
    };

    struct TraceZone
    {
        tt_uint64 start;
        tt_uint64 end;

        const char* pFmtStr;
        TtSourceInfo sourceInfo;
    };

    // TODO: don't bother storing argData if not passed.
    struct TraceZoneBuilder
    {
        TraceZone zone;
        tt_int32 argDataSize;
        ArgDataBuilder argData;
    };


    // some data for each thread!
    TELEM_ALIGNED_SYMBOL(struct TraceThread, 64)
    {
        TraceThread() {
            id = getThreadID();
            stackDepth = 0;

            // TODO: Debug only?
#if X_DEBUG
            zero_object(zones);
            zero_object(locks);
#endif // X_DEBUG
        }

        TtthreadId id;
        tt_int32 stackDepth;

        TraceZoneBuilder zones[MAX_ZONE_DEPTH];
        TraceLocks locks;
    };

    thread_local TraceThread* gThreadData = nullptr;

    struct TickBuffer
    {
        tt_uint8* pTickBuf;   // fixed
        volatile tt_int32 bufOffset;
    };

#if X_64
    #define X86_PAD(bytes)
#else
    #define X86_PAD(bytes) tt_uint8 __TELEMETRY_UNIQUE_NAME(__pad)[bytes];
#endif // X_64


    void defaultLogFunction(void* pUserData, TtLogType::Enum type, const char* pMsgNullTerm, tt_int32 lenWithoutTerm)
    {
        TELEM_UNUSED(pUserData);
        TELEM_UNUSED(lenWithoutTerm);
        TELEM_UNUSED(type);

        ::OutputDebugStringA(pMsgNullTerm);
        ::OutputDebugStringA("\n");
    }

    // This is padded to 64bit to make placing TraceThread data on it's own boundy more simple.
    TELEM_ALIGNED_SYMBOL(struct TraceContext, 64)
    {
        tt_uint64 lastTick;
        tt_uint64 lastTickNano;

        TraceThread* pThreadData;
        tt_int32 numThreadData;

        tt_uint64 ticksPerMicro;
        tt_uint64 baseTicks;
        tt_uint64 baseNano;

        bool isEnabled;
        bool _pad[7];

    //    tt_uint8 _lanePad0[6];
        X86_PAD(8)

        // -- Cace lane boundry --

        volatile tt_int32 activeTickBufIdx;
        TickBuffer tickBuffers[2];

        tt_int32 tickBufCapacity;

        tt_uint8 _lanePad1[20];
        X86_PAD(20)

        // -- Cace lane boundry --

        DWORD threadId_;
        HANDLE hThread_;
        HANDLE hSignal_;
        HANDLE hSignalIdle_;
        volatile tt_int32 shutDownFlag;
        platform::SOCKET socket;

        tt_uint8 _lanePad2[16];

        X86_PAD(24)

        // -- Cace lane boundry --

        CriticalSection cs_;
        tt_int32 numStalls;
        tt_int32 totalEvents;

        LogFunction logFunc;
        void* pUserData;

        // -- Cace lane boundry --

    };

 //   constexpr size_t size0 = sizeof(TraceContext);
 //   constexpr size_t size1 = TELEM_OFFSETOF(TraceContext, threadId_);
 //   constexpr size_t size2 = TELEM_OFFSETOF(TraceContext, cs_);

    static_assert(TELEM_OFFSETOF(TraceContext, activeTickBufIdx) == 64, "Cold fields not on firstcache lane");
    static_assert(TELEM_OFFSETOF(TraceContext, threadId_) == 128, "Cold fields not on next cache lane");
    static_assert(TELEM_OFFSETOF(TraceContext, cs_) == 192, "cache lane boundry changed");
    static_assert(sizeof(TraceContext) == 256, "Size changed");

    TELEM_INLINE TraceContexHandle contextToHandle(TraceContext* pCtx)
    {
        return reinterpret_cast<TraceContexHandle>(pCtx);
    }

    TELEM_INLINE TraceContext* handleToContext(TraceContexHandle handle)
    {
        return reinterpret_cast<TraceContext*>(handle);
    }

    TELEM_INLINE bool isValidContext(TraceContexHandle handle)
    {
        return handle != INVALID_TRACE_CONTEX;
    }

    TELEM_INLINE tt_uint64 ticksToNano(TraceContext* pCtx, tt_uint64 tsc)
    {
        // This is correct using ticksPerMicro to work out nano.
        // TODO: switch this to ticksPerMs to get better accuracy.
        const tt_uint64 ticksPerMicro = pCtx->ticksPerMicro;
        const tt_uint64 whole = (tsc / ticksPerMicro) * 1000;
        const tt_uint64 part = (tsc % ticksPerMicro) * 1000 / ticksPerMicro;

        return whole + part;
    }

    TELEM_INLINE tt_uint64 getRelativeTicks(TraceContext* pCtx)
    {
        return __rdtsc() - pCtx->baseTicks;
    }

    TELEM_INLINE tt_uint64 toRelativeTicks(TraceContext* pCtx, tt_uint64 ticks)
    {
        return ticks - pCtx->baseTicks;
    }

    TELEM_INLINE tt_int32 getActiveTickBufferSize(TraceContext* pCtx)
    {
        auto& buf = pCtx->tickBuffers[pCtx->activeTickBufIdx];
        return buf.bufOffset;
    }

    TraceLockBuilder* addLock(TraceThread* pThread, const void* pLockPtr)
    {
        auto& locks = pThread->locks;
        for (tt_int32 i = 0; i < MAX_LOCKS_HELD_PER_THREAD; i++)
        {
            if (!locks.pLockPtr[i])
            {
                locks.pLockPtr[i] = pLockPtr;
                return &locks.locks[i];
            }
        }

        return nullptr;
    }


    TraceLockBuilder* getLockAndClearSlot(TraceThread* pThread, const void* pLockPtr)
    {
        TraceLockBuilder* pLock = nullptr;

        auto& locks = pThread->locks;
        for (tt_int32 i = 0; i < MAX_LOCKS_HELD_PER_THREAD; i++)
        {
            if (locks.pLockPtr[i] == pLockPtr)
            {
                locks.pLockPtr[i] = nullptr; // clear the slot.

                pLock = &locks.locks[i];
                // TODO: is breaking faster here?
                // I'm guessing not as long as this loop is unrolled.
                // break;

            }
        }

        return pLock;
    }

    TELEM_NO_INLINE TraceThread* addThreadData(TraceContext* pCtx)
    {
        if (pCtx->numThreadData == MAX_ZONE_THREADS) {
            return nullptr;
        }

        ++pCtx->numThreadData;

        auto* pThreadData = new (&pCtx->pThreadData[pCtx->numThreadData]) TraceThread();
        // set the TLS value.
        gThreadData = pThreadData;

        return pThreadData;
    }

    TELEM_INLINE TraceThread* getThreadData(TraceContext* pCtx)
    {
        auto* pThreadData = gThreadData;
        if (!pThreadData) {
            return addThreadData(pCtx);
        }

        return pThreadData;
    }

    void writeLog(TraceContext* pCtx, TtLogType::Enum type, const char* pFmt, ...)
    {
        char buf[MAX_STRING_LEN] = {};

        va_list args;
        va_start(args, pFmt);
        tt_int32 len = vsprintf(buf, pFmt, args); // TODO: replace
        va_end(args);

        pCtx->logFunc(pCtx->pUserData, type, buf, len);
    }

    bool readPacket(TraceContext* pCtx, char* pBuffer, int& bufLengthInOut)
    {
        // this should return complete packets or error.
        int bytesRead = 0;
        int bufLength = sizeof(PacketBase);

        while (1) {
            int maxReadSize = bufLength - bytesRead;
            int res = platform::recv(pCtx->socket, &pBuffer[bytesRead], maxReadSize, 0);

            if (res == 0) {
                writeLog(pCtx, TtLogType::Error, "Connection closing...");
                return false;
            }
            else if (res < 0) {
                writeLog(pCtx, TtLogType::Error, "recv failed with error: %d", platform::WSAGetLastError());
                return false;
            }

            bytesRead += res;

            writeLog(pCtx, TtLogType::Msg, "got: %d bytes\n", res);

            if (bytesRead == sizeof(PacketBase))
            {
                auto* pHdr = reinterpret_cast<const PacketBase*>(pBuffer);
                if (pHdr->dataSize == 0) {
                    writeLog(pCtx, TtLogType::Error, "Client sent packet with length zero...");
                    return false;
                }

                if (pHdr->dataSize > bufLengthInOut) {
                    writeLog(pCtx, TtLogType::Error, "Client sent oversied packet of size %i...", static_cast<tt_int32>(pHdr->dataSize));
                    return false;
                }

                bufLength = pHdr->dataSize;
            }

            if (bytesRead == bufLength) {
                bufLengthInOut = bytesRead;
                return true;
            }
            else if (bytesRead > bufLength) {
                writeLog(pCtx, TtLogType::Error, "Overread packet bytesRead: %d recvbuflen: %d", bytesRead, bufLength);
                return false;
            }
        }
    }

    bool handleConnectionResponse(TraceContext* pCtx, tt_uint8* pData, tt_size len)
    {
        TELEM_UNUSED(len);

        auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);
        switch (pPacketHdr->type)
        {
            case PacketType::ConnectionRequestAccepted:
                // don't care about response currently.
                return true;
            case PacketType::ConnectionRequestRejected: {
                auto* pConRej = reinterpret_cast<const ConnectionRequestRejectedHdr*>(pData);
                auto* pStrData = reinterpret_cast<const char*>(pConRej + 1);
                writeLog(pCtx, TtLogType::Error, "Connection rejected: %.*s", pConRej->reasonLen, pStrData);

            }
            default:
                return false;
        }
    }

    struct SocketBuffer
    {
        tt_uint8* pPacketBuffer;
        tt_int32 packetBufSize;
        const tt_int32 packetBufCapacity;
    };

    void sendDataToServer(TraceContext* pCtx, const void* pData, tt_int32 len)
    {
#if X_DEBUG
        if (len > MAX_PACKET_SIZE) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        // send some data...
        // TODO: none blocking?
        int res = platform::send(pCtx->socket, reinterpret_cast<const char*>(pData), len, 0);
        if (res == SOCKET_ERROR) {
            writeLog(pCtx, TtLogType::Error, "Socket: send failed with error: %d", platform::WSAGetLastError());
            return;
        }
    }

#define PACKET_COMPRESSION 1

#if PACKET_COMPRESSION 
    constexpr tt_int32 PACKET_HDR_SIZE = 0;
#else
    constexpr tt_int32 PACKET_HDR_SIZE = sizeof(DataStreamHdr);
#endif // PACKET_COMPRESSION 

    void flushPacketBuffer(TraceContext* pCtx, SocketBuffer* pBuffer)
    {
        if (pBuffer->packetBufSize == PACKET_HDR_SIZE) {
            return;
        }

#if !PACKET_COMPRESSION 
        // patch the length
        auto* pHdr = reinterpret_cast<DataStreamHdr*>(pBuffer->pPacketBuffer);
        pHdr->dataSize = static_cast<tt_uint16>(pBuffer->packetBufSize);
#endif // PACKET_COMPRESSION

        // flush to socket.
        sendDataToServer(pCtx, pBuffer->pPacketBuffer, pBuffer->packetBufSize);
        pBuffer->packetBufSize = PACKET_HDR_SIZE;
    }


    void addToPacketBuffer(TraceContext* pCtx, SocketBuffer* pBuffer, const void* pData, tt_int32 len)
    {
#if X_DEBUG
        // even fit in a packet?
        if (len > pBuffer->packetBufCapacity - PACKET_HDR_SIZE) {
            ::DebugBreak();
        }
        if (pBuffer->packetBufSize < PACKET_HDR_SIZE) {
            ::DebugBreak();
        }
#endif // X_DEBUG


        // can we fit this data?
        const tt_int32 space = pBuffer->packetBufCapacity - pBuffer->packetBufSize;
        if (space >= len) {
            memcpy(&pBuffer->pPacketBuffer[pBuffer->packetBufSize], pData, len);
            pBuffer->packetBufSize += len;
            return;
        }
        
#if X_DEBUG
        if (len <= space) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        // lets copy what we can flush then copy trailing.
        const auto trailing = len - space;
        memcpy(&pBuffer->pPacketBuffer[pBuffer->packetBufSize], pData, space);
        pBuffer->packetBufSize += space;

        flushPacketBuffer(pCtx, pBuffer);

#if X_DEBUG
        if (pBuffer->packetBufSize != PACKET_HDR_SIZE) {
            ::DebugBreak();
        }
#endif // X_DEBUG


        memcpy(&pBuffer->pPacketBuffer[pBuffer->packetBufSize], reinterpret_cast<const tt_uint8*>(pData) + space, trailing);
        pBuffer->packetBufSize += trailing;
    }

    struct PacketCompressor
    {
        PacketCompressor() {
            static_assert(sizeof(cmpBuf) + 16 < MAX_PACKET_SIZE, "Can't fit worst case in packet");
            static_assert(sizeof(cmpBuf) < std::numeric_limits<decltype(packetHdr.dataSize)>::max(), 
                "Can't store max compressed len in packet header");
            static_assert(COMPRESSION_MAX_INPUT_SIZE * 2 <= COMPRESSION_RING_BUFFER_SIZE,
                "Can't even fit two buffers in ring");

            static_assert(TELEM_OFFSETOF(PacketCompressor, cmpBuf) ==
                TELEM_OFFSETOF(PacketCompressor, packetHdr) + sizeof(DataStreamHdr), "cmdBuf has padding after PacketHdr");

            pCtx = nullptr;
            pBuffer = nullptr;
            writeBegin = 0;
            writeEnd = 0;
            LZ4_resetStream(&lz4Stream);
        }

        TraceContext* pCtx;
        SocketBuffer* pBuffer;

        StringTable strTable;

        tt_int32 writeBegin;
        tt_int32 writeEnd;

        LZ4_stream_t lz4Stream;

        // a buffer for storing data to be compressed.
        tt_int8 srcRingBuf[COMPRESSION_RING_BUFFER_SIZE];

        // output compressed data.
        DataStreamHdr packetHdr;
        char cmpBuf[LZ4_COMPRESSBOUND(COMPRESSION_MAX_INPUT_SIZE)];
    };


    void flushCompressionBuffer(PacketCompressor* pComp)
    {
#if PACKET_COMPRESSION
        // compress it.
        const auto* pInBegin = &pComp->srcRingBuf[pComp->writeBegin];
        const auto inBytes = pComp->writeEnd - pComp->writeBegin;

#if X_DEBUG
        if (inBytes > COMPRESSION_MAX_INPUT_SIZE) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        if (inBytes == 0) {
            return;
        }

        const tt_int32 cmpBytes = LZ4_compress_fast_continue(&pComp->lz4Stream, pInBegin,
            pComp->cmpBuf, inBytes, sizeof(pComp->cmpBuf), 9);

        if (cmpBytes <= 0) {
            // TODO: error.
        }

        const tt_int32 totalLen = cmpBytes + sizeof(DataStreamHdr);

        // patch the length 
        pComp->packetHdr.type = PacketType::DataStream;
        pComp->packetHdr.dataSize = static_cast<tt_uint16>(totalLen);
        pComp->packetHdr.origSize = static_cast<tt_uint16>(inBytes + sizeof(DataStreamHdr));

        addToPacketBuffer(pComp->pCtx, pComp->pBuffer, &pComp->packetHdr, totalLen);

        pComp->writeBegin = pComp->writeEnd;
        if ((sizeof(pComp->srcRingBuf) - pComp->writeBegin) < COMPRESSION_MAX_INPUT_SIZE) {
            pComp->writeBegin = 0;
            pComp->writeEnd = 0;
        }
#else
        TELEM_UNUSED(pComp);
#endif // !PACKET_COMPRESSION
    }

    tt_int32 getCompressionBufferSpace(PacketCompressor* pComp)
    {
        const tt_int32 space = COMPRESSION_MAX_INPUT_SIZE - (pComp->writeEnd - pComp->writeBegin);
        return space;
    }

    void flushCompressionBufferIfrequired(PacketCompressor* pComp, tt_int32 requiredSpace)
    {
        const tt_int32 space = getCompressionBufferSpace(pComp);
        if (space < requiredSpace) {
            flushCompressionBuffer(pComp);
        }
    }

    void addToCompressionBufferNoFlush(PacketCompressor* pComp, const void* pData, tt_int32 len)
    {
#if PACKET_COMPRESSION

#if X_DEBUG
        if (len > COMPRESSION_MAX_INPUT_SIZE) {
            ::DebugBreak();
        }

        const tt_int32 space = getCompressionBufferSpace(pComp);
        if (space < len) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        memcpy(&pComp->srcRingBuf[pComp->writeEnd], pData, len);
        pComp->writeEnd += len;
#else
        addToPacketBuffer(pComp->pCtx, pComp->pBuffer, pData, len);
#endif // !PACKET_COMPRESSION
    }

    void addToCompressionBuffer(PacketCompressor* pComp, const void* pData, tt_int32 len)
    {
#if PACKET_COMPRESSION

#if X_DEBUG
        if (len > COMPRESSION_MAX_INPUT_SIZE) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        // can we fit this data?
        flushCompressionBufferIfrequired(pComp, len);

        memcpy(&pComp->srcRingBuf[pComp->writeEnd], pData, len);
        pComp->writeEnd += len;
#else
        addToPacketBuffer(pComp->pCtx, pComp->pBuffer, pData, len);
#endif // !PACKET_COMPRESSION
    }

    void writeStringCompressionBuffer(PacketCompressor* pComp, StringTableIndex idx, const char* pStr)
    {
        tt_size strLen = strlen(pStr);
        if (strLen > MAX_STRING_LEN) {
            strLen = MAX_STRING_LEN;
        }

        // TODO: skip the copy and write this directly to packet buffer?
        tt_int32 packetLen = sizeof(DataPacketStringTableAdd) + static_cast<tt_int32>(strLen);

        tt_uint8 strDataBuf[sizeof(DataPacketStringTableAdd) + MAX_STRING_LEN];
        auto* pHeader = reinterpret_cast<DataPacketStringTableAdd*>(strDataBuf);
        pHeader->type = DataStreamType::StringTableAdd;
        pHeader->id = idx.index;
        pHeader->length = static_cast<tt_uint16>(strLen);

        memcpy(strDataBuf + sizeof(DataPacketStringTableAdd), pStr, strLen);

        addToCompressionBuffer(pComp, &strDataBuf, packetLen);
    }

    // -----------------------------------

    inline constexpr tt_int32 RoundUpToMultiple(tt_int32 numToRound, tt_int32 multipleOf)
    {
        return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
    }

    template<typename T>
    inline constexpr tt_int32 GetSizeNotArgData(void)
    {
        static_assert(sizeof(T) == 64);
        return sizeof(T);
    }

    template<typename T>
    inline constexpr tt_int32 GetSizeWithoutArgData(void)
    {
        constexpr tt_int32 argDataSize = sizeof(T::argData);
        return RoundUpToMultiple(sizeof(T) - argDataSize, 64);
    }

    template<typename T>
    inline constexpr tt_int32 GetDataSize(tt_int32 argDataSize)
    {
        return RoundUpToMultiple(GetSizeWithoutArgData<T>() + argDataSize, 64);
    }


    auto _isDigit = [](char ch) -> bool {
        return (ch >= '0') && (ch <= '9');
    };

    auto _atoi = [](const char** pStr) -> tt_int32 {
        tt_int32 i = 0;

        while (_isDigit(**pStr)) {
            i = i * 10 + static_cast<tt_int32>(*((*pStr)++) - '0');
        }

        return i;
    };

    tt_int32 AddStrings(ArgDataBuilder& data, const char* pFmtString, tt_int32 numArgs, uintptr_t* pValues)
    {
        // now we need to parse any strings and add them to the buffer.
        const tt_int32 bytesUsed = numArgs * sizeof(uintptr_t);
        tt_int32 bytesLeft = sizeof(data.data) - bytesUsed;
        tt_uint8* pStrData = &data.data[bytesUsed];

        tt_int32 idx = 0;

        // %[flags][width][.precision][length]
        while (*pFmtString)
        {
            if (*pFmtString++ != '%') {
                continue;
            }

            // skip flags
            bool loop = false;

            do
            {
                switch (*pFmtString)
                {
                    case '0':
                    case '-':
                    case '+':
                    case ' ':
                    case '#':
                        ++pFmtString;
                        loop = true;
                        break;
                    default:
                        loop = false;
                        break;
                }
            } while (loop);

            // TODO: width field

            // evaluate precision field
            tt_int32 precision = MAX_STRING_LEN;
            if (*pFmtString == '.') {
                ++pFmtString;

                // fixed length or length passed?
                if (_isDigit(*pFmtString)) {
                    precision = _atoi(&pFmtString);
                }
                else if (*pFmtString == '*') {
                    const tt_int32 prec = static_cast<tt_int32>(pValues[idx++]);
                    precision = prec > 0 ? (unsigned int)prec : 0U;
                    pFmtString++;
                }
            }

            switch (*pFmtString) {
                case 'l':
                case 'h':
                case 'j':
                case 'z':
                    pFmtString++;
                    break;
                default:
                    break;
            }

            switch (*pFmtString) {
                case 's': {
                    const char* pStr = reinterpret_cast<const char*>(pValues[idx]);
                    auto len = static_cast<tt_uint32>(strnlen(pStr, precision));

                    static_assert(std::numeric_limits<tt_uint8>::max() >= MAX_STRING_LEN, "Can't store max string length");

                    // TODO: handle buffer not having space etc.
                    *pStrData++ = static_cast<tt_uint8>(len & 0xFF);
                    memcpy(pStrData, pStr, len);

                    pStrData += len;
                    bytesLeft -= len;
                    break;
                }

                case '%':
                default:
                    ++pFmtString;
                    break;
            }

            ++idx;
        }

        return (sizeof(data.numArgs) + sizeof(data)) - bytesLeft;
    }

    // template<typename T>
    tt_int32 BuildArgData(ArgDataBuilder& data, const char* pFmtString, tt_int32 numArgs, va_list& l)
    {
        data.numArgs = static_cast<tt_int8>(numArgs & 0xFF);
        uintptr_t* pValues = reinterpret_cast<uintptr_t*>(data.data);

        for (int32_t i = 0; i < numArgs; i++) {
            pValues[i] = va_arg(l, uintptr_t);
        }

        // if we know there are no strings we can just return here.
        bool noStrings = false;

        if (noStrings) {
            const tt_int32 bytesUsed = numArgs * sizeof(uintptr_t);
            return sizeof(data.numArgs) + bytesUsed;
        }

        return AddStrings(data, pFmtString, numArgs, pValues);
    }

    // -----------------------------------

    enum class QueueDataType : tt_uint8
    {
        Zone,
        TickInfo,
        ThreadSetName,
        CallStack,
        LockSetName,
        LockTry,
        LockState,
        LockCount,
        MemAlloc,
        MemFree,
        Message
    };

    TELEM_PACK_PUSH(8)

    struct QueueDataBase
    {
        QueueDataType type;
        tt_uint8 argDataSize;
    };
    
    TELEM_ALIGNED_SYMBOL(struct QueueDataTickInfo, 64) : public QueueDataBase
    {
        TtthreadId threadID;
        tt_uint64 start;
        tt_uint64 end;
        tt_uint64 startNano;
        tt_uint64 endNano;
    };

    struct QueueDataThreadSetName : public QueueDataBase
    {
        TtthreadId threadID;
        tt_uint64 time;
        const char* pFmtStr;

        ArgDataBuilder argData;
    };

    TELEM_ALIGNED_SYMBOL(struct QueueDataCallStack, 64) : public QueueDataBase
    {
        TtCallStack callstack;
    };

    struct QueueDataZone : public QueueDataBase
    {
        tt_int8 stackDepth;
        TtthreadId threadID;
        
        TraceZone zone;

        ArgDataBuilder argData;
    };

    struct QueueDataLockSetName : public QueueDataBase
    {
        const void* pLockPtr;
        const char* pFmtStr;
        tt_uint64 time;

        ArgDataBuilder argData;
    };

    struct QueueDataLockTry : public QueueDataBase
    {
        TraceLock lock;
        TtthreadId threadID;
        const void* pLockPtr;

        ArgDataBuilder argData;
    };

    struct QueueDataLockState : public QueueDataBase
    {
        TtLockState state;
        TtthreadId threadID;
        tt_uint64 time;
        TtSourceInfo sourceInfo;
        const void* pLockPtr;
        const char* pFmtStr;

        ArgDataBuilder argData;
    };

    struct QueueDataLockCount : public QueueDataBase
    {
        tt_uint16 count;
        TtthreadId threadID;
        tt_uint64 time;
        TtSourceInfo sourceInfo;
        const void* pLockPtr;
        const char* pFmtStr;

        ArgDataBuilder argData;
    };

    struct QueueDataMemAlloc : public QueueDataBase
    {
        TtthreadId threadID;
        tt_uint64 time;
        tt_uint32 size;
        TtSourceInfo sourceInfo;
        const void* pPtr;
        const char* pFmtStr;

        ArgDataBuilder argData;
    };

    struct QueueDataMemFree : public QueueDataBase
    {
        TtthreadId threadID;
        tt_uint64 time;
        TtSourceInfo sourceInfo;
        const void* pPtr;
        const char* pFmtStr;

        ArgDataBuilder argData;
    };

    struct QueueDataMessage : public QueueDataBase
    {
        TtLogType::Enum logType;
        tt_uint64 time;
        const char* pFmtStr;

        ArgDataBuilder argData;
    };

    TELEM_PACK_POP


    constexpr size_t size0 = sizeof(QueueDataThreadSetName);
    constexpr size_t size1 = sizeof(QueueDataZone);
    constexpr size_t size2 = sizeof(QueueDataLockSetName);
    constexpr size_t size3 = sizeof(QueueDataLockTry);
    constexpr size_t size4 = sizeof(QueueDataLockState);
    constexpr size_t size5 = sizeof(QueueDataLockCount);
    constexpr size_t size6 = sizeof(QueueDataMemAlloc);
    constexpr size_t size7 = sizeof(QueueDataMemFree);
    constexpr size_t size8 = sizeof(QueueDataMessage);

    constexpr size_t size9 = sizeof(ArgDataBuilder);
    

    static_assert(64 == GetSizeWithoutArgData<QueueDataThreadSetName>());
    static_assert(64 == sizeof(QueueDataTickInfo));
    static_assert(64 == GetSizeWithoutArgData<QueueDataZone>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataLockSetName>());
    static_assert(64 == sizeof(QueueDataCallStack));
    static_assert(64 == GetSizeWithoutArgData<QueueDataLockTry>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataLockState>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataLockCount>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataMemAlloc>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataMemFree>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataMessage>());

    void flipBufferInternal(TraceContext* pCtx, bool force)
    {
        auto bufSize = getActiveTickBufferSize(pCtx);
        if (bufSize == 0) {
            return;
        }

        // don't flip if not half full and force is false.
        auto halfBufferCap = pCtx->tickBufCapacity / 2;
        if (!force && bufSize < halfBufferCap) {
            return;
        }

        // trace ourself :D
        // i would like to know about trace stalls.
        ttZoneFilterd(contextToHandle(pCtx), 100, "FlipBuffers");

        {
            auto waitStart = getTicks();

            // wait for the background thread to finish process that last buffer.
            // TODO: maybe come up with a fast path for when we don't need to wait.
            // check if the signal has a userspace atomic it checks before waiting.
            DWORD result = WaitForSingleObjectEx(pCtx->hSignalIdle_, INFINITE, false);
            if (result != WAIT_OBJECT_0) {
                ::DebugBreak();
                return;
            }

            auto waitEnd = getTicks();
            auto ellapsedNano = ticksToNano(pCtx, waitEnd - waitStart);

            // did we have to wait more than 0.1ms?
            if (ellapsedNano > 100'000)
            {
                writeLog(pCtx, TtLogType::Warning, "Flip stalled for: %lluns", ellapsedNano);
            }
        }

        // the background thread has finished with old buffer.
        // make sure that buffers offset is reset before making it live.
        const auto oldIdx = pCtx->activeTickBufIdx ^ 1;
        _InterlockedExchange(reinterpret_cast<volatile long*>(&pCtx->tickBuffers[oldIdx].bufOffset), 0l);

        // flip the buffers.
        (void)_InterlockedXor(reinterpret_cast<volatile long*>(&pCtx->activeTickBufIdx), 1l);

        // tell the background thread we are HOT!
        ::SetEvent(pCtx->hSignal_);
    }


    void flipBuffer(TraceContext* pCtx, bool stalled, bool force)
    {
        // this can be entered from multiple threads but we only want to flip once.
        // TODO: if we just flipped we should not flip again
        if (pCtx->cs_.TryEnter())
        {
            if (stalled) {
                pCtx->numStalls++;
            }

            flipBufferInternal(pCtx, force);

            pCtx->cs_.Leave();
        }
        else
        {
            // need to wait.
            // lets just take the lock, not ideal if lots of threads end up here.
            // but got biggeer problems if that happening tbh.
            ScopedLock lock(pCtx->cs_);
        }
    }


    TELEM_NO_INLINE void tickBufferFull(TraceContext* pCtx)
    {
        // we are full.
        // in order to flip we need to make everythread has finished writing to the tick buffer
        // and is ready for the flip.
        // but then again, I don't do any of this for normal flips.
        // shit.

        // I was thinking of looking at all the threads and make sure they are outside this module or also waiting for 
        // flip.

        // but can't be doing that every tick.
        // so think i will need to keep like a tail and head value
        // so i update the tail after write is finished.
        // that way the background thread can know the writes are finished.

        flipBuffer(pCtx, true, false);
    }

    void addToTickBuffer(TraceContext* pCtx, const void* pPtr, tt_int32 size)
    {
        auto& buf = pCtx->tickBuffers[pCtx->activeTickBufIdx];
        long offset = _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&buf.bufOffset), static_cast<long>(size));
        
        if (offset + size <= pCtx->tickBufCapacity) {
            memcpy(buf.pTickBuf + offset, pPtr, size);
            return;
        }

        // no space,
#if X_DEBUG && 0
        if (offset + size > pCtx->tickBufCapacity) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        tickBufferFull(pCtx);
        addToTickBuffer(pCtx, pPtr, size);
    }
     
    TELEM_INLINE void queueTickInfo(TraceContext* pCtx, tt_uint64 startTick, tt_uint64 endTick, tt_uint64 startNano, tt_uint64 endNano)
    {
        QueueDataTickInfo data;
        data.type = QueueDataType::TickInfo;
        data.argDataSize = 0;
        data.threadID = getThreadID();
        data.start = toRelativeTicks(pCtx, startTick);
        data.end = toRelativeTicks(pCtx, endTick);
        data.startNano = startNano - pCtx->baseNano;
        data.endNano = endNano - pCtx->baseNano;

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    TELEM_INLINE void queueThreadSetName(TraceContext* pCtx, tt_uint32 threadID, const char* pName)
    {
        QueueDataThreadSetName data;
        data.type = QueueDataType::ThreadSetName;
        data.argDataSize = 0;
        data.time = getRelativeTicks(pCtx);
        data.threadID = threadID;
        data.pFmtStr = pName;

        addToTickBuffer(pCtx, &data, GetSizeWithoutArgData<decltype(data)>());
    }

    TELEM_INLINE void queueCallStack(TraceContext* pCtx, const TtCallStack& stack)
    {
        QueueDataCallStack data;
        data.type = QueueDataType::CallStack;
        data.argDataSize = 0;
        data.callstack = stack;

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    TELEM_INLINE void queueZone(TraceContext* pCtx, TraceThread* pThread, TraceZoneBuilder& scopeData)
    {
        QueueDataZone data;
        data.type = QueueDataType::Zone;
        data.argDataSize = static_cast<tt_uint8>(scopeData.argDataSize & 0xFF);
        data.stackDepth = static_cast<tt_uint8>(pThread->stackDepth & 0xFF);
        data.threadID = pThread->id;
        data.zone = scopeData.zone;

        if (scopeData.argDataSize) {
            memcpy(&data.argData, &scopeData.argData, scopeData.argDataSize);
        }

        addToTickBuffer(pCtx, &data, GetDataSize<decltype(data)>(scopeData.argDataSize));
    }

    TELEM_INLINE void queueLockTry(TraceContext* pCtx, TraceThread* pThread, const void* pPtr, TraceLockBuilder* pLock)
    {
        QueueDataLockTry data;
        data.type = QueueDataType::LockTry;
        data.argDataSize = 0;
        data.threadID = pThread->id;
        data.lock = pLock->lock;
        data.pLockPtr = pPtr;

        if (pLock->argDataSize) {
            memcpy(&data.argData, &pLock->argData, pLock->argDataSize);
        }

        addToTickBuffer(pCtx, &data, GetDataSize<decltype(data)>(pLock->argDataSize));
    }

    TELEM_INLINE void queueLockState(TraceContext* pCtx, const TtSourceInfo& sourceInfo, const void* pPtr, TtLockState state)
    {
        QueueDataLockState data;
        data.type = QueueDataType::LockState;
        data.argDataSize = 0;
        data.time = getRelativeTicks(pCtx);
        data.pLockPtr = pPtr;
        data.state = state;
        data.threadID = getThreadID();
        data.sourceInfo = sourceInfo;
        data.pFmtStr = "<none>";

        addToTickBuffer(pCtx, &data, GetSizeWithoutArgData<decltype(data)>());
    }

    TELEM_INLINE void queueLockCount(TraceContext* pCtx, const TtSourceInfo& sourceInfo, const void* pPtr, tt_int32 count)
    {
        QueueDataLockCount data;
        data.type = QueueDataType::LockCount;
        data.argDataSize = 0;
        data.time = getRelativeTicks(pCtx);
        data.pLockPtr = pPtr;
        data.count = static_cast<tt_uint16>(count);
        data.threadID = getThreadID();
        data.sourceInfo = sourceInfo;
        data.pFmtStr = "<none>";

        addToTickBuffer(pCtx, &data, GetSizeWithoutArgData<decltype(data)>());
    }

    TELEM_INLINE void queueMemFree(TraceContext* pCtx, const TtSourceInfo& sourceInfo, const void* pPtr)
    {
        QueueDataMemFree data;
        data.type = QueueDataType::MemFree;
        data.argDataSize = 0;
        data.time = getRelativeTicks(pCtx);
        data.pPtr = pPtr;
        data.threadID = getThreadID();
        data.sourceInfo = sourceInfo;

        addToTickBuffer(pCtx, &data, GetSizeWithoutArgData<decltype(data)>());
    }

    // Processing.
    tt_uint16 GetStringId(PacketCompressor* pComp, const char* pStr)
    {
        auto idx = StringTableGetIndex(pComp->strTable, pStr);

        if (idx.inserted) {
            writeStringCompressionBuffer(pComp, idx, pStr);
        }

        return idx.index;
    }


    tt_int32 queueProcessZone(PacketCompressor* pComp, const QueueDataZone* pBuf)
    {
        auto& zone = pBuf->zone;

        DataPacketZone packet;
        packet.type = DataStreamType::Zone;
        packet.stackDepth = static_cast<tt_uint8>(pBuf->stackDepth);
        packet.threadID = pBuf->threadID;
        packet.start = toRelativeTicks(pComp->pCtx, zone.start);
        packet.end = toRelativeTicks(pComp->pCtx, zone.end);
        packet.strIdxFile = GetStringId(pComp, zone.sourceInfo.pFile_);
        packet.strIdxFunction = GetStringId(pComp, zone.sourceInfo.pFunction_);
        packet.strIdxFmt = GetStringId(pComp, zone.pFmtStr);
        packet.lineNo = static_cast<tt_uint16>(zone.sourceInfo.line_);
        packet.argDataSize = pBuf->argDataSize;

        const auto dataSize = GetDataSize<std::remove_pointer_t<decltype(pBuf)>>(pBuf->argDataSize);
        flushCompressionBufferIfrequired(pComp, dataSize);

        addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
        if (pBuf->argDataSize) {
            addToCompressionBufferNoFlush(pComp, &pBuf->argData, pBuf->argDataSize);
        }

        return dataSize;
    }

    tt_int32 queueProcessTickInfo(PacketCompressor* pComp, const QueueDataTickInfo* pBuf)
    {
        DataPacketTickInfo packet;
        packet.type = DataStreamType::TickInfo;
        packet.threadID = pBuf->threadID;
        packet.start = pBuf->start;
        packet.end = pBuf->end;
        packet.startNano = pBuf->startNano;
        packet.endNano = pBuf->endNano;

        addToCompressionBuffer(pComp, &packet, sizeof(packet));
        return GetSizeNotArgData<std::remove_pointer_t<decltype(pBuf)>>();
    }

    tt_int32 queueProcessThreadSetName(PacketCompressor* pComp, const QueueDataThreadSetName* pBuf)
    {
        DataPacketThreadSetName packet;
        packet.type = DataStreamType::ThreadSetName;
        packet.threadID = pBuf->threadID;
        packet.strIdxFmt = GetStringId(pComp, pBuf->pFmtStr);
        packet.time = pBuf->time;
        packet.argDataSize = pBuf->argDataSize;

        const auto dataSize = GetDataSize<std::remove_pointer_t<decltype(pBuf)>>(pBuf->argDataSize);
        flushCompressionBufferIfrequired(pComp, dataSize);

        addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
        if (pBuf->argDataSize) {
            addToCompressionBufferNoFlush(pComp, &pBuf->argData, pBuf->argDataSize);
        }

        return dataSize;
    }

    tt_int32 queueProcessCallStack(PacketCompressor* pComp, const QueueDataCallStack* pBuf)
    {
        DataPacketCallStack packet;
        packet.type = DataStreamType::CallStack;

        TELEM_UNUSED(pComp);
        TELEM_UNUSED(pBuf);
        // so think going todo a callstack cache.
        // then send a id for it?
        // one thing is how to i match these callstacks up like what will i do with them.
        // guess it will just be the current threads zone.
        // so the zone would need call stack info?
        // think that makes the most sense.
        // but how to link them?
        // maybe just gets push/poped.
        // but callstack cache should be done in background thread.
        // meaning

        return GetSizeNotArgData<std::remove_pointer_t<decltype(pBuf)>>();
    }

    tt_int32 queueProcessLockSetName(PacketCompressor* pComp, const QueueDataLockSetName* pBuf)
    {
        DataPacketLockSetName packet;
        packet.type = DataStreamType::LockSetName;
        packet.lockHandle = reinterpret_cast<tt_uint64>(pBuf->pLockPtr);
        packet.strIdxFmt = GetStringId(pComp, pBuf->pFmtStr);
        packet.time = pBuf->time;
        packet.argDataSize = pBuf->argDataSize;

        const auto dataSize = GetDataSize<std::remove_pointer_t<decltype(pBuf)>>(pBuf->argDataSize);
        flushCompressionBufferIfrequired(pComp, dataSize);

        addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
        if (pBuf->argDataSize) {
            addToCompressionBufferNoFlush(pComp, &pBuf->argData, pBuf->argDataSize);
        }

        return dataSize;
    }

    tt_int32 queueProcessLockTry(PacketCompressor* pComp, const QueueDataLockTry* pBuf)
    {
        auto& lock = pBuf->lock;

        DataPacketLockTry packet;
        packet.type = DataStreamType::LockTry;
        packet.threadID = pBuf->threadID;
        packet.start = toRelativeTicks(pComp->pCtx, lock.start);
        packet.end = toRelativeTicks(pComp->pCtx, lock.end);
        packet.lockHandle = reinterpret_cast<tt_uint64>(pBuf->pLockPtr);
        packet.strIdxFile = GetStringId(pComp, lock.sourceInfo.pFile_);
        packet.strIdxFunction = GetStringId(pComp, lock.sourceInfo.pFunction_);
        packet.lineNo = static_cast<tt_uint16>(lock.sourceInfo.line_);
        packet.strIdxFmt = GetStringId(pComp, lock.pFmtStr);
        packet.result = lock.result;
        packet.depth = static_cast<tt_uint8>(pBuf->lock.depth);
        packet.argDataSize = pBuf->argDataSize;

        const auto dataSize = GetDataSize<std::remove_pointer_t<decltype(pBuf)>>(pBuf->argDataSize);
        flushCompressionBufferIfrequired(pComp, dataSize);

        addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
        if (pBuf->argDataSize) {
            addToCompressionBufferNoFlush(pComp, &pBuf->argData, pBuf->argDataSize);
        }

        return dataSize;
    }

    tt_int32 queueProcessLockState(PacketCompressor* pComp, const QueueDataLockState* pBuf)
    {
        DataPacketLockState packet;
        packet.type = DataStreamType::LockState;
        packet.threadID = pBuf->threadID;
        packet.state = pBuf->state;
        packet.time = pBuf->time;
        packet.lockHandle = reinterpret_cast<tt_uint64>(pBuf->pLockPtr);
        packet.lineNo = static_cast<tt_uint16>(pBuf->sourceInfo.line_);
        packet.strIdxFunction = GetStringId(pComp, pBuf->sourceInfo.pFunction_);
        packet.strIdxFile = GetStringId(pComp, pBuf->sourceInfo.pFile_);
        packet.strIdxFmt = GetStringId(pComp, pBuf->pFmtStr);
        packet.argDataSize = pBuf->argDataSize;

        const auto dataSize = GetDataSize<std::remove_pointer_t<decltype(pBuf)>>(pBuf->argDataSize);
        flushCompressionBufferIfrequired(pComp, dataSize);

        addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
        if (pBuf->argDataSize) {
            addToCompressionBufferNoFlush(pComp, &pBuf->argData, pBuf->argDataSize);
        }

        return dataSize;
    }

    tt_int32 queueProcessLockCount(PacketCompressor* pComp, const QueueDataLockCount* pBuf)
    {
        DataPacketLockCount packet;
        packet.type = DataStreamType::LockCount;
        packet.threadID = pBuf->threadID;
        packet.count = pBuf->count;
        packet.time = pBuf->time;
        packet.lockHandle = reinterpret_cast<tt_uint64>(pBuf->pLockPtr);
        packet.lineNo = static_cast<tt_uint16>(pBuf->sourceInfo.line_);
        packet.strIdxFunction = GetStringId(pComp, pBuf->sourceInfo.pFunction_);
        packet.strIdxFile = GetStringId(pComp, pBuf->sourceInfo.pFile_);

#if X_DEBUG
        // Should not have any args.
        if (pBuf->argDataSize) {
            ::DebugBreak();
        }
#endif // !TELEM_DEBUG

        addToCompressionBuffer(pComp, &packet, sizeof(packet));

        return GetSizeWithoutArgData<std::remove_pointer_t<decltype(pBuf)>>();
    }

    tt_int32 queueProcessMemAlloc(PacketCompressor* pComp, const QueueDataMemAlloc* pBuf)
    {
        DataPacketMemAlloc packet;
        packet.type = DataStreamType::MemAlloc;
        packet.threadID = pBuf->threadID;
        packet.size = pBuf->size;
        packet.time = pBuf->time;
        packet.ptr = reinterpret_cast<tt_uint64>(pBuf->pPtr);
        packet.lineNo = static_cast<tt_uint16>(pBuf->sourceInfo.line_);
        packet.strIdxFunction = GetStringId(pComp, pBuf->sourceInfo.pFunction_);
        packet.strIdxFile = GetStringId(pComp, pBuf->sourceInfo.pFile_);
        packet.strIdxFmt = GetStringId(pComp, pBuf->pFmtStr);
        packet.argDataSize = pBuf->argDataSize;

        const auto dataSize = GetDataSize<std::remove_pointer_t<decltype(pBuf)>>(pBuf->argDataSize);
        flushCompressionBufferIfrequired(pComp, dataSize);

        addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
        if (pBuf->argDataSize) {
            addToCompressionBufferNoFlush(pComp, &pBuf->argData, pBuf->argDataSize);
        }

        return dataSize;
    }

    tt_int32 queueProcessMemFree(PacketCompressor* pComp, const QueueDataMemFree* pBuf)
    {
        DataPacketMemFree packet;
        packet.type = DataStreamType::MemFree;
        packet.threadID = pBuf->threadID;
        packet.time = pBuf->time;
        packet.ptr = reinterpret_cast<tt_uint64>(pBuf->pPtr);
        packet.lineNo = static_cast<tt_uint16>(pBuf->sourceInfo.line_);
        packet.strIdxFunction = GetStringId(pComp, pBuf->sourceInfo.pFunction_);
        packet.strIdxFile = GetStringId(pComp, pBuf->sourceInfo.pFile_);

#if X_DEBUG
        // Should not have any args.
        if (pBuf->argDataSize) {
            ::DebugBreak();
        }
#endif // !TELEM_DEBUG

        addToCompressionBuffer(pComp, &packet, sizeof(packet));
        return GetSizeWithoutArgData<std::remove_pointer_t<decltype(pBuf)>>();
    }

    tt_int32 queueProcessMessage(PacketCompressor* pComp, const QueueDataMessage* pBuf)
    {
        DataPacketMessage packet;
        packet.type = DataStreamType::Message;
        packet.time = pBuf->time;
        packet.strIdxFmt = GetStringId(pComp, pBuf->pFmtStr);
        packet.logType = pBuf->logType;
        packet.argDataSize = pBuf->argDataSize;

        const auto dataSize = GetDataSize<std::remove_pointer_t<decltype(pBuf)>>(pBuf->argDataSize);
        flushCompressionBufferIfrequired(pComp, dataSize);

        addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
        if (pBuf->argDataSize) {
            addToCompressionBufferNoFlush(pComp, &pBuf->argData, pBuf->argDataSize);
        }

        return dataSize;
    }


    DWORD __stdcall WorkerThread(LPVOID pParam)
    {
        setThreadName(getThreadID(), "Telemetry");

        auto* pCtx = reinterpret_cast<TraceContext*>(pParam);

        ttSetThreadName(contextToHandle(pCtx), getThreadID(), "Telem - Worker");

        tt_uint8 stringTableBuf[STRING_TABLE_BUF_SIZE];
        tt_uint8 packetBuff[MAX_PACKET_SIZE];

        SocketBuffer buffer = {
            packetBuff,
            PACKET_HDR_SIZE,
            sizeof(packetBuff),
        };

        PacketCompressor comp;
        comp.pCtx = pCtx;
        comp.pBuffer = &buffer;
        comp.strTable = CreateStringTable(stringTableBuf, sizeof(stringTableBuf));

#if !PACKET_COMPRESSION
        {
            // pre fill the header.
            auto* pDataHeader = reinterpret_cast<DataStreamHdr*>(buffer.pPacketBuffer);
            pDataHeader->dataSize = 0;
            pDataHeader->type = PacketType::DataStream;
        }
#endif // !PACKET_COMPRESSION

        static_assert(sizeof(comp) + BACKGROUND_THREAD_STACK_SIZE_BASE >= BACKGROUND_THREAD_STACK_SIZE,
            "Thread stack is to small");

        for (;;)
        {
            ::SetEvent(pCtx->hSignalIdle_);

            if (pCtx->shutDownFlag) {
                break;
            }

            const bool alertable = false;
            DWORD result = WaitForSingleObjectEx(pCtx->hSignal_, INFINITE, alertable);
            if (result != WAIT_OBJECT_0) {
                // rip.
                break;
            }

            //ttZone(contextToHandle(pCtx), "Process buffer");

            auto start = gSysTimer.GetTicks();

            // process the bufffer.
            auto tickBuf = pCtx->tickBuffers[pCtx->activeTickBufIdx ^ 1];

            auto size = tickBuf.bufOffset;

            // if we are past the end it means we filled the buffer.
            // did nto actually write past the end.
            if (size > pCtx->tickBufCapacity) {
                size = pCtx->tickBufCapacity;
            }

            const auto* pBegin = tickBuf.pTickBuf;
            const auto* pEnd = pBegin + size;
            const auto* pBuf = pBegin;

            if (size == 0) {
                ::DebugBreak();
            }

            tt_int32 num = 0;

            // process the packets.
            while (pBuf < pEnd)
            {
                ++num;

                auto type = *reinterpret_cast<const QueueDataType*>(pBuf);

                switch (type)
                {
                    case QueueDataType::Zone:
                        pBuf += queueProcessZone(&comp, reinterpret_cast<const QueueDataZone*>(pBuf));
                        break;
                    case QueueDataType::TickInfo:
                        pBuf += queueProcessTickInfo(&comp, reinterpret_cast<const QueueDataTickInfo*>(pBuf));
                        break;
                    case QueueDataType::ThreadSetName:
                        pBuf += queueProcessThreadSetName(&comp, reinterpret_cast<const QueueDataThreadSetName*>(pBuf));
                        break;
                    case QueueDataType::CallStack:
                        pBuf += queueProcessCallStack(&comp, reinterpret_cast<const QueueDataCallStack*>(pBuf));
                        break;
                    case QueueDataType::LockSetName:
                        pBuf += queueProcessLockSetName(&comp, reinterpret_cast<const QueueDataLockSetName*>(pBuf));
                        break;
                    case QueueDataType::LockTry:
                        pBuf += queueProcessLockTry(&comp, reinterpret_cast<const QueueDataLockTry*>(pBuf));
                        break;
                    case QueueDataType::LockState:
                        pBuf += queueProcessLockState(&comp, reinterpret_cast<const QueueDataLockState*>(pBuf));
                        break;
                    case QueueDataType::LockCount:
                        pBuf += queueProcessLockCount(&comp, reinterpret_cast<const QueueDataLockCount*>(pBuf));
                        break;
                    case QueueDataType::MemAlloc:
                        pBuf += queueProcessMemAlloc(&comp, reinterpret_cast<const QueueDataMemAlloc*>(pBuf));
                        break;
                    case QueueDataType::MemFree:
                        pBuf += queueProcessMemFree(&comp, reinterpret_cast<const QueueDataMemFree*>(pBuf));
                        break;
                    case QueueDataType::Message:
                        pBuf += queueProcessMessage(&comp, reinterpret_cast<const QueueDataMessage*>(pBuf));
                        break;

                    default:
#if X_DEBUG
                        ::DebugBreak();
                        break;
#else
                        TELEM_NO_SWITCH_DEFAULT; // jump table me baby!
#endif
                }
            }

            pCtx->totalEvents += num;

            if (pBuf > pEnd) {
                ::DebugBreak();
            }

            // flush anything left over to the socket.
            flushCompressionBuffer(&comp);
            flushPacketBuffer(pCtx, &buffer);

            auto end = gSysTimer.GetTicks();
            auto ellapsed = end - start;

            writeLog(pCtx, TtLogType::Msg, "processed %iin: %fms", num, gSysTimer.ToMilliSeconds(ellapsed));
        }

        return 0;
    }

} // namespace


// --------------------------------------------------------------------

bool TelemInit(void)
{
    // Just stop it, ok?
    _CrtSetDebugFillThreshold(0);

    platform::WSADATA winsockInfo;

    if (platform::WSAStartup(MAKEWORD(2, 2), &winsockInfo) != 0) {
        return false;
    }

    if (!gSysTimer.StartUp()) {
        return false;
    }

    // want to work out ticks per micro.
    {
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

TtError TelemInitializeContext(TraceContexHandle& out, void* pArena, tt_size bufLen)
{
    out = INVALID_TRACE_CONTEX;

    const auto* pEnd = reinterpret_cast<tt_uint8*>(pArena) + bufLen;

    // need to align upto 64bytes.
    auto* pBuf = AlignTop(pArena, 64);
    const tt_uintptr alignmentSize = reinterpret_cast<tt_uintptr>(pBuf) - reinterpret_cast<tt_uintptr>(pArena);

    bufLen -= alignmentSize;

    // send packets this size?
    constexpr tt_size contexSize = sizeof(TraceContext);
    constexpr tt_size threadDataSize = sizeof(TraceThread) * MAX_ZONE_THREADS;
    constexpr tt_size minBufferSize = 1024 * 10; // 10kb.. enougth?
    constexpr tt_size internalSize = contexSize + threadDataSize;
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
    tt_uint8* pTickBuffer0 = reinterpret_cast<tt_uint8*>(AlignTop(pThreadDataBuf + threadDataSize, 64));
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

    TraceContext* pCtx = new (pBuf) TraceContext();
    pCtx->lastTick = getTicks();
    pCtx->lastTickNano = gSysTimer.GetNano();
    pCtx->isEnabled = true;
    pCtx->socket = INV_SOCKET;
    pCtx->pThreadData = reinterpret_cast<TraceThread*>(pThreadDataBuf);
    pCtx->numThreadData = 0;
    pCtx->ticksPerMicro = gTicksPerMicro;
    pCtx->baseTicks = pCtx->lastTick;
    pCtx->baseNano = pCtx->lastTickNano;

    pCtx->activeTickBufIdx = 0;
    pCtx->tickBuffers[0].pTickBuf = pTickBuffer0;
    pCtx->tickBuffers[0].bufOffset = 0;
    pCtx->tickBuffers[1].pTickBuf = pTickBuffer1;
    pCtx->tickBuffers[1].bufOffset = 0;
    pCtx->tickBufCapacity = static_cast<tt_uint32>(tickBufferSize);

    pCtx->hThread_ = ::CreateThread(nullptr, BACKGROUND_THREAD_STACK_SIZE, WorkerThread, pCtx, 0, &pCtx->threadId_);
    if (!pCtx->hThread_) {
        return TtError::Error;
    }

    // make sure we don't get starved, since the host program might make use of all cores
    if (!SetThreadPriority(pCtx->hThread_, THREAD_PRIORITY_ABOVE_NORMAL)) {
        // not fatal.
    }

    pCtx->hSignal_ = CreateEventW(nullptr, false, false, nullptr);
    if (!pCtx->hSignal_) {
        return TtError::Error;
    }

    pCtx->hSignalIdle_ = CreateEventW(nullptr, false, false, nullptr);
    if (!pCtx->hSignalIdle_) {
        return TtError::Error;
    }

    pCtx->shutDownFlag = 0;
    pCtx->numStalls = 0;
    pCtx->totalEvents = 0;
    
    pCtx->logFunc = defaultLogFunction;
    pCtx->pUserData = nullptr;

    out = contextToHandle(pCtx);
    return TtError::Ok;
}

void TelemShutdownContext(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);

    // wait for background thread to idle
    // then flag shutodwn and wake up.
    if (::WaitForSingleObject(pCtx->hSignalIdle_, INFINITE) == WAIT_FAILED) {
        // rip
    }

    pCtx->shutDownFlag = 1;

    ::SetEvent(pCtx->hSignal_);

    if (::WaitForSingleObject(pCtx->hThread_, INFINITE) == WAIT_FAILED) {
        // rip
        return;
    }

    if (pCtx->hSignal_) {
        ::CloseHandle(pCtx->hSignal_);
    }
    if (pCtx->hSignalIdle_) {
        ::CloseHandle(pCtx->hSignalIdle_);
    }
}

void TelemSetContextLogFunc(TraceContexHandle ctx, LogFunction func, void* pUserData)
{
    auto* pCtx = handleToContext(ctx);
    pCtx->pUserData = pUserData;
    // write this after.
    _WriteBarrier();
    pCtx->logFunc = func;
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

    TELEM_UNUSED(timeoutMS);

    // need to connect to the server :O
    struct platform::addrinfo hints, *servinfo = nullptr;
    zero_object(hints);
    hints.ai_family = AF_UNSPEC; // ipv4/6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = platform::IPPROTO_TCP;

    char portStr[32] = {};
    sprintf(portStr, "%d", serverPort); // TODO: replace

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

    // how big?
    tt_int32 sock_opt = 1024 * 16;
    res = platform::setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sock_opt, sizeof(sock_opt));
    if (res != 0) {
        writeLog(pCtx, TtLogType::Error, "Failed to set sndbuf on socket. Error: %d", platform::WSAGetLastError());
        return TtError::Error;
    }
   
    pCtx->socket = connectSocket;

    ConnectionRequestHdr cr;
    zero_object(cr);
    cr.type = PacketType::ConnectionRequest;
    cr.clientVer.major = TELEM_VERSION_MAJOR;
    cr.clientVer.minor = TELEM_VERSION_MINOR;
    cr.clientVer.patch = TELEM_VERSION_PATCH;
    cr.clientVer.build = TELEM_VERSION_BUILD;

    LPWSTR pCmdLine = GetCommandLineW();

    const auto appNameLen = static_cast<tt_uint32>(strlen(pAppName));
    const auto buildInfoLen = static_cast<tt_uint32>(strlen(pBuildInfo));
    const auto cmdLineLen = static_cast<tt_uint32>(wcslen(pCmdLine));

    if (appNameLen > MAX_STRING_LEN) {
        return TtError::InvalidParam;
    }
    if (buildInfoLen > MAX_STRING_LEN) {
        return TtError::InvalidParam;
    }
    if (cmdLineLen > MAX_CMDLINE_LEN) {
        return TtError::InvalidParam;
    }

    char cmdLine[MAX_CMDLINE_LEN] = {};
    tt_int32 cmdLenUtf8;
    if (!Convert(pCmdLine, cmdLineLen, cmdLine, sizeof(cmdLine), cmdLenUtf8)) {
        return TtError::Error;
    }

    cr.appNameLen = static_cast<tt_uint16>(appNameLen);
    cr.buildInfoLen = static_cast<tt_uint16>(buildInfoLen);
    cr.cmdLineLen = static_cast<tt_uint16>(cmdLenUtf8);
    cr.ticksPerMicro = gTicksPerMicro;
    cr.ticksPerMs = gTicksPerMicro * 1000;
    cr.dataSize = sizeof(cr) + cr.appNameLen + cr.buildInfoLen + cr.cmdLineLen;

    sendDataToServer(pCtx, &cr, sizeof(cr));

    sendDataToServer(pCtx, pAppName, appNameLen);
    sendDataToServer(pCtx, pBuildInfo, buildInfoLen);
    sendDataToServer(pCtx, cmdLine, cmdLenUtf8);

    // wait for a response O.O
    char recvbuf[MAX_PACKET_SIZE];
    int recvbuflen = sizeof(recvbuf);

    // TODO: support timeout.
    if (!readPacket(pCtx, recvbuf, recvbuflen)) {
        return TtError::Error;
    }

    if (!handleConnectionResponse(pCtx, reinterpret_cast<tt_uint8*>(recvbuf), static_cast<tt_size>(recvbuflen))) {
        return TtError::HandeshakeFail;
    }

    return TtError::Ok;
}

bool TelemClose(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);

#if 0 // TODO: needed?
    int res = platform::shutdown(pCtx->socket, SD_BOTH);
    if (res == SOCKET_ERROR) {
        writeLog(pCtx, LogType::Error, "shutdown failed with error: %d", platform::WSAGetLastError());
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

    auto curTick = getTicks();
    auto curTimeNano = gSysTimer.GetNano();
    auto sinceLastNano = curTimeNano - pCtx->lastTickNano;

    // we queue a tick even if don't flush.
    queueTickInfo(pCtx, pCtx->lastTick, curTick, pCtx->lastTickNano, curTimeNano);

    // I update these even if we don't flip, so the tick timing is correct.
    // if want the time to accumlate need another field to track it.
    pCtx->lastTick = curTick;
    pCtx->lastTickNano = curTimeNano;

    // if we are been called at a very high freq don't bother sending unless needed.
    if (sinceLastNano < 100'000'000) {
        // if the buffer is half full send it!
        auto halfBufferCap = pCtx->tickBufCapacity / 2;
        auto bufSize = getActiveTickBufferSize(pCtx);

        if (bufSize < halfBufferCap) {
            return;
        }
    }

    flipBuffer(pCtx, false, true);
    return;
}

void TelemFlush(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);

    // TODO: maybe not return
    if (!pCtx->isEnabled) {
        return;
    }

    flipBuffer(pCtx, false, true);
}

void TelemUpdateSymbolData(TraceContexHandle ctx)
{
    TELEM_UNUSED(ctx);

    // So do i actually need todo this?
    // can't i just listen for loader events.
    // will see...
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

void TelemSetThreadName(TraceContexHandle ctx, tt_uint32 threadID, const char* pName)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    queueThreadSetName(pCtx, threadID, pName);
}

bool TelemGetCallStack(TraceContexHandle ctx, TtCallStack& stackOut)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return true;
    }

    zero_object(stackOut.frames);

    RtlCaptureStackBackTrace(0, TtCallStack::MAX_FRAMES, stackOut.frames, 0);
    return true;
}

void TelemSendCallStack(TraceContexHandle ctx, const TtCallStack* pStack)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    if (pStack) {
        queueCallStack(pCtx, *pStack);
        return;
    }

    TtCallStack stack;
    TelemGetCallStack(ctx, stack);
    queueCallStack(pCtx, stack);
    return;
}

// ----------- Zones -----------

void TelemEnter(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = getThreadData(pCtx);

    if (!pCtx->isEnabled || !pThreadData) {
        return;
    }

    auto depth = pThreadData->stackDepth;
    ++pThreadData->stackDepth;

    auto& scopeData = pThreadData->zones[depth];
    scopeData.zone.start = getTicks();
    scopeData.zone.pFmtStr = pFmtString;
    scopeData.zone.sourceInfo = sourceInfo;

    // for this arg data it would be nicer to be like a linera array.
    // basically a fixed sized buffer we just offset from.
    // can do that later tho.
    if (numArgs)
    {
        va_list l;
        va_start(l, numArgs);

        scopeData.argDataSize = BuildArgData(scopeData.argData, pFmtString, numArgs, l);

        va_end(l);
    }
    else
    {
        scopeData.argDataSize = 0;
    }
}

void TelemLeave(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | !pThreadData) {
        return;
    }

    auto depth = --pThreadData->stackDepth;

    auto& scopeData = pThreadData->zones[depth];
    scopeData.zone.end = getTicks();

    queueZone(pCtx, pThreadData, scopeData);
}

void TelemEnterEx(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pFmtString, tt_int32 numArgs, ...)
{
    // This is a copy of TelemEnter since can't pick up the va_args later unless we always pass them.
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = getThreadData(pCtx);

    if (!pCtx->isEnabled || !pThreadData) {
        return;
    }

    // only do the copy when enabled?
    matchIdOut = minMicroSec;

    auto depth = pThreadData->stackDepth;
    ++pThreadData->stackDepth;

    auto& scopeData = pThreadData->zones[depth];
    scopeData.zone.start = getTicks();
    scopeData.zone.pFmtStr = pFmtString;
    scopeData.zone.sourceInfo = sourceInfo;

    if (numArgs)
    {
        va_list l;
        va_start(l, numArgs);

        scopeData.argDataSize = BuildArgData(scopeData.argData, pFmtString, numArgs, l);

        va_end(l);
    }
    else
    {
        scopeData.argDataSize = 0;
    }
}


void TelemLeaveEx(TraceContexHandle ctx, tt_uint64 matchId)
{
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | !pThreadData) {
        return;
    }

    auto depth = --pThreadData->stackDepth;

    auto& scopeData = pThreadData->zones[depth];
    scopeData.zone.end = getTicks();

    // work out if we send it.
    auto minMicroSec = matchId;
    auto elpased = scopeData.zone.end - scopeData.zone.start;
    auto elapsedNano = ticksToNano(pCtx, elpased);

    if (elapsedNano > minMicroSec * 1000) {
        return;
    }

    queueZone(pCtx, pThreadData, scopeData);
}


// ----------- Lock stuff -----------

void TelemSetLockName(TraceContexHandle ctx, const void* pPtr, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataLockSetName data;
    data.type = QueueDataType::LockSetName;
    data.time = getRelativeTicks(pCtx);
    data.pLockPtr = pPtr;
    data.pFmtStr = pFmtString;

    if (!numArgs)
    {
        constexpr auto size = GetSizeWithoutArgData<decltype(data)>();
        data.argDataSize = 0;
        addToTickBuffer(pCtx, &data, size);
    }
    else
    {
        va_list l;
        va_start(l, numArgs);

        auto argDataSize = BuildArgData(data.argData, pFmtString, numArgs, l);
        data.argDataSize = static_cast<tt_int8>(argDataSize & 0xFF);

        va_end(l);

        addToTickBuffer(pCtx, &data, GetDataSize<decltype(data)>(argDataSize));
    }
}

void TelemTryLock(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const void* pPtr, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    auto* pThreadData = getThreadData(pCtx);
    if (!pThreadData) {
        return;
    }

    auto* pLock = addLock(pThreadData, pPtr);
    if (!pLock) {
        return;
    }

    auto& lock = pLock->lock;
    lock.start = getTicks();
    lock.pFmtStr = pFmtString;
    lock.sourceInfo = sourceInfo;

    if (numArgs)
    {
        va_list l;
        va_start(l, numArgs);

        pLock->argDataSize = BuildArgData(pLock->argData, pFmtString, numArgs, l);

        va_end(l);
    }
    else
    {
        pLock->argDataSize = 0;
    }
}

void TelemTryLockEx(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, 
    const void* pPtr, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    auto* pThreadData = getThreadData(pCtx);
    if (!pThreadData) {
        return;
    }

    auto* pLock = addLock(pThreadData, pPtr);
    if (!pLock) {
        return;
    }

    matchIdOut = minMicroSec;

    auto& lock = pLock->lock;
    lock.start = getTicks();
    lock.pFmtStr = pFmtString;
    lock.sourceInfo = sourceInfo;

    if (numArgs)
    {
        va_list l;
        va_start(l, numArgs);

        pLock->argDataSize = BuildArgData(pLock->argData, pFmtString, numArgs, l);

        va_end(l);
    }
    else
    {
        pLock->argDataSize = 0;
    }
}

void TelemEndTryLock(TraceContexHandle ctx, const void* pPtr, TtLockResult result)
{
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | !pThreadData) {
        return;
    }

    auto* pLock = getLockAndClearSlot(pThreadData, pPtr);
    if (!pLock) {
        return;
    }

    auto& lock = pLock->lock;
    lock.end = getTicks();
    lock.result = result;
    lock.depth = static_cast<decltype(lock.depth)>(pThreadData->stackDepth);

    queueLockTry(pCtx, pThreadData, pPtr, pLock);
}

void TelemEndTryLockEx(TraceContexHandle ctx, tt_uint64 matchId, const void* pPtr, TtLockResult result)
{
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | !pThreadData) {
        return;
    }

    auto* pLock = getLockAndClearSlot(pThreadData, pPtr);
    if (!pLock) {
        return;
    }

    auto& lock = pLock->lock;
    lock.end = getTicks();
    lock.result = result;
    lock.depth = static_cast<decltype(lock.depth)>(pThreadData->stackDepth);

    // work out if we send it.
    auto minMicroSec = matchId;
    auto elpased = lock.end - lock.start;
    auto elapsedNano = ticksToNano(pCtx, elpased);

    if (elapsedNano > minMicroSec * 1000) {
        return;
    }
    
    queueLockTry(pCtx, pThreadData, pPtr, pLock);
}

void TelemSetLockState(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const void* pPtr, TtLockState state)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    queueLockState(pCtx, sourceInfo, pPtr, state);
}

void TelemSignalLockCount(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const void* pPtr, tt_int32 count)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    queueLockCount(pCtx, sourceInfo, pPtr, count);
}

// ----------- Allocation stuff -----------

void TelemAlloc(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, void* pPtr, tt_size allocSize, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataMemAlloc data;
    data.type = QueueDataType::MemAlloc;
    data.time = getRelativeTicks(pCtx);
    data.pPtr = pPtr;
    data.size = static_cast<tt_uint32>(allocSize);
    data.threadID = getThreadID();
    data.sourceInfo = sourceInfo;
    data.pFmtStr = pFmtString;

    if (!numArgs)
    {
        constexpr auto size = GetSizeWithoutArgData<decltype(data)>();
        data.argDataSize = 0;
        addToTickBuffer(pCtx, &data, size);
    }
    else
    {
        va_list l;
        va_start(l, numArgs);

        auto argDataSize = BuildArgData(data.argData, pFmtString, numArgs, l);
        data.argDataSize = static_cast<tt_int8>(argDataSize & 0xFF);

        va_end(l);

        addToTickBuffer(pCtx, &data, GetDataSize<decltype(data)>(argDataSize));
    }
}

void TelemFree(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, void* pPtr)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataMemFree data;
    data.type = QueueDataType::MemFree;
    data.argDataSize = 0;
    data.time = getRelativeTicks(pCtx);
    data.pPtr = pPtr;
    data.threadID = getThreadID();
    data.sourceInfo = sourceInfo;

    addToTickBuffer(pCtx, &data, GetSizeWithoutArgData<decltype(data)>());
}

// ----------- Plot stuff -----------

void TelemPlotF32(TraceContexHandle ctx, TtPlotType type, float value, const char* pFmtString, tt_int32 numArgs, ...)
{
    TELEM_UNUSED(ctx);
    TELEM_UNUSED(type);
    TELEM_UNUSED(value);
    TELEM_UNUSED(pFmtString);
    TELEM_UNUSED(numArgs);
}

void TelemPlotF64(TraceContexHandle ctx, TtPlotType type, double value, const char* pFmtString, tt_int32 numArgs, ...)
{
    TELEM_UNUSED(ctx);
    TELEM_UNUSED(type);
    TELEM_UNUSED(value);
    TELEM_UNUSED(pFmtString);
    TELEM_UNUSED(numArgs);
}

void TelemPlotI32(TraceContexHandle ctx, TtPlotType type, tt_int32 value, const char* pFmtString, tt_int32 numArgs, ...)
{
    TELEM_UNUSED(ctx);
    TELEM_UNUSED(type);
    TELEM_UNUSED(value);
    TELEM_UNUSED(pFmtString);
    TELEM_UNUSED(numArgs);
}

void TelemPlotI64(TraceContexHandle ctx, TtPlotType type, tt_int64 value, const char* pFmtString, tt_int32 numArgs, ...)
{
    TELEM_UNUSED(ctx);
    TELEM_UNUSED(type);
    TELEM_UNUSED(value);
    TELEM_UNUSED(pFmtString);
    TELEM_UNUSED(numArgs);
}

void TelemPlotU32(TraceContexHandle ctx, TtPlotType type, tt_uint32 value, const char* pFmtString, tt_int32 numArgs, ...)
{
    TELEM_UNUSED(ctx);
    TELEM_UNUSED(type);
    TELEM_UNUSED(value);
    TELEM_UNUSED(pFmtString);
    TELEM_UNUSED(numArgs);
}

void TelemPlotU64(TraceContexHandle ctx, TtPlotType type, tt_uint64 value, const char* pFmtString, tt_int32 numArgs, ...)
{
    TELEM_UNUSED(ctx);
    TELEM_UNUSED(type);
    TELEM_UNUSED(value);
    TELEM_UNUSED(pFmtString);
    TELEM_UNUSED(numArgs);
}

// ----------- Message Stuff -----------


void TelemMessage(TraceContexHandle ctx, TtLogType::Enum type, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataMessage data;
    data.type = QueueDataType::Message;
    data.time = getRelativeTicks(pCtx);
    data.pFmtStr = pFmtString;
    data.logType = type;

    if (!numArgs)
    {
        constexpr auto size = GetSizeWithoutArgData<decltype(data)>();
        data.argDataSize = 0;
        addToTickBuffer(pCtx, &data, size);
    }
    else
    {
        va_list l;
        va_start(l, numArgs);

        auto argDataSize = BuildArgData(data.argData, pFmtString, numArgs, l);
        data.argDataSize = static_cast<tt_int8>(argDataSize & 0xFF);

        va_end(l);

        addToTickBuffer(pCtx, &data, GetDataSize<decltype(data)>(argDataSize));
    }
}
