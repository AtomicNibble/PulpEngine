#include "stdafx.h"
#include "AssetList.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(linker)

AssetList::AssetList(core::MemoryArenaBase* arena) :
    assets_{{ X_PP_REPEAT_COMMA_SEP(21, arena) }}
{

}

bool AssetList::loadFromJson(core::StringRange<char> json)
{
    core::json::MemoryStream ms(json.begin(), json.getLength());

    core::json::Document d;
    if (d.ParseStream<core::json::kParseDefaultFlags>(ms).HasParseError()) {
        auto err = d.GetParseError();
        const char* pErrStr = core::json::GetParseError_En(err);
        size_t offset = d.GetErrorOffset();
        size_t line = core::strUtil::LineNumberForOffset(json.begin(), json.end(), offset);

        X_ERROR("AssetList", "Failed to parse font desc(%" PRIi32 "): Offset: %" PRIuS " Line: %" PRIuS " Err: %s", err, offset, line, pErrStr);
        return false;
    }

    if (d.GetType() != core::json::Type::kObjectType) {
        X_ERROR("AssetList", "Unexpected type");
        return false;
    }

    if (!d.HasMember("version")) {
        X_ERROR("AssetList", "Missing version field");
        return false;
    }

    {
        auto version = d["version"].GetInt();
        if (version != ASSET_LIST_VERSION)
        {
            X_ERROR("AssetList", "Invalid vesion: %" PRIi32 " expected %" PRIi32, version, ASSET_LIST_VERSION);
            return false;
        }
    }

    if (!d.HasMember("assets")) {
        X_ERROR("AssetList", "Missing assets object");
        return false;
    }

    auto& assetsJson = d["assets"];
    if (assetsJson.GetType() != core::json::Type::kObjectType) {
        X_ERROR("AssetList", "Unexpected type");
        return false;
    }

    for (uint32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
    {
        auto assetType = static_cast<assetDb::AssetType::Enum>(i);
        auto* pTypeName = assetDb::AssetType::ToString(assetType);

        if (!assetsJson.HasMember(pTypeName)) {
            continue;
        }

        auto& assetList = assetsJson[pTypeName];
        if (assetList.GetType() != core::json::Type::kObjectType) {
            X_ERROR("AssetList", "Unexpected type");
            return false;
        }

        if (!assetList.HasMember("num")) {
            X_ERROR("AssetList", "Asset missing num field");
            return false;
        }

        if (!assetList.HasMember("names")) {
            X_ERROR("AssetList", "Asset missing names array");
            return false;
        }

        auto num = assetList["num"].GetInt();
        auto& nameList = assetList["names"];

        auto& namesArr = assets_[assetType];
        namesArr.reserve(num);

        for (auto& nameVal : nameList.GetArray())
        {
            namesArr.emplace_back(nameVal.GetString(), nameVal.GetStringLength());
        }

        if (safe_static_cast<int32_t>(namesArr.size()) != num)
        {
            X_ERROR("AssetList", "Asset name count mismatch. got: %" PRIuS " expected: %" PRIi32, namesArr.size(), num);
            return false;
        }
    }
    return true;
}

bool AssetList::loadFromFile(core::Path<char>& path)
{
    core::XFileMemScoped file;
    core::fileModeFlags mode;
    mode.Set(core::fileMode::READ);
    mode.Set(core::fileMode::SHARE);

    if (!file.openFile(path.c_str(), mode)) {
        return false;
    }

    auto* pFile = file.GetFile();

    return loadFromJson(core::StringRange<>(pFile->getBufferStart(), pFile->getBufferEnd()));
}

bool AssetList::saveToFile(core::XFile* pFile) const
{
    X_UNUSED(pFile);

    core::json::StringBuffer s;
    core::json::PrettyWriter<core::json::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("version");
    writer.Int(ASSET_LIST_VERSION);
    writer.Key("assets");
    writer.StartObject();

    for (uint32_t i = 0; i < assetDb::AssetType::ENUM_COUNT; i++)
    {
        auto assetType = static_cast<assetDb::AssetType::Enum>(i);
        auto& namesArr = assets_[assetType];

        if (namesArr.isEmpty()) {
            continue;
        }

        writer.Key(assetDb::AssetType::ToString(assetType));
        writer.StartObject();

        writer.Key("num");
        writer.Int64(namesArr.size());
        writer.Key("names");
        writer.StartArray();

        for (auto& name : namesArr)
        {
            writer.String(name.begin(), safe_static_cast<core::json::SizeType>(name.length()), false);
        }

        writer.EndArray();
        writer.EndObject();
    }

    writer.EndObject();
    writer.EndObject();

    if (pFile->write(s.GetString(), s.GetSize()) != s.GetSize()) {
        X_ERROR("AssetList", "Failed to write assetlist data");
        return false;
    }

    return true;
}



X_NAMESPACE_END
