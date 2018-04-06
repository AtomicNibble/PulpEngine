#pragma once

#include "Util\NamespaceMacros.h"

X_NAMESPACE_BEGIN(misc)

typedef wchar_t AppDataPath[512];

bool GetAppDataPath(AppDataPath& pathOut);

X_NAMESPACE_END