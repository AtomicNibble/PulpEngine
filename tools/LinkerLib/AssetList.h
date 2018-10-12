#pragma once

#include <IAssetDb.h>

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(linker)

// used for saving/loading a list of assets to a file.
// Intented to be used for telling the linker what assets to include.
struct DirEntry
{
    assetDb::AssetType::Enum type;
    core::Path<char> path;
};

class AssetList
{
    static const int32_t ASSET_LIST_VERSION = 1;

public:
    typedef core::ArrayGrowMultiply<core::string> StringArr;
    typedef core::Array<DirEntry> DirEntryArr;
    typedef std::array<StringArr, assetDb::AssetType::ENUM_COUNT> AssetNameLists;

public:
    LINKERLIB_EXPORT AssetList(core::MemoryArenaBase* arena);

    LINKERLIB_EXPORT bool loadFromJson(core::StringRange<char> json);
    LINKERLIB_EXPORT bool loadFromFile(core::Path<char>& path);
    LINKERLIB_EXPORT bool saveToFile(core::XFile* pFile) const;
    LINKERLIB_EXPORT bool saveToFile(core::Path<char>& path) const;

    X_INLINE void add(assetDb::AssetType::Enum type, core::string&& name);
    X_INLINE void add(assetDb::AssetType::Enum type, const core::string& name);

    X_INLINE const AssetNameLists& getAssetList(void) const;
    X_INLINE const DirEntryArr& getDirList(void) const;

private:
    AssetNameLists assets_;
    DirEntryArr dirs_;
};

X_NAMESPACE_END

#include "AssetList.inl"