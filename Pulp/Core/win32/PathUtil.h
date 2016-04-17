#pragma once


#include <String\Path.h>

X_NAMESPACE_BEGIN(core)

namespace PathUtil
{

	core::Path<wchar_t> GetCurrentDirectory(void);

	bool CreateDirectory(const core::Path<wchar_t>& dir);
	bool CreateDirectory(const wchar_t* pDir);

	bool CreateDirectoryTree(const core::Path<wchar_t>& dir);
	bool CreateDirectoryTree(const wchar_t* pDir);

	uint64_t GetFileSize(const core::Path<wchar_t>& filePath);
	uint64_t GetFileSize(const wchar_t* pFilePath);

	bool DoesDirectoryExist(const core::Path<wchar_t>& dirPath);
	bool DoesDirectoryExist(const wchar_t* pDir);

	bool DoesFileExist(const core::Path<wchar_t>& filePath);
	bool DoesFileExist(const wchar_t* pFilePath);

	bool DoesPathExist(const core::Path<wchar_t>& path);
	bool DoesPathExist(const wchar_t* pDir);

	bool IsDirectory(const core::Path<wchar_t>& path);
	bool IsDirectory(const wchar_t* pDir);


} // namespace PathUtil


X_NAMESPACE_END