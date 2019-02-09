#pragma once

#include "Types.h"

enum PacketType
{
    ConnectionRequest,
    ConnectionRequestAccepted,

    Num
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

constexpr tt_size MAX_APP_NAME_LEN = 64;
constexpr tt_size MAX_BUILD_INFO_LEN = 128;

struct PacketBase
{
    tt_uint8 type;
};

struct ConnectionRequestData : public PacketBase
{
    VersionInfo clientVer;

    char appName[MAX_APP_NAME_LEN];
    char buildInfo[MAX_BUILD_INFO_LEN];
};