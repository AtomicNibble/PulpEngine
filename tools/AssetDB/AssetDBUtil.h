#pragma once

#include <IAssetDb.h>

X_NAMESPACE_BEGIN(assetDb)

namespace Util
{
    DLL_EXPORT AssetType::Enum assetTypeFromStr(const char* pBegin, const char* pEnd);

} // namespace Util

X_NAMESPACE_END