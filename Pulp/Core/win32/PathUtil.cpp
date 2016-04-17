#include "stdafx.h"
#include "PathUtil.h"

#include <String\StringTokenizer.h>

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


	bool CreateDirectory(core::Path<wchar_t>& dir)
	{
		bool res = ::CreateDirectoryW(dir.c_str(), NULL) == TRUE;

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


	bool CreateDirectoryTree(core::Path<wchar_t>& dir)
	{
		if (DoesDirectoryExist(dir)) {
			return true;
		}

		Path<wchar_t> path(L"");

		core::StringTokenizer<wchar_t> tokenizer(dir.begin(), dir.end(), Path<wchar_t>::NATIVE_SLASH_W);
		core::StringRange<wchar_t> range(nullptr, nullptr);

		while (tokenizer.ExtractToken(range))
		{
			path.append(range.GetStart(), range.GetEnd());

			if (!DoesDirectoryExist(path))
			{
				if (CreateDirectory(path)) {
					return false;
				}
			}

			path.ensureSlash();
		}

		return true;
	}


	uint64_t GetFileSize(core::Path<wchar_t>& filePath)
	{
		WIN32_FILE_ATTRIBUTE_DATA fad;
		core::zero_object(fad);

		if (!GetFileAttributesExW(filePath.c_str(), GetFileExInfoStandard, &fad)) {
			core::lastError::Description Dsc;
			X_ERROR("FileSys", "GetFileSize failed. Error: %s", lastError::ToString(Dsc));
			return 0; 
		}

		LARGE_INTEGER size;
		size.HighPart = fad.nFileSizeHigh;
		size.LowPart = fad.nFileSizeLow;
		return size.QuadPart;
	}


	bool DoesDirectoryExist(core::Path<wchar_t>& dirPath)
	{
		DWORD dwAttrib = GetFileAttributesW(dirPath.c_str());

		if (dwAttrib != INVALID_FILE_ATTRIBUTES) // make sure we did not fail for some shit, like permissions
		{
			if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
			{
				// found
				return true;
			}

			X_ERROR("FileSys", "DirectoryExists check was ran on a File: \"%ls\"", dirPath.c_str());
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


	bool DoesFileExist(core::Path<wchar_t>& filePath)
	{
		DWORD dwAttrib = GetFileAttributesW(filePath.c_str());

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
					filePath.c_str());
			}
			else if (err != ERROR_FILE_NOT_FOUND)
			{
				lastError::Description Dsc;
				X_ERROR("FileSys", "FileExsits failed. Error: %s", lastError::ToString(err, Dsc));
			}
		}

		return result;
	}


	bool DoesPathExist(core::Path<wchar_t>& path)
	{
		return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
	}





} // namespace PathUtil


X_NAMESPACE_END