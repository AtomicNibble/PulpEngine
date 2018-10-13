#pragma once

#include <IAssetDb.h>

#if !defined(ASSETDB_EXPORT)

#if !defined(X_LIB)
#define ASSETDB_EXPORT X_IMPORT
#else
#define ASSETDB_EXPORT
#endif

#endif

X_NAMESPACE_BEGIN(assetDb)

ASSETDB_EXPORT const char* getAssetTypeExtension(AssetType::Enum type);

X_NAMESPACE_END