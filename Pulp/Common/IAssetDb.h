#pragma once

#include <Hashing\Fnva1Hash.h>

X_NAMESPACE_BEGIN(assetDb)

static const char* ASSET_LIST_EXT = "assList"; // show me dat ass.

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
    SCRIPT,     // Not in db
    FONT,   
    SHADER,     // Not in db
    LEVEL,      // Not in db
    CONFIG,     // Not in db
    TECHDEF,    // Not in db
    MENU,
    RAW         // unclassified data, mainly used for pak storage.
);

namespace api
{
    static const size_t MESSAGE_BUFFER_SIZE = 0x200; // max size of message sent by api.
}

X_INLINE bool assetTypeFromStr(const char* pBegin, const char* pEnd, AssetType::Enum& type)
{
    using namespace core::Hash::Literals;

    const size_t len = (pEnd - pBegin);
    if (len >= 32) {
        return false;
    }

    core::StackString<32> tmp(pBegin, pEnd);
    tmp.toLower();

    static_assert(AssetType::ENUM_COUNT == 22, "More asset types :[] ? this code need updating.");

    switch (core::Hash::Fnv1aHash(tmp.c_str(), tmp.length())) {
        case "model"_fnv1a:
            type = AssetType::MODEL;
            break;
        case "anim"_fnv1a:
            type = AssetType::ANIM;
            break;
        case "material"_fnv1a:
            type = AssetType::MATERIAL;
            break;
        case "img"_fnv1a:
            type = AssetType::IMG;
            break;
        case "weapon"_fnv1a:
            type = AssetType::WEAPON;
            break;
        case "turret"_fnv1a:
            type = AssetType::TURRET;
            break;
        case "light"_fnv1a:
            type = AssetType::LIGHT;
            break;
        case "fx"_fnv1a:
            type = AssetType::FX;
            break;
        case "rumble"_fnv1a:
            type = AssetType::RUMBLE;
            break;
        case "shellshock"_fnv1a:
            type = AssetType::SHELLSHOCK;
            break;
        case "character"_fnv1a:
            type = AssetType::CHARACTER;
            break;
        case "vehicle"_fnv1a:
            type = AssetType::VEHICLE;
            break;
        case "camera"_fnv1a:
            type = AssetType::CAMERA;
            break;
        case "video"_fnv1a:
            type = AssetType::VIDEO;
            break;
        case "script"_fnv1a:
            type = AssetType::SCRIPT;
            break;
        case "font"_fnv1a:
            type = AssetType::FONT;
            break;
        case "shader"_fnv1a:
            type = AssetType::SHADER;
            break;
        case "level"_fnv1a:
            type = AssetType::LEVEL;
            break;
        case "config"_fnv1a:
            type = AssetType::CONFIG;
            break;
        case "techdef"_fnv1a:
            type = AssetType::TECHDEF;
            break;
        case "menu"_fnv1a:
            type = AssetType::MENU;
            break;
        case "raw"_fnv1a:
            type = AssetType::RAW;
            break;
        default:
            return false;
    }

    return true;
}

X_NAMESPACE_END
