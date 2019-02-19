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

        QueryApps,
        QueryAppsResp,
        QueryAppTraces,
        QueryAppTracesResp,

        QuerySrvStats,
        QuerySrvStatsResp,

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
};

struct ConnectionRequestViewerHdr : public PacketBase
{
    VersionInfo viewerVer;
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

struct QueryApps : public PacketBase
{
    tt_int32 offset;
    tt_int32 max;
};

struct QueryAppsResponseHdr : public PacketBase
{
    tt_int32 num;   // how many returned in request
    tt_int32 total; // total on server.
};

// TODO: variable length strings, or just compress :P ?
struct QueryAppsResponseData
{
    tt_int32 numTraces;
    char appName[MAX_STRING_LEN];
};

struct QueryAppTraces : public PacketBase
{
    char appName[MAX_STRING_LEN];
};

struct QueryAppTracesResponseHdr : public PacketBase
{
    tt_int32 num;
};

struct QueryAppTracesResponseData
{
    char name[MAX_STRING_LEN];
    char buildInfo[MAX_STRING_LEN];
};


struct QueryServerStatsReponse
{
    tt_int32 activeTraces;
    tt_int64 bpsIngest;
    tt_int64 storageUsed;
};


TELEM_PACK_POP;

static_assert(sizeof(VersionInfo) == 4, "Incorrect size");
static_assert(sizeof(PacketBase) == 3, "Incorrect size");
static_assert(sizeof(ConnectionRequestHdr) == 21, "Incorrect size");
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

        Num
    };
};


using TtthreadId = tt_uint32;
using TtZoneId = tt_uint32;

struct DataPacketBase
{
    DataStreamType::Enum type;
};

struct DataPacketStringTableAdd : public DataPacketBase
{
    tt_uint16 id;
    tt_uint16 length;
};

TELEM_PACK_PUSH(4)

struct DataPacketZone : public DataPacketBase
{
    // 3
    tt_int8 stackDepth;
    tt_int8  _pad0[2];

    // 4
    TtthreadId threadID;

    // 16
    tt_uint64 start;
    tt_uint64 end;

    // 8
    tt_uint16        lineNo;
    StringTableIndex strIdxFunction;
    StringTableIndex strIdxFile;
    StringTableIndex strIdxZone;
};

struct DataPacketTickInfo : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 16
    tt_uint64 ticks;
    tt_uint64 timeMicro;
};

struct DataPacketThreadSetName : public DataPacketBase
{
    // 2
    StringTableIndex strIdxName;
    // 4
    TtthreadId threadID;
    // 8
    tt_uint64 time;
};

struct DataPacketCallStack : public DataPacketBase
{
    // 4
    TtthreadId threadID;
};

struct DataPacketLockSetName : public DataPacketBase
{
    // 2
    StringTableIndex strIdxName;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 lockHandle;
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

    // 2
    StringTableIndex strIdxDescrption;
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
};

struct DataPacketLockCount : public DataPacketBase
{
    // 2
    tt_uint16 count;

    // 4
    TtthreadId threadID;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 lockHandle;
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
};

struct DataPacketMemFree : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 8
    tt_uint64 time;

    // 8
    tt_uint64 ptr;
};


static_assert(sizeof(DataPacketBase) == 1, "Incorrect size");
static_assert(sizeof(DataPacketStringTableAdd) == 6, "Incorrect size");
static_assert(sizeof(DataPacketZone) == 32, "Incorrect size");
static_assert(sizeof(DataPacketThreadSetName) == 16, "Incorrect size");
static_assert(sizeof(DataPacketLockSetName) == 20, "Incorrect size");
static_assert(sizeof(DataPacketLockTry) == 36, "Incorrect size");
static_assert(sizeof(DataPacketLockState) == 28, "Incorrect size");
static_assert(sizeof(DataPacketLockCount) == 24, "Incorrect size");
static_assert(sizeof(DataPacketMemAlloc) == 28, "Incorrect size");
static_assert(sizeof(DataPacketMemFree) == 24, "Incorrect size");


TELEM_PACK_POP