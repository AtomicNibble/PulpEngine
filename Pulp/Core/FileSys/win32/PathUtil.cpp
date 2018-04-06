#include "stdafx.h"
#include "PathUtil.h"

#include <String\StringTokenizer.h>
#include <Shellapi.h>

#include <io.h>

X_NAMESPACE_BEGIN(core)

namespace PathUtil
{
    Path GetCurrentDirectory(void)
    {
        WCHAR workingDir[Path::BUF_SIZE] = {0};
        Path dir;

        if (!GetCurrentDirectoryW(sizeof(workingDir), workingDir)) {
            core::lastError::Description Dsc;
            X_ERROR("FileSys", "GetCurrentDirectory failed. Error: %s", lastError::ToString(Dsc));
        }
        else {
            dir = Path(workingDir);
            dir.ensureSlash();
        }

        return dir;
    }

    // ------------------------------------------------

    bool GetFullPath(const Path& filePath, Path& pathOut)
    {
        return GetFullPath(filePath.c_str(), pathOut);
    }

    bool GetFullPath(const wchar_t* pFilePath, Path& pathOut)
    {
        wchar_t buf[Path::BUF_SIZE];

        DWORD retval = GetFullPathNameW(pFilePath, X_ARRAY_SIZE(buf), buf, nullptr);
        if (retval > 0 && retval < X_ARRAY_SIZE(buf)) {
            pathOut = buf;
            return true;
        }

        core::lastError::Description Dsc;
        X_ERROR("FileSys", "GetFullPathName failed. Error: %s", lastError::ToString(Dsc));
        return false;
    }

    // ------------------------------------------------

    bool DeleteFile(const Path& filePath)
    {
        return DeleteFile(filePath.c_str());
    }

    bool DeleteFile(const wchar_t* pFilePath)
    {
        if (!::DeleteFileW(pFilePath)) {
            core::lastError::Description Dsc;
            X_ERROR("FileSys", "DeleteFile failed. Error: %s", lastError::ToString(Dsc));
            return false;
        }

        return true;
    }

    // ------------------------------------------------

    bool DeleteDirectory(const Path& dir, bool resursive)
    {
        return DeleteDirectory(dir.c_str(), resursive);
    }

    bool DeleteDirectory(const wchar_t* pDir, bool resursive)
    {
        if (!IsDirectory(pDir)) {
            X_ERROR("FileSys", "DeleteDirectory was ran on a File: \"%ls\"", pDir);
            return false;
        }

        if (!resursive) {
            // not sure if this logic works or how id even want / expect it to work.
            X_ASSERT_NOT_IMPLEMENTED();
        }

#if 1
        // SHFileOperationW seams to be crashing when called in a qt app.
        // so just gonna do the delete logic myself.
        // we need to empty the dir before we can call RemoveDirectory.
        Path searchPath(pDir);

        const wchar_t* pDirEnd = pDir + searchPath.length();

        searchPath.ensureSlash();
        searchPath.append(L"*");

        PathUtil::findData fd;
        uintptr_t handle = PathUtil::findFirst(searchPath.c_str(), fd);
        if (handle != PathUtil::INVALID_FIND_HANDLE) {
            Path dirItem;

            do {
                if (core::strUtil::IsEqual(fd.name, L".") || core::strUtil::IsEqual(fd.name, L"..")) {
                    continue;
                }

                dirItem.set(pDir, pDirEnd);
                dirItem /= fd.name;

                if (PathUtil::IsDirectory(fd)) {
                    if (!DeleteDirectory(dirItem, resursive)) {
                        PathUtil::findClose(handle);
                        return false;
                    }
                }
                else {
                    if (!DeleteFile(dirItem)) {
                        PathUtil::findClose(handle);
                        return false;
                    }
                }

            } while (PathUtil::findNext(handle, fd));

            PathUtil::findClose(handle);
        }

        if (!RemoveDirectoryW(pDir)) {
            core::lastError::Description Dsc;
            X_ERROR("FileSys", "RemoveDirectory failed. Error: %s", lastError::ToString(Dsc));
            return false;
        }

        return true;
#else
        FILEOP_FLAGS flags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

        if (!resursive) {
            flags |= FOF_NORECURSION;
        }

        SHFILEOPSTRUCTW file_op = {
            nullptr,
            FO_DELETE,
            pDir,
            nullptr,
            flags,
            false,
            nullptr,
            nullptr};

        int ret = SHFileOperationW(&file_op);

        X_ERROR_IF(ret != 0, "FileSys", "Failed to delete directory: \"%ls\"", pDir);

        return ret == 0; // returns 0 on success, non zero on failure.
#endif
    }

    // ------------------------------------------------

    bool CreateDirectory(const Path& dir)
    {
        return CreateDirectory(dir.c_str());
    }

    bool CreateDirectory(const wchar_t* pDir)
    {
        bool res = ::CreateDirectoryW(pDir, NULL) == TRUE;

        if (!res) {
            DWORD lastErr = lastError::Get();
            if (lastErr != ERROR_ALREADY_EXISTS) {
                lastError::Description Dsc;
                X_ERROR("FileSys", "Failed to create directory. Error: %s",
                    lastError::ToString(lastErr, Dsc));
                return false;
            }
        }
        return true;
    }

    // ------------------------------------------------
    bool CreateDirectoryTree(const Path& dir)
    {
        return CreateDirectoryTree(dir.c_str());
    }

    bool CreateDirectoryTree(const wchar_t* pDir)
    {
        if (DoesDirectoryExist(pDir)) {
            return true;
        }

        Path path(L"");

        core::StringTokenizer<wchar_t> tokenizer(pDir, pDir + core::strUtil::strlen(pDir), Path::NATIVE_SLASH_W);
        core::StringRange<wchar_t> range(nullptr, nullptr);

        while (tokenizer.ExtractToken(range)) {
            path.append(range.GetStart(), range.GetEnd());

            if (!DoesDirectoryExist(path)) {
                if (!CreateDirectory(path)) {
                    return false;
                }
            }

            path.ensureSlash();
        }

        return true;
    }

    // ------------------------------------------------

    uint64_t GetFileSize(const Path& filePath)
    {
        return GetFileSize(filePath.c_str());
    }

    uint64_t GetFileSize(const wchar_t* pDir)
    {
        WIN32_FILE_ATTRIBUTE_DATA fad;
        core::zero_object(fad);

        if (!GetFileAttributesExW(pDir, GetFileExInfoStandard, &fad)) {
            core::lastError::Description Dsc;
            X_ERROR("FileSys", "GetFileSize failed. Error: %s", lastError::ToString(Dsc));
            return 0;
        }

        LARGE_INTEGER size;
        size.HighPart = fad.nFileSizeHigh;
        size.LowPart = fad.nFileSizeLow;
        return size.QuadPart;
    }

    // ------------------------------------------------

    bool DoesDirectoryExist(const Path& dirPath)
    {
        return DoesDirectoryExist(dirPath.c_str());
    }

    bool DoesDirectoryExist(const wchar_t* pDir)
    {
        DWORD dwAttrib = GetFileAttributesW(pDir);

        if (dwAttrib != INVALID_FILE_ATTRIBUTES) // make sure we did not fail for some shit, like permissions
        {
            if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
                // found
                return true;
            }

            X_ERROR("FileSys", "DirectoryExists check was ran on a File: \"%ls\"", pDir);
            return false;
        }

        DWORD err = lastError::Get();
        if (err != ERROR_PATH_NOT_FOUND && err != ERROR_FILE_NOT_FOUND) {
            lastError::Description Dsc;
            X_ERROR("FileSys", "DirectoryExists failed. Error: %s", lastError::ToString(err, Dsc));
        }

        return false;
    }

    // ------------------------------------------------

    bool DoesFileExist(const Path& filePath, bool supressMissingDirWarn)
    {
        return DoesFileExist(filePath.c_str(), supressMissingDirWarn);
    }

    bool DoesFileExist(const wchar_t* pFilePath, bool supressMissingDirWarn)
    {
        DWORD dwAttrib = GetFileAttributesW(pFilePath);

        if (dwAttrib != INVALID_FILE_ATTRIBUTES) // make sure we did not fail for some shit, like permissions
        {
            if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0) // herp derp.
            {
                return true;
            }

            X_ERROR("FileSys", "FileExsits check was ran on a directory");
        }

        DWORD err = lastError::Get();
        // This means we checked for a file in a directory that don't exsists.
        if (err == ERROR_PATH_NOT_FOUND) {
            X_LOG2_IF(!supressMissingDirWarn, "FileSys", "FileExsits failed, a parent directory does not exsist: \"%ls\"", pFilePath);
        }
        else if (err != ERROR_FILE_NOT_FOUND) {
            lastError::Description Dsc;
            X_ERROR("FileSys", "FileExsits failed. Error: %s", lastError::ToString(err, Dsc));
        }

        return false;
    }

    // ------------------------------------------------

    bool DoesPathExist(const Path& path)
    {
        return DoesPathExist(path.c_str());
    }

    bool DoesPathExist(const wchar_t* pDir)
    {
        return GetFileAttributesW(pDir) != INVALID_FILE_ATTRIBUTES;
    }

    // ------------------------------------------------

    bool IsDirectory(const Path& path)
    {
        return IsDirectory(path.c_str());
    }

    bool IsDirectory(const wchar_t* pDir)
    {
        DWORD dwAttrib = GetFileAttributesW(pDir);

        if (dwAttrib != INVALID_FILE_ATTRIBUTES) {
            if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
                return true;
            }
            return false;
        }

        if (dwAttrib != INVALID_FILE_ATTRIBUTES) {
            lastError::Description Dsc;
            X_ERROR("FileSys", "isDirectory failed. Error: %s", lastError::ToString(Dsc));
        }

        return false;
    }

    bool IsDirectory(const findData& fd)
    {
        return (fd.attrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    // ------------------------------------------------

    bool MoveFile(const Path& fullPath, const Path& fullPathNew)
    {
        return MoveFile(fullPath.c_str(), fullPathNew.c_str());
    }

    bool MoveFile(const wchar_t* pFullPath, const wchar_t* pFullPathNew)
    {
        BOOL res = MoveFileExW(
            pFullPath,
            pFullPathNew,
            MOVEFILE_REPLACE_EXISTING);

        if (res == 0) {
            lastError::Description Dsc;
            X_ERROR("FileSys", "RenameFile failed. Error: %s", lastError::ToString(Dsc));
        }

        return res != 0;
    }

    // ------------------------------------------------

    findhandle findFirst(const wchar_t* path, findData& findInfo)
    {
        intptr_t handle = _wfindfirst64(path, &findInfo);
        if (handle == -1) {
            return INVALID_FIND_HANDLE;
        }

        return handle;
    }

    bool findNext(findhandle handle, findData& findInfo)
    {
        if (_wfindnext64(handle, &findInfo) == 0) {
            return true;
        }

        return false;
    }

    bool findClose(findhandle handle)
    {
        if (handle != INVALID_FIND_HANDLE) {
            return _findclose(handle) == 0;
        }

        return false;
    }

} // namespace PathUtil

X_NAMESPACE_END