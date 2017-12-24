#pragma once

#include "AssetPak.h"

#include <../AssetDB/AssetDB.h>


X_NAMESPACE_BEGIN(linker)

class Linker
{
	typedef core::Array<uint8_t> DataVec;

public:
	LINKERLIB_EXPORT Linker(assetDb::AssetDB& db, core::MemoryArenaBase* scratchArea);
	LINKERLIB_EXPORT ~Linker();

	LINKERLIB_EXPORT void PrintBanner(void);
	LINKERLIB_EXPORT bool Init(void);


	LINKERLIB_EXPORT bool Build(void);

private:
	bool AddAsset(assetDb::AssetType::Enum assType, const core::string& name);


	void GetOutputPathForAsset(assetDb::AssetType::Enum assType, const core::string& name,
		const core::Path<char>& modPath, core::Path<char>& pathOut);

private:
	core::MemoryArenaBase* scratchArea_;
	assetDb::AssetDB& db_;


	AssetPak::AssetPakBuilder builder_;
};


X_NAMESPACE_END
