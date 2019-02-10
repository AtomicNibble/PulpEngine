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

constexpr tt_size MAX_PACKET_SIZE = 1024;
constexpr tt_size MAX_APP_NAME_LEN = 64;
constexpr tt_size MAX_BUILD_INFO_LEN = 128;
constexpr tt_size MAX_ERR_MSG_LEN = 256;
constexpr tt_size MAX_STRING_LEN = 256;

constexpr tt_size MAX_ZONE_THREADS = 64;
constexpr tt_size MAX_ZONE_DEPTH = 32;

struct PacketBase
{
    PacketType::Enum type;
};

struct ConnectionRequestData : public PacketBase
{
    tt_int8     _pad0[3];
    VersionInfo clientVer;

    char appName[MAX_APP_NAME_LEN];
    char buildInfo[MAX_BUILD_INFO_LEN];
};

struct ConnectionRequestAcceptedData : public PacketBase
{
    VersionInfo serverVer;
};

struct ConnectionRequestRejectedData : public PacketBase
{
    char reason[MAX_ERR_MSG_LEN];
};

struct DataStreamData : public PacketBase
{
    tt_uint32 dataSize;
};

static_assert(sizeof(VersionInfo) == 4, "Incorrect size");
static_assert(sizeof(PacketBase) == 1, "Incorrect size");
static_assert(sizeof(ConnectionRequestData) == 200, "Incorrect size");
static_assert(sizeof(ConnectionRequestAcceptedData) == 5, "Incorrect size");
static_assert(sizeof(ConnectionRequestRejectedData) == 257, "Incorrect size");

// Not packet types but part of data
// TODO: move?

struct DataStreamType
{
    enum Enum : tt_uint8
    {
        StringTableAdd,

        Zone,
        Num
    };
};


using TtthreadId = tt_uint32;
using TtZoneId = tt_uint32;

struct DataPacketBase
{
    DataStreamType::Enum type;
};

struct StringTableAddData : public DataPacketBase
{
    tt_uint16 length;
};

X_PACK_PUSH(4)

struct ZoneData : public DataPacketBase
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

static_assert(sizeof(DataPacketBase) == 1, "Incorrect size");
static_assert(sizeof(ZoneData) == 32, "Incorrect size");


X_PACK_POP