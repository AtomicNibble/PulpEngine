#include "pch.h"
#include "TelemetryLib.h"

// TODO: get rid of
#include <memory>
#include <cstdio>
#include <stddef.h> // for offsetof rip

#include <intrin.h>

#include <psapi.h> // EnumProcessModules

TELEM_DISABLE_WARNING(4091)
#include <DbgHelp.h>
TELEM_ENABLE_WARNING(4091)

#include <../../3rdparty/source/lz4-1.8.3/lz4_lib.h>

TELEM_LINK_LIB("engine_TelemetryCommonLib.lib");
TELEM_LINK_LIB("dbghelp.lib");

TELEM_DISABLE_WARNING(4324) //  structure was padded due to alignment specifier

#define PACKET_COMPRESSION 1
#define RUNTIME_ZONE_WRITES 1
#define RUNTIME_ZONE_PDB_SEND 1

TELEM_INTRINSIC(_BitScanReverse)
TELEM_INTRINSIC(_BitScanForward)

#if X_64
TELEM_INTRINSIC(_BitScanReverse64)
TELEM_INTRINSIC(_BitScanForward64)
#endif // !X_64


namespace
{
    static const char* TELEM_ZONE_SOURCE_FILE = "Telem.cpp";

    typedef unsigned long(__stdcall *RtlWalkFrameChainFunc)(void**, unsigned long, unsigned long);
    RtlWalkFrameChainFunc pRtlWalkFrameChain = nullptr;

    namespace Hash
    {
        typedef tt_uint32 Fnv1aVal;

        static const tt_uint32 FNV_32_PRIME = 16777619u;
        static const tt_uint32 FNV1_32_INIT = 2166136261u;


        TELEM_INLINE Fnv1aVal Fnv1aHash(const void* key, size_t length, Fnv1aVal seed)
        {
            Fnv1aVal hash = seed;
            auto* s = reinterpret_cast<const uint8_t*>(key);

            for (tt_uint32 i = 0; i < length; ++i) {
                hash ^= (tt_uint32)*s++;
                hash *= FNV_32_PRIME;
            }

            return hash;
        }

        TELEM_INLINE Fnv1aVal Fnv1aHash(const void* key, size_t length)
        {
            return Fnv1aHash(key, length, FNV1_32_INIT);
        }

    } // namespace Hash

    namespace Io
    {
        #define TELEM_TAG(a, b, c, d) (tt_uint32)((d << 24) | (c << 16) | (b << 8) | a)

        static const char* TRACE_FILE_EXTENSION = "trace";
        static const tt_uint32 TRACR_FOURCC = TELEM_TAG('t', 'r', 'a', 'c');
        static const tt_uint8 TRACE_VERSION = 1;

        // write a header.
        struct TelemFileHdr
        {
            TelemFileHdr() {
                zero_this(this);
            }

            tt_uint32 fourCC;
            tt_uint8 version;
            tt_uint8 _pad[3];

            bool isValid(void) const {
                return fourCC == TRACR_FOURCC;
            }
        };

        static_assert(sizeof(TelemFileHdr) == 8, "Size changed");

        TtFileHandle fileOpen(void* pUserData, const char* pPath)
        {
            TELEM_UNUSED(pUserData);

            DWORD access = FILE_WRITE_DATA;
            DWORD share = 0;
            DWORD dispo = CREATE_ALWAYS;
            DWORD flags = 0;

            HANDLE hHandle = ::CreateFileA(pPath, access, share, NULL, dispo, flags, NULL);
            if (hHandle == INVALID_HANDLE_VALUE) {
                return TELEM_INVALID_HANDLE;
            }

            return reinterpret_cast<TtFileHandle>(hHandle);
        }

        void fileClose(void* pUserData, TtFileHandle handle)
        {
            TELEM_UNUSED(pUserData);
            ::CloseHandle(reinterpret_cast<HANDLE>(handle));
        }

        tt_int32 fileWrite(void* pUserData, TtFileHandle handle, const void* pData, tt_int32 length)
        {
            TELEM_UNUSED(pUserData);

            // i need to support 64bit files.
            // but don't care about offset.
            DWORD bytesWritten = 0;

            if (!::WriteFile(reinterpret_cast<HANDLE>(handle), pData, length, &bytesWritten, nullptr)) {
                return -1;
            }

            return static_cast<tt_int32>(bytesWritten);
        }

    } // namespace Io

    namespace PE
    {

        struct PdbInfo
        {
            DWORD     Signature;
            BYTE      Guid[16];
            DWORD     Age;
            char      PdbFileName[1];
        };

        const WORD DOS_HEADER_MAGIC = IMAGE_DOS_SIGNATURE;
        const WORD OPTIONAL_HEADER_MAGIC = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
        const WORD OPTIONAL_HEADER_MAGIC64 = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
        const DWORD NT_SIGNATURE = IMAGE_NT_SIGNATURE;

        const WORD IMAGE_FILE_I386 = IMAGE_FILE_MACHINE_I386;
        const WORD IMAGE_FILE_IA64 = IMAGE_FILE_MACHINE_IA64;
        const WORD IMAGE_FILE_AMD64 = IMAGE_FILE_MACHINE_AMD64;

        const size_t NUM_DATA_DIRECTORYS = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

        struct MyNtHeader;
        struct MyOptionalHeader;

#if X_64

        struct MyDosHeader : public IMAGE_DOS_HEADER
        {
            bool isValid(void) const {
                return this->e_magic == DOS_HEADER_MAGIC;
            }

            MyNtHeader* GetNtHeader(void) const {
                return reinterpret_cast<MyNtHeader*>(e_lfanew + reinterpret_cast<size_t>(this));
            }
        };

        struct MyNtHeader : public IMAGE_NT_HEADERS64
        {
            bool isValid(void) const {
                return this->Signature == NT_SIGNATURE;
            }
            MyOptionalHeader* GetOptionalHeader(void) {
                return reinterpret_cast<MyOptionalHeader*>(&OptionalHeader);
            }
        };

        struct MyOptionalHeader : public IMAGE_OPTIONAL_HEADER64
        {
            bool isValid(void) const {
                return this->Magic == OPTIONAL_HEADER_MAGIC64;
            }
        };

#else

        struct MyDosHeader : public IMAGE_DOS_HEADER
        {
            bool isValid(void) const {
                return this->e_magic == DOS_HEADER_MAGIC;
            }
            MyNtHeader* GetNtHeader(void) const {
                return reinterpret_cast<MyNtHeader*>(e_lfanew + reinterpret_cast<LONG>(this));
            }
        };

        struct MyNtHeader : public IMAGE_NT_HEADERS32
        {
            bool isValid(void) const {
                return this->Signature == NT_SIGNATURE;
            }
            MyOptionalHeader* GetOptionalHeader(void) {
                return reinterpret_cast<MyOptionalHeader*>(&OptionalHeader);
            }
        };

        struct MyOptionalHeader : public IMAGE_OPTIONAL_HEADER32
        {
            bool isValid(void) const {
                return this->Magic == OPTIONAL_HEADER_MAGIC;
            }

            bool is64Bit(void) const {
                return this->Magic == OPTIONAL_HEADER_MAGIC64;
            }

        };
#endif // X_64

        struct PDBSig
        {
#if X_DEBUG
            PDBSig() {
                zero_this(this);
            }
#endif

            int32_t     imageSize;

            BYTE        guid[16];
            DWORD       age;
            char        pdbFileName[256]; // Do i care for paths?
        };


        struct PDBInfo
        {
            static constexpr tt_int32 MAX_MODULES_PDB = 256;

            using HModArr = HMODULE[MAX_MODULES_PDB];
            using PDBSigArr = PDBSig[MAX_MODULES_PDB];

        public:
            PDBInfo() :
                num(0)
            {
                memset(mods, 0, sizeof(mods));
            }

            tt_int32 num;
            HModArr mods;
            PDBSigArr sigs;
        };

        bool getPDBSig(HMODULE hMod, PDBSig& sig)
        {
            uintptr_t base = reinterpret_cast<uintptr_t>(hMod);

            const auto* pHeader = reinterpret_cast<MyDosHeader*>(base);
            if (!pHeader->isValid()) {
                return false;
            }

            auto* pNt = pHeader->GetNtHeader();
            if (!pNt->isValid()) {
                return false;
            }

            auto* pOtional = pNt->GetOptionalHeader();
            if (!pOtional->isValid()) {
                return false;
            }

            IMAGE_DATA_DIRECTORY* pDir = &pOtional->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

            // Convert that data to the right type.
            IMAGE_DEBUG_DIRECTORY* pDbgDir = reinterpret_cast<IMAGE_DEBUG_DIRECTORY*>(base + pDir->VirtualAddress);

            // Check to see that the data has the right type
            if (pDbgDir->Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
                return false;
            }

            PdbInfo* pPdbInfo = reinterpret_cast<PdbInfo*>(base + pDbgDir->AddressOfRawData);
            if (memcmp(&pPdbInfo->Signature, "RSDS", 4) != 0) {
                return false;
            }

            sig.imageSize = pOtional->SizeOfImage;
            sig.age = pPdbInfo->Age;
            memcpy(sig.guid, pPdbInfo->Guid, sizeof(pPdbInfo->Guid));
            strcpy(sig.pdbFileName, pPdbInfo->PdbFileName);
            return true;
        }

        // so i want to get the PE sigs for every loaded module?
        // i guess so.
        // guess should just do it at startup?

        bool haveInfoForPDB(const PDBInfo& info, HMODULE hMod)
        {
            for (tt_int32 i = 0; i < info.num; i++)
            {
                if (info.mods[i] == hMod) {
                    return true;
                }
            }

            return false;
        }

        bool updatePDBInfo(PDBInfo& info)
        {
            auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ::GetCurrentProcessId());
            if (!hProcess) {
                return true;
            }

            // Get a list of all the modules in this process.
            // we use bigger buffer than MAX_MODULES_PDB since not all modules have PDB info.
            HMODULE hMods[1024];
            DWORD cbNeeded;

            bool ok = EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded);
            if (ok)
            {
                const tt_size num = (cbNeeded / sizeof(HMODULE));

                for (tt_size i = 0; i < num; i++)
                {
                    auto hMod = hMods[i];

                    // do we have info for this mod?
                    if (haveInfoForPDB(info, hMod)) {
                        continue;
                    }

                    if (info.num == PDBInfo::MAX_MODULES_PDB) {
                        break;
                    }

                    PDBSig sig;
                    if (getPDBSig(hMod, sig)) {
                        info.mods[info.num] = hMod;
                        info.sigs[info.num] = sig;
                        ++info.num;
                    }
                }
            }

            CloseHandle(hProcess);
            return ok;
        }

        PDBInfo pdbInfo;

    } // namespace PE

    namespace bitUtil
    {
        constexpr tt_uint32 NO_BIT_SET = 255;

        template<size_t N>
        struct Implementation
        {
        };

        template<>
        struct Implementation<8u>
        {
            template<typename T>
            static inline tt_uint32 ScanBitsForward(T value)
            {
                static_assert(sizeof(T) == 8, "sizeof(T) is not 8 bytes.");

                DWORD index;
#if X_64
                const unsigned char result = _BitScanForward64(&index, static_cast<tt_uint64>(value));
                if (result == 0) {
                    return NO_BIT_SET;
                }

                return index;
#else
                if (value) {
                    value = (value ^ (value - 1)) >> 1; // Set v's trailing 0s to 1s and zero rest
                    for (index = 0; value; index++) {
                        value >>= 1;
                    }
                    return index;
                }
                return NO_BIT_SET;
#endif // !X_64
            }
        };

        template<>
        struct Implementation<4u>
        {
            template<typename T>
            static inline tt_uint32 ScanBitsForward(T value)
            {
                static_assert(sizeof(T) == 4, "sizeof(T) is not 4 bytes.");

                DWORD index;
                const unsigned char result = _BitScanForward(&index, static_cast<tt_uint32>(value));
                if (result == 0) {
                    return NO_BIT_SET;
                }

                return index;
            }
        };


        template<typename T>
        inline tt_uint32 ScanBitsForward(T value)
        {
            return Implementation<sizeof(T)>::ScanBitsForward(value);
        }

    } // namespace bitUtil

    namespace strUtil
    {

#if X_64

    static size_t strlen(const char* str)
    {
        __m128i zero = _mm_set1_epi8(0);
        __m128i* s_aligned = (__m128i*)(((uint64_t)str) & -0x10L);
        uint8_t misbits = (uint8_t)(((uint64_t)str) & 0xf);
        __m128i s16cs = _mm_load_si128(s_aligned);
        __m128i bytemask = _mm_cmpeq_epi8(s16cs, zero);
        int bitmask = _mm_movemask_epi8(bytemask);
        bitmask = (bitmask >> misbits) << misbits;

        // Alternative: use TEST instead of BSF, then BSF at end (only). Much better on older CPUs
        // TEST has latency 1, while BSF has 3!
        while (bitmask == 0) {
            s16cs = _mm_load_si128(++s_aligned);
            bytemask = _mm_cmpeq_epi8(s16cs, zero);
            bitmask = _mm_movemask_epi8(bytemask);
        }

        return (((const char*)s_aligned) - str) + (size_t)bitUtil::ScanBitsForward(bitmask);
    }

#else

    static size_t strlen(const char* str)
    {
        __m128i zero = _mm_set1_epi8(0);
        __m128i* s_aligned = (__m128i*)(((long)str) & -0x10L);
        uint8_t misbits = (uint8_t)(((long)str) & 0xf);
        __m128i s16cs = _mm_load_si128(s_aligned);
        __m128i bytemask = _mm_cmpeq_epi8(s16cs, zero);
        int bitmask = _mm_movemask_epi8(bytemask);
        bitmask = (bitmask >> misbits) << misbits;

        while (bitmask == 0) {
            s16cs = _mm_load_si128(++s_aligned);
            bytemask = _mm_cmpeq_epi8(s16cs, zero);
            bitmask = _mm_movemask_epi8(bytemask);
        }

        return (((const char*)s_aligned) - str) + bitUtil::ScanBitsForward(bitmask);
    }

#endif

    } // namespace StrUtil

    template<class T>
    TELEM_INLINE constexpr const T& Max(const T& x, const T& y)
    {
        return (x > y) ? x : y;
    }

    template<class T>
    TELEM_INLINE constexpr const T& Min(const T& x, const T& y)
    {
        return (x < y) ? x : y;
    }

    tt_int64 GetSystemTimeAsUnixTime(void)
    {
        // January 1, 1970 (start of Unix epoch) in "ticks"
        const tt_int64 UNIX_TIME_START = 0x019DB1DED53E8000;
        // tick is 100ns
        const tt_int64 TICKS_PER_SECOND = 10000000;

        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        LARGE_INTEGER li;
        li.LowPart = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;

        return (li.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;
    }


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
#if X_64
        auto val = __readgsqword(0x30) + 0x48;
        return *reinterpret_cast<const tt_uint32*>(val);
#else
        return ::GetCurrentThreadId();
#endif
    }

    TELEM_INLINE tt_uint64 getTicks(void)
    {
        return __rdtsc();
    }

    template<typename T>
    TELEM_INLINE constexpr bool IsPowerOfTwo(T x)
    {
        return (x & (x - 1)) == 0;
    }

    const platform::SOCKET INV_SOCKET = (platform::SOCKET)(~0);

    struct ArgData
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
        TtLockResult::Enum result;
        tt_uint16 depth;
        TtSourceInfo sourceInfo;

        static_assert(std::numeric_limits<decltype(depth)>::max() >= MAX_ZONE_DEPTH, "Can't store max zone depth");
    };

    TELEM_PACK_POP;

    struct TraceLockBuilder
    {
        TraceLock lock;
        tt_int32 argDataSize;
        ArgData argData;
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

    struct TraceZoneBuilder
    {
        TraceZone zone;
        tt_int32 argDataSize;
    };

    // some data for each thread!
    TELEM_ALIGNED_SYMBOL(struct TraceThread, 64)
    {
        TraceThread() {
            id = getThreadID();
            stackDepth = 0;

#if X_DEBUG
            zero_object(zones);
            zero_object(locks);
#endif // X_DEBUG
        }

        TtthreadId id;
        tt_int32 stackDepth;

        TraceZoneBuilder zones[MAX_ZONE_DEPTH];
        ArgData argData[MAX_ZONE_DEPTH];
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

    struct RuntimeZone
    {
        tt_uint64 start;
        tt_uint64 end;
    };

    TELEM_ALIGNED_SYMBOL(struct WriteZones, 64)
    {
        static constexpr int32_t NUM_ZONES = 16;
        static_assert(IsPowerOfTwo(NUM_ZONES), "Must be pow2");

        WriteZones() {
            num = 0;
            zero_object(zones);
        }

        int32_t num;
        RuntimeZone zones[NUM_ZONES];
    };

    TELEM_ALIGNED_SYMBOL(struct PDBZones, 64)
    {
        static constexpr int32_t NUM_ZONES = 8;
        static_assert(IsPowerOfTwo(NUM_ZONES), "Must be pow2");

        PDBZones() {
            num = 0;
            zero_object(zones);
        }

        int32_t num;
        RuntimeZone zones[NUM_ZONES];
    };

    // This is padded to 64bit to make placing TraceThread data on it's own boundy more simple.
    TELEM_ALIGNED_SYMBOL(struct TraceContext, 64)
    {
        tt_uint64 lastTick;
        tt_uint64 lastTickNano;

        TraceThread* pThreadData; // this is only read from when adding new thread.
        tt_int32 numThreadData;
        tt_int32 _pad0;

        tt_uint64 ticksPerMicro;
        tt_uint64 baseTicks;
        tt_uint64 baseNano;

        bool isEnabled;
        tt_uint8 flags;
        tt_uint8 connFlags;
        bool _pad1[1];

        tt_int32 numPDBSync;

        X86_PAD(8)

        // -- Cace lane boundry --
        // This lane is very hot and should have no writes expect tickerBuf flip.

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
        WriteZones* pWriteZones;

        platform::SOCKET socket;
        TtFileHandle fileHandle;

        X86_PAD(32)

        // -- Cace lane boundry --

        CriticalSection cs_;

        // -- Cace lane boundry --

        tt_uint64 lastFlipTick;

        tt_int32 numStalls;
        tt_int32 totalEvents;

        LogFunction logFunc;
        void* pUserData;

        tt_uint8 _lanePad2[32];

        X86_PAD(24)

        // -- Cace lane boundry --

        FileOpenFunc pFileOpen;
        FileCloseFunc pFileClose;
        FileWriteFunc pFileWrite;
        void* pIOUserData;

        PDBZones* pPDBZones;
    };

//    constexpr size_t size0 = sizeof(TraceContext);
//    constexpr size_t size1 = TELEM_OFFSETOF(TraceContext, pFileOpen);
//    constexpr size_t size2 = TELEM_OFFSETOF(TraceContext, pFileOpen);

    static_assert(TELEM_OFFSETOF(TraceContext, activeTickBufIdx) == 64, "Cold fields not on firstcache lane");
    static_assert(TELEM_OFFSETOF(TraceContext, threadId_) == 128, "Cold fields not on next cache lane");
    static_assert(TELEM_OFFSETOF(TraceContext, cs_) == 192, "cache lane boundry changed");
    static_assert(TELEM_OFFSETOF(TraceContext, lastFlipTick) == 256, "cache lane boundry changed");
    static_assert(TELEM_OFFSETOF(TraceContext, pFileOpen) == 320, "cache lane boundry changed");
    static_assert(sizeof(TraceContext) == 384, "Size changed");


    TELEM_INLINE tt_int32 fileWrite(TraceContext* pCtx, const void* pData, tt_int32 length)
    {
        return pCtx->pFileWrite(pCtx->pIOUserData, pCtx->fileHandle, pData, length);
    }

    template<typename T>
    TELEM_INLINE tt_int32 fileWrite(TraceContext* pCtx, const T& obj)
    {
        return fileWrite(pCtx, reinterpret_cast<const void*>(&obj), sizeof(obj));
    }

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

                // breaking here is faster, even tho it's unrolling the loop without break.
                // forcing inline of unrolled code don't help much.
                break;
            }
        }

        return pLock;
    }

    TELEM_NO_INLINE void resolveThreadName(TraceContext* pCtx)
    {
#if _WIN32
        auto id = getThreadID();

        HANDLE handle = OpenThread(
            THREAD_QUERY_LIMITED_INFORMATION,
            FALSE,
            id
        );

        if (handle == NULL) {
            return;
        }

        using pGetThreadDescription = HRESULT (WINAPI *)(HANDLE hThread, PWSTR * ppszThreadDescription);

        HMODULE hMod = ::GetModuleHandleW(L"kernel32.dll");

        auto pFunc = (pGetThreadDescription)::GetProcAddress(hMod, "GetThreadDescription");
        if (pFunc) {
            PWSTR name;
            auto hr = pFunc(handle, &name);
            if (SUCCEEDED(hr)) {
                // make it narrow.
                char buf[MAX_STRING_LEN];

                const int32_t narrowLen = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    name,
                    -1,
                    buf,
                    sizeof(buf),
                    nullptr,
                    nullptr);

                if (narrowLen > 1) {
                    ttSetThreadName(contextToHandle(pCtx), id, "%s", buf);
                }

                LocalFree(name);
            }

            ::CloseHandle(handle);
        }
#else
        TELEM_UNUSED(pCtx);
#endif // _WIN32
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

        resolveThreadName(pCtx);

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
                lastErrorWSA::Description Dsc;
                const auto err = lastErrorWSA::Get();
                writeLog(pCtx, TtLogType::Error, "recv failed with Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
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

    TELEM_NO_INLINE void onFatalWorkerError(TraceContext* pCtx)
    {
        if (pCtx->socket != INV_SOCKET) {
            platform::closesocket(pCtx->socket);
            pCtx->socket = INV_SOCKET;
        }

        if (pCtx->fileHandle != TELEM_INVALID_HANDLE) {
            pCtx->pFileClose(pCtx->pIOUserData, pCtx->fileHandle);
            pCtx->fileHandle = TELEM_INVALID_HANDLE;
        }

        // Also disable telem otherwise we can deadlock.
        pCtx->isEnabled = false;
    }

    void sendDataToServer(TraceContext* pCtx, const void* pData, tt_int32 len)
    {
        if(pCtx->socket == INV_SOCKET && pCtx->fileHandle == TELEM_INVALID_HANDLE) {
            return;
        }

#if X_DEBUG
        if (len > MAX_PACKET_SIZE) {
            ::DebugBreak();
        }
#endif // X_DEBUG

#if RUNTIME_ZONE_WRITES
        RuntimeZone nz;
        nz.start = getTicks();
#endif // RUNTIME_ZONE_WRITES

        // send some data...

        if (pCtx->fileHandle != TELEM_INVALID_HANDLE) {
            // Write to file.
            auto numWrite = fileWrite(pCtx, reinterpret_cast<const char*>(pData), len);
            if (numWrite != len) {
                onFatalWorkerError(pCtx);
                writeLog(pCtx, TtLogType::Error, "File: write failed");
                return;
            }
        }
        else {

            // TODO: none blocking?
            int res = platform::send(pCtx->socket, reinterpret_cast<const char*>(pData), len, 0);
            if (res == SOCKET_ERROR) {
                lastErrorWSA::Description Dsc;
                const auto err = lastErrorWSA::Get();

                onFatalWorkerError(pCtx);
                writeLog(pCtx, TtLogType::Error, "Socket: send failed with Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
                return;
            }
        }

#if RUNTIME_ZONE_WRITES
        nz.end = getTicks();
        auto& zones = *pCtx->pWriteZones;
        if (zones.num < WriteZones::NUM_ZONES) {
            zones.zones[zones.num++] = nz;
        }
        else {
            writeLog(pCtx, TtLogType::Warning, "Data write zone buffer is full");
        }
#endif // RUNTIME_ZONE_WRITES
    }

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
        CallstackCache callstackCache;

        tt_int32 writeBegin;
        tt_int32 writeEnd;

        LZ4_stream_t lz4Stream;

        // a buffer for storing data to be compressed.
        tt_int8 srcRingBuf[COMPRESSION_RING_BUFFER_SIZE];

        // output compressed data.
        DataStreamHdr packetHdr;
        char cmpBuf[LZ4_COMPRESSBOUND(COMPRESSION_MAX_INPUT_SIZE)];
    };

    tt_uint16 GetStringId(PacketCompressor* pComp, const char* pStr);
    void addToCompressionBufferNoFlush(PacketCompressor* pComp, const void* pData, tt_int32 len);

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

        auto start = getTicks();

        const tt_int32 cmpBytes = LZ4_compress_fast_continue(&pComp->lz4Stream, pInBegin,
            pComp->cmpBuf, inBytes, sizeof(pComp->cmpBuf), 9);

        auto end = getTicks();

        if (cmpBytes <= 0) {
            onFatalWorkerError(pComp->pCtx);
            writeLog(pComp->pCtx, TtLogType::Error, "Compression Error(0x%x)", cmpBytes);
            return;
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

        // we can add the zone here.
        DataPacketZone packet;
        packet.type = DataStreamType::Zone;
        packet.stackDepth = 1;
        packet.threadID = getThreadID();
        packet.start = toRelativeTicks(pComp->pCtx, start);
        packet.end = toRelativeTicks(pComp->pCtx, end);
        packet.strIdxFile = GetStringId(pComp, TELEM_ZONE_SOURCE_FILE);
        packet.strIdxFunction = GetStringId(pComp, "Compress");
        packet.strIdxFmt = GetStringId(pComp, "Telem Compress");
        packet.lineNo = static_cast<tt_uint16>(0);
        packet.argDataSize = 0;

        addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));

#if RUNTIME_ZONE_WRITES
        // we also add net zones here.
        {
            auto& writeZones = *pComp->pCtx->pWriteZones;
            if (writeZones.num) {

                packet.stackDepth = 2;

                if (pComp->pCtx->fileHandle != TELEM_INVALID_HANDLE) {
                    packet.strIdxFunction = GetStringId(pComp, "WriteToFile");
                    packet.strIdxFmt = GetStringId(pComp, "File Write");
                }
                else {
                    packet.strIdxFunction = GetStringId(pComp, "SendToServer");
                    packet.strIdxFmt = GetStringId(pComp, "Socket send");
                }

                for (int32_t i = 0; i < writeZones.num; i++) {
                    auto& zone = writeZones.zones[i];

                    packet.start = toRelativeTicks(pComp->pCtx, zone.start);
                    packet.end = toRelativeTicks(pComp->pCtx, zone.end);
                    addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
                }

                writeZones.num = 0;
            }
        }
#endif // RUNTIME_ZONE_WRITES

#if RUNTIME_ZONE_PDB_SEND
        {
            auto& pdbZones = *pComp->pCtx->pPDBZones;
            if (pdbZones.num) {

                packet.stackDepth = 0;
                packet.strIdxFunction = GetStringId(pComp, "PDBSendToServer");
                packet.strIdxFmt = GetStringId(pComp, "PDB Send");

                for (int32_t i = 0; i < pdbZones.num; i++) {
                    auto& zone = pdbZones.zones[i];

                    packet.start = toRelativeTicks(pComp->pCtx, zone.start);
                    packet.end = toRelativeTicks(pComp->pCtx, zone.end);
                    addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
                }

                pdbZones.num = 0;
            }
        }
#endif // RUNTIME_ZONE_WRITES


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

    TELEM_NO_INLINE void writeStringCompressionBuffer(PacketCompressor* pComp, StringTableIndex idx, const char* pStr)
    {
        tt_int32 strLen = static_cast<tt_int32>(strUtil::strlen(pStr));
        if (strLen > MAX_STRING_LEN) {
            strLen = MAX_STRING_LEN;
        }

        tt_int32 packetLen = sizeof(DataPacketStringTableAdd) + static_cast<tt_int32>(strLen);

        DataPacketStringTableAdd hdr;
        hdr.type = DataStreamType::StringTableAdd;
        hdr.id = idx.index;
        hdr.length = static_cast<tt_uint16>(strLen);

        flushCompressionBufferIfrequired(pComp, packetLen);

        addToCompressionBufferNoFlush(pComp, &hdr, sizeof(hdr));
        addToCompressionBufferNoFlush(pComp, pStr, strLen);
    }

    tt_uint16 GetStringId(PacketCompressor* pComp, const char* pStr)
    {
        auto idx = StringTableGetIndex(pComp->strTable, pStr);

        if (idx.inserted) {
            writeStringCompressionBuffer(pComp, idx, pStr);
        }

        return idx.index;
    }

    // -----------------------------------

    inline constexpr tt_int32 RoundUpToMultiple(tt_int32 numToRound, tt_int32 multipleOf)
    {
        return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
    }

    template<typename T>
    inline constexpr tt_int32 GetSizeNotArgData(void)
    {
        // static_assert(sizeof(T) == 64);
        return RoundUpToMultiple(sizeof(T), 64);
    }

    template<typename T>
    inline constexpr tt_int32 GetSizeWithoutArgDataNoAlign(void)
    {
        constexpr tt_int32 argDataSize = sizeof(T::argData);
        return sizeof(T) - argDataSize;
    }

    template<typename T>
    inline constexpr tt_int32 GetSizeWithoutArgData(void)
    {
        constexpr tt_int32 argDataSize = sizeof(T::argData);
        return RoundUpToMultiple(sizeof(T) - argDataSize, 64);
    }

    template<typename T>
    inline constexpr tt_int32 GetDataSizeNoAlign(tt_int32 argDataSize)
    {
        return GetSizeWithoutArgData<T>() + argDataSize;
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

    tt_int32 AddStrings(ArgData& data, const char* pFmtString, tt_int32 numArgs, uintptr_t* pValues)
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
                case 't':
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

                    const auto offset = std::distance(data.data, pStrData);

                    // TODO: handle buffer not having space etc.
                    *pStrData++ = static_cast<tt_uint8>(len & 0xFF);
                    memcpy(pStrData, pStr, len);

                    pStrData += len;
                    bytesLeft -= (len + sizeof(tt_uint8));

                    // replace this with offset, should also compress better
                    pValues[idx] = offset;

                    // TODO: make this sentinal a bit more robust?
                    if (idx > 0 && precision != MAX_STRING_LEN) {
                        pValues[idx-1] = 0;
                    }
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
    tt_int32 BuildArgData(ArgData& data, const char* pFmtString, tt_int32 numArgs, va_list& l)
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
        ThreadSetGroup,
        ThreadSetGroupSort,
        CallStack,
        LockSetName,
        LockTry,
        LockState,
        LockCount,
        MemAlloc,
        MemFree,
        Message,
        Plot,
        PDBInfo
    };

    TELEM_PACK_PUSH(8)

    struct QueueDataBase
    {
        QueueDataType type;
        tt_uint8 argDataSize;
    };
    
    struct QueueDataTickInfo : public QueueDataBase
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

        ArgData argData;
    };

    struct QueueDataThreadSetGroup : public QueueDataBase
    {
        TtthreadId threadID;
        tt_int32 groupID;
    };

    struct QueueDataThreadSetGroupSort : public QueueDataBase
    {
        tt_int32 groupID;
        tt_int32 sortVal;
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

        ArgData argData;
    };

    struct QueueDataLockSetName : public QueueDataBase
    {
        const void* pLockPtr;
        const char* pFmtStr;
        tt_uint64 time;

        ArgData argData;
    };

    struct QueueDataLockTry : public QueueDataBase
    {
        TraceLock lock;
        TtthreadId threadID;
        const void* pLockPtr;

        ArgData argData;
    };

    struct QueueDataLockState : public QueueDataBase
    {
        TtLockState::Enum state;
        TtthreadId threadID;
        tt_uint64 time;
        TtSourceInfo sourceInfo;
        const void* pLockPtr;
        const char* pFmtStr;

        ArgData argData;
    };

    struct QueueDataLockCount : public QueueDataBase
    {
        tt_uint16 count;
        TtthreadId threadID;
        tt_uint64 time;
        TtSourceInfo sourceInfo;
        const void* pLockPtr;
        const char* pFmtStr;

        ArgData argData;
    };

    struct QueueDataMemAlloc : public QueueDataBase
    {
        TtthreadId threadID;
        tt_uint64 time;
        tt_uint32 size;
        TtSourceInfo sourceInfo;
        const void* pPtr;
        const char* pFmtStr;

        ArgData argData;
    };

    struct QueueDataMemFree : public QueueDataBase
    {
        TtthreadId threadID;
        tt_uint64 time;
        TtSourceInfo sourceInfo;
        const void* pPtr;
        const char* pFmtStr;
    };

    struct QueueDataMessage : public QueueDataBase
    {
        TtLogType::Enum logType;
        tt_uint64 time;
        const char* pFmtStr;

        ArgData argData;
    };

    struct QueueDataPlot : public QueueDataBase
    {
        tt_uint64 time;
        TtPlotValue value;
        const char* pFmtStr;
        ArgData argData;
    };

    struct QueueDataPDBInfo : public QueueDataBase
    {
        tt_uint64 modAddr;
        tt_uint32 imageSize;
        tt_uint32 age;

        tt_uint8 guid[16];
        const char* pFilename;
    };

    TELEM_PACK_POP


    constexpr size_t size0 = sizeof(QueueDataThreadSetName);
    constexpr size_t size01 = sizeof(QueueDataThreadSetGroup);
    constexpr size_t size02 = sizeof(QueueDataThreadSetGroupSort);
    constexpr size_t size1 = sizeof(QueueDataZone);
    constexpr size_t size2 = sizeof(QueueDataLockSetName);
    constexpr size_t size3 = sizeof(QueueDataLockTry);
    constexpr size_t size4 = sizeof(QueueDataLockState);
    constexpr size_t size5 = sizeof(QueueDataLockCount);
    constexpr size_t size6 = sizeof(QueueDataMemAlloc);
    constexpr size_t size7 = sizeof(QueueDataMemFree);
    constexpr size_t size8 = sizeof(QueueDataMessage);
    constexpr size_t size9 = sizeof(QueueDataPlot);
    constexpr size_t size10 = sizeof(QueueDataCallStack);
    constexpr size_t size11 = sizeof(QueueDataTickInfo);
    constexpr size_t size12 = sizeof(QueueDataPDBInfo);

    constexpr size_t size15 = sizeof(ArgData);
    

    static_assert(64 == GetSizeWithoutArgData<QueueDataThreadSetName>());
    static_assert(12 == sizeof(QueueDataThreadSetGroup));
    static_assert(12 == sizeof(QueueDataThreadSetGroupSort));
    static_assert(40 == sizeof(QueueDataTickInfo));
    static_assert(64 == GetSizeWithoutArgData<QueueDataZone>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataLockSetName>());
    static_assert((sizeof(QueueDataCallStack) % 64) == 0);
    static_assert(64 == GetSizeWithoutArgData<QueueDataLockTry>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataLockState>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataLockCount>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataMemAlloc>());
    static_assert(56 == sizeof(QueueDataMemFree));
    static_assert(64 == GetSizeWithoutArgData<QueueDataMessage>());
    static_assert(64 == GetSizeWithoutArgData<QueueDataPlot>());
    static_assert(48 == sizeof(QueueDataPDBInfo));

    void flipBufferInternal(TraceContext* pCtx)
    {
        ttZoneFilterd(contextToHandle(pCtx), 100, "FlipBuffers");

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
        (void)_InterlockedExchange(reinterpret_cast<volatile long*>(&pCtx->tickBuffers[oldIdx].bufOffset), 0l);

        // flip the buffers.
        (void)_InterlockedXor(reinterpret_cast<volatile long*>(&pCtx->activeTickBufIdx), 1l);

        // tell the background thread we are HOT!
        ::SetEvent(pCtx->hSignal_);
    }


    void flipBuffer(TraceContext* pCtx, bool stalled, bool force)
    {
        // this can be entered from multiple threads but we only want to flip once.
        if (pCtx->cs_.TryEnter())
        {
            ScopedLock<decltype(pCtx->cs_)> lock(pCtx->cs_, adopt_lock);

            if (stalled) {
                // Catch threads trying to flip right after we just flipped.
                // basically a race condition where we think still need to flip.
                auto halfBufferCap = pCtx->tickBufCapacity / 2;
                auto bufSize = getActiveTickBufferSize(pCtx);
                if (bufSize < halfBufferCap) {
                    return;
                }

                pCtx->numStalls++;
            }

            // work out how long since last tick.
            auto nowTicks = getTicks();

            if (!stalled && !force)
            {
                auto ellapsedTicks = nowTicks - pCtx->lastFlipTick;
                auto ellapsedNano = ticksToNano(pCtx, ellapsedTicks);

                constexpr tt_uint64 nanoPerMS = 1'000'000;
                constexpr tt_uint64 nanoThreshold = nanoPerMS * 10;

                if (ellapsedNano < nanoThreshold) {

                    auto halfBufferCap = pCtx->tickBufCapacity / 2;
                    auto bufSize = getActiveTickBufferSize(pCtx);

                    if (bufSize < halfBufferCap) {
                        // Skip the flip.
                        return;
                    }
                }
            }

            pCtx->lastFlipTick = nowTicks;

            flipBufferInternal(pCtx);
        }
        else
        {
            // need to wait.
            // lets just take the lock, not ideal if lots of threads end up here.
            // but got biggeer problems if that happening tbh.
            ScopedLock lock(pCtx->cs_);
        }
    }


    TELEM_INLINE void addToTickBuffer(TraceContext* pCtx, const void* pPtr, tt_int32 copySize, tt_int32 size);

    // This is just a helper to flip buffer than add the data.
    // But keeps this logic outside the hot functio.n
    TELEM_NO_INLINE void addToTickBufferFull(TraceContext* pCtx, const void* pPtr, tt_int32 copySize, tt_int32 size)
    {
        // We  don't pass force so that if multiple threads end up in here
        // the buffer is only flipped if actually running out of space still.
        // Prevents multiple flips.
        flipBuffer(pCtx, true, false);
        addToTickBuffer(pCtx, pPtr, copySize, size);
    }

    TELEM_INLINE void addToTickBuffer(TraceContext* pCtx, const void* pPtr, tt_int32 copySize, tt_int32 size)
    {
        auto& buf = pCtx->tickBuffers[pCtx->activeTickBufIdx];
        long offset = _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&buf.bufOffset), static_cast<long>(size));
        
        if (offset + size <= pCtx->tickBufCapacity) {
            memcpy(buf.pTickBuf + offset, pPtr, copySize);
            return;
        }

        // no space,
#if X_DEBUG && 0
        if (copySize > size) {
            ::DebugBreak();
        }

        if (offset + size > pCtx->tickBufCapacity) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        addToTickBufferFull(pCtx, pPtr, copySize, size);
    }

    TELEM_INLINE void addToTickBuffer(TraceContext* pCtx, const void* pPtr, tt_int32 size)
    {
        addToTickBuffer(pCtx, pPtr, size, size);
    }

    template<typename T>
    TELEM_INLINE void addToTickBuffer64(TraceContext* pCtx, const T& obj)
    {
        static_assert(sizeof(T) == 64);

        auto& buf = pCtx->tickBuffers[pCtx->activeTickBufIdx];
        long offset = _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&buf.bufOffset), 64);

        if (offset + 64 <= pCtx->tickBufCapacity) {
            memcpy(buf.pTickBuf + offset, &obj, 64);
            return;
        }

        // no space,
#if X_DEBUG && 0
        if (offset + 64 > pCtx->tickBufCapacity) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        addToTickBufferFull(pCtx, &obj, 64, 64);
    }

    void syncPDBInfo(TraceContext* pCtx, const PE::PDBInfo& info)
    {
        if (pCtx->numPDBSync == info.num) {
            return;
        }

        auto base = pCtx->numPDBSync;
        auto num = info.num - pCtx->numPDBSync;
        for (tt_int32 i = 0; i < num; i++) {

            QueueDataPDBInfo data;
            data.type = QueueDataType::PDBInfo;
            data.argDataSize = 0;
            data.modAddr = reinterpret_cast<tt_uint64>(info.mods[base + i]);

            const auto& sig = info.sigs[base + i];

            data.imageSize = sig.imageSize;
            data.age = sig.age;
            memcpy(data.guid, sig.guid, sizeof(sig.guid));
            data.pFilename = sig.pdbFileName; // this is static so taking pointer will work for the hash table etc.

            constexpr tt_int32 alignedSize = RoundUpToMultiple(sizeof(data), 64);
            addToTickBuffer(pCtx, &data, sizeof(data), alignedSize);
        }

        pCtx->numPDBSync = info.num;
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

        constexpr tt_int32 alignedSize = RoundUpToMultiple(sizeof(data), 64);
        addToTickBuffer(pCtx, &data, sizeof(data), alignedSize);
    }

    TELEM_INLINE void queueCallStack(TraceContext* pCtx, const TtCallStack& stack)
    {
        QueueDataCallStack data; // this is full size so we don't read past end of stack..
        data.type = QueueDataType::CallStack;
        data.argDataSize = 0;
        data.callstack.num = stack.num;
        memcpy(data.callstack.frames, stack.frames, stack.num * sizeof(stack.frames[0]));

        tt_int32 copySize = sizeof(QueueDataBase) + (sizeof(stack.frames[0]) * (stack.num - 1));
        tt_int32 size = RoundUpToMultiple(copySize, static_cast<tt_int32>(64));

#if X_DEBUG
        if (size > sizeof(data)) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        addToTickBuffer(pCtx, &data, copySize, size);
    }

    TELEM_INLINE void queueZone(TraceContext* pCtx, TraceThread* pThread, tt_int32 depth)
    {
#if X_DEBUG
        if (depth < 0) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        const auto& scopeData = pThread->zones[depth];
        const bool hasArgData = scopeData.argDataSize;

        QueueDataZone data;
        data.type = QueueDataType::Zone;
        data.argDataSize = static_cast<tt_uint8>(scopeData.argDataSize & 0xFF);
        data.stackDepth = static_cast<tt_uint8>(pThread->stackDepth & 0xFF);
        data.threadID = pThread->id;
        data.zone = scopeData.zone;

        tt_int32 copySize;
        tt_int32 size;

        if (!hasArgData) {
            // we can skip the runtime size calculation here.
            copySize = GetSizeWithoutArgDataNoAlign<decltype(data)>();
            size = GetSizeWithoutArgData<decltype(data)>();
        }
        else {
            auto& argData = pThread->argData[depth];
            memcpy(&data.argData, &argData, scopeData.argDataSize);
            copySize = GetDataSizeNoAlign<decltype(data)>(scopeData.argDataSize);
            size = RoundUpToMultiple(copySize, 64);
        }

        addToTickBuffer(pCtx, &data, copySize, size);
    }

    TELEM_INLINE void queueLockTry(TraceContext* pCtx, TraceThread* pThread, const void* pPtr, TraceLockBuilder* pLock)
    {
        QueueDataLockTry data;
        data.type = QueueDataType::LockTry;
        data.argDataSize = 0;
        data.threadID = pThread->id;
        data.lock = pLock->lock;
        data.pLockPtr = pPtr;

        if (!pLock->argDataSize) {
            memcpy(&data.argData, &pLock->argData, pLock->argDataSize);
            addToTickBuffer(pCtx, &data, GetDataSize<decltype(data)>(pLock->argDataSize));
        }
        else {
            addToTickBuffer(pCtx, &data, GetSizeWithoutArgDataNoAlign<decltype(data)>(), GetSizeWithoutArgData<decltype(data)>());
        }
    }

    TELEM_INLINE void queueLockState(TraceContext* pCtx, const TtSourceInfo& sourceInfo, const void* pPtr, TtLockState::Enum state)
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

        constexpr tt_int32 copySize = GetSizeWithoutArgDataNoAlign<decltype(data)>();
        addToTickBuffer(pCtx, &data, copySize, GetSizeWithoutArgData<decltype(data)>());
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

    tt_int32 queueProcessThreadSetGroup(PacketCompressor* pComp, const QueueDataThreadSetGroup* pBuf)
    {
        DataPacketThreadSetGroup packet;
        packet.type = DataStreamType::ThreadSetGroup;
        packet.threadID = pBuf->threadID;
        packet.groupID = pBuf->groupID;

        addToCompressionBuffer(pComp, &packet, sizeof(packet));
        return GetSizeNotArgData<std::remove_pointer_t<decltype(pBuf)>>();
    }

    tt_int32 queueProcessThreadSetGroupSort(PacketCompressor* pComp, const QueueDataThreadSetGroupSort* pBuf)
    {
        DataPacketThreadSetGroupSort packet;
        packet.type = DataStreamType::ThreadSetGroupSort;
        packet.groupID = pBuf->groupID;
        packet.sortVal = pBuf->sortVal;

        addToCompressionBuffer(pComp, &packet, sizeof(packet));
        return GetSizeNotArgData<std::remove_pointer_t<decltype(pBuf)>>();
    }

    tt_int32 queueProcessCallStack(PacketCompressor* pComp, const QueueDataCallStack* pBuf)
    {
        // incoming buffer size.
        tt_int32 size = sizeof(QueueDataBase) + (sizeof(pBuf->callstack.frames[0]) * (pBuf->callstack.num - 1));
        size = RoundUpToMultiple(size, static_cast<tt_int32>(64));

        // seen this callstack before?
        if (CallstackCacheContainsAdd(pComp->callstackCache, pBuf->callstack.id)) {
            return size;
        }

        DataPacketCallStack packet;
        packet.type = DataStreamType::CallStack;
        packet.id = pBuf->callstack.id;
        packet.numFrames = pBuf->callstack.num;

        const tt_int32 baseSize = sizeof(packet);
        const tt_int32 framesSize = (sizeof(pBuf->callstack.frames[0]) * pBuf->callstack.num);
        const tt_int32 dataSize = baseSize + framesSize;
        flushCompressionBufferIfrequired(pComp, dataSize);

        addToCompressionBufferNoFlush(pComp, &packet, baseSize);
        addToCompressionBufferNoFlush(pComp, pBuf->callstack.frames, framesSize);

        return size;
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
        return GetSizeNotArgData<std::remove_pointer_t<decltype(pBuf)>>();
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

    tt_int32 queueProcessPlot(PacketCompressor* pComp, const QueueDataPlot* pBuf)
    {
        DataPacketPlot packet;
        packet.type = DataStreamType::Plot;
        packet.time = pBuf->time;
        packet.strIdxFmt = GetStringId(pComp, pBuf->pFmtStr);
        packet.value = pBuf->value;
        packet.argDataSize = pBuf->argDataSize;

        const auto dataSize = GetDataSize<std::remove_pointer_t<decltype(pBuf)>>(pBuf->argDataSize);
        flushCompressionBufferIfrequired(pComp, dataSize);

        addToCompressionBufferNoFlush(pComp, &packet, sizeof(packet));
        if (pBuf->argDataSize) {
            addToCompressionBufferNoFlush(pComp, &pBuf->argData, pBuf->argDataSize);
        }

        return dataSize;
    }

    tt_int32 queueProcessPDBInfo(PacketCompressor* pComp, const QueueDataPDBInfo* pBuf)
    {
        DataPacketPDBInfo packet;
        packet.type = DataStreamType::PDBInfo;
        packet.argDataSize = 0;
        packet.modAddr = pBuf->modAddr;
        packet.imageSize = pBuf->imageSize;
        memcpy(packet.guid, pBuf->guid, sizeof(pBuf->guid));
        packet.age = pBuf->age;
        packet.strIdxName = GetStringId(pComp, pBuf->pFilename);

        addToCompressionBuffer(pComp, &packet, sizeof(packet));

        constexpr tt_int32 alignedSize = RoundUpToMultiple(sizeof(*pBuf), 64);
        return alignedSize;
    }


    struct SocketRecvState
    {
        SocketRecvState() {
            recvPending = false;
            zero_object(overlapped);
            totalBytes = 0;
            buf.buf = recvbuf;
            buf.len = sizeof(recvbuf);
        }

        bool recvPending;
        OVERLAPPED overlapped;
        tt_uint32 totalBytes;
        platform::WSABUF buf;
        char recvbuf[MAX_PACKET_SIZE];
    };

#if 0
    struct ScopedHandle
    {
        ScopedHandle(HANDLE hHandle) :
            hHandle(hHandle)
        {}

        ~ScopedHandle() {
            ::CloseHandle(hHandle);
        }

        HANDLE hHandle;
    };
#endif

    TELEM_ALIGNED_SYMBOL(struct MyOverlapped, 64) : public OVERLAPPED
    {
        tt_uint32 readSize;
        tt_uint32 bytesRead;
        bool pending = true;
    };

    struct PDBSender
    {
        static constexpr tt_uint32 BUF_SIZE = MAX_PDB_DATA_BLOCK_SIZE;

        PDBSender(PacketCompressor* pComp, tt_uint64 modAddr) :
            pComp_(pComp),
            modAddr_(modAddr)
        {
            hFile_ = INVALID_HANDLE_VALUE;
            zero_object(overlapped_);
            zero_object(modInfo_);

        }

        ~PDBSender() {
            if (hFile_ != INVALID_HANDLE_VALUE) {
                ::CloseHandle(hFile_);
            }

            if (overlapped_[0].hEvent) {
                ::CloseHandle(overlapped_[0].hEvent);
            }
            if (overlapped_[1].hEvent) {
                ::CloseHandle(overlapped_[1].hEvent);
            }
        }

        bool send(void);

    private:
        bool dispatchRead(tt_int32 bufIdx);
        bool waitForBuf(tt_int32 bufIdx);
        void addToCompBuf(tt_int32 bufIdx);
        bool waitForBufAndComp(tt_int32 bufIdx);
        bool isPending(tt_int32 bufIdx);

    private:
        PacketCompressor* pComp_;
        tt_uint64 modAddr_;

        HANDLE hFile_;
        tt_uint32 fileSize_;
        tt_uint32 offset_;
        IMAGEHLP_MODULEW64 modInfo_;

        MyOverlapped overlapped_[2];
        tt_uint8 bufs[2][RoundUpToMultiple(BUF_SIZE + sizeof(DataPacketPDBBlock), 64)];
    };

    bool PDBSender::dispatchRead(tt_int32 bufIdx)
    {
        const auto bytesLeft = fileSize_ - offset_;
        const auto readSize = static_cast<tt_uint32>(Min(bytesLeft, BUF_SIZE));

        auto* pBuf = bufs[bufIdx];
        auto* pDataBuf = pBuf + sizeof(DataPacketPDBBlock);

        // Fill in header
        DataPacketPDBBlock* pBlockHdr = reinterpret_cast<DataPacketPDBBlock*>(pBuf);
        pBlockHdr->type = DataStreamType::PDBBlock;
        pBlockHdr->modAddr = modInfo_.BaseOfImage;
        pBlockHdr->blockSize = readSize;
        pBlockHdr->offset = offset_;

        auto& over = overlapped_[bufIdx];
        if (over.pending) {
            ::DebugBreak();
        }

        over.Offset = offset_;
        over.readSize = readSize;
        over.pending = true;

        if (!::ReadFile(hFile_, pDataBuf, readSize, nullptr, &over)) {
            auto err = ::GetLastError();
            if (err != ERROR_IO_PENDING) {
                return false;
            }
        }

        offset_ += readSize;
        return true;
    };

    bool PDBSender::waitForBuf(tt_int32 bufIdx)
    {
        auto& over = overlapped_[bufIdx];

        if (!over.pending) {
            ::DebugBreak();
        }

        DWORD bytesRead;
        if (!::GetOverlappedResult(hFile_, &over, &bytesRead, TRUE)) {
            return false;
        }

        over.bytesRead = bytesRead;
        over.pending = false;

        return over.bytesRead == over.readSize;
    };

    void PDBSender::addToCompBuf(tt_int32 bufIdx)
    {
        auto* pBuf = bufs[bufIdx];
        const auto& over = overlapped_[bufIdx];
        tt_int32 totalSize = over.bytesRead + sizeof(DataPacketPDBBlock);

        addToCompressionBuffer(pComp_, pBuf, totalSize);
    };

    bool PDBSender::waitForBufAndComp(tt_int32 bufIdx)
    {
        if (!waitForBuf(bufIdx)) {
            return false;
        }

        addToCompBuf(bufIdx);
        return true;
    };

    bool PDBSender::isPending(tt_int32 bufIdx)
    {
        const auto& over = overlapped_[bufIdx];
        return over.pending;
    };


    bool PDBSender::send(void)
    {
        modInfo_.SizeOfStruct = sizeof(modInfo_);
        if (!SymGetModuleInfoW64(::GetCurrentProcess(), modAddr_, &modInfo_)) {
            // TODO: tell server we suck.
            return false;
        }

        if (modInfo_.SymType != SymPdb) {
            // rip.
            return false;
        }

        auto* pPath = modInfo_.LoadedPdbName;

        DWORD access = FILE_READ_DATA;
        DWORD share = FILE_SHARE_READ;
        DWORD dispo = OPEN_EXISTING;
        DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED;

        hFile_ = ::CreateFileW(pPath, access, share, NULL, dispo, flags, NULL);
        if (hFile_ == INVALID_HANDLE_VALUE) {
            // TODO: rip
            return false;
        }

        _BY_HANDLE_FILE_INFORMATION info;
        if (!GetFileInformationByHandle(hFile_, &info)) {
            // TODO: rip
            return false;
        }

        overlapped_[0].hEvent = ::CreateEventW(nullptr, true, false, nullptr);
        if (!overlapped_[0].hEvent) {
            return false;
        }
        overlapped_[1].hEvent = ::CreateEventW(nullptr, true, false, nullptr);
        if (!overlapped_[0].hEvent) {
            return false;
        }

        // should never be above 1<<31,
        fileSize_ = info.nFileSizeLow;
        offset_ = 0;

        DataPacketPDB hdr;
        hdr.type = DataStreamType::PDB;
        hdr.modAddr = modInfo_.BaseOfImage;
        hdr.imageSize = modInfo_.ImageSize;
        hdr.fileSize = fileSize_;
        memcpy(&hdr.guid, &modInfo_.PdbSig70, sizeof(modInfo_.PdbSig70));
        hdr.age = modInfo_.PdbAge;

        addToCompressionBuffer(pComp_, &hdr, sizeof(hdr));

        tt_int32 bufIdx = 0;

        if (!dispatchRead(bufIdx)) {
            return false;
        }

        // we already finished reading?
        if (offset_ < fileSize_) {
            if (!dispatchRead(bufIdx ^ 1)) {
                return false;
            }
        }

        while (offset_ < fileSize_)
        {
            if (!waitForBufAndComp(bufIdx)) {
                return false;
            }

            // we can dispatch another read now this buffer is done.
            if (!dispatchRead(bufIdx)) {
                return false;
            }

            bufIdx ^= 1;

#if X_DEBUG
            // Make sure th other buffer was reading in the background.
            if (!isPending(bufIdx)) {
                ::DebugBreak();
            }
#endif // X_DEBUG
        }

#if X_DEBUG
        // This one should be pending
        if (!isPending(bufIdx)) {
            ::DebugBreak();
        }
#endif // X_DEBUG

        // finish off.
        if (isPending(bufIdx)) {
            if (!waitForBufAndComp(bufIdx)) {
                return false;
            }
        }

        if (isPending(bufIdx ^ 1)) {
            if (!waitForBufAndComp(bufIdx ^ 1)) {
                return false;
            }
        }

        return true;
    }

    void handlePDBRequest(PacketCompressor* pComp, const RequestPDBHdr* pPDBReq)
    {
        // Return to sender... do do do
        PDBSender sender(pComp, pPDBReq->modAddr);

#if RUNTIME_ZONE_PDB_SEND
        RuntimeZone zone;
        zone.start = getTicks();
#endif // RUNTIME_ZONE_PDB_SEND

        if (!sender.send()) {
            // need to tell server we failed.
            DataPacketPDBError errorHdr;
            errorHdr.type = DataStreamType::PDBError;
            errorHdr.modAddr = pPDBReq->modAddr;
            addToCompressionBuffer(pComp, &errorHdr, sizeof(errorHdr));
        }

#if RUNTIME_ZONE_PDB_SEND
        zone.end = getTicks();

        auto& zones = *pComp->pCtx->pPDBZones;
        if (zones.num < WriteZones::NUM_ZONES) {
            zones.zones[zones.num++] = zone;
        }
        else {
            writeLog(pComp->pCtx, TtLogType::Warning, "PDB zone buffer is full");
        }
#endif // RUNTIME_ZONE_PDB_SEND
    }

    void processServerRequest(PacketCompressor* pComp, const PacketBase* pPacket)
    {
        switch (pPacket->type)
        {
            case PacketType::ReqPDB: {
                handlePDBRequest(pComp, reinterpret_cast<const RequestPDBHdr*>(pPacket));
                break;
            }

            default:
#if X_DEBUG
                ::DebugBreak();
                break;
#else
                TELEM_NO_SWITCH_DEFAULT;
#endif
        }

    }

    void readPackets(TraceContext* pCtx, PacketCompressor* pComp, SocketRecvState& recvState)
    {
        // don't bother reading packets if not going to get any.
        if ((pCtx->connFlags & TtConnectionFlag::StreamPDB) == 0) {
            return;
        }

        lastErrorWSA::Description Dsc;
        DWORD bytesTransferred = 0;
        DWORD flags = 0;

        if (!recvState.recvPending) {
            // make request
            recvState.buf.buf = recvState.recvbuf + recvState.totalBytes;
            recvState.buf.len = sizeof(recvState.recvbuf) - recvState.totalBytes;

            auto res = platform::WSARecv(pCtx->socket, &recvState.buf, 1, &bytesTransferred, &flags, &recvState.overlapped, nullptr);
            if (res == SOCKET_ERROR) {
                auto err = lastErrorWSA::Get();
                if (err != ERROR_IO_PENDING) {
                    writeLog(pCtx, TtLogType::Error, "WSARecv failed. Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
                    return;
                }
            }

            recvState.recvPending = true;
        }

        if (!platform::WSAGetOverlappedResult(pCtx->socket, &recvState.overlapped, &bytesTransferred, FALSE, &flags)) {
            auto err = lastErrorWSA::Get();
            if (err != WSA_IO_INCOMPLETE) {
                writeLog(pCtx, TtLogType::Error, "WSAGetOverlappedResult failed. Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
                return;
            }

            return;
        }

        recvState.recvPending = false;
        recvState.totalBytes += bytesTransferred;

        if (recvState.totalBytes < sizeof(PacketBase)) {
            return;
        }

        tt_uint32 bytesLeft = recvState.totalBytes;
        auto* pData = recvState.recvbuf;

        while(bytesLeft > sizeof(PacketBase)) {
            auto* pHdr = reinterpret_cast<const PacketBase*>(pData);
            const tt_uint32 packetSize = pHdr->dataSize;
            if (bytesLeft < packetSize) {
                break;
            }

            processServerRequest(pComp, pHdr);

            pData += packetSize;
            bytesLeft -= packetSize;
        }

        // shift bytes down.
        if (bytesLeft > 0) {
            memcpy(recvState.recvbuf, pData, bytesLeft);
        }

        recvState.totalBytes = bytesLeft;
    }

    DWORD __stdcall WorkerThread(LPVOID pParam)
    {
        setThreadName(getThreadID(), "Telemetry");

        auto* pCtx = reinterpret_cast<TraceContext*>(pParam);

        ttSetThreadName(contextToHandle(pCtx), getThreadID(), "Telem - Worker");
        ttSetThreadGroup(contextToHandle(pCtx), getThreadID(), TELEM_INTERNAL_THREAD_GROUP_ID);

        tt_uint8 stringTableBuf[STRING_TABLE_BUF_SIZE];
        tt_uint8 callstackCacheBuf[CALLSTACK_CACHE_BUF_SIZE];
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
        comp.callstackCache = CreateCallstackCache(callstackCacheBuf, sizeof(callstackCacheBuf));

#if !PACKET_COMPRESSION
        {
            // pre fill the header.
            auto* pDataHeader = reinterpret_cast<DataStreamHdr*>(buffer.pPacketBuffer);
            pDataHeader->dataSize = 0;
            pDataHeader->type = PacketType::DataStream;
        }
#endif // !PACKET_COMPRESSION

        SocketRecvState recvState;

        for (;;)
        {
            // Are we writing to socket?
            if(pCtx->socket != INV_SOCKET) {
                readPackets(pCtx, &comp, recvState);
            }

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

            auto start = getTicks();

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
                if (pCtx->shutDownFlag) {
                    continue;
                }
                ::DebugBreak();
            }

            // If the socket is dead don't bother processing, but we need to keep flipping to prevent stall.
            if (pCtx->socket == INV_SOCKET && pCtx->fileHandle == TELEM_INVALID_HANDLE) {
                continue;
            }

            if ((pCtx->flags & TtFlag::DropData) != 0) {
                continue;
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
                    case QueueDataType::ThreadSetGroup:
                        pBuf += queueProcessThreadSetGroup(&comp, reinterpret_cast<const QueueDataThreadSetGroup*>(pBuf));
                        break;
                    case QueueDataType::ThreadSetGroupSort:
                        pBuf += queueProcessThreadSetGroupSort(&comp, reinterpret_cast<const QueueDataThreadSetGroupSort*>(pBuf));
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
                    case QueueDataType::Plot:
                        pBuf += queueProcessPlot(&comp, reinterpret_cast<const QueueDataPlot*>(pBuf));
                        break;
                    case QueueDataType::PDBInfo:
                        pBuf += queueProcessPDBInfo(&comp, reinterpret_cast<const QueueDataPDBInfo*>(pBuf));
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

            // Send zone info for this thread
            {
                auto end = getTicks();

                QueueDataZone zone;
                zone.type = QueueDataType::Zone;
                zone.stackDepth = 0;
                zone.argDataSize = 0;
                zone.threadID = getThreadID();
                zone.zone.start = start;
                zone.zone.end = end;
                zone.zone.pFmtStr = "Telem process buffer";
                zone.zone.sourceInfo.line_ = 0;
                zone.zone.sourceInfo.pFile_ = TELEM_ZONE_SOURCE_FILE;
                zone.zone.sourceInfo.pFunction_ = "WorkerThread";

                queueProcessZone(&comp, &zone);
            }

            // flush anything left over to the socket.
            flushCompressionBuffer(&comp);
            flushPacketBuffer(pCtx, &buffer);
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

    // resolve callstack shizz.
    pRtlWalkFrameChain = (RtlWalkFrameChainFunc)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlWalkFrameChain");
    if (!pRtlWalkFrameChain) {
        return false;
    }

    SymSetOptions(SYMOPT_LOAD_LINES);
    if (!SymInitialize(GetCurrentProcess(), nullptr, true)) {
        return false;
    }

    PE::updatePDBInfo(PE::pdbInfo);

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

    if (!SymCleanup(GetCurrentProcess())) {
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
    constexpr tt_size writeZonesSize = sizeof(WriteZones);
    constexpr tt_size pdbZonesSize = sizeof(PDBZones);
    constexpr tt_size minBufferSize = 1024 * 10; // 10kb.. enougth?
    constexpr tt_size internalSize = contexSize + threadDataSize + writeZonesSize + pdbZonesSize;
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
    tt_uint8* pWriteZoneBuffer = pThreadDataBuf + threadDataSize;
    tt_uint8* pPDBZoneBuffer = pWriteZoneBuffer + writeZonesSize;
    tt_uint8* pTickBuffer0 = reinterpret_cast<tt_uint8*>(AlignTop(pPDBZoneBuffer + pdbZonesSize, 64));
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
    pCtx->lastFlipTick = getTicks();
    pCtx->lastTickNano = gSysTimer.GetNano();
    pCtx->isEnabled = true;
    pCtx->flags = 0;
    pCtx->connFlags = 0;
    pCtx->socket = INV_SOCKET;
    pCtx->fileHandle = TELEM_INVALID_HANDLE;
    pCtx->pThreadData = reinterpret_cast<TraceThread*>(pThreadDataBuf);
    pCtx->numThreadData = 0;
    pCtx->ticksPerMicro = gTicksPerMicro;
    pCtx->baseTicks = pCtx->lastTick;
    pCtx->baseNano = pCtx->lastTickNano;
    pCtx->pWriteZones = new (pWriteZoneBuffer) WriteZones();
    pCtx->pPDBZones = new (pPDBZoneBuffer) PDBZones();

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

    pCtx->numPDBSync = 0;

    pCtx->pFileOpen = Io::fileOpen;
    pCtx->pFileClose = Io::fileClose;
    pCtx->pFileWrite = Io::fileWrite;

    // initial sync of PDB info.
    syncPDBInfo(pCtx, PE::pdbInfo);

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

void TelemSetIoFuncs(TraceContexHandle ctx, FileOpenFunc open, FileCloseFunc close, FileWriteFunc write, void* pUserData)
{
    auto* pCtx = handleToContext(ctx);
    pCtx->pUserData = pUserData;
    pCtx->pFileOpen = open;
    pCtx->pFileClose = close;
    pCtx->pFileWrite = write;
}

TtError TelemOpen(TraceContexHandle ctx, const char* pAppName, const char* pBuildInfo, const char* pPath,
    TtConnectionType conType, tt_uint16 serverPort, tt_int32 timeoutMS, TtConnectionFlags flags)
{
    if (!isValidContext(ctx)) {
        return TtError::InvalidContex;
    }

    switch (conType)
    {
        case TtConnectionType::Tcp:
        case TtConnectionType::File:
            break;

        default:
            return TtError::InvalidParam;
    }

    auto* pCtx = handleToContext(ctx);

    ConnectionRequestHdr cr;
    zero_object(cr);
    cr.type = PacketType::ConnectionRequest;
    cr.clientVer.major = TELEM_VERSION_MAJOR;
    cr.clientVer.minor = TELEM_VERSION_MINOR;
    cr.clientVer.patch = TELEM_VERSION_PATCH;
    cr.clientVer.build = TELEM_VERSION_BUILD;

    LPWSTR pCmdLine = GetCommandLineW();

    const auto appNameLen = static_cast<tt_int32>(strUtil::strlen(pAppName));
    const auto buildInfoLen = static_cast<tt_int32>(strUtil::strlen(pBuildInfo));
    const auto cmdLineLen = static_cast<tt_int32>(wcslen(pCmdLine));

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
    cr.unixTimestamp = GetSystemTimeAsUnixTime();
    cr.workerThreadID = pCtx->threadId_;
    cr.dataSize = sizeof(cr) + cr.appNameLen + cr.buildInfoLen + cr.cmdLineLen;
    cr.connFlags = flags;
    pCtx->connFlags = static_cast<tt_uint8>(flags);

    if (conType == TtConnectionType::Tcp)
    {
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
        auto res = platform::getaddrinfo(pPath, portStr, &hints, &servinfo);
        if (res != 0) {
            lastErrorWSA::Description Dsc;
            const auto err = lastErrorWSA::Get();
            writeLog(pCtx, TtLogType::Error, "Failed to getaddrinfo. Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
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
        tt_int32 sock_opt = 1024 * 16;
        res = platform::setsockopt(connectSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sock_opt, sizeof(sock_opt));
        if (res != 0) {
            lastErrorWSA::Description Dsc;
            const auto err = lastErrorWSA::Get();
            writeLog(pCtx, TtLogType::Error, "Failed to set sndbuf on socket. Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
            return TtError::Error;
        }

        pCtx->socket = connectSocket;

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
    }
    else
    {
        // open a file.
        pCtx->fileHandle = pCtx->pFileOpen(pCtx->pIOUserData, pPath);
        if (pCtx->fileHandle == TELEM_INVALID_HANDLE) {
            return TtError::Error;
        }

        // lets be nice and only make one IO call.
        // lengths are validated above.
        uint8_t buf[sizeof(Io::TelemFileHdr) +
            sizeof(cr) +
            MAX_CMDLINE_LEN +
            MAX_STRING_LEN +
            MAX_STRING_LEN +
            128
        ];

        Io::TelemFileHdr hdr;
        hdr.fourCC = Io::TRACR_FOURCC;
        hdr.version = Io::TRACE_VERSION;

        tt_int32 offset = 0;
        memcpy(&buf[offset], &hdr, sizeof(hdr));
        offset += sizeof(hdr);
        memcpy(&buf[offset], &cr, sizeof(cr));
        offset += sizeof(cr);

        memcpy(&buf[offset], pAppName, appNameLen);
        offset += appNameLen;
        memcpy(&buf[offset], pBuildInfo, buildInfoLen);
        offset += buildInfoLen;
        memcpy(&buf[offset], cmdLine, cmdLenUtf8);
        offset += cmdLenUtf8;

        if (fileWrite(pCtx, buf, offset) != offset) {
            pCtx->pFileClose(pCtx->pIOUserData, pCtx->fileHandle);
            pCtx->fileHandle = TELEM_INVALID_HANDLE;
            return TtError::Error;
        }
        
        // sorted?

    }

    return TtError::Ok;
}


bool TelemClose(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);

    flipBuffer(pCtx, false, true);

    // TODO: make sure we finish sending PDB data to server and shit?
    // should we do a shutdown handshake or something?
    // maybe thread should just make sure it's responded to everything.

    // flush
    int res = platform::shutdown(pCtx->socket, SD_BOTH);
    if (res == SOCKET_ERROR) {
        lastErrorWSA::Description Dsc;
        const auto err = lastErrorWSA::Get();
        writeLog(pCtx, TtLogType::Error, "socket shutdown failed with Error(0x%x): \"%s\"", err, lastErrorWSA::ToString(err, Dsc));
    }

    if (pCtx->socket != INV_SOCKET) {
        platform::closesocket(pCtx->socket);
        pCtx->socket = INV_SOCKET;
    }

    if (pCtx->fileHandle != TELEM_INVALID_HANDLE) {
        pCtx->pFileClose(pCtx->pIOUserData, pCtx->fileHandle);
        pCtx->fileHandle = TELEM_INVALID_HANDLE;
    }

    return true;
}

void TelemTick(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    auto curTick = getTicks();
    auto curTimeNano = gSysTimer.GetNano();

    queueTickInfo(pCtx, pCtx->lastTick, curTick, pCtx->lastTickNano, curTimeNano);

    pCtx->lastTick = curTick;
    pCtx->lastTickNano = curTimeNano;

    flipBuffer(pCtx, false, false);
    return;
}

void TelemFlush(TraceContexHandle ctx)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    // this forces the flip.
    flipBuffer(pCtx, false, true);
}

void TelemUpdateSymbolData(TraceContexHandle ctx)
{
    // Sending new PDB sig info to server for this given context.
    PE::updatePDBInfo(PE::pdbInfo);

    auto* pCtx = handleToContext(ctx);

    syncPDBInfo(pCtx, PE::pdbInfo);
}

void TelemSyncSymbols(TraceContexHandle ctx)
{
    // Send PDB files to server.
    auto* pCtx = handleToContext(ctx);

    // This is going to be a little bit complex.
    // We need to send packet to server and it will request PDB.
    // how do i wait for that?

    TELEM_UNUSED(pCtx);
}

void TelemPause(TraceContexHandle ctx, bool pause)
{
    if (!isValidContext(ctx)) {
        return;
    }

    bool enabled = !pause;

    if (handleToContext(ctx)->isEnabled != enabled) {
        handleToContext(ctx)->isEnabled = enabled;
    }
}

bool TelemIsPaused(TraceContexHandle ctx)
{
    if (!isValidContext(ctx)) {
        return true;
    }

    return handleToContext(ctx)->isEnabled;
}

void TelemSetFlag(TraceContexHandle ctx, TtFlag::Enum flag, bool set)
{
    if (!isValidContext(ctx)) {
        return;
    }

    auto* pCtx = handleToContext(ctx);

    if (set) {
        pCtx->flags |= flag;
    }
    else {
        pCtx->flags &= ~flag;
    }
}

tt_int32 TelemGetStatI(TraceContexHandle ctx, TtStat::Enum stat)
{
    if (!isValidContext(ctx)) {
        return 0;
    }

    auto* pCtx = handleToContext(ctx);

    switch (stat)
    {
        case TtStat::NumStalls:
            return pCtx->numStalls;
        default:
            break;
    }

    return 0;
}

void TelemSetThreadName(TraceContexHandle ctx, tt_uint32 threadID, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    if (threadID == 0) {
        threadID = getThreadID();
    }

    QueueDataThreadSetName data;
    data.type = QueueDataType::ThreadSetName;
    data.time = getRelativeTicks(pCtx);
    data.threadID = threadID;
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

void TelemSetThreadGroup(TraceContexHandle ctx, tt_uint32 threadID, tt_int32 groupID)
{
    auto* pCtx = handleToContext(ctx);

    if (threadID == 0) {
        threadID = getThreadID();
    }

    QueueDataThreadSetGroup data;
    data.type = QueueDataType::ThreadSetGroup;
    data.threadID = threadID;
    data.groupID = groupID;

    addToTickBuffer(pCtx, &data, sizeof(data), GetSizeNotArgData<decltype(data)>());
}

void TelemSetThreadGroupDefaultSort(TraceContexHandle ctx, tt_int32 groupID, tt_int32 idx)
{
    auto* pCtx = handleToContext(ctx);

    QueueDataThreadSetGroupSort data;
    data.type = QueueDataType::ThreadSetGroupSort;
    data.groupID = groupID;
    data.sortVal = idx;

    addToTickBuffer(pCtx, &data, sizeof(data), GetSizeNotArgData<decltype(data)>());
}

tt_int32 TelemGetCallStack(TraceContexHandle ctx, TtCallStack& stackOut)
{
    static_assert((sizeof(TtCallStack) % 64) == 0, "Should be multiple of 64");

    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return -1;
    }

#if X_DEBUG
    zero_object(stackOut.frames);
#endif // X_DEBUG

    stackOut.num = pRtlWalkFrameChain(stackOut.frames, TtCallStack::MAX_FRAMES, 0);

    // don't think i'm going to cap the hash actually since if we cap it.
    // displaying the stack above that is pointless as it could be totally wrong.
    // so we might as well just collect less data.
    stackOut.id = Hash::Fnv1aHash(stackOut.frames, sizeof(stackOut.frames[0]) * stackOut.num);

    return stackOut.id;
}

tt_int32 TelemSendCallStack(TraceContexHandle ctx, const TtCallStack* pStack)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return -1;
    }

    if (pStack) {
        queueCallStack(pCtx, *pStack);
        return pStack->id;
    }

    TtCallStack stack;
    TelemGetCallStack(ctx, stack);
    queueCallStack(pCtx, stack);
    return stack.id;
}

tt_int32 TelemSendCallStackSkip(TraceContexHandle ctx, const TtCallStack* pStack, tt_int32 numToSkip)
{
    TELEM_UNUSED(numToSkip);
    return TelemSendCallStack(ctx, pStack);
}

// ----------- Zones -----------

// TODO: have a overload that don't take args so can skip the conditional?
void TelemEnter(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = getThreadData(pCtx);

    if (!pCtx->isEnabled | (pThreadData == nullptr)) {
        return;
    }

    auto depth = pThreadData->stackDepth;
    ++pThreadData->stackDepth;

    auto& scopeData = pThreadData->zones[depth];
    scopeData.zone.start = getTicks(); // Moving this call to the bottom slows the function down a few ns.
    scopeData.zone.pFmtStr = pFmtString;
    scopeData.zone.sourceInfo = sourceInfo;

    // for this arg data it would be nicer to be like a linera array.
    // basically a fixed sized buffer we just offset from.
    // can do that later tho.
    if (numArgs)
    {
        va_list l;
        va_start(l, numArgs);

        scopeData.argDataSize = BuildArgData(pThreadData->argData[depth], pFmtString, numArgs, l);

        va_end(l);
    }
    else
    {
        scopeData.argDataSize = 0;
    }
}

void TelemLeave(TraceContexHandle ctx)
{
    auto ticks = getTicks();
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | (pThreadData == nullptr)) {
        return;
    }

    auto depth = --pThreadData->stackDepth;

    auto& scopeData = pThreadData->zones[depth];
    scopeData.zone.end = ticks;

    queueZone(pCtx, pThreadData, depth);
}

void TelemEnterEx(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, tt_uint64& matchIdOut, tt_uint64 minMicroSec, const char* pFmtString, tt_int32 numArgs, ...)
{
    // This is a copy of TelemEnter since can't pick up the va_args later unless we always pass them.
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = getThreadData(pCtx);

    if (!pCtx->isEnabled | (pThreadData == nullptr)) {
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

        scopeData.argDataSize = BuildArgData(pThreadData->argData[depth], pFmtString, numArgs, l);

        va_end(l);
    }
    else
    {
        scopeData.argDataSize = 0;
    }
}


void TelemLeaveEx(TraceContexHandle ctx, tt_uint64 matchId)
{
    auto ticks = getTicks();
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | (pThreadData == nullptr)) {
        return;
    }

    auto depth = --pThreadData->stackDepth;

    auto& scopeData = pThreadData->zones[depth];
    scopeData.zone.end = ticks;

    // work out if we send it.
    auto minMicroSec = matchId;
    auto elpased = scopeData.zone.end - scopeData.zone.start;
    auto elapsedNano = ticksToNano(pCtx, elpased);

    if (elapsedNano > minMicroSec * 1000) {
        return;
    }

    queueZone(pCtx, pThreadData, depth);
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
    auto ticks = getTicks();
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
    lock.start = ticks;
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
    auto ticks = getTicks();
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
    lock.start = ticks;
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

void TelemEndTryLock(TraceContexHandle ctx, const void* pPtr, TtLockResult::Enum result)
{
    auto ticks = getTicks();
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | (pThreadData == nullptr)) {
        return;
    }

    auto* pLock = getLockAndClearSlot(pThreadData, pPtr);
    if (!pLock) {
        return;
    }

    auto& lock = pLock->lock;
    lock.end = ticks;
    lock.result = result;
    lock.depth = static_cast<decltype(lock.depth)>(pThreadData->stackDepth);

    queueLockTry(pCtx, pThreadData, pPtr, pLock);
}

void TelemEndTryLockEx(TraceContexHandle ctx, tt_uint64 matchId, const void* pPtr, TtLockResult::Enum result)
{
    auto ticks = getTicks();
    auto* pCtx = handleToContext(ctx);
    auto* pThreadData = gThreadData;

    if (!pCtx->isEnabled | (pThreadData == nullptr)) {
        return;
    }

    auto* pLock = getLockAndClearSlot(pThreadData, pPtr);
    if (!pLock) {
        return;
    }

    auto& lock = pLock->lock;
    lock.end = ticks;
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

void TelemSetLockState(TraceContexHandle ctx, const TtSourceInfo& sourceInfo, const void* pPtr, TtLockState::Enum state)
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
    data.time = getRelativeTicks(pCtx);
    data.pPtr = pPtr;
    data.threadID = getThreadID();
    data.sourceInfo = sourceInfo;

    addToTickBuffer(pCtx, &data, sizeof(data), GetSizeNotArgData<decltype(data)>());
}

// ----------- Plot stuff -----------

void TelemPlotF32(TraceContexHandle ctx, TtPlotType::Enum type, float value, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataPlot data;
    data.type = QueueDataType::Plot;
    data.time = getRelativeTicks(pCtx);
    data.value.plotType = type;
    data.value.valueType = TtPlotValueType::f32;
    data.value.f32 = value;
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

void TelemPlotF64(TraceContexHandle ctx, TtPlotType::Enum type, double value, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataPlot data;
    data.type = QueueDataType::Plot;
    data.time = getRelativeTicks(pCtx);
    data.value.plotType = type;
    data.value.valueType = TtPlotValueType::f64;
    data.value.f64 = value;
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

void TelemPlotI32(TraceContexHandle ctx, TtPlotType::Enum type, tt_int32 value, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataPlot data;
    data.type = QueueDataType::Plot;
    data.time = getRelativeTicks(pCtx);
    data.value.plotType = type;
    data.value.valueType = TtPlotValueType::Int32;
    data.value.int32 = value;
    data.pFmtStr = pFmtString;

    if (!numArgs)
    {
        constexpr auto copySize = GetSizeWithoutArgDataNoAlign<decltype(data)>();
        constexpr auto size = GetSizeWithoutArgData<decltype(data)>();
        data.argDataSize = 0;
        addToTickBuffer(pCtx, &data, copySize, size);
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

void TelemPlotI64(TraceContexHandle ctx, TtPlotType::Enum type, tt_int64 value, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataPlot data;
    data.type = QueueDataType::Plot;
    data.time = getRelativeTicks(pCtx);
    data.value.plotType = type;
    data.value.valueType = TtPlotValueType::Int64;
    data.value.int64 = value;
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

void TelemPlotU32(TraceContexHandle ctx, TtPlotType::Enum type, tt_uint32 value, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataPlot data;
    data.type = QueueDataType::Plot;
    data.time = getRelativeTicks(pCtx);
    data.value.plotType = type;
    data.value.valueType = TtPlotValueType::UInt32;
    data.value.uint32 = value;
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

void TelemPlotU64(TraceContexHandle ctx, TtPlotType::Enum type, tt_uint64 value, const char* pFmtString, tt_int32 numArgs, ...)
{
    auto* pCtx = handleToContext(ctx);
    if (!pCtx->isEnabled) {
        return;
    }

    QueueDataPlot data;
    data.type = QueueDataType::Plot;
    data.time = getRelativeTicks(pCtx);
    data.value.plotType = type;
    data.value.valueType = TtPlotValueType::UInt64;
    data.value.uint64 = value;
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

// ----------- Message Stuff -----------


void TelemMessage(TraceContexHandle ctx, TtMsgType::Enum type, const char* pFmtString, tt_int32 numArgs, ...)
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
