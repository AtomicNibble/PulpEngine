#pragma once

X_NAMESPACE_BEGIN(assetDb)

static const char ASSET_NAME_SLASH = '/';
static const char ASSET_NAME_INVALID_SLASH = '\\';
static const char ASSET_NAME_PREFIX = '$'; // allow names with this prefix.
static const size_t ASSET_NAME_MAX_LENGTH = 128;
static const size_t ASSET_NAME_MIN_LENGTH = 2; // ban single char asset names?

// Valid chars for asset name are:
// [a-z0-9_] & ASSET_NAME_SLASH. Upper case is not allowed!!!

typedef int32_t AssetId;
typedef int32_t ModId;
typedef int32_t ProfileId;
typedef int32_t ThumbId;

static const ModId INVALID_MOD_ID = -1;
static const AssetId INVALID_ASSET_ID = -1;
static const AssetId INVALID_RAWFILE_ID = -1;
static const ThumbId INVALID_THUMB_ID = -1;

X_DECLARE_ENUM8(AssetType)
(
    MODEL,
    ANIM,
    MATERIAL,
    IMG,
    WEAPON, // do we want diffent weapon types, or will the asset contain the sub type?
    TURRET,
    // will allow for data driven lights, that can then be placed in editor rather than having fixed light types.
    LIGHT,
    FX,
    // cam shake
    RUMBLE,

    SHELLSHOCK,
    // pre defined collection of models and settings to make up a char.
    CHARACTER,
    // beep beep, be a long time before this type of asset gets implemented...
    VEHICLE,
    // these will be pre defined camera paths
    // that can be played.
    // option to loop, hude hud, trigger note tracks..
    // basically a anim for the camera
    CAMERA,

    VIDEO,
    SCRIPT,
    FONT,
    SHADER,
    LEVEL);

namespace api
{
    static const size_t MESSAGE_BUFFER_SIZE = 0x200; // max size of message sent by api.
}

X_NAMESPACE_END
