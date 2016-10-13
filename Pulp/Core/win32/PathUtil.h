#pragma once


#include <String\Path.h>

X_NAMESPACE_BEGIN(core)

#ifdef DeleteFile
#undef DeleteFile
#endif // !DeleteFile

#ifdef MoveFile
#undef MoveFile
#endif // !MoveFile

namespace PathUtil
{

	core::Path<wchar_t> GetCurrentDirectory(void);

	bool DeleteFile(const core::Path<wchar_t>& filePath);
	bool DeleteFile(const wchar_t* pFilePath);

	bool DeleteDirectory(const core::Path<wchar_t>& dir, bool resursive);
	bool DeleteDirectory(const wchar_t* pDir, bool resursive);

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


	bool MoveFile(const core::Path<wchar_t>& fullPath, const core::Path<wchar_t>& fullPathNew);
	bool MoveFile(const wchar_t* pFullPath, const wchar_t* pFullPathNew);


} // namespace PathUtil


X_NAMESPACE_END