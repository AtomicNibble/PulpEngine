#include "stdafx.h"
#include "xFileSys.h"
#include "DiskFile.h"
#include "DiskFileAsync.h"

#include "PathUtil.h"

#include <Util\LastError.h>

#include <IConsole.h>
#include <IDirectoryWatcher.h>

#include "XFindData.h"

#include <IAssetPak.h>

#include <Memory\VirtualMem.h>
#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(core)

namespace
{
	// might need more since memory files will be from pool.
	static const size_t MAX_FILE_HANDLES = 1024;


	static const size_t FILE_ALLOCATION_SIZE = core::Max(sizeof(XDiskFile), 
		core::Max(sizeof(XDiskFileAsync), sizeof(XFileMem)));


	static const size_t FILE_ALLOCATION_ALIGN = core::Max( X_ALIGN_OF(XDiskFile),
		core::Max(X_ALIGN_OF(XDiskFileAsync), X_ALIGN_OF(XFileMem)));


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
	memFileArena_(&memfileAllocator_, "MemFileArena"),
	ioQue_(gEnv->pArena, IO_QUE_SIZE)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);

	gEnv->pArena->addChildArena(&filePoolArena_);
}

xFileSys::~xFileSys()
{

}


bool xFileSys::Init(const SCoreInitParams& params)
{
	X_ASSERT_NOT_NULL(gEnv->pCore);
	X_LOG0("FileSys", "Starting Filesys..");

	if (!InitDirectorys(params.FileSysWorkingDir())) {
		X_ERROR("FileSys", "Failed to set game directories");
		return false;
	}


	return true;
}

bool xFileSys::InitWorker(void)
{
	if (!StartRequestWorker()) {
		X_ERROR("FileSys", "Failed to start io request worker");
		return false;
	}

	return true;
}

void xFileSys::ShutDown()
{
	X_LOG0("FileSys", "Shutting Down");

	ShutDownRequestWorker();

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

	ADD_CVAR_REF("filesys_debug", vars_.debug, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, 
		"Filesystem debug. 0=off 1=on");
	ADD_CVAR_REF("filesys_Quedebug", vars_.QueDebug, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, 
		"Filesystem que debug. 0=off 1=on");

	// create vars for the virtual directories which we then update with the paths once set.
	size_t i;
	core::StackString<64> name;
	for (i = 0; i < MAX_VIRTUAL_DIR; i++)
	{
		name.set("filesys_mod_dir_");
		name.appendFmt("%" PRIuS, i);
		vars_.pVirtualDirs[i] = ADD_CVAR_STRING(name.c_str(), "",
			core::VarFlag::SYSTEM | 
			core::VarFlag::READONLY |
			core::VarFlag::CPY_NAME,
			"Virtual mod directory");
	}
}

bool xFileSys::InitDirectorys(bool working)
{
	// check if game dir set via cmd line.
	const wchar_t* pGameDir = gEnv->pCore->GetCommandLineArgForVarW(L"fs_basepath");
	if (pGameDir)
	{
		core::Path<wchar_t> base(pGameDir);
		base.ensureSlash();

		core::Path<wchar_t> core(base);
		core /= L"core_assets\\";

		core::Path<wchar_t> mod(base);
		mod /= L"mod\\";

		core::Path<wchar_t> testAssets(base);
		testAssets /= L"test_assets\\";

		if (setGameDir(core.c_str()))
		{
			// add mod dir's
			addModDir(mod.c_str());
			addModDir(testAssets.c_str());
			return true;
		}
	}
	else if (working)
	{
		// working dir added.
		core::Path<wchar_t> path = PathUtil::GetCurrentDirectory();
		return setGameDir(path.c_str());
	}
	else
	{
		core::Path<wchar_t> curDir = PathUtil::GetCurrentDirectory();


		core::Path<wchar_t> base(curDir);
		base /= L"\\..\\..\\..\\potatoengine\\game_folder\\";

		core::Path<wchar_t> core(base);
		core /= L"core_assets\\";

		core::Path<wchar_t> mod(base);
		mod /= L"mod\\";

		core::Path<wchar_t> testAssets(base);
		testAssets /= L"test_assets\\";

		// TODO: yup.
		if (setGameDir(core.c_str()))
		{
			// add mod dir's
			addModDir(mod.c_str());
			addModDir(testAssets.c_str());
			return true;
		}
		else
		{

			// set as working.
			return setGameDir(curDir.c_str());
		}
	}

	return false;
}

// --------------------- Open / Close ---------------------

XFile* xFileSys::openFile(pathType path, fileModeFlags mode, VirtualDirectory::Enum location)
{
	X_ASSERT_NOT_NULL(path);

	XDiskFile* file = nullptr;
	core::Path<wchar_t> real_path;

	if (mode.IsSet(fileMode::READ) && !mode.IsSet(fileMode::WRITE) )
	{
		_wfinddatai64_t findinfo;

		XFindData FindData(path, this);
		if (FindData.findnext(&findinfo))
		{
			FindData.getOSPath(real_path, &findinfo);

			if (isDebug()) {
				X_LOG0("FileSys", "openFile: \"%ls\"", real_path.c_str());
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
		if (location == VirtualDirectory::GAME) {
			createOSPath(gameDir_, path, real_path);
		}
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		if (isDebug()) {
			X_LOG0("FileSys", "openFile: \"%ls\"", real_path.c_str());
		}

		file = X_NEW(XDiskFile, &filePoolArena_, "Diskfile")(real_path.c_str(), mode);

		if (!file->valid()) {
			closeFile(file);
			file = nullptr;
		}
	}

	return file;
}

XFile* xFileSys::openFile(pathTypeW path, fileModeFlags mode, VirtualDirectory::Enum location)
{
	X_ASSERT_NOT_NULL(path);

	XDiskFile* file = nullptr;
	core::Path<wchar_t> real_path;

	if (mode.IsSet(fileMode::READ) && !mode.IsSet(fileMode::WRITE))
	{
		_wfinddatai64_t findinfo;

		XFindData FindData(path, this);
		if (FindData.findnext(&findinfo))
		{
			FindData.getOSPath(real_path, &findinfo);

			if (isDebug()) {
				X_LOG0("FileSys", "openFile: \"%ls\"", real_path.c_str());
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
			X_WARNING("FileSys", "Failed to find file: %ls, Flags: %s",
				path, mode.ToString(Dsc));
		}
	}
	else
	{
		// TODO: make createOSPath variation that takes a VirtualDirectory arg
		if (location == VirtualDirectory::GAME) {
			createOSPath(gameDir_, path, real_path);
		}
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		if (isDebug()) {
			X_LOG0("FileSys", "openFile: \"%ls\"", real_path.c_str());
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
	core::Path<wchar_t> real_path;

	if (mode.IsSet(fileMode::READ) && !mode.IsSet(fileMode::WRITE))
	{
		_wfinddatai64_t findinfo;

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

XFileAsync* xFileSys::openFileAsync(pathTypeW path, fileModeFlags mode, VirtualDirectory::Enum location)
{
	X_ASSERT_NOT_NULL(path);

	XDiskFileAsync* pFile = nullptr;
	core::Path<wchar_t> real_path;

	if (mode.IsSet(fileMode::READ) && !mode.IsSet(fileMode::WRITE))
	{
		_wfinddatai64_t findinfo;

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
			X_WARNING("FileSys", "Failed to find file: %ls, Flags: %s",
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
		_wfinddatai64_t findinfo;

		XFindData FindData(path, this);
		if (FindData.findnext(&findinfo))
		{
			core::Path<wchar_t> real_path;
			FindData.getOSPath(real_path, &findinfo);

			OsFile file(real_path.c_str(), mode);

			if (file.valid())
			{
				size_t size = safe_static_cast<size_t, int64_t>(file.remainingBytes());
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


XFileMem* xFileSys::openFileMem(pathTypeW path, fileModeFlags mode)
{
	X_ASSERT_NOT_NULL(path);

	XFileMem* pFile = nullptr;

	if (mode.IsSet(fileMode::READ))
	{
		_wfinddatai64_t findinfo;

		XFindData FindData(path, this);
		if (FindData.findnext(&findinfo))
		{
			core::Path<wchar_t> real_path;
			FindData.getOSPath(real_path, &findinfo);

			OsFile file(real_path.c_str(), mode);

			if (file.valid())
			{
				size_t size = safe_static_cast<size_t, int64_t>(file.remainingBytes());
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
			X_WARNING("FileSys", "Failed to find file: %ls, Flags: %s",
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

bool xFileSys::setGameDir(pathTypeW path)
{
	X_ASSERT(gameDir_ == nullptr, "can only set one game directoy")(path,gameDir_);

	// check if the irectory is even valid.
	if (!this->directoryExistsOS(path)) {
		X_ERROR("FileSys", "Faled to set game drectory the directory does not exsists: \"%ls\"", path);
		return false;
	}

	addModDir(path);
	X_ASSERT_NOT_NULL(searchPaths_);
	X_ASSERT_NOT_NULL(searchPaths_->dir);
	gameDir_ = searchPaths_->dir;

	return true;
}

void xFileSys::addModDir(pathTypeW path)
{
	if (isDebug()) {
		X_LOG0("FileSys", "addModDir: \"%ls\"", path);
	}

	if (!this->directoryExistsOS(path)) {
		X_ERROR("FileSys", "Faled to add mod drectory, the directory does not exsists: \"%s\"", path);
		return;
	}

	// ok remove any ..//
	DWORD  retval = 0;
	TCHAR  fixedPath[512];

	retval = GetFullPathNameW(path, 512, fixedPath, nullptr);
	if (retval == 0)
	{
		core::lastError::Description Dsc;
		X_ERROR("FileSys", "addModDir full path name creation failed: %s", 
			core::lastError::ToString(Dsc));
		return;
	}

	if (!this->directoryExistsOS(fixedPath)) {
		X_ERROR("FileSys", "Fixed path does not exsists: \"%ls\"", fixedPath);
		return;
	}

	// work out if this directory is a sub directory of any of the current
	// searxh paths.
	for (search_s* s = searchPaths_; s; s = s->next_)
	{
		if (strUtil::FindCaseInsensitive(fixedPath, s->dir->path.c_str()) != nullptr)
		{
			X_ERROR("FileSys", "mod dir is identical or inside a current mod dir: \"%ls\" -> \"%ls\"",
				fixedPath, s->dir->path.c_str());
			return;
		}
	}

	// at it to virtual file system.
	search_s* search = X_NEW( search_s, g_coreArena, "FileSysSearch");
	search->dir = X_NEW( directory_s, g_coreArena, "FileSysDir");
	search->dir->path = fixedPath;
	search->dir->path.ensureSlash();
	search->pak = nullptr;
	search->next_ = searchPaths_;
	searchPaths_ = search;

	// add hotreload dir.
	gEnv->pDirWatcher->addDirectory(fixedPath);
}



// --------------------- Find util ---------------------

uintptr_t xFileSys::findFirst(pathType path, _wfinddatai64_t* findinfo)
{
	X_ASSERT_NOT_NULL(path);
	X_ASSERT_NOT_NULL(findinfo);


	XFindData* pFindData = X_NEW( XFindData, g_coreArena, "FileSysFindData")(path, this);

	if (!pFindData->findnext(findinfo)) {
		X_DELETE(pFindData, g_coreArena);
		return static_cast<uintptr_t>(-1);
	}

	// i could store this info in a member so that i can check that
	// the handle is really the object we want.
	// but then that adds a lookup to every findnext call.
	// I'll add one for debugging.
#if X_DEBUG == 1
	findData_.insert(pFindData);
#endif // !X_DEBUG

	return reinterpret_cast<uintptr_t>(pFindData);
}

bool xFileSys::findnext(uintptr_t handle, _wfinddatai64_t* findinfo)
{
	X_ASSERT_NOT_NULL(findinfo);

#if X_DEBUG == 1
	if (findData_.find(reinterpret_cast<XFindData*>(handle)) == findData_.end()) {
		X_ERROR("FileSys", "FindData is not a valid handle.");
		return false;
	}
#endif // !X_DEBUG

	return reinterpret_cast<XFindData*>(handle)->findnext(findinfo);
}

void xFileSys::findClose(uintptr_t handle)
{
	XFindData* pFindData = reinterpret_cast<XFindData*>(handle);
#if X_DEBUG == 1

	if (findData_.find(pFindData) == findData_.end()) {
		X_ERROR("FileSys", "FindData is not a valid handle.");
		return;
	}
#endif // !X_DEBUG

	X_DELETE(pFindData, g_coreArena);


#if X_DEBUG == 1
	findData_.erase(pFindData);
#endif // !X_DEBUG
}


// --------------------- Delete ---------------------

bool xFileSys::deleteFile(pathType path, VirtualDirectory::Enum location) const
{
	X_UNUSED(location);

	Path<wchar_t> buf;
	createOSPath(gameDir_, path, buf);

	if (isDebug()) {
		X_LOG0("FileSys", "deleteFile: \"%ls\"", buf.c_str());
	}

	return PathUtil::DeleteFile(buf);
}

bool xFileSys::deleteDirectory(pathType path, bool recursive) const
{
	Path<wchar_t> temp;

	// this needs replacing with more robost logic.
	core::zero_object(temp); // ensure 2 null bytes at end.

	createOSPath(gameDir_, path, temp);

	if (isDebug()) {
		X_LOG0("FileSys", "deleteDirectory: \"%ls\"", temp.c_str());
	}

	return PathUtil::DeleteDirectory(temp, recursive);
}


// --------------------- Create ---------------------

bool xFileSys::createDirectory(pathType path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);
	X_UNUSED(location);

	Path<wchar_t> buf;
	createOSPath(gameDir_, path, buf);

	if (isDebug()) {
		X_LOG0("FileSys", "createDirectory: \"%ls\"", buf.c_str());
	}

	return PathUtil::CreateDirectory(buf);
}

bool xFileSys::createDirectoryTree(pathType _path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(_path);
	X_UNUSED(location);

	// we want to just loop and create like a goat.
	Path<wchar_t> buf;

	createOSPath(gameDir_, _path, buf);

	buf.removeFileName();

	if (isDebug()) {
		X_LOG0("FileSys", "CreateDirectoryTree: \"%ls\"", buf.c_str());
	}

	return PathUtil::CreateDirectoryTree(buf);
}



// --------------------- exsists ---------------------

bool xFileSys::fileExists(pathType path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);
	Path<wchar_t> buf;

	if (location == VirtualDirectory::GAME) {
		createOSPath(gameDir_, path, buf);
	}
	else {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return fileExistsOS(buf);
}

bool xFileSys::fileExists(pathTypeW path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);
	Path<wchar_t> buf;

	if (location == VirtualDirectory::GAME) {
		createOSPath(gameDir_, path, buf);
	}
	else {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return fileExistsOS(buf);
}

bool xFileSys::directoryExists(pathType path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);
	Path<wchar_t> buf;

	if (location == VirtualDirectory::GAME) {
		createOSPath(gameDir_, path, buf);
	}
	else {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return directoryExistsOS(buf);
}

bool xFileSys::directoryExists(pathTypeW path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);
	Path<wchar_t> buf;

	if (location == VirtualDirectory::GAME) {
		createOSPath(gameDir_, path, buf);
	}
	else {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return directoryExistsOS(buf);
}


bool xFileSys::isDirectory(pathType path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);
	Path<wchar_t> buf;

	if (location == VirtualDirectory::GAME) {
		createOSPath(gameDir_, path, buf);
	}
	else {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return isDirectoryOS(buf);
}

bool xFileSys::isDirectory(pathTypeW path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);
	Path<wchar_t> buf;

	if (location == VirtualDirectory::GAME) {
		createOSPath(gameDir_, path, buf);
	}
	else {
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return isDirectoryOS(buf);
}

bool xFileSys::moveFile(pathType path, pathType newPath) const
{
	Path<wchar_t> pathOs, newPathOs;

	createOSPath(gameDir_, path, pathOs);
	createOSPath(gameDir_, newPath, newPathOs);

	return moveFileOS(pathOs, newPathOs);
}

bool xFileSys::moveFile(pathTypeW path, pathTypeW newPath) const
{
	Path<wchar_t> pathOs, newPathOs;

	createOSPath(gameDir_, path, pathOs);
	createOSPath(gameDir_, newPath, newPathOs);

	return moveFileOS(pathOs, newPathOs);
}


size_t xFileSys::getMinimumSectorSize(void) const
{
	// get all the driver letters.
	size_t sectorSize = 0;
	fileModeFlags mode(fileMode::READ);

	core::FixedArray<wchar_t, 128> DriveLettters;


	for (search_s* s = searchPaths_; s; s = s->next_)
	{
		if (s->dir)
		{
			const core::Path<wchar_t>& path = s->dir->path;	
			int8_t driveIdx = path.getDriveNumber(); // watch this be a bug at somepoint.
			if(driveIdx >= 0)
			{
				wchar_t driveLetter = (L'A' + driveIdx);

				if (std::find(DriveLettters.begin(), DriveLettters.end(), driveLetter) == DriveLettters.end()) {
					DriveLettters.append(driveLetter);
				}		
			}
		}
	}

	for (const auto d : DriveLettters)
	{
		core::StackString<64, wchar_t> device(L"\\\\.\\");

		device.append(d, 1);
		device.append(L":");

		OsFile::DiskInfo info;
		if (OsFile::getDiskInfo(device.c_str(), info)) {
			sectorSize = core::Max(sectorSize, info.physicalSectorSize);
		}
		else {
			// if we fail to query bump it up to 4096 incase the one that failed needed 4096.
			sectorSize = core::Max<size_t>(sectorSize, 0x4096);
		}
	}


	if (sectorSize == 0) {
		sectorSize = 4096;
	}

	X_LOG2("FileSys", "Minimumsector size for all virtual dir: %" PRIuS, sectorSize);
	return sectorSize;
}


// --------------------------------------------------

bool xFileSys::fileExistsOS(const core::Path<wchar_t>& fullPath) const
{
	return core::PathUtil::DoesFileExist(fullPath);
}

bool xFileSys::directoryExistsOS(const core::Path<wchar_t>& fullPath) const
{
	return core::PathUtil::DoesDirectoryExist(fullPath);
}


bool xFileSys::isDirectoryOS(const core::Path<wchar_t>& fullPath) const
{
	bool result = core::PathUtil::IsDirectory(fullPath);

	if (isDebug()) {
		X_LOG0("FileSys", "isDirectory: \"%ls\" res: ^6%s",
			fullPath.c_str(), result ? "TRUE" : "FALSE");
	}

	return result;
}


bool xFileSys::moveFileOS(const core::Path<wchar_t>& fullPath, const core::Path<wchar_t>& fullPathNew) const
{
	bool result = core::PathUtil::MoveFile(fullPath, fullPathNew);

	if (isDebug()) {
		X_LOG0("FileSys", "moveFile: \"%ls\" -> \"%ls\" res: ^6%s",
			fullPath.c_str(), fullPathNew.c_str(), result ? "TRUE" : "FALSE");
	}

	return result;
}

bool xFileSys::fileExistsOS(const wchar_t* pFullPath) const
{
	return core::PathUtil::DoesFileExist(pFullPath);
}

bool xFileSys::directoryExistsOS(const wchar_t* pFullPath) const
{
	return core::PathUtil::DoesDirectoryExist(pFullPath);
}


bool xFileSys::isDirectoryOS(const wchar_t* pFullPath) const
{
	bool result = core::PathUtil::IsDirectory(pFullPath);

	if (isDebug()) {
		X_LOG0("FileSys", "isDirectory: \"%ls\" res: ^6%s",
			pFullPath, result ? "TRUE" : "FALSE");
	}

	return result;
}


// --------------------------------------------------

// Ajust path
const wchar_t* xFileSys::createOSPath(directory_s* dir, pathType path, Path<wchar_t>& buffer) const
{
	wchar_t pathW[256];
	strUtil::Convert(path, pathW, sizeof(pathW));

	// is it absolute?
	if (!isAbsolute(path)) {

		buffer = dir->path / pathW;
	}
	else {
		// the engine should never be trying to load a absolute path.
		// unless filesystem been used in a tool.
		// but then again tool could probs just use relative paths also.
		// i think I'll only disable it when used in game.exe

		buffer = pathW;
	}

	buffer.replaceSeprators();
	return buffer.c_str();
}

const wchar_t* xFileSys::createOSPath(directory_s* dir, pathTypeW path, Path<wchar_t>& buffer) const
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

bool xFileSys::isAbsolute(pathTypeW path) const
{
	return	path[0] == NATIVE_SLASH_W ||
		path[0] == NON_NATIVE_SLASH_W ||
		path[1] == L':';
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


// ----------------------- io que ---------------------------


void xFileSys::AddIoRequestToQue(const IoRequestData& request)
{
	if (request.getType() == IoRequest::CLOSE) {
		if (request.callback) {
			X_ERROR("FileSys", "Close request can't have a callback. pfile: %p", request.closeInfo.pFile);
			return;
		}
	}

	if (request.getType() == IoRequest::READ) {
		if (request.readInfo.dataSize == 592) {
	//		::DebugBreak();
		}
	}

	if (vars_.QueDebug)
	{
		uint32_t threadId = core::Thread::GetCurrentID();

		X_LOG0("FileSys","IoRequest(0x%x) type: %s", threadId,
			IoRequest::ToString(request.getType()));
	}

	ioQue_.push(request);
}

bool xFileSys::StartRequestWorker(void)
{
	ThreadAbstract::Create("IoWorker", 1024 * 128); // small stack
	ThreadAbstract::Start();
	return true;
}


void xFileSys::ShutDownRequestWorker(void)
{
	ThreadAbstract::Stop();

	// post a close job with a none null callback.
	IoRequestData closeRequest;
	closeRequest.setType(IoRequest::CLOSE);
	::memset(&closeRequest.callback, 1, sizeof(closeRequest.callback));

	// push a few close jobs on to give chance for thread should run to get updated.
	for (size_t i = 0; i < 10; i++) {
		ioQue_.push(closeRequest);
	}
	
	ThreadAbstract::Join();
}


Thread::ReturnValue xFileSys::ThreadRun(const Thread& thread)
{
	IoRequestData request;

	struct PendingOp
	{
		PendingOp(const IoRequestData& req, const XFileAsyncOperation& op) :
			request(req), op(op) {
		}

		IoRequestData request;
		XFileAsyncOperation op;
	};


	typedef core::FixedArray<PendingOp, IO_QUE_SIZE> AsyncOps;

	AsyncOps pendingOps;

	gEnv->pJobSys->CreateQueForCurrentThread();

	xFileSys& fileSys = *this;

	while (thread.ShouldRun())
	{
		if (pendingOps.isEmpty())
		{
			ioQue_.pop(request);
		}
		else
		{
			// poll the pending io requests to see if any have finished.
			AsyncOps::size_type i;
			for (i = 0; i < pendingOps.size(); i++)
			{
				PendingOp& asyncOp = pendingOps[i];

				uint32_t bytesTransferd = 0;
				if (asyncOp.op.hasFinished(&bytesTransferd))
				{
					const IoRequestData& asyncReq = asyncOp.request;

					if (vars_.QueDebug)
					{
						uint32_t threadId = core::Thread::GetCurrentID();

						X_LOG0("FileSys", "IoRequest(0x%x) '%s' async request complete. "
							"bytesTrans: 0x%x pBuf: %p", 
							threadId, IoRequest::ToString(asyncReq.getType()),
							bytesTransferd, asyncReq.readInfo.pBuf);
					}

					XFileAsync* pReqFile = nullptr;

					if (asyncReq.getType() == IoRequest::READ) {
						pReqFile = asyncReq.readInfo.pFile;
					}
					else {
						pReqFile = asyncReq.writeInfo.pFile;
					}

					asyncReq.callback.Invoke(fileSys, asyncOp.request,
						pReqFile, bytesTransferd);

					pendingOps.removeIndex(i);
				}
			}

			// got any requests?
			if (!ioQue_.tryPop(request)) {
				core::Thread::Sleep(0);
				continue;
			}
		}

		if (request.getType() == IoRequest::OPEN)
		{
			IoRequestOpen& open = request.openInfo;
			XFileAsync* pFile =	openFileAsync(open.name.c_str(), open.mode);

			request.callback.Invoke(fileSys, request, pFile, 0);
		}
		else if (request.getType() == IoRequest::OPEN_READ_ALL)
		{
			X_ASSERT_NOT_IMPLEMENTED();
		//	IoRequestOpen& open = request.openInfo;
		//	XFileMem* pFile = openFileMem(open.name.c_str(), open.mode);
		//	bool operationSucced = pFile != nullptr;


		//	request.callback.Invoke(this, request.getType(), pFile, operationSucced);
		}
		else if (request.getType() == IoRequest::CLOSE)
		{
			// if the close job has a callback, we are shutting down.
			if (request.callback) {
				continue;
			}

			// normal close request.
			IoRequestClose& close = request.closeInfo;

			closeFileAsync(close.pFile);
		}
		else if (request.getType() == IoRequest::READ)
		{
			IoRequestRead& read = request.readInfo;
			XFileAsync* pFile = read.pFile;

			XFileAsyncOperation operation = pFile->readAsync(
				read.pBuf,
				read.dataSize,
				read.offset
			);

			pendingOps.append(PendingOp(request,operation));
		}
		else if (request.getType() == IoRequest::WRITE)
		{
			IoRequestRead& write = request.writeInfo;
			XFileAsync* pFile = write.pFile;

			XFileAsyncOperation operation = pFile->readAsync(
				write.pBuf,
				write.dataSize,
				write.offset
			);

			pendingOps.append(PendingOp(request, operation));
		}
	}

	return Thread::ReturnValue(0);
}



X_NAMESPACE_END