#pragma once

#include <IAssetDb.h>

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(linker)

// used for saving/loading a list of assets to a file.
// Intented to be used for telling the linker what assets to include.
class AssetList
{
public:
    typedef core::ArrayGrowMultiply<core::string> StringArr;
    typedef std::array<StringArr, assetDb::AssetType::ENUM_COUNT> AssetNameLists;

public:
    AssetList(core::MemoryArenaBase* arena);

    bool loadFromJson(core::StringRange<char> json);
    bool loadFromFile(core::Path<char>& path);
    bool saveToFile(core::XFile* pFile) const;

    void add(assetDb::AssetType::Enum type, core::string& name);

    const AssetNameLists& getAssetList(void) const;

private:
    AssetNameLists assets_;
};


X_NAMESPACE_END
