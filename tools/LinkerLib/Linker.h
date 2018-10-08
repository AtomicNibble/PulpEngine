#pragma once

#include "AssetPak.h"

#include <../AssetDB/AssetDB.h>
#include <../ConverterLib/ConverterLib.h>

X_NAMESPACE_BEGIN(linker)

struct BuildOptions
{
    core::Path<char> assetList;
    core::Path<char> outFile;
    core::string mod;
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

    LINKERLIB_EXPORT bool dumpMeta(core::Path<wchar_t>& inputFile);

    LINKERLIB_EXPORT bool Build(BuildOptions& options);

private:
    bool AddAssetAndDepenency(assetDb::AssetType::Enum assType, const core::string& name);
    bool AddAsset(assetDb::AssetType::Enum assType, const core::string& name);

private:
    core::MemoryArenaBase* scratchArea_;
    assetDb::AssetDB& db_;
    converter::Converter converter_;

    AssetPak::AssetPakBuilder builder_;
};

X_NAMESPACE_END
