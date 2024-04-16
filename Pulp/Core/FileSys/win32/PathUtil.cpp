#include "stdafx.h"
#include "PathUtil.h"

#include <String\StringTokenizer.h>
#include <Shellapi.h>

#include <io.h>

X_NAMESPACE_BEGIN(core)

namespace PathUtil
{
    namespace
    {
        const wchar_t NATIVE_SLASH_W = L'\\';
        const wchar_t NON_NATIVE_SLASH_W = L'/';

        void win32FindDataToFindData(const _wfinddata64_t& fi, FindData& findInfo)
        {
            findInfo.attrib.Clear();
            if ((fi.attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                findInfo.attrib.Set(FindData::AttrFlag::DIRECTORY);
            }

            findInfo.size = fi.size;
            findInfo.name.set(fi.name, fi.name + strUtil::strlen(fi.name));
            // "." && ".." should not reach here.
            X_ASSERT(!findInfo.name.isEqual(".") && !findInfo.name.isEqual(".."), "Handle .. in findNext")();
        }

    } // namespace

    void ensureSlash(Path& path)
    {
        if (path.isEmpty()) {
            return;
        }

        path.stripTrailing(NATIVE_SLASH);
        path.stripTrailing(NON_NATIVE_SLASH);
        path.append(NATIVE_SLASH, 1);
    }

    void replaceSeprators(Path& path)
    {
        path.replaceAll(NON_NATIVE_SLASH, NATIVE_SLASH);
    }

    // ------------------------------------------------

    bool GetCurrentDirectory(Path& pathOut)
    {
        wchar_t buf[Path::BUF_SIZE] = {};

        auto retval = GetCurrentDirectoryW(sizeof(buf), buf);
        if (retval > 0 && retval < X_ARRAY_SIZE(buf)) {
            pathOut.set(buf, buf + retval);
            ensureSlash(pathOut);
            return true;
        }

        lastError::Description Dsc;
        X_ERROR("FileSys", "GetCurrentDirectory failed. Error: %s", lastError::ToString(Dsc));
        return false;
    }

    // ------------------------------------------------

    bool GetFullPath(const Path& filePath, Path& pathOut)
    {
        return GetFullPath(filePath.c_str(), pathOut);
    }

    bool GetFullPath(const wchar_t* pFilePath, Path& pathOut)
    {
        wchar_t buf[Path::BUF_SIZE] = {};

        DWORD retval = GetFullPathNameW(pFilePath, X_ARRAY_SIZE(buf), buf, nullptr);
        if (retval > 0 && retval < X_ARRAY_SIZE(buf)) {
            pathOut.set(buf, buf + retval);
            return true;
        }

        lastError::Description Dsc;
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
            lastError::Description Dsc;
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

        // SHFileOperationW seams to be crashing when called in a qt app.
        // so just gonna do the delete logic myself.
        // we need to empty the dir before we can call RemoveDirectory.
        Path searchPath(pDir);

        const wchar_t* pDirEnd = pDir + searchPath.length();

        ensureSlash(searchPath);
        searchPath.append(L"*");

        _wfinddata64_t fi;
        intptr_t handle = ::_wfindfirst64(searchPath.c_str(), &fi);
        if (handle != -1) {
            Path dirItem;

            do {
                const auto isDir = (fi.attrib & FILE_ATTRIBUTE_DIRECTORY) != 0;

                if (isDir) {
                    if (strUtil::IsEqual(L".", fi.name) || strUtil::IsEqual(L"..", fi.name)) {
                        continue;
                    }
                }

                dirItem.set(pDir, pDirEnd);
                dirItem /= fi.name;

                if (isDir) {
                    if (!DeleteDirectory(dirItem, resursive)) {
                        ::_findclose(handle);
                        return false;
                    }
                }
                else {
                    if (!DeleteFile(dirItem)) {
                        ::_findclose(handle);
                        return false;
                    }
                }

            } while (::_wfindnext64(handle, &fi));

            ::_findclose(handle);
        }

        if (!RemoveDirectoryW(pDir)) {
            lastError::Description Dsc;
            X_ERROR("FileSys", "RemoveDirectory failed. Error: %s", lastError::ToString(Dsc));
            return false;
        }

        return true;
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
        if (DirectoryExist(pDir)) {
            return true;
        }

        Path path(L"");

        StringTokenizer<wchar_t> tokenizer(pDir, pDir + strUtil::strlen(pDir), NATIVE_SLASH_W);
        StringRange<wchar_t> range(nullptr, nullptr);

        while (tokenizer.extractToken(range)) {
            path.append(range.getStart(), range.getEnd());

            if (!DirectoryExist(path)) {
                if (!CreateDirectory(path)) {
                    return false;
                }
            }

            ensureSlash(path);
        }

        return true;
    }

    // ------------------------------------------------

    uint64_t GetFileSize(const Path& filePath)
    {
        return GetFileSize(filePath.c_str());
    }

    uint64_t GetFileSize(const wchar_t* pFilePath)
    {
        WIN32_FILE_ATTRIBUTE_DATA fad;
        zero_object(fad);

        if (!GetFileAttributesExW(pFilePath, GetFileExInfoStandard, &fad)) {
            lastError::Description Dsc;
            X_ERROR("FileSys", "GetFileSize failed. Error: %s", lastError::ToString(Dsc));
            return 0;
        }

        LARGE_INTEGER size;
        size.HighPart = fad.nFileSizeHigh;
        size.LowPart = fad.nFileSizeLow;
        return size.QuadPart;
    }

    // ------------------------------------------------

    bool DirectoryExist(const Path& dirPath)
    {
        return DirectoryExist(dirPath.c_str());
    }

    bool DirectoryExist(const wchar_t* pDir)
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

    bool FileExist(const Path& filePath, bool supressMissingDirWarn)
    {
        return FileExist(filePath.c_str(), supressMissingDirWarn);
    }

    bool FileExist(const wchar_t* pFilePath, bool supressMissingDirWarn)
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
        // This means we checked for a file in a directory that don't exists.
        if (err == ERROR_PATH_NOT_FOUND) {
            X_LOG2_IF(!supressMissingDirWarn, "FileSys", "FileExsits failed, a parent directory does not exist: \"%ls\"", pFilePath);
        }
        else if (err != ERROR_FILE_NOT_FOUND) {
            lastError::Description Dsc;
            X_ERROR("FileSys", "FileExsits failed. Error: %s", lastError::ToString(err, Dsc));
        }

        return false;
    }

    // ------------------------------------------------

    bool PathExist(const Path& path)
    {
        return PathExist(path.c_str());
    }

    bool PathExist(const wchar_t* pDir)
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

    bool IsDirectory(const FindData& fd)
    {
        return fd.attrib.IsSet(FindData::AttrFlag::DIRECTORY);
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

    static_assert(reinterpret_cast<uintptr_t>(INVALID_HANDLE_VALUE) == INVALID_FIND_HANDLE, "Invalid handles don't match");

    FindPair findFirst(const Path& path, FindData& findInfo)
    {
        _wfinddata64_t fi;
        intptr_t handle = ::_wfindfirst64(path.c_str(), &fi);
        if (handle == -1) {
            auto err = errno;
            if (err == ENOENT) {
                // if it don't exist return valid?
                // seams resonable.
                return{ INVALID_FIND_HANDLE, true };
            }

            return{ INVALID_FIND_HANDLE, false };
        }

        // skip "." & ".."
        while ((fi.attrib & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            (strUtil::IsEqual(L".", fi.name) || strUtil::IsEqual(L"..", fi.name)))
        {
            auto res = ::_wfindnext64(handle, &fi);
            if (res != 0) {
                ::_findclose(handle);
                return{ INVALID_FIND_HANDLE, true };
            }
        }

        win32FindDataToFindData(fi, findInfo);
        return{ handle, true };
    }

    bool findNext(findhandle handle, FindData& findInfo)
    {
        _wfinddata64_t fi;

        auto res = ::_wfindnext64(handle, &fi);
        if (res == 0) {
            win32FindDataToFindData(fi, findInfo);
            return true;
        }

        return false;
    }

    bool findClose(findhandle handle)
    {
        if (handle != INVALID_FIND_HANDLE) {
            return ::_findclose(handle) == 0;
        }

        return false;
    }

} // namespace PathUtil

X_NAMESPACE_END
