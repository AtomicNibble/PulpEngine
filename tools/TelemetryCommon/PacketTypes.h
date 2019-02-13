#pragma once

#include "Types.h"
#include "StringTable.h"

struct PacketType
{
    enum Enum : tt_uint8
    {
        ConnectionRequest,
        ConnectionRequestAccepted,
        ConnectionRequestRejected,
        DataStream,

        Num
    };
};

struct VersionInfo
{
    bool operator==(const VersionInfo& oth) const {
        return major == oth.major &&
            minor == oth.minor &&
            patch == oth.patch &&
            build == oth.build;
    }

    bool operator!=(const VersionInfo& oth) const {
        return !(*this == oth);
    }

    tt_uint8 major;
    tt_uint8 minor;
    tt_uint8 patch;
    tt_uint8 build;
};

constexpr tt_size MAX_PACKET_SIZE = 1450;
constexpr tt_size MAX_APP_NAME_LEN = 64;
constexpr tt_size MAX_BUILD_INFO_LEN = 128;
constexpr tt_size MAX_CMDLINE_LEN = 1024 * 8;
constexpr tt_size MAX_ERR_MSG_LEN = 256;
constexpr tt_size MAX_STRING_LEN = 256;

// TODO: move?
constexpr tt_size MAX_ZONE_THREADS = 32;
constexpr tt_size MAX_ZONE_DEPTH = 32;
constexpr tt_size MAX_THREAD_LOCKS = 16;
constexpr tt_size BACKGROUND_THREAD_STACK_SIZE = 1024 * 32;

X_PACK_PUSH(1)

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
};

X_PACK_POP;

static_assert(sizeof(VersionInfo) == 4, "Incorrect size");
static_assert(sizeof(PacketBase) == 3, "Incorrect size");
static_assert(sizeof(ConnectionRequestHdr) == 13, "Incorrect size");
static_assert(sizeof(ConnectionRequestAcceptedHdr) == 7, "Incorrect size");
static_assert(sizeof(ConnectionRequestRejectedHdr) == 5, "Incorrect size");
static_assert(sizeof(DataStreamHdr) == 3, "Incorrect size");

// Not packet types but part of data
// TODO: move?

struct DataStreamType
{
    enum Enum : tt_uint8
    {
        StringTableAdd,

        Zone,
        ThreadSetName,
        LockSetName,
        LockTry,
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
    tt_uint16 length;
};

X_PACK_PUSH(4)

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

    // 6
    StringTableIndex strIdxFile;
    StringTableIndex strIdxFunction;
    StringTableIndex strIdxZone;
    tt_int8  _pad1[2];
};

struct DataPacketThreadSetName : public DataPacketBase
{
    // 2
    StringTableIndex strIdxName;

    TtthreadId threadID;
};

struct DataPacketLockSetName : public DataPacketBase
{
    // 2
    StringTableIndex strIdxName;

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
    tt_uint64 lockHandle;
};

struct DataPacketLockCount : public DataPacketBase
{
    // 2
    tt_uint16 count;

    // 4
    TtthreadId threadID;

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
    tt_uint64 ptr;
};

struct DataPacketMemFree : public DataPacketBase
{
    // 4
    TtthreadId threadID;

    // 8
    tt_uint64 ptr;
};


static_assert(sizeof(DataPacketBase) == 1, "Incorrect size");
static_assert(sizeof(DataPacketStringTableAdd) == 4, "Incorrect size");
static_assert(sizeof(DataPacketZone) == 32, "Incorrect size");
static_assert(sizeof(DataPacketThreadSetName) == 8, "Incorrect size");
static_assert(sizeof(DataPacketLockSetName) == 12, "Incorrect size");
static_assert(sizeof(DataPacketLockTry) == 36, "Incorrect size");
static_assert(sizeof(DataPacketLockState) == 20, "Incorrect size");
static_assert(sizeof(DataPacketLockCount) == 16, "Incorrect size");
static_assert(sizeof(DataPacketMemAlloc) == 20, "Incorrect size");
static_assert(sizeof(DataPacketMemFree) == 16, "Incorrect size");


X_PACK_POP