#pragma once


#include <String\Path.h>

X_NAMESPACE_BEGIN(core)

namespace PathUtil
{

	core::Path<wchar_t> GetCurrentDirectory(void);

	bool CreateDirectory(core::Path<wchar_t>& dir);

	bool CreateDirectoryTree(core::Path<wchar_t>& dir);

	uint64_t GetFileSize(core::Path<wchar_t>& filePath);

	bool DoesDirectoryExist(core::Path<wchar_t>& dirPath);

	bool DoesFileExist(core::Path<wchar_t>& filePath);

	bool DoesPathExist(core::Path<wchar_t>& path);


} // namespace PathUtil


X_NAMESPACE_END