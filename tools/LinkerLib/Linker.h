#pragma once

#include "AssetPak.h"

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(linker)

struct BuildOptions
{
    core::Path<char> assetList;
    core::Path<char> outFile;
    AssetPak::PakBuilderFlags flags;
};

class Linker
{
    typedef core::Array<uint8_t> DataVec;

public:
    LINKERLIB_EXPORT Linker(assetDb::AssetDB& db, core::MemoryArenaBase* scratchArea);
    LINKERLIB_EXPORT ~Linker();

    LINKERLIB_EXPORT void PrintBanner(void);
    LINKERLIB_EXPORT bool Init(void);

    LINKERLIB_EXPORT bool dumpMeta(core::Path<char>& inputFile);

    LINKERLIB_EXPORT bool Build(BuildOptions& options);

private:
    bool AddAsset(assetDb::AssetType::Enum assType, const core::string& name);

private:
    core::MemoryArenaBase* scratchArea_;
    assetDb::AssetDB& db_;

    AssetPak::AssetPakBuilder builder_;
};

X_NAMESPACE_END
