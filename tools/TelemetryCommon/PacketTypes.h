#pragma once

#include "Types.h"
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

        // Used by viewer.
        AppList,
        // AppTraceList, <- we just send the traces with the apps.
        
        QueryTraceInfo,
        QueryTraceInfoResp,

        OpenTrace,
        OpenTraceResp,

        ReqTraceZoneSegment,
        ReqTraceLocks,
        ReqTraceStrings,
        ReqTraceThreadNames,
        ReqTraceLockNames,

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
constexpr tt_size MAX_STRING_LEN = 256;

// TODO: move?
constexpr tt_size MAX_ZONE_THREADS = 32;
constexpr tt_size MAX_ZONE_DEPTH = 32;
constexpr tt_size MAX_LOCKS_HELD_PER_THREAD = 16;
constexpr tt_size MAX_LOCKS = 128; // max locks we can track

constexpr tt_size COMPRESSION_MAX_INPUT_SIZE = 1024 * 8;
constexpr tt_size COMPRESSION_RING_BUFFER_SIZE = 1024 * 64;

constexpr tt_size STRING_TABLE_BUF_SIZE = sizeof(void*) * 1024; // TODO: ?

constexpr tt_size BACKGROUND_THREAD_STACK_SIZE_BASE = 1024 * 8; // base size for anything that's not a compression buffer.
constexpr tt_size BACKGROUND_THREAD_STACK_SIZE = Internal::RoundUpToMultiple<tt_size>(
    COMPRESSION_RING_BUFFER_SIZE + 
    MAX_PACKET_SIZE + 
    BACKGROUND_THREAD_STACK_SIZE_BASE + 
    STRING_TABLE_BUF_SIZE, 
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
};

struct ConnectionRequestAcceptedHdr : public PacketBase
{
    VersionInfo serverVer;
};

struct ConnectionRequestRejectedHdr : public PacketBase
{
    tt_uint16 reasonLen;
};

struct DataStreamHdr : public PacketBase
{
    tt_uint16 origSize;
};

TELEM_PACK_POP;

static_assert(sizeof(VersionInfo) == 4, "Incorrect size");
static_assert(sizeof(PacketBase) == 3, "Incorrect size");
static_assert(sizeof(ConnectionRequestHdr) == 29, "Incorrect size");
static_assert(sizeof(ConnectionRequestAcceptedHdr) == 7, "Incorrect size");
static_assert(sizeof(ConnectionRequestRejectedHdr) == 5, "Incorrect size");
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
        CallStack,
        LockSetName,
        LockTry,
        LockState,
        LockCount,
        MemAlloc,
        MemFree,
        Message,

        Num
    };
};

TELEM_PACK_PUSH(1)

struct DataPacketBase
{
    DataStreamType::Enum type;
};

struct DataPacketStringTableAdd : public DataPacketBase
{
    tt_uint16 id;
    tt_uint16 length;
};

struct DataPacketZone : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 16
    tt_uint64 start;
    tt_uint64 end;

    // 8
    tt_uint16 lineNo;
    tt_uint16 strIdxFunction;
    tt_uint16 strIdxFile;
    tt_uint16 strIdxZone;

    // 2
    tt_int8 stackDepth;
    tt_uint8 argDataSize;
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

struct DataPacketThreadSetName : public DataPacketBase
{
    // 8
    tt_uint64 time;
    // 4
    TtthreadId threadID;
    // 3
    tt_uint16 strIdxName;
    tt_uint8 argDataSize;
};

struct DataPacketCallStack : public DataPacketBase
{
    // 4
    TtthreadId threadID;
};

struct DataPacketLockSetName : public DataPacketBase
{
    // 8
    tt_uint64 time;

    // 8
    tt_uint64 lockHandle;

    // 3
    tt_uint16 strIdxName;
    tt_uint8 argDataSize;
};

struct DataPacketLockTry : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 16
    tt_uint64 start;
    tt_uint64 end;

    // 8
    tt_uint64 lockHandle;

    // 8
    tt_uint16 lineNo;
    tt_uint16 strIdxFunction;
    tt_uint16 strIdxFile;
    tt_uint16 strIdxDescrption;

    // 3
    TtLockResult result;
    tt_uint8 depth;
    tt_uint8 argDataSize;
};


struct DataPacketLockState : public DataPacketBase
{
    // 4
    TtLockState state;

    // 4
    TtthreadId threadID;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 lockHandle;

    // 6
    tt_uint16 lineNo;
    tt_uint16 strIdxFunction;
    tt_uint16 strIdxFile;
    // 1
    tt_uint8 argDataSize;
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
    tt_uint16 lineNo;
    tt_uint16 strIdxFunction;
    tt_uint16 strIdxFile;
};

struct DataPacketMemAlloc : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 4
    tt_uint32 size;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 ptr;

    // 6
    tt_uint16 lineNo;
    tt_uint16 strIdxFunction;
    tt_uint16 strIdxFile;
 
    tt_uint8 argDataSize;
};

struct DataPacketMemFree : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 ptr;

    // 6
    tt_uint16 lineNo;
    tt_uint16 strIdxFunction;
    tt_uint16 strIdxFile;

    tt_uint8 argDataSize;
};

struct DataPacketMessage : public DataPacketBase
{
    // 8
    tt_uint64 time;

    // 4
    tt_uint16 strIdxFmt;
    tt_uint8 logType;
    tt_uint8 argDataSize;
};

static_assert(sizeof(DataPacketBase) == 1, "Incorrect size");
static_assert(sizeof(DataPacketStringTableAdd) == 5, "Incorrect size");
static_assert(sizeof(DataPacketZone) == 32, "Incorrect size");
static_assert(sizeof(DataPacketThreadSetName) == 16, "Incorrect size");
static_assert(sizeof(DataPacketLockSetName) == 20, "Incorrect size");
static_assert(sizeof(DataPacketLockTry) == 40, "Incorrect size");
static_assert(sizeof(DataPacketLockState) == 32, "Incorrect size");
static_assert(sizeof(DataPacketLockCount) == 29, "Incorrect size");
static_assert(sizeof(DataPacketMemAlloc) == 32, "Incorrect size");
static_assert(sizeof(DataPacketMemFree) == 28, "Incorrect size");


TELEM_PACK_POP