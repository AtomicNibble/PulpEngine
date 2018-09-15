#include "stdafx.h"
#include "AssetDBUtil.h"

#include <Hashing\Fnva1Hash.h>

X_NAMESPACE_BEGIN(assetDb)

using namespace core::Hash::Literals;

namespace Util
{
    AssetType::Enum assetTypeFromStr(const char* pBegin, const char* pEnd)
    {
        const size_t len = (pEnd - pBegin);

        static_assert(AssetType::ENUM_COUNT == 22, "More asset types :[] ? this code need updating.");

        switch (core::Hash::Fnv1aHash(pBegin, len)) {
            case "model"_fnv1a:
                return AssetType::MODEL;
                break;
            case "anim"_fnv1a:
                return AssetType::ANIM;
                break;
            case "material"_fnv1a:
                return AssetType::MATERIAL;
                break;
            case "img"_fnv1a:
                return AssetType::IMG;
                break;
            case "weapon"_fnv1a:
                return AssetType::WEAPON;
                break;
            case "turret"_fnv1a:
                return AssetType::TURRET;
                break;
            case "light"_fnv1a:
                return AssetType::LIGHT;
                break;
            case "fx"_fnv1a:
                return AssetType::FX;
                break;
            case "rumble"_fnv1a:
                return AssetType::RUMBLE;
                break;
            case "shellshock"_fnv1a:
                return AssetType::SHELLSHOCK;
                break;
            case "character"_fnv1a:
                return AssetType::CHARACTER;
                break;
            case "vehicle"_fnv1a:
                return AssetType::VEHICLE;
                break;
            case "camera"_fnv1a:
                return AssetType::CAMERA;
                break;
            case "video"_fnv1a:
                return AssetType::VIDEO;
                break;
            case "script"_fnv1a:
                return AssetType::SCRIPT;
                break;
            case "font"_fnv1a:
                return AssetType::FONT;
                break;
            case "shader"_fnv1a:
                return AssetType::SHADER;
                break;
            case "level"_fnv1a:
                return AssetType::LEVEL;
                break;
            case "config"_fnv1a:
                return AssetType::CONFIG;
                break;
            case "techdef"_fnv1a:
                return AssetType::TECHDEF;
                break;
            case "menu"_fnv1a:
                return AssetType::MENU;
                break;
            case "raw"_fnv1a:
                return AssetType::RAW;
                break;
            default:
                X_ERROR("AssetDB", "Unknown AssetType: '%.*s' (case-sen)", len, pBegin);
                return AssetType::MODEL; // don't have a invalid type currently.
        }
    }

} // namespace Util

X_NAMESPACE_END