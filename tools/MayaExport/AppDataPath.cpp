

#ifdef NEAR
#undef NEAR
#endif

#ifdef FAR
#undef FAR
#endif

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

#include <windows.h>
#include <Shlobj.h> // SHGetKnownFolderPath

#include "AppDataPath.h"

X_NAMESPACE_BEGIN(misc)

bool GetAppDataPath(AppDataPath& pathOut)
{
    PWSTR patchPointer;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &patchPointer))) {
        size_t len = ::wcslen(patchPointer);

        ::memset(pathOut, 0, sizeof(pathOut));
        ::memcpy_s(pathOut, sizeof(pathOut), patchPointer, len * sizeof(wchar_t));

        CoTaskMemFree(patchPointer);
        return true;
    }

    return false;
}

X_NAMESPACE_END