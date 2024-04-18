#pragma once

#include <String\Path.h>
#include <IFileSys.h>

X_NAMESPACE_BEGIN(core)

#ifdef DeleteFile
#undef DeleteFile
#endif // DeleteFile

#ifdef MoveFile
#undef MoveFile
#endif // MoveFile

namespace PathUtil
{
    typedef FindData FindData;
    typedef findhandle findhandle;

    // Native slashes
    static const wchar_t NATIVE_SLASH = L'\\';
    static const wchar_t NON_NATIVE_SLASH = L'/';

    typedef core::Path<wchar_t> Path;

    void ensureSlash(Path& path);
    void replaceSeprators(Path& path);

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

    bool DirectoryExist(const Path& dirPath);
    bool DirectoryExist(const wchar_t* pDir);

    bool FileExist(const Path& filePath, bool supressMissingDirWarn);
    bool FileExist(const wchar_t* pFilePath, bool supressMissingDirWarn);

    bool PathExist(const Path& path);
    bool PathExist(const wchar_t* pDir);

    bool IsDirectory(const Path& path);
    bool IsDirectory(const wchar_t* pDir);
    bool IsDirectory(const FindData& fd);

    bool MoveFile(const Path& fullPath, const Path& fullPathNew);
    bool MoveFile(const wchar_t* pFullPath, const wchar_t* pFullPathNew);

    FindPair findFirst(const Path& path, FindData& findInfo);
    bool findNext(findhandle handle, FindData& findInfo);
    bool findClose(findhandle handle);

} // namespace PathUtil

X_NAMESPACE_END