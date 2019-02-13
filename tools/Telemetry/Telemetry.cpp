#include "stdafx.h"
#include "TelemetryLib.h"

// TODO: get rid of
#include <cstdio>
#include <stddef.h> // for offsetof rip

#include <intrin.h>

#include <../TelemetryCommon/TelemetryCommonLib.h>

X_LINK_LIB("engine_TelemetryCommonLib.lib");

X_DISABLE_WARNING(4324) //  structure was padded due to alignment specifier


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

    SysTimer gSysTimer;
    tt_uint64 gTicksPerMicro;

    X_INLINE tt_uint64 TicksToMicro(tt_uint64 tsc) 
    {
        return (tsc / gTicksPerMicro);
    }

    X_INLINE tt_uint32 getThreadID(void)
    {
        // TODO: remove function call.
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

    bool Convert(const wchar_t* pSrc, tt_size srcSize,
        char* output, tt_size outputBytes, tt_size& lengthOut)
    {
        lengthOut = 0;
        if (!outputBytes) {
            return false;
        }

        const tt_int32 srcLenChars = static_cast<tt_int32>(srcSize);
        const tt_int32 bytesWritten = WideCharToMultiByte(
            CP_UTF8,
            0,
            pSrc,
            srcLenChars,
            output,
            static_cast<tt_int32>(outputBytes),
            nullptr,
            nullptr
        );

        if (bytesWritten == 0) {
            // error
            return false;
        }

        lengthOut = bytesWritten;
        return true;
    }


    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

    struct TraceLock
    {
        tt_uint64 start;
        tt_uint64 end;

        TtLockResult result;
        const char* pDescription;
    };

    struct TraceLocks
    {
        const void* pLockPtr[MAX_LOCKS_HELD_PER_THREAD];
        TraceLock locks[MAX_LOCKS_HELD_PER_THREAD];
    };

    struct TraceZone
    {
        tt_uint64 start;
        tt_uint64 end;

        const char* pZoneName;
        TtSourceInfo sourceInfo;
    };

    // some data for each thread!
    X_ALIGNED_SYMBOL(struct TraceThread, 64)
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

        TraceZone zones[MAX_ZONE_DEPTH];
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
    #define X86_PAD(bytes) tt_uint8 X_TELEMETRY_UNIQUE_NAME(__pad)[bytes];
#endif // X_64

    // This is padded to 64bit to make placing TraceThread data on it's own boundy more simple.
    X_ALIGNED_SYMBOL(struct TraceContext, 64)
    {
        bool isEnabled;
        bool _pad[3];

        tt_uint64 lastTick;

        TraceThread* pThreadData;
        tt_int32 numThreadData;

        StringTable strTable;

        tt_uint8 _lanePad0[14];
        X86_PAD(12)

        // -- Cace lane boundry --

        volatile tt_int32 activeTickBufIdx;
        TickBuffer tickBuffers[2];

        tt_int32 tickBufCapacity;

        tt_uint8 _lanePad1[20];
        X86_PAD(20)

        // -- Cace lane boundry --

        // used by background thread
        tt_uint8* pPacketBuffer;
        tt_int32 packetBufSize;
        tt_int32 packetBufCapacity;

        DWORD threadId_;
        HANDLE hThread_;
        HANDLE hSignal_;
        HANDLE hSignalIdle_;
        volatile tt_int32 shutDownFlag;
        platform::SOCKET socket;

        X86_PAD(28)

        // -- Cace lane boundry --

        CriticalSection cs_;
        tt_int32 numStalls;
        tt_int32 totalEvents;
    };

//    constexpr size_t size0 = sizeof(TraceContext);
//    constexpr size_t size1 = X_OFFSETOF(TraceContext, activeTickBufIdx);
//    constexpr size_t size2 = X_OFFSETOF(TraceContext, cs_);

    static_assert(X_OFFSETOF(TraceContext, activeTickBufIdx) == 64, "Cold fields not on firstcache lane");
    static_assert(X_OFFSETOF(TraceContext, pPacketBuffer) == 128, "Cold fields not on next cache lane");
    static_assert(X_OFFSETOF(TraceContext, cs_) == 192, "cache lane boundry changed");
    static_assert(sizeof(TraceContext) == 256, "Size changed");

    tt_int32 getActiveTickBufferSize(TraceContext* pCtx)
    {
        auto& buf = pCtx->tickBuffers[pCtx->activeTickBufIdx];
        return buf.bufOffset;
    }


    TraceLock* addLock(TraceThread* pThread, const void* pLockPtr)
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


    TraceLock* getLock(TraceThread* pThread, const void* pLockPtr)
    {
        TraceLock* pLock = nullptr;

        auto& locks = pThread->locks;
        for (tt_int32 i = 0; i < MAX_LOCKS_HELD_PER_THREAD; i++)
        {
            if (locks.pLockPtr[i] == pLockPtr)
            {
                pLock = &locks.locks[i];
                // TODO: is breaking faster here?
                // I'm guessing not as long as this loop is unrolled.
                // break;
            }
        }

        return pLock;
    }

    X_NO_INLINE TraceThread* addThreadData(TraceContext* pCtx)
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

    X_INLINE TraceThread* getThreadData(TraceContext* pCtx)
    {
        auto* pThreadData = gThreadData;
        if (!pThreadData) {
            return addThreadData(pCtx);
        }

        return pThreadData;
    }

    bool handleConnectionResponse(tt_uint8* pData, tt_size len)
    {
        X_UNUSED(len);

        auto* pPacketHdr = reinterpret_cast<const PacketBase*>(pData);
        switch (pPacketHdr->type)
        {
            case PacketType::ConnectionRequestAccepted:
                // don't care about response currently.
                return true;
            case PacketType::ConnectionRequestRejected: {
                auto* pConRej = reinterpret_cast<const ConnectionRequestRejectedHdr*>(pData);
                auto* pStrData = reinterpret_cast<const char*>(pConRej + 1);
                printf("Connection rejected: %.*s\n", pConRej->reasonLen, pStrData);

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

    void flushDataPacketBuffer(TraceContext* pCtx)
    {
        if (pCtx->packetBufSize == sizeof(DataStreamHdr)) {
            return;
        }

        // patch the length
        auto* pHdr = reinterpret_cast<DataStreamHdr*>(pCtx->pPacketBuffer);
        pHdr->dataSize = static_cast<tt_uint16>(pCtx->packetBufSize);

        // flush to socket.
        sendDataToServer(pCtx, pCtx->pPacketBuffer, pCtx->packetBufSize);
        pCtx->packetBufSize = sizeof(DataStreamHdr);
    }

    void addToDataPacketBuffer(TraceContext* pCtx, const void* pData, tt_size len)
    {
        // even fit in a packet?
        if (len > pCtx->packetBufCapacity - sizeof(DataStreamHdr)) {
            ::DebugBreak();
        }

        // can we fit this data?
        const auto space = pCtx->packetBufCapacity - pCtx->packetBufSize;
        if (space < static_cast<tt_int32>(len)) {
            flushDataPacketBuffer(pCtx);
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
        tt_size packetLen = sizeof(DataPacketStringTableAdd) + strLen;

        tt_uint8 strDataBuf[sizeof(DataPacketStringTableAdd) + MAX_STRING_LEN];
        auto* pHeader = reinterpret_cast<DataPacketStringTableAdd*>(strDataBuf);
        pHeader->type = DataStreamType::StringTableAdd;
        pHeader->length = static_cast<tt_uint16>(strLen);

        memcpy(strDataBuf + sizeof(DataPacketStringTableAdd), pStr, strLen);

        addToDataPacketBuffer(pCtx, &strDataBuf, packetLen);
    }

    enum class QueueDataType
    {
        Zone,
        TickInfo,
        ThreadSetName,
        LockSetName,
        LockTry,
        LockState,
        LockCount,
        MemAlloc,
        MemFree
    };

    struct QueueDataBase
    {
        QueueDataType type;
    };

    
    X_ALIGNED_SYMBOL(struct QueueDataTickInfo, 64) : public QueueDataBase
    {
        TtthreadId threadID;
        tt_uint64 ticks;
        tt_uint64 timeMicro;
    };

    X_ALIGNED_SYMBOL(struct QueueDataThreadSetName, 64) : public QueueDataBase
    {
        TtthreadId threadID;
        const char* pName;
    };

    X_ALIGNED_SYMBOL(struct QueueDataZone, 64) : public QueueDataBase
    {
        tt_int8 stackDepth;
        TtthreadId threadID;
        
        TraceZone zone;
    };

    X_ALIGNED_SYMBOL(struct QueueDataLockSetName, 64) : public QueueDataBase
    {
        const void* pLockPtr;
        const char* pLockName;
    };

    X_ALIGNED_SYMBOL(struct QueueDataLockTry, 64) : public QueueDataBase
    {
        TtthreadId threadID;
        TraceLock lock;
        const void* pLockPtr;
    };

    X_ALIGNED_SYMBOL(struct QueueDataLockState, 64) : public QueueDataBase
    {
        TtthreadId threadID;
        TtLockState state;
        const void* pLockPtr;
    };

    X_ALIGNED_SYMBOL(struct QueueDataLockCount, 64) : public QueueDataBase
    {
        tt_uint16 count;
        TtthreadId threadID;
        const void* pLockPtr;
    };

    X_ALIGNED_SYMBOL(struct QueueDataMemAlloc, 64) : public QueueDataBase
    {
        TtthreadId threadID;
        const void* pPtr;
        tt_uint32 size;
    };

    X_ALIGNED_SYMBOL(struct QueueDataMemFree, 64) : public QueueDataBase
    {
        TtthreadId threadID;
        const void* pPtr;
    };

    constexpr size_t size0 = sizeof(QueueDataThreadSetName);
    constexpr size_t size1 = sizeof(QueueDataZone);
    constexpr size_t size2 = sizeof(QueueDataLockSetName);
    constexpr size_t size3 = sizeof(QueueDataLockTry);
    constexpr size_t size4 = sizeof(QueueDataLockState);
    constexpr size_t size5 = sizeof(QueueDataLockCount);
    constexpr size_t size6 = sizeof(QueueDataMemAlloc);
    constexpr size_t size7 = sizeof(QueueDataMemFree);

    static_assert(64 == sizeof(QueueDataThreadSetName));
    static_assert(64 == sizeof(QueueDataTickInfo));
    static_assert(64 == sizeof(QueueDataZone));
    static_assert(64 == sizeof(QueueDataLockSetName));
    static_assert(64 == sizeof(QueueDataLockTry));
    static_assert(64 == sizeof(QueueDataLockState));
    static_assert(64 == sizeof(QueueDataLockCount));
    static_assert(64 == sizeof(QueueDataMemAlloc));
    static_assert(64 == sizeof(QueueDataMemFree));

    void flipBufferInternal(TraceContext* pCtx, bool force)
    {
        auto bufSize = getActiveTickBufferSize(pCtx);
        if (bufSize == 0) {
            return;
        }

        // don't flip if not half full and force is false.
        auto halfBufferCap = pCtx->tickBufCapacity / 2;
        if (force && bufSize < halfBufferCap) {
            return;
        }

        // wait for the background thread to finish process that last buffer.
        // TODO: maybe come up with a fast path for when we don't need to wait.
        // check if the signal has a userspace atomic it checks before waiting.
        DWORD result = WaitForSingleObjectEx(pCtx->hSignalIdle_, INFINITE, false);
        if (result != WAIT_OBJECT_0) {
            ::DebugBreak();
            return;
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


    X_NO_INLINE void tickBufferFull(TraceContext* pCtx)
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
#if X_DEBUG
        if (offset + size > pCtx->tickBufCapacity) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        tickBufferFull(pCtx);
        addToTickBuffer(pCtx, pPtr, size);
    }

    void queueTickInfo(TraceContext* pCtx)
    {
        QueueDataTickInfo data;
        data.type = QueueDataType::TickInfo;
        data.threadID = getThreadID();
        data.ticks = getTicks();
        data.timeMicro = gSysTimer.GetMicro();

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    void queueThreadSetName(TraceContext* pCtx, tt_uint32 threadID, const char* pName)
    {
        QueueDataThreadSetName data;
        data.type = QueueDataType::ThreadSetName;
        data.threadID = threadID;
        data.pName = pName;

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    void queueZone(TraceContext* pCtx, TraceThread* pThread, TraceZone& zone)
    {
        QueueDataZone data;
        data.type = QueueDataType::Zone;
        data.stackDepth = static_cast<tt_uint8>(pThread->stackDepth);
        data.threadID = pThread->id;
        data.zone = zone;

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    void queueLockSetName(TraceContext* pCtx, const void* pPtr, const char* pLockName)
    {
        QueueDataLockSetName data;
        data.type = QueueDataType::LockSetName;
        data.pLockPtr = pPtr;
        data.pLockName = pLockName;

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    void queueLock(TraceContext* pCtx, TraceThread* pThread, TraceLock* pLock)
    {
        QueueDataLockTry data;
        data.type = QueueDataType::LockTry;
        data.threadID = pThread->id;
        data.lock = *pLock;
        data.pLockPtr = nullptr;

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    void queueLockState(TraceContext* pCtx, const void* pPtr, TtLockState state)
    {
        QueueDataLockState data;
        data.type = QueueDataType::LockState;
        data.pLockPtr = pPtr;
        data.state = state;
        data.threadID = getThreadID();

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    void queueLockCount(TraceContext* pCtx, const void* pPtr, tt_int32 count)
    {
        QueueDataLockCount data;
        data.type = QueueDataType::LockCount;
        data.pLockPtr = pPtr;
        data.count = static_cast<tt_uint16>(count);
        data.threadID = getThreadID();

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    void queueMemAlloc(TraceContext* pCtx, const void* pPtr, tt_size size)
    {
        QueueDataMemAlloc data;
        data.type = QueueDataType::MemAlloc;
        data.pPtr = pPtr;
        data.size = static_cast<tt_uint32>(size);;
        data.threadID = getThreadID();

        addToTickBuffer(pCtx, &data, sizeof(data));
    }

    void queueMemFree(TraceContext* pCtx, const void* pPtr)
    {
        QueueDataMemFree data;
        data.type = QueueDataType::MemFree;
        data.pPtr = pPtr;
        data.threadID = getThreadID();

        addToTickBuffer(pCtx, &data, sizeof(data));
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

    void queueProcessZone(TraceContext* pCtx, const QueueDataZone* pBuf)
    {
        auto& zone = pBuf->zone;

        DataPacketZone packet;
        packet.type = DataStreamType::Zone;
        packet.stackDepth = static_cast<tt_uint8>(pBuf->stackDepth);
        packet.threadID = pBuf->threadID;
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

        addToDataPacketBuffer(pCtx, &packet, sizeof(packet));
    }

    void queueProcessTickInfo(TraceContext* pCtx, const QueueDataTickInfo* pBuf)
    {
        DataPacketTickInfo packet;
        packet.type = DataStreamType::TickInfo;
        packet.threadID = pBuf->threadID;
        packet.ticks = pBuf->ticks;
        packet.timeMicro = pBuf->timeMicro;

        addToDataPacketBuffer(pCtx, &packet, sizeof(packet));
    }

    void queueProcessThreadSetName(TraceContext* pCtx, const QueueDataThreadSetName* pBuf)
    {
        DataPacketThreadSetName packet;
        packet.type = DataStreamType::ThreadSetName;
        packet.threadID = pBuf->threadID;
        packet.strIdxName = StringTableGetIndex(pCtx->strTable, pBuf->pName);

        if (packet.strIdxName.inserted) {
            writeStringPacket(pCtx, pBuf->pName);
        }

        addToDataPacketBuffer(pCtx, &packet, sizeof(packet));
    }

    void queueProcessLockSetName(TraceContext* pCtx, const QueueDataLockSetName* pBuf)
    {
        DataPacketLockSetName packet;
        packet.type = DataStreamType::LockSetName;
        packet.lockHandle = reinterpret_cast<tt_uint64>(pBuf->pLockPtr);
        packet.strIdxName = StringTableGetIndex(pCtx->strTable, pBuf->pLockName);

        if (packet.strIdxName.inserted) {
            writeStringPacket(pCtx, pBuf->pLockName);
        }

        addToDataPacketBuffer(pCtx, &packet, sizeof(packet));
    }

    void queueProcessLockTry(TraceContext* pCtx, const QueueDataLockTry* pBuf)
    {
        auto& lock = pBuf->lock;

        DataPacketLockTry packet;
        packet.type = DataStreamType::LockTry;
        packet.threadID = pBuf->threadID;
        packet.start = lock.start;
        packet.end = lock.end;
        packet.lockHandle = reinterpret_cast<tt_uint64>(pBuf->pLockPtr);
        packet.strIdxDescrption = StringTableGetIndex(pCtx->strTable, lock.pDescription);

        if (packet.strIdxDescrption.inserted) {
            writeStringPacket(pCtx, lock.pDescription);
        }

        addToDataPacketBuffer(pCtx, &packet, sizeof(packet));
    }

    void queueProcessLockState(TraceContext* pCtx, const QueueDataLockState* pBuf)
    {
        DataPacketLockState packet;
        packet.type = DataStreamType::LockTry;
        packet.threadID = pBuf->threadID;
        packet.state = pBuf->state;
        packet.lockHandle = reinterpret_cast<tt_uint64>(pBuf->pLockPtr);

        addToDataPacketBuffer(pCtx, &packet, sizeof(packet));
    }

    void queueProcessLockCount(TraceContext* pCtx, const QueueDataLockCount* pBuf)
    {
        DataPacketLockCount packet;
        packet.type = DataStreamType::LockCount;
        packet.threadID = pBuf->threadID;
        packet.count = pBuf->count;
        packet.lockHandle = reinterpret_cast<tt_uint64>(pBuf->pLockPtr);

        addToDataPacketBuffer(pCtx, &packet, sizeof(packet));
    }

    void queueProcessMemAlloc(TraceContext* pCtx, const QueueDataMemAlloc* pBuf)
    {
        DataPacketMemAlloc packet;
        packet.type = DataStreamType::MemAlloc;
        packet.threadID = pBuf->threadID;
        packet.size = pBuf->size;
        packet.ptr = reinterpret_cast<tt_uint64>(pBuf->pPtr);

        addToDataPacketBuffer(pCtx, &packet, sizeof(packet));
    }

    void queueProcessMemFree(TraceContext* pCtx, const QueueDataMemFree* pBuf)
    {
        DataPacketMemFree packet;
        packet.type = DataStreamType::MemFree;
        packet.threadID = pBuf->threadID;
        packet.ptr = reinterpret_cast<tt_uint64>(pBuf->pPtr);

        addToDataPacketBuffer(pCtx, &packet, sizeof(packet));
    }


    DWORD __stdcall WorkerThread(LPVOID pParam)
    {
        setThreadName(getThreadID(), "Telemetry");

        auto* pCtx = reinterpret_cast<TraceContext*>(pParam);

        const bool alertable = false;

        for (;;)
        {
            ::SetEvent(pCtx->hSignalIdle_);

            if (pCtx->shutDownFlag) {
                break;
            }

            DWORD result = WaitForSingleObjectEx(pCtx->hSignal_, INFINITE, alertable);
            if (result != WAIT_OBJECT_0) {
                // rip.
                break;
            }

            auto start = gSysTimer.GetMicro();

            // process the bufffer.
            auto tickBuf = pCtx->tickBuffers[pCtx->activeTickBufIdx ^ 1];

            auto size = tickBuf.bufOffset;

            // if we are past the end it means we filled the buffer.
            // did nto actually write past the end.
            if (size > pCtx->tickBufCapacity) {
                size = pCtx->tickBufCapacity;
            }

            const auto num = size / 64;
            pCtx->totalEvents += num;

            const auto* pBegin = tickBuf.pTickBuf;
            const auto* pEnd = pBegin + size;
            const auto* pBuf = pBegin;

            if (size == 0) {
                ::DebugBreak();
            }

            // process the packets.
            while (pBuf < pEnd)
            {
                auto type = *reinterpret_cast<const QueueDataType*>(pBuf);

                switch (type)
                {
                    case QueueDataType::Zone:
                        queueProcessZone(pCtx, reinterpret_cast<const QueueDataZone*>(pBuf));
                        pBuf += sizeof(QueueDataZone);
                        break;
                    case QueueDataType::TickInfo:
                        queueProcessTickInfo(pCtx, reinterpret_cast<const QueueDataTickInfo*>(pBuf));
                        pBuf += sizeof(QueueDataTickInfo);
                        break;
                    case QueueDataType::ThreadSetName:
                        queueProcessThreadSetName(pCtx, reinterpret_cast<const QueueDataThreadSetName*>(pBuf));
                        pBuf += sizeof(QueueDataThreadSetName);
                        break;
                    case QueueDataType::LockSetName:
                        queueProcessLockSetName(pCtx, reinterpret_cast<const QueueDataLockSetName*>(pBuf));
                        pBuf += sizeof(QueueDataLockSetName);
                        break;
                    case QueueDataType::LockTry:
                        queueProcessLockTry(pCtx, reinterpret_cast<const QueueDataLockTry*>(pBuf));
                        pBuf += sizeof(QueueDataLockTry);
                        break;
                    case QueueDataType::LockState:
                        queueProcessLockState(pCtx, reinterpret_cast<const QueueDataLockState*>(pBuf));
                        pBuf += sizeof(QueueDataLockState);
                        break;
                    case QueueDataType::LockCount:
                        queueProcessLockCount(pCtx, reinterpret_cast<const QueueDataLockCount*>(pBuf));
                        pBuf += sizeof(QueueDataLockCount);
                        break;
                    case QueueDataType::MemAlloc:
                        queueProcessMemAlloc(pCtx, reinterpret_cast<const QueueDataMemAlloc*>(pBuf));
                        pBuf += sizeof(QueueDataMemAlloc);
                        break;
                    case QueueDataType::MemFree:
                        queueProcessMemFree(pCtx, reinterpret_cast<const QueueDataMemFree*>(pBuf));
                        pBuf += sizeof(QueueDataMemFree);
                        break;

                    default:
#if X_DEBUG
                        ::DebugBreak();
                        break;
#else
                        X_NO_SWITCH_DEFAULT; // jump table me baby!
#endif
                }
            }

            if (pBuf > pEnd) {
                ::DebugBreak();
            }

            // flush anything left over to the socket.
            flushDataPacketBuffer(pCtx);

            auto end = gSysTimer.GetMicro();
            auto ellapsed = end - start;

            printf("processed in: %lld\n", ellapsed);
        }

        return 0;
    }

} // namespace


// --------------------------------------------------------------------

bool TelemInit(void)
{
#if 1 // TODO: temp
    // freopen("CONOUT$", "w", stdout);
#endif

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

    TraceContext* pCtx = new (pBuf) TraceContext();
    pCtx->lastTick = gSysTimer.GetMicro();
    pCtx->isEnabled = true;
    pCtx->socket = INV_SOCKET;
    pCtx->pThreadData = reinterpret_cast<TraceThread*>(pThreadDataBuf);
    pCtx->numThreadData = 0;
    pCtx->strTable = CreateStringTable(pStrTableBuf, strTableSize);
    pCtx->pPacketBuffer = pPacketBuffer;
    pCtx->packetBufSize = sizeof(DataStreamHdr);
    pCtx->packetBufCapacity = packetBufSize;

    // pre fill the header.
    auto* pDataHeader = reinterpret_cast<DataStreamHdr*>(pCtx->pPacketBuffer);
    pDataHeader->dataSize = 0;
    pDataHeader->type = PacketType::DataStream;

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

    // how big?
    tt_int32 sock_opt = 1024 * 32;
    res = platform::setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sock_opt, sizeof(sock_opt));
    if (res != 0) {
        printf("Failed to set sndbuf on socket. Error: %d\n", platform::WSAGetLastError());
        return TtError::Error;
    }

    auto* pCtx = handleToContext(ctx);
    pCtx->socket = connectSocket;

    ConnectionRequestHdr cr;
    zero_object(cr);
    cr.type = PacketType::ConnectionRequest;
    cr.clientVer.major = X_TELEMETRY_VERSION_MAJOR;
    cr.clientVer.minor = X_TELEMETRY_VERSION_MINOR;
    cr.clientVer.patch = X_TELEMETRY_VERSION_PATCH;
    cr.clientVer.build = X_TELEMETRY_VERSION_BUILD;

    LPWSTR pCmdLine = GetCommandLineW();

    const auto appNameLen = strlen(pAppName);
    const auto buildInfoLen = strlen(pBuildInfo);
    const auto cmdLineLen = wcslen(pCmdLine);

    if (appNameLen > MAX_APP_NAME_LEN) {
        return TtError::InvalidParam;
    }
    if (buildInfoLen > MAX_BUILD_INFO_LEN) {
        return TtError::InvalidParam;
    }
    if (cmdLineLen > MAX_CMDLINE_LEN) {
        return TtError::InvalidParam;
    }

    char cmdLine[MAX_CMDLINE_LEN] = {};
    tt_size cmdLenUtf8;
    if (!Convert(pCmdLine, cmdLineLen, cmdLine, sizeof(cmdLine), cmdLenUtf8)) {
        return TtError::Error;
    }

    cr.appNameLen = static_cast<tt_uint16>(appNameLen);
    cr.buildInfoLen = static_cast<tt_uint16>(buildInfoLen);
    cr.cmdLineLen = static_cast<tt_uint16>(cmdLenUtf8);
    cr.dataSize = sizeof(cr) + cr.appNameLen + cr.buildInfoLen + cr.cmdLineLen;

    sendDataToServer(pCtx, &cr, sizeof(cr));

    sendDataToServer(pCtx, pAppName, appNameLen);
    sendDataToServer(pCtx, pBuildInfo, buildInfoLen);
    sendDataToServer(pCtx, cmdLine, cmdLenUtf8);

    // wait for a response O.O
    char recvbuf[MAX_PACKET_SIZE];
    int recvbuflen = sizeof(recvbuf);

    // TODO: support timeout.
    if (!readPacket(connectSocket, recvbuf, recvbuflen)) {
        return TtError::Error;
    }

    if (!handleConnectionResponse(reinterpret_cast<tt_uint8*>(recvbuf), static_cast<tt_size>(recvbuflen))) {
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

    queueTickInfo(pCtx);

    // if we are been called at a very high freq don't bother sending unless needed.
    if (sinceLast < 1000000) {
        // if the buffer is half full send it!
        auto halfBufferCap = pCtx->tickBufCapacity / 2;
        auto bufSize = getActiveTickBufferSize(pCtx);

        if (bufSize < halfBufferCap) {
            return;
        }
    }

    pCtx->lastTick = curTime;

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
    X_UNUSED(ctx);

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

// ----------- Zones -----------

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
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | !pThreadData) {
        return;
    }

    auto depth = --pThreadData->stackDepth;

    auto& zone = pThreadData->zones[depth];
    zone.end = getTicks();

    queueZone(pCtx, pThreadData, zone);
}

void TelemEnterEx(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pZoneName)
{
    // we can just copy it?
    matchIdOut = minMicroSec;
    TelemEnter(ctx, sourceInfo, pZoneName);
}


void TelemLeaveEx(TraceContexHandle ctx, tt_uint64 matchId)
{
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | !pThreadData) {
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


// ----------- Lock stuff -----------

void TelemSetLockName(TraceContexHandle ctx, const void* pPtr, const char* pLockName)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    queueLockSetName(pCtx, pPtr, pLockName);
}

void TelemTryLock(TraceContexHandle ctx, const void* pPtr, const char* pDescription)
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

    pLock->start = getTicks();
    pLock->pDescription = pDescription;
}

void TelemTryLockEx(TraceContexHandle ctx, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const void* pPtr, const char* pDescription)
{
    matchIdOut = minMicroSec;
    TelemTryLock(ctx, pPtr, pDescription);
}

void TelemEndTryLock(TraceContexHandle ctx, const void* pPtr, TtLockResult result)
{
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | !pThreadData) {
        return;
    }

    auto* pLock = getLock(pThreadData, pPtr);
    if (!pLock) {
        return;
    }

    pLock->end = getTicks();
    pLock->result = result;

    queueLock(pCtx, pThreadData, pLock);
}

void TelemEndTryLockEx(TraceContexHandle ctx, tt_uint64 matchId, const void* pPtr, TtLockResult result)
{
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | !pThreadData) {
        return;
    }

    auto* pLock = getLock(pThreadData, pPtr);
    if (!pLock) {
        return;
    }

    pLock->end = getTicks();
    pLock->result = result;

    // work out if we send it.
    auto minMicroSec = matchId;
    auto elpased = pLock->end - pLock->start;
    auto elapsedMicro = TicksToMicro(elpased);

    if (elapsedMicro > minMicroSec) {
        return;
    }
    
    queueLock(pCtx, pThreadData, pLock);
}

void TelemSetLockState(TraceContexHandle ctx, const void* pPtr, TtLockState state)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    queueLockState(pCtx, pPtr, state);
}

void TelemSignalLockCount(TraceContexHandle ctx, const void* pPtr, tt_int32 count)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    queueLockCount(pCtx, pPtr, count);
}

// ----------- Allocation stuff -----------

void TelemAlloc(TraceContexHandle ctx, void* pPtr, tt_size size)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    queueMemAlloc(pCtx, pPtr, size);
}

void TelemFree(TraceContexHandle ctx, void* pPtr)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    queueMemFree(pCtx, pPtr);
}