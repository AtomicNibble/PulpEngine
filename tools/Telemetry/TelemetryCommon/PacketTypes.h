#pragma once

#include <../Telemetry/Telemetry.h>
#include "Compiler.h"
#include "StringTable.h"

#include <cstdio>

struct PacketType
{
    enum Enum : tt_uint8
    {
        ConnectionRequest,
        ConnectionRequestViewer,
        ConnectionRequestAccepted,
        ConnectionRequestRejected,
        DataStream,
        ReqPDB, // sent by server to runtime.

        // Used by viewer.
        AppList,
        TraceEnded,
        // AppTraceList, <- we just send the traces with the apps.
        
        QueryTraceInfo,
        QueryTraceInfoResp,

        OpenTrace,
        OpenTraceResp,

        ReqTraceZoneSegment,
        ReqTraceLocks,
        ReqTraceStrings,
        ReqTraceThreadNames,
        ReqTraceThreadGroupNames,
        ReqTraceThreadGroups,
        ReqTraceLockNames,
        ReqTraceZoneTree,
        ReqTraceMessages,

        Num
    };
};

struct VersionInfo
{
    typedef char Description[64];

    bool operator==(const VersionInfo& oth) const {
        return major == oth.major &&
            minor == oth.minor &&
            patch == oth.patch &&
            build == oth.build;
    }

    bool operator!=(const VersionInfo& oth) const {
        return !(*this == oth);
    }

    const char* toString(Description& desc) const {
        sprintf(desc, "%hhu.%hhu.%hhu.%hhu", major, minor, patch, build);
        return desc;
    }

    tt_uint8 major;
    tt_uint8 minor;
    tt_uint8 patch;
    tt_uint8 build;
};

namespace Internal
{
    template<typename T>
    inline constexpr T RoundUpToMultiple(T numToRound, T multipleOf)
    {
        return (numToRound + multipleOf - 1) & ~(multipleOf - 1);
    }
}

// if i set this to datagram size it's too slow.
// need to tune this with data for a real program.
constexpr tt_size MAX_PACKET_SIZE = 1024 * 16; 
constexpr tt_size MAX_CMDLINE_LEN = 1024 * 8;
constexpr tt_size MAX_STRING_LEN = 255;

// TODO: move?
constexpr tt_size MAX_ZONE_THREADS = 16;
constexpr tt_size MAX_ZONE_DEPTH = 32;
constexpr tt_size MAX_LOCKS_HELD_PER_THREAD = 16;
constexpr tt_size MAX_LOCKS = 128; // max locks we can track
constexpr tt_size MAX_STATIC_STRINGS = 1024 * 4;

constexpr tt_size MAX_PDB_DATA_BLOCK_SIZE = 1024 * 4; // must be smaller than COMPRESSION_MAX_INPUT_SIZE

constexpr tt_size COMPRESSION_MAX_INPUT_SIZE = 1024 * 8;
constexpr tt_size COMPRESSION_RING_BUFFER_SIZE = 1024 * 64;

constexpr tt_size STRING_TABLE_BUF_SIZE = sizeof(void*) * MAX_STATIC_STRINGS;
constexpr tt_size CALLSTACK_CACHE_BUF_SIZE = sizeof(tt_uint32) * 2048;

constexpr tt_size BACKGROUND_THREAD_STACK_SIZE_BASE = 1024 * 32; // base size for anything that's not a compression buffer.
constexpr tt_size BACKGROUND_THREAD_STACK_SIZE = Internal::RoundUpToMultiple<tt_size>(
    COMPRESSION_RING_BUFFER_SIZE + 
    (MAX_PACKET_SIZE * 2) + 
    BACKGROUND_THREAD_STACK_SIZE_BASE + 
    STRING_TABLE_BUF_SIZE +
    CALLSTACK_CACHE_BUF_SIZE +
    (MAX_PDB_DATA_BLOCK_SIZE * 2),
    1024 * 4
);

using TtthreadId = tt_uint32;
using TtZoneId = tt_uint32;


TELEM_PACK_PUSH(1)

struct PacketBase
{
    tt_uint16 dataSize;
    PacketType::Enum type;
};

struct ConnectionRequestHdr : public PacketBase
{
    VersionInfo clientVer;

    tt_uint16 appNameLen;
    tt_uint16 buildInfoLen;
    tt_uint16 cmdLineLen;
    tt_uint64 ticksPerMicro;
    tt_uint64 ticksPerMs;
    tt_uint64 unixTimestamp;
    tt_uint32 workerThreadID;
    tt_uint32 connFlags;
};

struct ConnectionRequestAcceptedHdr : public PacketBase
{
    VersionInfo serverVer;
};

struct ConnectionRequestRejectedHdr : public PacketBase
{
    tt_uint16 reasonLen;
};

struct RequestPDBHdr : public PacketBase
{
    // need some info about the PDB we want.
    tt_uint64 modAddr;
    tt_uint32 imageSize;

    tt_uint8 guid[16];
    tt_uint32 age;
};

struct DataStreamHdr : public PacketBase
{
    tt_uint16 origSize;
};

TELEM_PACK_POP;

static_assert(sizeof(VersionInfo) == 4, "Incorrect size");
static_assert(sizeof(PacketBase) == 3, "Incorrect size");
static_assert(sizeof(ConnectionRequestHdr) == 45, "Incorrect size");
static_assert(sizeof(ConnectionRequestAcceptedHdr) == 7, "Incorrect size");
static_assert(sizeof(ConnectionRequestRejectedHdr) == 5, "Incorrect size");
static_assert(sizeof(RequestPDBHdr) == 35, "Incorrect size");
static_assert(sizeof(DataStreamHdr) == 5, "Incorrect size");

// Not packet types but part of data
// TODO: move?

struct DataStreamType
{
    enum Enum : tt_uint8
    {
        StringTableAdd,

        Zone,
        TickInfo,
        ThreadSetName,
        ThreadSetGroup,
        ThreadSetGroupName,
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
        PDBInfo,
        PDB,
        PDBBlock,
        PDBError,

        Num
    };
};

TELEM_PACK_PUSH(1)

struct DataPacketBase
{
    DataStreamType::Enum type;
};

struct DataPacketBaseArgData : public DataPacketBase
{
    tt_uint8 argDataSize;
};


struct DataPacketStringTableAdd : public DataPacketBase
{
    tt_uint16 id;
    tt_uint16 length;
};

struct DataPacketZone : public DataPacketBaseArgData
{
    // 4
    TtthreadId threadID;

    // 16
    tt_uint64 start;
    tt_uint64 end;

    // 8
    tt_uint32 lineNo;
    tt_uint16 strIdxFile;
    tt_uint16 strIdxFmt;

    // 2
    tt_int8 stackDepth;
    tt_uint8 _pad;
};

struct DataPacketTickInfo : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 16
    tt_uint64 start;
    tt_uint64 end;
    tt_uint64 startNano;
    tt_uint64 endNano;
};

struct DataPacketThreadSetName : public DataPacketBaseArgData
{
    // 8
    tt_uint64 time;
    // 4
    TtthreadId threadID;
    // 2
    tt_uint16 strIdxFmt;
};

struct DataPacketThreadSetGroup : public DataPacketBase
{
    // 4
    TtthreadId threadID;
    // 4
    tt_int32 groupID;
};

struct DataPacketThreadSetGroupName : public DataPacketBaseArgData
{
    // 4
    tt_int32 groupID;
    // 2
    tt_uint16 strIdxFmt;
};

struct DataPacketThreadSetGroupSort : public DataPacketBase
{
    // 4
    tt_int32 groupID;
    // 4
    tt_int32 sortVal;
};

struct DataPacketCallStack : public DataPacketBase
{
    tt_uint32 id;   // this is currently hash.
    tt_uint32 numFrames;

    // tt_uint64 frames[1];
};

struct DataPacketLockSetName : public DataPacketBaseArgData
{
    // 8
    tt_uint64 time;

    // 8
    tt_uint64 lockHandle;

    // 3
    tt_uint16 strIdxFmt;
};

struct DataPacketLockTry : public DataPacketBaseArgData
{
    // 4
    TtthreadId threadID;

    // 16
    tt_uint64 start;
    tt_uint64 end;

    // 8
    tt_uint64 lockHandle;

    // 8
    tt_uint32 lineNo;
    tt_uint16 strIdxFile;
    tt_uint16 strIdxFmt;

    // 2
    tt_uint8 result;
    tt_uint8 depth;
};


struct DataPacketLockState : public DataPacketBaseArgData
{
    // 1
    tt_uint8 state;

    // 4
    TtthreadId threadID;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 lockHandle;

    // 8
    tt_uint32 lineNo;
    tt_uint16 strIdxFile;
    tt_uint16 strIdxFmt;
};

struct DataPacketLockCount : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 lockHandle;

    // 2
    tt_uint16 count;

    // 6
    tt_uint32 lineNo;
    tt_uint16 strIdxFile;
};

struct DataPacketMemAlloc : public DataPacketBaseArgData
{
    // 4
    TtthreadId threadID;

    // 4
    tt_uint32 size;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 ptr;

    // 8
    tt_uint32 lineNo;
    tt_uint16 strIdxFile;
    tt_uint16 strIdxFmt;
};

struct DataPacketMemFree : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 ptr;

    // 8
    tt_uint32 lineNo;
    tt_uint16 strIdxFile;
};

struct DataPacketMessage : public DataPacketBaseArgData
{
    // 8
    tt_uint64 time;

    // 4
    tt_uint16 strIdxFmt;
    tt_uint8 flags;
};

struct TtPlotValueType
{
    enum Enum : tt_uint8
    {
        Int32,
        UInt32,
        Int64,
        UInt64,
        f32,
        f64
    };
};

struct TtPlotValue
{
    tt_uint8 plotType;
    TtPlotValueType::Enum valueType;

    union {
        tt_int32 int32;
        tt_uint32 uint32;
        tt_int64 int64;
        tt_uint64 uint64;
        float f32;
        double f64;
    };
};

struct DataPacketPlot : public DataPacketBaseArgData
{
    // 8
    tt_uint64 time;

    // 2
    tt_uint16 strIdxFmt;
    TtPlotValue value;
};

struct DataPacketPDBInfo : public DataPacketBaseArgData
{
    tt_uint64 modAddr;
    tt_uint32 imageSize;

    tt_uint8 guid[16];
    tt_uint32 age;

    tt_uint16 strIdxName;
};

// the server will request this.
struct DataPacketPDB : public DataPacketBaseArgData
{
    tt_uint64 modAddr;
    tt_uint32 imageSize;
    tt_uint32 fileSize;

    tt_uint8 guid[16];
    tt_uint32 age;
};

struct DataPacketPDBBlock : public DataPacketBaseArgData
{
    tt_uint64 modAddr; // something to link the blocks.

    tt_uint32 blockSize;
    tt_uint32 offset;
};

struct DataPacketPDBError : public DataPacketBaseArgData
{
    tt_uint64 modAddr;
};



static_assert(sizeof(DataPacketBase) == 1, "Incorrect size");
static_assert(sizeof(DataPacketStringTableAdd) == 5, "Incorrect size");
static_assert(sizeof(DataPacketZone) == 32, "Incorrect size");
static_assert(sizeof(DataPacketThreadSetName) == 16, "Incorrect size");
static_assert(sizeof(DataPacketLockSetName) == 20, "Incorrect size");
static_assert(sizeof(DataPacketLockTry) == 40, "Incorrect size");
static_assert(sizeof(DataPacketLockState) == 31, "Incorrect size");
static_assert(sizeof(DataPacketLockCount) == 29, "Incorrect size");
static_assert(sizeof(DataPacketMemAlloc) == 34, "Incorrect size");
static_assert(sizeof(DataPacketMemFree) == 27, "Incorrect size");
static_assert(sizeof(DataPacketMessage) == 13, "Incorrect size");
static_assert(sizeof(DataPacketPlot) == 22, "Incorrect size");
static_assert(sizeof(DataPacketPDBInfo) == 36, "Incorrect size");
static_assert(sizeof(DataPacketPDB) == 38, "Incorrect size");
static_assert(sizeof(DataPacketPDBBlock) == 18, "Incorrect size");


TELEM_PACK_POP
