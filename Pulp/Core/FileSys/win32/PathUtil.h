#pragma once

#include <String\Path.h>
#include <IFileSys.h>

X_NAMESPACE_BEGIN(core)

#ifdef DeleteFile
#undef DeleteFile
#endif // !DeleteFile

#ifdef MoveFile
#undef MoveFile
#endif // !MoveFile

namespace PathUtil
{
    typedef FindData FindData;
    typedef intptr_t findhandle;
    static const findhandle INVALID_FIND_HANDLE = -1;

    typedef core::Path<wchar_t> Path;

    bool GetCurrentDirectory(Path& pathOut);

    bool GetFullPath(const Path& filePath, Path& pathOut);
    bool GetFullPath(const wchar_t* pFilePath, Path& pathOut);

    bool DeleteFile(const Path& filePath);
    bool DeleteFile(const wchar_t* pFilePath);

    bool DeleteDirectory(const Path& dir, bool resursive);
    bool DeleteDirectory(const wchar_t* pDir, bool resursive);

    bool CreateDirectory(const Path& dir);
    bool CreateDirectory(const wchar_t* pDir);

    bool CreateDirectoryTree(const Path& dir);
    bool CreateDirectoryTree(const wchar_t* pDir);

    uint64_t GetFileSize(const Path& filePath);
    uint64_t GetFileSize(const wchar_t* pFilePath);

    bool DoesDirectoryExist(const Path& dirPath);
    bool DoesDirectoryExist(const wchar_t* pDir);

    bool DoesFileExist(const Path& filePath, bool supressMissingDirWarn = false);
    bool DoesFileExist(const wchar_t* pFilePath, bool supressMissingDirWarn = false);

    bool DoesPathExist(const Path& path);
    bool DoesPathExist(const wchar_t* pDir);

    bool IsDirectory(const Path& path);
    bool IsDirectory(const wchar_t* pDir);
    bool IsDirectory(const FindData& fd);

    bool MoveFile(const Path& fullPath, const Path& fullPathNew);
    bool MoveFile(const wchar_t* pFullPath, const wchar_t* pFullPathNew);

    findhandle findFirst(const wchar_t* path, FindData& findInfo);
    bool findNext(findhandle handle, FindData& findInfo);
    bool findClose(findhandle handle);

} // namespace PathUtil

X_NAMESPACE_END