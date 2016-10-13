#include "stdafx.h"
#include "PathUtil.h"

#include <String\StringTokenizer.h>
#include <Shellapi.h>

X_NAMESPACE_BEGIN(core)

namespace PathUtil
{

	core::Path<wchar_t> GetCurrentDirectory(void)
	{
		WCHAR workingDir[_MAX_PATH] = { 0 };
		core::Path<wchar_t> dir;

		if (!GetCurrentDirectoryW(sizeof(workingDir), workingDir)) {
			core::lastError::Description Dsc;
			X_ERROR("FileSys", "GetCurrentDirectory failed. Error: %s", lastError::ToString(Dsc));
		}
		else {
			dir = core::Path<wchar_t>(workingDir);
			dir.ensureSlash();
		}

		return dir;
	}

	// ------------------------------------------------


	bool DeleteFile(const core::Path<wchar_t>& filePath)
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


	bool DeleteDirectory(const core::Path<wchar_t>& dir, bool resursive)
	{
		return DeleteDirectory(dir.c_str(), resursive);
	}

	bool DeleteDirectory(const wchar_t* pDir, bool resursive)
	{
		FILEOP_FLAGS flags = FOF_NOCONFIRMATION |
			FOF_NOERRORUI |
			FOF_SILENT;

		if (!resursive) {
			// not sure if this logic works or how id even want / expect it to work.
			X_ASSERT_NOT_IMPLEMENTED();

			flags |= FOF_NORECURSION;
		}

		SHFILEOPSTRUCTW file_op = {
			NULL,
			FO_DELETE,
			pDir,
			L"",
			flags,
			false,
			0,
			L"" };

		int ret = SHFileOperationW(&file_op);

		X_ERROR_IF(ret != 0, "FileSys", "Failed to delete directory: \"%ls\"", pDir);

		return ret == 0; // returns 0 on success, non zero on failure.
	}

	// ------------------------------------------------

	bool CreateDirectory(const core::Path<wchar_t>& dir)
	{
		return CreateDirectory(dir.c_str());
	}

	bool CreateDirectory(const wchar_t* pDir)
	{
		bool res = ::CreateDirectoryW(pDir, NULL) == TRUE;

		if (!res)
		{
			DWORD lastErr = lastError::Get();
			if (lastErr != ERROR_ALREADY_EXISTS)
			{
				lastError::Description Dsc;
				X_ERROR("FileSys", "Failed to create directory. Error: %s",
					lastError::ToString(lastErr, Dsc));
				return false;
			}
		}
		return true;
	}

	// ------------------------------------------------
	bool CreateDirectoryTree(const core::Path<wchar_t>& dir)
	{
		return CreateDirectoryTree(dir.c_str());
	}

	bool CreateDirectoryTree(const wchar_t* pDir)
	{
		if (DoesDirectoryExist(pDir)) {
			return true;
		}

		Path<wchar_t> path(L"");

		core::StringTokenizer<wchar_t> tokenizer(pDir, pDir + core::strUtil::strlen(pDir), Path<wchar_t>::NATIVE_SLASH_W);
		core::StringRange<wchar_t> range(nullptr, nullptr);

		while (tokenizer.ExtractToken(range))
		{
			path.append(range.GetStart(), range.GetEnd());

			if (!DoesDirectoryExist(path))
			{
				if (!CreateDirectory(path)) {
					return false;
				}
			}

			path.ensureSlash();
		}

		return true;
	}

	// ------------------------------------------------

	uint64_t GetFileSize(const core::Path<wchar_t>& filePath)
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


	bool DoesDirectoryExist(const core::Path<wchar_t>& dirPath)
	{
		return DoesDirectoryExist(dirPath.c_str());
	}

	bool DoesDirectoryExist(const wchar_t* pDir)
	{
		DWORD dwAttrib = GetFileAttributesW(pDir);

		if (dwAttrib != INVALID_FILE_ATTRIBUTES) // make sure we did not fail for some shit, like permissions
		{
			if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
			{
				// found
				return true;
			}

			X_ERROR("FileSys", "DirectoryExists check was ran on a File: \"%ls\"", pDir);
			return false;
		}

		DWORD err = lastError::Get();
		if (err != ERROR_PATH_NOT_FOUND && err != ERROR_FILE_NOT_FOUND)
		{
			lastError::Description Dsc;
			X_ERROR("FileSys", "DirectoryExists failed. Error: %s", lastError::ToString(err, Dsc));
		}

		return false;
	}

	// ------------------------------------------------

	bool DoesFileExist(const core::Path<wchar_t>& filePath)
	{
		return DoesFileExist(filePath.c_str());
	}

	bool DoesFileExist(const wchar_t* pFilePath)
	{
		DWORD dwAttrib = GetFileAttributesW(pFilePath);

		bool result = false;

		if (dwAttrib != INVALID_FILE_ATTRIBUTES) // make sure we did not fail for some shit, like permissions
		{
			if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) // herp derp.
			{
				X_ERROR("FileSys", "FileExsits check was ran on a directory");
			}
			else
			{
				// hi there Mr File
				result = true;
			}
		}

		if (!result)
		{
			DWORD err = lastError::Get();
			// This means we checked for a file in a directory that don't exsists.
			if (err == ERROR_PATH_NOT_FOUND)
			{
				X_LOG2("FileSys", "FileExsits failed, the target directory does not exsist: \"%ls\"",
					pFilePath);
			}
			else if (err != ERROR_FILE_NOT_FOUND)
			{
				lastError::Description Dsc;
				X_ERROR("FileSys", "FileExsits failed. Error: %s", lastError::ToString(err, Dsc));
			}
		}

		return result;
	}


	// ------------------------------------------------


	bool DoesPathExist(const core::Path<wchar_t>& path)
	{
		return DoesPathExist(path.c_str());
	}

	bool DoesPathExist(const wchar_t* pDir)
	{
		return GetFileAttributesW(pDir) != INVALID_FILE_ATTRIBUTES;
	}

	// ------------------------------------------------

	bool IsDirectory(const core::Path<wchar_t>& path)
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

	// ------------------------------------------------



	bool MoveFile(const core::Path<wchar_t>& fullPath, const core::Path<wchar_t>& fullPathNew)
	{
		return MoveFile(fullPath.c_str(), fullPathNew.c_str());
	}

	bool MoveFile(const wchar_t* pFullPath, const wchar_t* pFullPathNew)
	{
		BOOL res = MoveFileExW(
			pFullPath,
			pFullPathNew,
			MOVEFILE_REPLACE_EXISTING
		);

		if (res == 0) {
			lastError::Description Dsc;
			X_ERROR("FileSys", "RenameFile failed. Error: %s", lastError::ToString(Dsc));
		}

		return res != 0;
	}

} // namespace PathUtil


X_NAMESPACE_END