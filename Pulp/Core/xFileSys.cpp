#include "stdafx.h"
#include "xFileSys.h"
#include "DiskFile.h"
#include "DiskFileAsync.h"

#include <Util\LastError.h>
#include <String\StackString.h>

#include <direct.h>
#include <io.h>
#include <IConsole.h>

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


bool xFileSys::Init()
{
	X_LOG0("FileSys", "Starting Filesys..");

	// TODO: yup.
	return setGameDir("C:\\Users\\WinCat\\Documents\\code\\WinCat\\engine\\potatoengine\\game_folder");
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

void xFileSys::CreateVars(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);

	ADD_CVAR_REF("filesys_debug", vars_.debug, 0, 0, 1, core::VarFlag::SYSTEM, "Filesystem debug. 0=off 1=on");
}

// --------------------- Open / Close ---------------------

XFile* xFileSys::openFile(pathType path, fileModeFlags mode, VirtualDirectory::Enum location)
{
	X_ASSERT_NOT_NULL(path);

	XDiskFile* file = nullptr;
	core::Path real_path;

	if (mode.IsSet(fileMode::READ) && !mode.IsSet(fileMode::WRITE) )
	{
		_finddatai64_t findinfo;

		XFindData FindData(path, this);
		if (FindData.findnext(&findinfo))
		{
			FindData.getOSPath(real_path, &findinfo);

			if (isDebug()) {
				X_LOG0("FileSys", "openFile: \"%s\"", real_path.c_str());
			}

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
		// TODO: make createOSPath variation that takes a VirtualDirectory arg
		if (location == VirtualDirectory::GAME)
			createOSPath(gameDir_, path, real_path);
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		if (isDebug()) {
			X_LOG0("FileSys", "openFile: \"%s\"", real_path.c_str());
		}

		file = X_NEW(XDiskFile, &filePoolArena_, "Diskfile")(real_path.c_str(), mode);

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
XFileAsync* xFileSys::openFileAsync(pathType path, fileModeFlags mode, VirtualDirectory::Enum location)
{
	X_ASSERT_NOT_NULL(path);

	XDiskFileAsync* pFile = nullptr;
	core::Path real_path;

	if (mode.IsSet(fileMode::READ) && !mode.IsSet(fileMode::WRITE))
	{
		_finddatai64_t findinfo;

		XFindData FindData(path, this);
		if (FindData.findnext(&findinfo))
		{
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
		if (location == VirtualDirectory::GAME)
			createOSPath(gameDir_, path, real_path);
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode);

		if (!pFile->valid()) {
			closeFileAsync(pFile);
			pFile = nullptr;
		}
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
					pFile = X_NEW(XFileMem, &filePoolArena_, "MemFile")(pBuf, pBuf + size, &memFileArena_);

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

bool xFileSys::setGameDir(pathType path)
{
	X_ASSERT(gameDir_ == nullptr, "can only set one game directoy")(path,gameDir_);

	// check if the irectory is even valid.
	if (!this->directoryExists(path)) {
		X_ERROR("FileSys", "Faled to set game drectory the directory does not exsists: \"%s\"", path);
		return false;
	}

	addModDir(path);
	X_ASSERT_NOT_NULL(searchPaths_);
	X_ASSERT_NOT_NULL(searchPaths_->dir);
	gameDir_ = searchPaths_->dir;

	// add hotreload dir.
	gEnv->pDirWatcher->addDirectory(path);
	return true;
}

void xFileSys::addModDir(pathType path)
{
	if (isDebug()) {
		X_LOG0("FileSys", "addModDir: \"%s\"", path);
	}

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

bool xFileSys::deleteFile(pathType path, VirtualDirectory::Enum location) const
{
	Path buf;
	createOSPath(gameDir_, path, buf);

	if (isDebug()) {
		X_LOG0("FileSys", "deleteFile: \"%s\"", buf.c_str());
	}

	return ::DeleteFileA(buf.c_str()) == TRUE;
}

bool xFileSys::deleteDirectory(pathType path, bool recursive) const
{
	Path temp;
	core::zero_object(temp); // ensure 2 null bytes at end.

	createOSPath(gameDir_, path, temp);

	if (isDebug()) {
		X_LOG0("FileSys", "deleteDirectory: \"%s\"", temp.c_str());
	}

	wchar_t wPath[512];

	SHFILEOPSTRUCT file_op = {
		NULL,
		FO_DELETE,
		core::strUtil::Convert(temp.c_str(), wPath),
		L"",
		FOF_NOCONFIRMATION |
		FOF_NOERRORUI |
		FOF_SILENT,
		false,
		0,
		L"" };

	int ret = SHFileOperation(&file_op);

	X_ERROR_IF(ret != 0, "FileSys", "Failed to delete directory: %s", path);

	return ret == 0; // returns 0 on success, non zero on failure.
}


// --------------------- Create ---------------------

bool xFileSys::createDirectory(pathType path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);

	Path buf;
	createOSPath(gameDir_, path, buf);

	if (isDebug()) {
		X_LOG0("FileSys", "createDirectory: \"%s\"", buf.c_str());
	}

	if (!::CreateDirectoryA(buf.c_str(), NULL) && lastError::Get() != ERROR_ALREADY_EXISTS)
	{
		lastError::Description Dsc;
		X_ERROR("FileSys", "Failed to create directory. Error: %s", lastError::ToString(Dsc));
		return false;
	}

	return true;
}

bool xFileSys::createDirectoryTree(pathType _path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(_path);

	// we want to just loop and create like a goat.
	Path buf;

	createOSPath(gameDir_, _path, buf);

	if (isDebug()) {
		X_LOG0("FileSys", "CreateDirectoryTree: \"%s\"", buf.c_str());
	}

	// c:\\dir\\goat\\win\\bin
	Path Path("");

	const char* Start = buf.begin();
	const char* End = buf.begin();

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

bool xFileSys::fileExists(pathType path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);

	Path buf;
	createOSPath(gameDir_, path, buf);

	if (isDebug()) {
		X_LOG0("FileSys", "fileExists: \"%s\"", buf.c_str());
	}

	DWORD dwAttrib = GetFileAttributesA(buf.c_str());

	if (dwAttrib != INVALID_FILE_ATTRIBUTES) // make sure we did not fail for some shit, like permissions
	{
		if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) // herp derp.
		{
			X_ERROR("FileSys", "FileExsits check was ran on a directory");
			return false;
		}

		return true; // hi there Mr File
	}

	// This means we checked for a file in a directory that don't exsists.
	if (lastError::Get() == ERROR_PATH_NOT_FOUND)
	{
		X_LOG2("FileSys", "FileExsits failed, the target directory does not exsist: \"%s\"", 
			buf.c_str());
	}
	else if (lastError::Get() != ERROR_FILE_NOT_FOUND)
	{
		lastError::Description Dsc;
		X_ERROR("FileSys", "FileExsits failed. Error: %s", lastError::ToString(Dsc));
	}

	return false;
}

bool xFileSys::directoryExists(pathType path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);

	if (isDebug()) {
		X_LOG0("FileSys", "directoryExists: \"%s\"", path);
	}

	DWORD dwAttrib = GetFileAttributesA(path);

	if (dwAttrib != INVALID_FILE_ATTRIBUTES) // make sure we did not fail for some shit, like permissions
	{
		if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
		{
			// found
			return true;
		}

		X_ERROR("FileSys", "DirectoryExists check was ran on a File: \"%s\"", path);
		return false; 
	}

	if (lastError::Get() != ERROR_PATH_NOT_FOUND && lastError::Get() != ERROR_FILE_NOT_FOUND)
	{
		lastError::Description Dsc;
		X_ERROR("FileSys", "DirectoryExists failed. Error: %s", lastError::ToString(Dsc));
	}

	return false;
}

bool xFileSys::isDirectory(pathType path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);

	if (isDebug()) {
		X_LOG0("FileSys", "isDirectory: \"%s\"", path);
	} 

	DWORD dwAttrib = GetFileAttributesA(path);

	if (dwAttrib != INVALID_FILE_ATTRIBUTES) {
		if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
			return true;
		}
		return false;
	}

	if (lastError::Get() != INVALID_FILE_ATTRIBUTES)
	{
		lastError::Description Dsc;
		X_ERROR("FileSys", "isDirectory failed. Error: %s", lastError::ToString(Dsc));
	}

	return false;
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

bool xFileSys::isDebug(void) const
{
	return vars_.debug != 0;
}

// ----------------------- stats ---------------------------


XFileStats& xFileSys::getStats(void) const
{
	return OsFile::fileStats();
}




X_NAMESPACE_END