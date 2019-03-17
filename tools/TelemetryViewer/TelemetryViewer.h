#pragma once

#include <Threading/Signal.h>
#include <Containers/FixedHashTable.h>

X_NAMESPACE_BEGIN(telemetry)

struct Region
{
    bool active = false;
    int64_t start;
    int64_t end;
};

struct Animation
{
    bool active = false;
    int64_t start0, start1;
    int64_t end0, end1;
    double progress;
    double lenMod;
};

// --------------------------------

struct ZoneData
{
    // 16
    int64_t startNano;
    int64_t endNano;

    // 16
    int64_t startTicks;
    int64_t endTicks;

    // 8
    int16_t lineNo;
    int16_t strIdxFunction;
    int16_t strIdxFile;
    int16_t strIdxZone;

    // 1 :/
    int8_t stackDepth;
};

struct TickData
{
    int64_t start;
    int64_t end;

    int64_t startNano;
    int64_t endNano;
};

struct LockState
{
    int64_t time;
    int64_t timeNano;

    TtLockState state;
    uint16_t threadIdx; // used for colors.

    uint32_t threadID; // TODO: can just use threadIdx?

    // 8
    uint16_t lineNo;
    uint16_t strIdxFunction;
    uint16_t strIdxFile;
};

struct LockTry
{
    uint64_t lockHandle;

    int64_t startTick;
    int64_t endTick;

    int64_t startNano;
    int64_t endNano;

    uint32_t threadID; // TODO: can just use threadIdx?

    uint16_t threadIdx; // used for colors.
    TtLockResult result;

    // 8
    uint16_t lineNo;
    uint16_t strIdxFunction;
    uint16_t strIdxFile;
    uint16_t strIdxDescrption;
};


struct LockData
{
    using LockStateArr = core::ArrayGrowMultiply<LockState>;
    using LockTryArr = core::ArrayGrowMultiply<LockTry>;

    LockData(uint64_t lockHandle, core::MemoryArenaBase* arena) :
        lockHandle(lockHandle),
        lockStates(arena),
        lockTry(arena)
    {
    }

    uint64_t lockHandle;

    LockStateArr lockStates;
    LockTryArr lockTry;
};

using LockDataMap = core::FixedHashTable<uint64_t, LockData>;

struct StackLevelData
{
    using ZoneDataArr = core::ArrayGrowMultiply<ZoneData>;
    using LockTryArr = core::ArrayGrowMultiply<LockTry>;

public:
    StackLevelData(core::MemoryArenaBase* arena) :
        zones(arena),
        lockTry(arena)
    {}


    ZoneDataArr zones;
    LockTryArr lockTry;
};

struct ZoneSegmentThread
{
    // this is all the zones for this thread.
    using StackLevelDataArr = core::FixedArray<StackLevelData, MAX_ZONE_DEPTH>;


public:
    ZoneSegmentThread(ZoneSegmentThread&& oth) = default;
    ZoneSegmentThread(uint32_t id, core::MemoryArenaBase* arena) :
        id(id),
        locks(arena, MAX_LOCKS)
    {
        X_UNUSED(arena);
    }

    uint32_t id;
    
    StackLevelDataArr levels;
    LockDataMap locks;
};

struct ZoneSegment
{
    using ZoneSegmentThreadArr = core::ArrayGrowMultiply<ZoneSegmentThread>;
    using TickDataArr = core::ArrayGrowMultiply<TickData>;

public:
    ZoneSegment(ZoneSegment&& oth) = default;
    ZoneSegment(core::MemoryArenaBase* arena) :
        startNano(0),
        endNano(0),
        ticks(arena),
        threads(arena),
        locks(arena, MAX_LOCKS)
    {
    }

    // this should just be for a range of time.
    // it may have no ticks, should probs move ticks out of this then.
    int64_t startNano;
    int64_t endNano;

    TickDataArr ticks;
    ZoneSegmentThreadArr threads;

    LockDataMap locks;
};

struct TraceStrings
{
    struct StringHdr
    {
        tt_int16 length;
    };

    using OffsetArr = core::Array<int32_t>;
    using TraceThreadNameDataArr = core::Array<TraceThreadNameData>;
    using TraceLockNameDataArr = core::Array<TraceLockNameData>;

public:
    TraceStrings(core::MemoryArenaBase* arena) :
        threadNames(arena),
        lockNames(arena),
        idxOffset(0),
        lookUp(arena),
        data(arena)
    {
    }

    void init(int32_t num, int32_t minId, int32_t maxId, int32_t strDataSize) {

        const int32_t range = (maxId - minId) + 1;

        const int32_t extraBytesPerString = 3;
        const int32_t totalBytes = strDataSize + (num * extraBytesPerString);

        idxOffset = minId;
        lookUp.resize(range, -1);
        data.reserve(totalBytes);
    }

    void addString(int16_t id, int16_t len, const char* pData) {
        auto idx = id - idxOffset;

        auto offset = safe_static_cast<int32_t>(data.size());

        StringHdr hdr;
        hdr.length = len;

        data.write(hdr);
        data.write(pData, len);
        data.write('\0');

        X_ASSERT(lookUp[idx] == -1, "Index taken")(idx);
        lookUp[idx] = offset;
    }

    core::string_view getThreadName(uint32_t threadId) const {

        const TraceThreadNameData* pData = nullptr;

        for (int32_t i = 0; i < threadNames.size(); i++)
        {
            auto& name = threadNames[i];
            if (name.threadId == threadId)
            {
                if (!pData)
                {
                    pData = &name;
                }
                else
                {
                    // TODO: is this a better fit based on time? 

                }
            }
        }

        if (pData) {
            return getString(pData->strIdx);
        }

        return core::string_view("???");
    }

    core::string_view getLockName(uint64_t lockId) const {

        for (int32_t i = 0; i < lockNames.size(); i++)
        {
            auto& name = lockNames[i];
            if (name.lockId == lockId)
            {
                return getString(name.strIdx);
            }
        }

        return core::string_view("???");
    }

    core::string_view getString(int16_t id) const {

        using namespace core::string_view_literals;

        // TODO: check in debug builds only?
        if (id < idxOffset) {
            return "???"_sv;
        }

        auto idx = id - idxOffset;
        auto offset = lookUp[idx];

        if (offset < 0) {
            return "???"_sv;
        }

        auto* pHdr = reinterpret_cast<const StringHdr*>(data.data() + offset);
        auto* pStr = reinterpret_cast<const char*>(pHdr + 1);

        return { pStr, static_cast<uint32_t>(pHdr->length) };
    }

    TraceThreadNameDataArr threadNames;
    TraceLockNameDataArr lockNames;

    int32_t idxOffset;
    OffsetArr lookUp;

    core::ByteStream data;
};

struct TraceView
{
    using ZoneSegmentArr = core::Array<ZoneSegment>;
    using TraceLockDataArr = core::Array<TraceLockData>;

public:
    TraceView(core::Guid guid, uint64_t ticksPerMicro, TraceStats stats, int8_t handle, core::MemoryArenaBase* arena) :
        guid(guid),
        ticksPerMicro(ticksPerMicro),
        stats(stats),
        handle(handle),
        segments(arena),
        locks(arena),
        strings(arena)
    {
        open_ = true;

    //    numFrames_ = 1024 * 1024;
    //    frameStart_ = 0;
    //    frameScale_ = 0;

        zvStartNS_ = 1000 * 1000 * 800;
        zvEndNS_ = 1000 * 1000 * 1500;

        zvHeight_ = 0;
        zvScroll_ = 0;

        tabName.set("Trace - Zones");
    }

    X_INLINE int64_t GetTimeBegin(void) const
    {
        // we always start from zero?
        return 0;
    }

    X_INLINE int64_t GetVisiableNS(void) const
    {
        return zvEndNS_ - zvStartNS_;
    }

    X_INLINE int64_t GetVisibleStartNS(void) const
    {
        return zvStartNS_ - GetTimeBegin();
    }

    X_INLINE uint64_t ticksToNano(uint64_t tsc) const
    {
        const uint64_t whole = (tsc / ticksPerMicro) * 1000;
        const uint64_t part = (tsc % ticksPerMicro) * 1000 / ticksPerMicro;

        return whole + part;
    }

    core::StackString<64,char> tabName;

    bool paused_; // don't auto scroll
    bool open_;
    bool _pad[2];

    core::Guid guid;
    uint64_t ticksPerMicro;
    TraceStats stats;
    int8_t handle;

    // int32_t numFrames_;
    // int32_t frameStart_;
    // int32_t frameScale_;

    int64_t zvStartNS_;
    int64_t zvEndNS_;

    int32_t zvHeight_;
    int32_t zvScroll_;

    Region highlight_;
    Region highlightZoom_;
    Animation zoomAnim_;

    ZoneSegmentArr segments;
    TraceLockDataArr locks;
    TraceStrings strings;
};

using GuidTraceStats = std::pair<core::Guid, TraceStats>;

struct Client
{
    using GuidTraceStatsArr = core::Array<GuidTraceStats>;
    using TraceViewArr = core::Array<TraceView>;

    enum class ConnectionState {
        Offline,
        Connecting,
        Connected,
    };

    Client(core::MemoryArenaBase* arena);

    bool isConnected(void) const;
    void sendDataToServer(const void* pData, int32_t len);
    TraceView* viewForHandle(tt_int8 handle);

    core::StackString256 addr;
    uint16_t port;
    ConnectionState conState;
    core::Signal connectSignal;

    platform::SOCKET socket;
    VersionInfo serverVer;

    // Server compress it's responses.
    int32_t cmpBufferOffset;
    int8_t cmpRingBuf[COMPRESSION_RING_BUFFER_SIZE];

    core::Compression::LZ4StreamDecode lz4DecodeStream;

    // stuff set by background thread..?
    char _pad[64]; // todo align.

    core::CriticalSection dataCS;

    TraceAppArr apps;

    GuidTraceStatsArr traceStats;
    TraceViewArr views;
};


bool run(Client& client);

X_NAMESPACE_END