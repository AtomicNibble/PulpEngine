#include "stdafx.h"
#include "xFileSys.h"
#include "DiskFile.h"
#include "DiskFileAsync.h"

#include <Util\LastError.h>
#include <String\StackString.h>

#include <direct.h>
#include <io.h>

#include <Shellapi.h>

#include "XFindData.h"

#include <IAssetPak.h>
#include <IModel.h>
#include <IAnimation.h>

#include <Memory\VirtualMem.h>

X_NAMESPACE_BEGIN(core)

namespace
{
	// might need more since memory files will be from pool.
	static const size_t MAX_FILE_HANDLES = 1024;


	static const size_t FILE_ALLOCATION_SIZE = core::Max_static_size<sizeof(XDiskFile), 
		core::Max_static_size<sizeof(XDiskFileAsync), sizeof(XFileMem)>::value>::value;


	static const size_t FILE_ALLOCATION_ALIGN = core::Max_static_size< X_ALIGN_OF(XDiskFile),
		core::Max_static_size<X_ALIGN_OF(XDiskFileAsync), X_ALIGN_OF(XFileMem)>::value>::value;


}


xFileSys::xFileSys() :
	searchPaths_(nullptr),
	gameDir_(nullptr),

	filePoolHeap_(
	bitUtil::RoundUpToMultiple<size_t>(
	FilePoolArena::getMemoryRequirement(FILE_ALLOCATION_SIZE) * MAX_FILE_HANDLES,
	VirtualMem::GetPageSize()
	)
	),
	filePoolAllocator_(filePoolHeap_.start(), filePoolHeap_.end(),
	FilePoolArena::getMemoryRequirement(FILE_ALLOCATION_SIZE),
	FilePoolArena::getMemoryAlignmentRequirement(FILE_ALLOCATION_ALIGN),
	FilePoolArena::getMemoryOffsetRequirement()
	),
	filePoolArena_(&filePoolAllocator_, "FilePool"),
	memFileArena_(&memfileAllocator_, "MemFileArena")
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);

	gEnv->pArena->addChildArena(&filePoolArena_);
}

xFileSys::~xFileSys()
{

}


void xFileSys::Init()
{
	X_LOG0("FileSys", "Starting Filesys..");

	// TODO: yup.
	setGameDir("C:\\Users\\Tom\\Documents\\Visual Studio 2013\\Projects\\WinEngine\\code\\game_folder");
}

void xFileSys::ShutDown()
{
	X_LOG0("FileSys", "Shutting Down");

	for (search_s* s = searchPaths_; s; ) {
		search_s* cur = s;
		s = cur->next_;
		if (cur->dir)
			X_DELETE( cur->dir, g_coreArena);
		else
			X_DELETE(cur->pak, g_coreArena);
		X_DELETE(cur, g_coreArena);
	}


}


// --------------------- Open / Close ---------------------

XFile* xFileSys::openFile(pathType path, fileModeFlags mode, WriteLocation::Enum location)
{
	X_ASSERT_NOT_NULL(path);

	XDiskFile* file = nullptr;
	
	if (mode.IsSet(fileMode::READ))
	{
		_finddatai64_t findinfo;

		XFindData FindData(path, this);
		if (FindData.findnext(&findinfo))
		{
			core::Path real_path;
			FindData.getOSPath(real_path, &findinfo);

			// TODO: pool allocations.
			file = X_NEW(XDiskFile, &filePoolArena_, "DiskFile")(real_path.c_str(), mode);

			if (!file->valid()) {
				closeFile(file);
				file = nullptr;
			}
		}
		else
		{
			fileModeFlags::Description Dsc;
			X_WARNING("FileSys", "Failed to find file: %s, Flags: %s",
				path, mode.ToString(Dsc));
		}
	}
	else
	{
		Path buf;

		// TODO: make createOSPath variation that takes a WriteLocation arg
		if (location == WriteLocation::GAME)
			createOSPath(gameDir_, path, buf);
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		file = X_NEW(XDiskFile, &filePoolArena_, "Diskfile")(buf.c_str(), mode);

		if (!file->valid()) {
			closeFile(file);
			file = nullptr;
		}
	}

	return file;

}

void xFileSys::closeFile(XFile* file)
{
	X_ASSERT_NOT_NULL(file);

	X_DELETE(file, &filePoolArena_);
}

// --------------------------------------------------


// async
XFileAsync* xFileSys::openFileAsync(pathType path, fileModeFlags mode, WriteLocation::Enum location)
{
	X_ASSERT_NOT_NULL(path);

	XDiskFileAsync* pFile = nullptr;

	if (mode.IsSet(fileMode::READ))
	{
		_finddatai64_t findinfo;

		XFindData FindData(path, this);
		if (FindData.findnext(&findinfo))
		{
			core::Path real_path;
			FindData.getOSPath(real_path, &findinfo);

			// TODO: pool allocations.
			pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode);

			if (!pFile->valid()) {
				closeFileAsync(pFile);
				pFile = nullptr;
			}
		}
		else
		{
			fileModeFlags::Description Dsc;
			X_WARNING("FileSys", "Failed to find file: %s, Flags: %s",
				path, mode.ToString(Dsc));
		}
	}
	else
	{
		X_ASSERT_NOT_IMPLEMENTED();

	
	}

	return pFile;
}

void xFileSys::closeFileAsync(XFileAsync* file)
{
	X_ASSERT_NOT_NULL(file);
	// meow meow!
	X_DELETE(file, &filePoolArena_);
}

// --------------------------------------------------

XFileMem* xFileSys::openFileMem(pathType path, fileModeFlags mode)
{
	X_ASSERT_NOT_NULL(path);

	XFileMem* pFile = nullptr;

	if (mode.IsSet(fileMode::READ))
	{
		_finddatai64_t findinfo;

		XFindData FindData(path, this);
		if (FindData.findnext(&findinfo))
		{
			core::Path real_path;
			FindData.getOSPath(real_path, &findinfo);

			OsFile file(real_path.c_str(), mode);

			if (file.valid())
			{
				uint32_t size = safe_static_cast<uint32_t,size_t>(file.remainingBytes());
				char* pBuf = X_NEW_ARRAY(char, size, &memFileArena_, "MemBuffer");

				if (file.read(pBuf, size) == size)
				{
					pFile = X_NEW(XFileMem, &filePoolArena_, "Memile")(pBuf, pBuf + size, &memFileArena_);

				}
				else
				{
					X_DELETE_ARRAY(pBuf, &memFileArena_);
				}

			}
		}
		else
		{
			fileModeFlags::Description Dsc;
			X_WARNING("FileSys", "Failed to find file: %s, Flags: %s",
				path, mode.ToString(Dsc));
		}
	}
	else
	{
		X_ASSERT_NOT_IMPLEMENTED();

		X_ERROR("FileSys", "can't open a memory file for writing.");
	}

	return pFile;
}

void xFileSys::closeFileMem(XFileMem* file)
{
	X_ASSERT_NOT_NULL(file);
	// class free's the buffer.
	X_DELETE(file, &filePoolArena_);
}


// --------------------- folders ---------------------

void xFileSys::setGameDir(pathType path)
{
	X_ASSERT(gameDir_ == nullptr, "can only set one game directoy")(path,gameDir_);

	addModDir(path);
	X_ASSERT_NOT_NULL(searchPaths_);
	X_ASSERT_NOT_NULL(searchPaths_->dir);
	gameDir_ = searchPaths_->dir;

	// add hotreload dir.
	gEnv->pDirWatcher->addDirectory(path);
}

void xFileSys::addModDir(pathType path)
{
	// at it to virtual file system.
	search_s* search = X_NEW( search_s, g_coreArena, "FileSysSearch");
	search->dir = X_NEW( directory_s, g_coreArena, "FileSysDir");
	search->dir->path = path;
	search->dir->path.ensureSlash();
	search->pak = nullptr;
	search->next_ = searchPaths_;
	searchPaths_ = search;
}



// --------------------- Find util ---------------------

uintptr_t xFileSys::findFirst(pathType path, _finddatai64_t* findinfo)
{
	X_ASSERT_NOT_NULL(path);
	X_ASSERT_NOT_NULL(findinfo);


	XFindData* pFindData = X_NEW( XFindData, g_coreArena, "FileSysFindData")(path, this);

	if (!pFindData->findnext(findinfo)) {
		X_DELETE(pFindData, g_coreArena);
		return -1;
	}

	// i could store this info in a member so that i can check that
	// the handle is really the object we want.
	// but then that adds a lookup to every findnext call.
	// I'll add one for debugging.
#if X_DEBUG == 1
	findData_.insert(pFindData);
#endif // !X_DEBUG

	return (intptr_t)pFindData;
}

bool xFileSys::findnext(uintptr_t handle, _finddatai64_t* findinfo)
{
	X_ASSERT_NOT_NULL(findinfo);

#if X_DEBUG == 1
	if (findData_.find((XFindData*)handle) == findData_.end()) {
		X_ERROR("FileSys", "FindData is not a valid handle.");
		return false;
	}
#endif // !X_DEBUG

	return ((XFindData*)handle)->findnext(findinfo);
}

void xFileSys::findClose(uintptr_t handle)
{
#if X_DEBUG == 1
	if (findData_.find((XFindData*)handle) == findData_.end()) {
		X_ERROR("FileSys", "FindData is not a valid handle.");
		return;
	}
#endif // !X_DEBUG

	X_DELETE(((XFindData*)handle), g_coreArena);


#if X_DEBUG == 1
	findData_.erase((XFindData*)handle);
#endif // !X_DEBUG
}


// --------------------- Delete ---------------------

bool xFileSys::deleteFile(pathType path) const
{
	return ::DeleteFile(path) == TRUE;
}

bool xFileSys::deleteDirectory(pathType path, bool recursive) const
{
	Path temp;
	core::zero_object(temp); // ensure 2 null bytes at end.

	temp.append(path);

	SHFILEOPSTRUCT file_op = {
		NULL,
		FO_DELETE,
		temp.c_str(),
		"",
		FOF_NOCONFIRMATION |
		FOF_NOERRORUI |
		FOF_SILENT,
		false,
		0,
		"" };

	int ret = SHFileOperation(&file_op);

	X_ERROR_IF(ret != 0, "FileSys", "Failed to delete directory: %s", path);

	return ret == 0; // returns 0 on success, non zero on failure.
}


// --------------------- Create ---------------------

bool xFileSys::createDirectory(pathType path) const
{
	X_ASSERT_NOT_NULL(path);

	if (!::CreateDirectoryA(path, NULL) && lastError::Get() != ERROR_ALREADY_EXISTS)
	{
		lastError::Description Dsc;
		X_ERROR("xDir", "Failed to create directory. Error: %s", lastError::ToString(Dsc));
		return false;
	}

	return true;
}

bool xFileSys::createDirectoryTree(pathType _path) const
{
	X_ASSERT_NOT_NULL(_path);

	// we want to just loop and create like a goat.

	// c:\\dir\\goat\\win\\bin
	Path Path("");

	const char* Start = _path;
	const char* End = _path;

	lopi(MAX_PATH)
	{
		if (*End == '\0')
			break;

		if (*End == ':')
			End += 2;

		if (*End == '\\')
		{
			Path.append(Start, ++End);

			Start = End;

			if (!directoryExists(Path.c_str()))
			{
				if (!::CreateDirectoryA(Path.c_str(), NULL))
				{
					lastError::Description Dsc;
					X_ERROR("xDir", "Failed to create directory. Error: %s", lastError::ToString(Dsc));
					return false;
				}
			}
		}

		++End;
	}

	return true;
}



// --------------------- exsists ---------------------

bool xFileSys::fileExists(pathType path) const
{
	X_ASSERT_NOT_NULL(path);

	DWORD dwAttrib = GetFileAttributes(path);

	if (dwAttrib != INVALID_FILE_ATTRIBUTES) // make sure we did not fail for some shit, like permissions
	{
		if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) // herp derp.
		{
			X_ERROR("Dir", "fileExsits check was ran on a directory");
			return false;
		}

		return true; // hi there Mr File
	}

	if (lastError::Get() != ERROR_FILE_NOT_FOUND)
	{
		lastError::Description Dsc;
		X_ERROR("Dir", "fileExsits failed. Error: %s", lastError::ToString(Dsc));
	}

	return false;
}

bool xFileSys::directoryExists(pathType path) const
{
	X_ASSERT_NOT_NULL(path);

	DWORD dwAttrib = GetFileAttributes(path);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


// --------------------------------------------------


// Ajust path
const char* xFileSys::createOSPath(directory_s* dir, pathType path, Path& buffer) const
{
	// is it absolute?
	if (!isAbsolute(path)) {

		buffer = dir->path / path;
	}
	else {
		// the engine should never be trying to load a absolute path.
		// unless filesystem been used in a tool.
		// but then again tool could probs just use relative paths also.
		// i think I'll only disable it when used in game.exe

		buffer = path;
	}

	buffer.replaceSeprators();
	return buffer.c_str();
}

bool xFileSys::isAbsolute(pathType path) const
{
	return	path[0] == NATIVE_SLASH ||
			path[0] == NON_NATIVE_SLASH ||
			path[1] == ':';
}

// ----------------------- stats ---------------------------


XFileStats& xFileSys::getStats(void) const
{
	return OsFile::fileStats();
}




X_NAMESPACE_END