#include "stdafx.h"
#include "xFileSys.h"
#include "DiskFile.h"
#include "DiskFileAsync.h"

#include "PathUtil.h"

#include <Util\LastError.h>
#include <Util\UniquePointer.h>

#include <IConsole.h>
#include <IDirectoryWatcher.h>

#include "XFindData.h"

#include <IAssetPak.h>

#include <Memory\VirtualMem.h>
#include <Memory\MemCursor.h>
#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>
#include <String\StringHash.h>

X_NAMESPACE_BEGIN(core)

namespace
{
	// might need more since memory files will be from pool.
	const size_t MAX_FILE_HANDLES = 1024;
	const size_t MAX_ASYNC_OP = 1024;

	const size_t FILE_ALLOCATION_SIZE = core::Max(sizeof(XDiskFile),
		core::Max(sizeof(XDiskFileAsync), sizeof(XFileMem)));

	const size_t FILE_ALLOCATION_ALIGN = core::Max(X_ALIGN_OF(XDiskFile),
		core::Max(X_ALIGN_OF(XDiskFileAsync), X_ALIGN_OF(XFileMem)));


	const size_t ASYNC_OP_ALLOCATION_SIZE = sizeof(XOsFileAsyncOperation::AsyncOp);
	const size_t ASYNC_OP_ALLOCATION_ALIGN = X_ALIGN_OF(XOsFileAsyncOperation::AsyncOp);


	static_assert(IoRequest::ENUM_COUNT == 6, "Enum count changed? this logic needs updating");

	constexpr const size_t ioReqSize[IoRequest::ENUM_COUNT] = {
		sizeof(IoRequestClose),
		sizeof(IoRequestOpen),
		sizeof(IoRequestOpenWrite),
		sizeof(IoRequestOpenRead),
		sizeof(IoRequestWrite),
		sizeof(IoRequestRead)
	};

	static_assert(ioReqSize[IoRequest::OPEN] == sizeof(IoRequestOpen), "Enum mismtach?");
	static_assert(ioReqSize[IoRequest::OPEN_WRITE_ALL] == sizeof(IoRequestOpenWrite), "Enum mismtach?");
	static_assert(ioReqSize[IoRequest::OPEN_READ_ALL] == sizeof(IoRequestOpenRead), "Enum mismtach?");
	static_assert(ioReqSize[IoRequest::CLOSE] == sizeof(IoRequestClose), "Enum mismtach?");
	static_assert(ioReqSize[IoRequest::READ] == sizeof(IoRequestRead), "Enum mismtach?");
	static_assert(ioReqSize[IoRequest::WRITE] == sizeof(IoRequestWrite), "Enum mismtach?");

} // namespace


xFileSys::PendingOp::PendingOp(IoRequestBase* pReq, const XFileAsyncOperationCompiltion& op) :
	pRequest(X_ASSERT_NOT_NULL(pReq)),
	op(op)
{

}


X_INLINE IoRequest::Enum xFileSys::PendingOp::getType(void) const
{
	return pRequest->getType();
}


// --------------------------------------------------------------


xFileSys::xFileSys(core::MemoryArenaBase* arena) :
	gameDir_(nullptr),
	searchPaths_(nullptr),
	// ..
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
	filePoolArena_(&filePoolAllocator_, "FileHandlePool"),
	// ..
	asyncOpPoolHeap_(
		bitUtil::RoundUpToMultiple<size_t>(
			AsyncOpPoolArena::getMemoryRequirement(ASYNC_OP_ALLOCATION_SIZE) * MAX_ASYNC_OP,
			VirtualMem::GetPageSize()
		)
	),
	asyncOpPoolAllocator_(asyncOpPoolHeap_.start(), asyncOpPoolHeap_.end(),
		AsyncOpPoolArena::getMemoryRequirement(ASYNC_OP_ALLOCATION_SIZE),
		AsyncOpPoolArena::getMemoryAlignmentRequirement(ASYNC_OP_ALLOCATION_ALIGN),
		AsyncOpPoolArena::getMemoryOffsetRequirement()
	),
	asyncOpPoolArena_(&asyncOpPoolAllocator_, "AsyncOpPool"),
	// ..
	memFileArena_(&memfileAllocator_, "MemFileData"),
	currentRequestIdx_(0),
	requestSignal_(true),
	requests_(arena),
	ioQueueDataArena_(arena)
{
	arena->addChildArena(&filePoolArena_);
	arena->addChildArena(&asyncOpPoolArena_);
	arena->addChildArena(&memFileArena_);
}

xFileSys::~xFileSys()
{

}



void xFileSys::registerVars(void)
{
	vars_.registerVars();
}


void xFileSys::registerCmds(void)
{
	ADD_COMMAND_MEMBER("listPaks", this, xFileSys, &xFileSys::Cmd_ListPaks, core::VarFlag::SYSTEM, "List open asset pak's");
}

bool xFileSys::init(const SCoreInitParams& params)
{
	X_ASSERT_NOT_NULL(gEnv->pCore);
	X_LOG0("FileSys", "Starting Filesys..");

	if (!InitDirectorys(params.FileSysWorkingDir())) {
		X_ERROR("FileSys", "Failed to set game directories");
		return false;
	}


	return true;
}

bool xFileSys::initWorker(void)
{
	if (!StartRequestWorker()) {
		X_ERROR("FileSys", "Failed to start io request worker");
		return false;
	}

	return true;
}

void xFileSys::shutDown(void)
{
	X_LOG0("FileSys", "Shutting Down");

	ShutDownRequestWorker();

	for (Search* s = searchPaths_; s; ) {
		Search* cur = s;
		s = cur->pNext;
		if (cur->pDir) {
			X_DELETE(cur->pDir, g_coreArena);
		}
		else {
			X_DELETE(cur->pPak, g_coreArena);
		}
		X_DELETE(cur, g_coreArena);
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

core::Path<wchar_t> xFileSys::getWorkingDirectory(void) const
{
	return PathUtil::GetCurrentDirectory();
}

// --------------------- Open / Close ---------------------

XFile* xFileSys::openFile(pathType path, fileModeFlags mode, VirtualDirectory::Enum location)
{
	X_ASSERT_NOT_NULL(path);

	XDiskFile* pFile = nullptr;
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

			pFile = X_NEW(XDiskFile, &filePoolArena_, "DiskFile")(real_path.c_str(), mode);
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

		pFile = X_NEW(XDiskFile, &filePoolArena_, "Diskfile")(real_path.c_str(), mode);
	}

	if (!pFile->valid()) {
		closeFile(pFile);
		return nullptr;
	}

	return pFile;
}

XFile* xFileSys::openFile(pathTypeW path, fileModeFlags mode, VirtualDirectory::Enum location)
{
	X_ASSERT_NOT_NULL(path);

	XDiskFile* pFile = nullptr;
	core::Path<wchar_t> real_path;
	
	XFindData FindData(path, this);
	
	if (mode.IsSet(fileMode::READ) && !mode.IsSet(fileMode::WRITE))
	{
		_wfinddatai64_t findinfo;

		if (FindData.findnext(&findinfo))
		{
			FindData.getOSPath(real_path, &findinfo);

			if (isDebug()) {
				X_LOG0("FileSys", "openFile: \"%ls\"", real_path.c_str());
			}

			pFile = X_NEW(XDiskFile, &filePoolArena_, "DiskFile")(real_path.c_str(), mode);
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

		pFile = X_NEW(XDiskFile, &filePoolArena_, "Diskfile")(real_path.c_str(), mode);
	}

	if (!pFile->valid()) {
		closeFile(pFile);
		return nullptr;
	}

	return pFile;
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

			pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode, &asyncOpPoolArena_);
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
		{
			createOSPath(gameDir_, path, real_path);
		}
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode, &asyncOpPoolArena_);
	}

	if (!pFile->valid()) {
		closeFileAsync(pFile);
		return nullptr;
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

			pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode, &asyncOpPoolArena_);
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
		{
			createOSPath(gameDir_, path, real_path);
		}
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode, &asyncOpPoolArena_);
	}

	if (!pFile->valid()) {
		closeFileAsync(pFile);
		return nullptr;
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
	X_ASSERT_NOT_NULL(searchPaths_->pDir);
	gameDir_ = searchPaths_->pDir;

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
	core::Path<wchar_t> fixedPath;

	if (!PathUtil::GetFullPath(path, fixedPath)) {
		X_ERROR("FileSys", "addModDir full path name creation failed");
		return;
	}

	if (!this->directoryExistsOS(fixedPath)) {
		X_ERROR("FileSys", "Fixed path does not exsists: \"%ls\"", fixedPath);
		return;
	}

	// work out if this directory is a sub directory of any of the current
	// searxh paths.
	for (Search* s = searchPaths_; s; s = s->pNext)
	{
		if (strUtil::FindCaseInsensitive(fixedPath.c_str(), s->pDir->path.c_str()) != nullptr)
		{
			X_ERROR("FileSys", "mod dir is identical or inside a current mod dir: \"%ls\" -> \"%ls\"",
				fixedPath, s->pDir->path.c_str());
			return;
		}
	}

	// at it to virtual file system.
	Search* search = X_NEW( Search, g_coreArena, "FileSysSearch");
	search->pDir = X_NEW( directory_s, g_coreArena, "FileSysDir");
	search->pDir->path = fixedPath;
	search->pDir->path.ensureSlash();
	search->pPak = nullptr;
	search->pNext = searchPaths_;
	searchPaths_ = search;

	// add hotreload dir.
	gEnv->pDirWatcher->addDirectory(fixedPath.c_str());
}



// --------------------- Find util ---------------------

uintptr_t xFileSys::findFirst(pathType path, findData* findinfo)
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

bool xFileSys::findnext(uintptr_t handle, findData* findinfo)
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


uintptr_t xFileSys::findFirst2(pathType path, findData& findinfo)
{
	// i don't like how the findData shit works currently it's anoying!
	// so this is start of new version but i dunno how i want it to work yet.
	// i want somthing better but don't know what it is :)
	// list of shit:
	// 1. should return relative paths to the requested search dir.
	// 2. should expose some nicer scoped based / maybe iterative design to make nicer to use.
	// 3. <insert idea here>
	//

	Path<wchar_t> buf;
	createOSPath(gameDir_, path, buf);

	uintptr_t handle = PathUtil::findFirst(buf.c_str(), findinfo);

	return handle;
}

bool xFileSys::findnext2(uintptr_t handle, findData& findinfo)
{
	X_ASSERT(handle != PathUtil::INVALID_FIND_HANDLE, "FindNext called with invalid handle")(handle);

	return PathUtil::findNext(handle, findinfo);
}

void xFileSys::findClose2(uintptr_t handle)
{
	if (handle != PathUtil::INVALID_FIND_HANDLE)
	{
		PathUtil::findClose(handle);
	}
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
	Path<wchar_t> buf;
	createOSPath(gameDir_, path, buf);

	if (buf.fillSpaceWithNullTerm() < 1) {
		X_ERROR("FileSys", "Failed to pad puffer for OS operation");
		return false;
	}

	if (isDebug()) {
		X_LOG0("FileSys", "deleteDirectory: \"%ls\"", buf.c_str());
	}

	return PathUtil::DeleteDirectory(buf, recursive);
}


bool xFileSys::deleteDirectoryContents(pathType path)
{
	Path<wchar_t> buf;
	createOSPath(gameDir_, path, buf);

	if (isDebug()) {
		X_LOG0("FileSys", "deleteDirectoryContents: \"%s\"", path);
	}

	// check if the dir exsists.
	if (!directoryExistsOS(buf)) {
		return false;
	}

	// we build a relative search path.
	// as findFirst works on game dir's
	core::Path<wchar_t> searchPath(buf);
	searchPath.ensureSlash();
	searchPath.append(L"*");


	PathUtil::findData fd;
	uintptr_t handle = PathUtil::findFirst(searchPath.c_str(), fd);
	if (handle != PathUtil::INVALID_FIND_HANDLE)
	{
		do
		{
			if (core::strUtil::IsEqual(fd.name, L".") || core::strUtil::IsEqual(fd.name, L"..")) {
				continue;
			}

			// build a OS Path.
			core::Path<wchar_t> dirItem(buf);
			dirItem.ensureSlash();
			dirItem.append(fd.name);

			if (PathUtil::IsDirectory(fd))
			{
				if (dirItem.fillSpaceWithNullTerm() < 1) {
					X_ERROR("FileSys", "Failed to pad puffer for OS operation");
					return false;
				}

				if (!PathUtil::DeleteDirectory(dirItem, true)) {
					return false;
				}
			}
			else
			{
				if (!PathUtil::DeleteFile(dirItem)) {
					return false;
				}
			}


		} while (PathUtil::findNext(handle, fd));

		PathUtil::findClose(handle);
	}

	return true;
}



// --------------------- Create ---------------------

bool xFileSys::createDirectory(pathType path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);
	X_UNUSED(location);

	Path<wchar_t> buf;
	createOSPath(gameDir_, path, buf);

	buf.removeFileName();

	if (isDebug()) {
		X_LOG0("FileSys", "createDirectory: \"%ls\"", buf.c_str());
	}

	return PathUtil::CreateDirectory(buf);
}

bool xFileSys::createDirectory(pathTypeW path, VirtualDirectory::Enum location) const
{
	X_ASSERT_NOT_NULL(path);
	X_UNUSED(location);

	Path<wchar_t> buf;
	createOSPath(gameDir_, path, buf);

	buf.removeFileName();

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

bool xFileSys::createDirectoryTree(pathTypeW _path, VirtualDirectory::Enum location) const
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

	core::FixedArray<wchar_t, 128> driveLettters;

	for (Search* s = searchPaths_; s; s = s->pNext)
	{
		if (s->pDir)
		{
			const auto& path = s->pDir->path;
			int8_t driveIdx = path.getDriveNumber(); // watch this be a bug at somepoint.
			if(driveIdx >= 0)
			{
				wchar_t driveLetter = (L'A' + driveIdx);

				if (std::find(driveLettters.begin(), driveLettters.end(), driveLetter) == driveLettters.end()) {
					driveLettters.append(driveLetter);
				}		
			}
		}
	}

	for (const auto d : driveLettters)
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
			sectorSize = core::Max<size_t>(sectorSize, 4096);
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

bool xFileSys::moveFileOS(const wchar_t* pFullPath, const wchar_t* pFullPathNew) const
{
	bool result = core::PathUtil::MoveFile(pFullPath, pFullPathNew);

	if (isDebug()) {
		X_LOG0("FileSys", "moveFile: \"%ls\" -> \"%ls\" res: ^6%s",
			pFullPath, pFullPathNew, result ? "TRUE" : "FALSE");
	}

	return result;
}

// --------------------------------------------------

// Ajust path
const wchar_t* xFileSys::createOSPath(directory_s* dir, pathType path, Path<wchar_t>& buffer) const
{
	wchar_t pathW[core::Path<wchar_t>::BUF_SIZE];
	strUtil::Convert(path, pathW, sizeof(pathW));

	return createOSPath(dir, pathW, buffer);
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
	return vars_.debug_ != 0;
}

// ----------------------- stats ---------------------------

#if X_ENABLE_FILE_STATS


XFileStats xFileSys::getStats(void) const
{
	return OsFile::fileStats();
}

XFileStats xFileSys::getStatsAsync(void) const
{
	return OsFileAsync::fileStats();
}

IOQueueStats xFileSys::getIOQueueStats(void) const
{
	return stats_;
}

#endif // !X_ENABLE_FILE_STATS

// ----------------------- io que ---------------------------

RequestHandle xFileSys::getNextRequestHandle(void)
{
	RequestHandle handle = currentRequestIdx_++;
	if (handle == INVALID_IO_REQ_HANDLE) {
		return getNextRequestHandle();
	}

	return handle;
}

RequestHandle xFileSys::AddCloseRequestToQue(core::XFileAsync* pFile)
{
	IoRequestClose closeReq;
	closeReq.pFile = pFile;

	return AddIoRequestToQue(closeReq);
}

RequestHandle xFileSys::AddIoRequestToQue(IoRequestBase& request)
{
	IoRequestBase* pRequest = nullptr;

	static_assert(IoRequest::ENUM_COUNT == 6, "Enum count changed? this logic needs updating");

	switch (request.getType())
	{
		case IoRequest::OPEN:
			pRequest = X_NEW(IoRequestOpen, ioQueueDataArena_, "IORequestOpen")(static_cast<IoRequestOpen&>(request));
			break;
		case IoRequest::OPEN_WRITE_ALL:
			pRequest = X_NEW(IoRequestOpenWrite, ioQueueDataArena_, "IoRequestOpenWrite")(std::move(static_cast<IoRequestOpenWrite&>(request)));
			break;
		case IoRequest::OPEN_READ_ALL:
			pRequest = X_NEW(IoRequestOpenRead, ioQueueDataArena_, "IoRequestOpenRead")(static_cast<IoRequestOpenRead&>(request));
			break;
		case IoRequest::READ:
			pRequest = X_NEW(IoRequestRead, ioQueueDataArena_, "IoRequestRead")(static_cast<IoRequestRead&>(request));
			break;
		case IoRequest::WRITE:
			pRequest = X_NEW(IoRequestWrite, ioQueueDataArena_, "IoRequestWrite")(static_cast<IoRequestWrite&>(request));
			break;
		case IoRequest::CLOSE:
			pRequest = X_NEW(IoRequestClose, ioQueueDataArena_, "IoRequestClose")(static_cast<IoRequestClose&>(request));
			break;
		default:
			X_NO_SWITCH_DEFAULT;
			break;
	}

	return AddIoRequestToQue(pRequest);
}

RequestHandle xFileSys::AddIoRequestToQue(IoRequestBase* pRequest)
{
	static_assert(std::is_unsigned<RequestHandle>::value, "Logic makes use of wrap around, so must be unsigned");
	X_ASSERT_NOT_NULL(pRequest);

	if (pRequest->getType() == IoRequest::CLOSE) {
		if (pRequest->callback) {
			const IoRequestClose* pCloseReq = static_cast<const IoRequestClose*>(pRequest);
			X_ERROR("FileSys", "Close request can't have a callback. pfile: %p", pCloseReq->pFile);
			return INVALID_IO_REQ_HANDLE;
		}
	}
	else {
		X_ASSERT(pRequest->callback, "Callback can't be null")(IoRequest::ToString(pRequest->getType()), pRequest->callback);
	}


	if (vars_.queueDebug_)
	{
		uint32_t threadId = core::Thread::GetCurrentID();
		X_LOG0("FileSys", "IoRequest(0x%x) type: %s", threadId, IoRequest::ToString(pRequest->getType()));
	}


	RequestHandle reqHandle;
	{
		CriticalSection::ScopedLock lock(requestLock_);

#if X_ENABLE_FILE_STATS
		auto addTime = core::StopWatch::GetTimeNow();
		pRequest->setAddTime(addTime);
#endif // !X_ENABLE_FILE_STATS

		reqHandle = getNextRequestHandle(); 

		requests_.push(pRequest);
	}

	requestSignal_.raise();
	return reqHandle;
}


void xFileSys::CancelRequest(RequestHandle handle)
{
	X_UNUSED(handle);
	X_ASSERT_NOT_IMPLEMENTED();
}

bool xFileSys::StartRequestWorker(void)
{
	ThreadAbstract::Create("IoWorker", 1024 * 128); // small stack
	ThreadAbstract::Start();
	return true;
}


void xFileSys::ShutDownRequestWorker(void)
{
	if (ThreadAbstract::GetState() != core::Thread::State::RUNNING) {
		return;
	}

	ThreadAbstract::Stop();
	
	{
		// post a close job with a none null callback.
		IoRequestClose* pRequest = X_NEW(IoRequestClose, ioQueueDataArena_, "IORequestClose");
		::memset(&pRequest->callback, 1, sizeof(pRequest->callback));
		
		CriticalSection::ScopedLock lock(requestLock_);

		requests_.push(pRequest);
	}
		
	requestSignal_.raise();
	ThreadAbstract::Join();
}

IoRequestBase* xFileSys::popRequest(void)
{
	CriticalSection::ScopedLock lock(requestLock_);

	while (requests_.isEmpty())
	{
		requestLock_.Leave();
		requestSignal_.wait();
		requestLock_.Enter();
	}

	// read the request you dirty little grass hoper.
	auto* pReq = requests_.peek();
	requests_.pop();
	return pReq;
}

IoRequestBase* xFileSys::tryPopRequest(void)
{
	CriticalSection::ScopedLock lock(requestLock_);

	if (requests_.isEmpty()) {
		return nullptr;
	}

	// if we got one make sure signal is cleared.
	requestSignal_.clear();

	auto* pReq = requests_.peek();
	requests_.pop();
	return pReq;
}

void xFileSys::onOpFinsihed(PendingOp& asyncOp, uint32_t bytesTransferd)
{
	if (asyncOp.getType() == IoRequest::READ)
	{
		const IoRequestRead* pAsyncReq = asyncOp.as<const IoRequestRead>();
		XFileAsync* pReqFile = pAsyncReq->pFile;

		static_assert(core::compileTime::IsTrivialDestruct<IoRequestRead>::Value, "Need to call destructor");

		if (vars_.queueDebug_)
		{
			uint32_t threadId = core::Thread::GetCurrentID();

			X_LOG0("FileSys", "IoRequest(0x%x) '%s' async read request complete. "
				"bytesTrans: 0x%x pBuf: %p",
				threadId, IoRequest::ToString(pAsyncReq->getType()),
				bytesTransferd, pAsyncReq->pBuf);
		}

		pAsyncReq->callback.Invoke(*this, pAsyncReq, pReqFile, bytesTransferd);
	}
	else if (asyncOp.getType() == IoRequest::WRITE)
	{
		const IoRequestWrite* pAsyncReq = asyncOp.as<const IoRequestWrite>();
		XFileAsync* pReqFile = pAsyncReq->pFile;

		static_assert(core::compileTime::IsTrivialDestruct<IoRequestWrite>::Value, "Need to call destructor");

		if (vars_.queueDebug_)
		{
			uint32_t threadId = core::Thread::GetCurrentID();

			X_LOG0("FileSys", "IoRequest(0x%x) '%s' async write request complete. "
				"bytesTrans: 0x%x pBuf: %p",
				threadId, IoRequest::ToString(pAsyncReq->getType()),
				bytesTransferd, pAsyncReq->pBuf);
		}

		pAsyncReq->callback.Invoke(*this, pAsyncReq, pReqFile, bytesTransferd);
	}
	else if (asyncOp.getType() == IoRequest::OPEN_READ_ALL)
	{
		IoRequestOpenRead* pAsyncReq = asyncOp.as<IoRequestOpenRead>();
		XFileAsync* pReqFile = pAsyncReq->pFile;

		static_assert(core::compileTime::IsTrivialDestruct<IoRequestOpenRead>::Value, "Need to call destructor");

		if (vars_.queueDebug_)
		{
			uint32_t threadId = core::Thread::GetCurrentID();

			X_LOG0("FileSys", "IoRequest(0x%x) '%s' async open-read request complete. "
				"bytesTrans: 0x%x pBuf: %p",
				threadId, IoRequest::ToString(pAsyncReq->getType()),
				bytesTransferd, pAsyncReq->pBuf);
		}

		if (pAsyncReq->dataSize != bytesTransferd)
		{
			X_ERROR("FileSys", "Failed to read whole file contents. requested: %" PRIu32 " got %" PRIu32,
				pAsyncReq->dataSize, bytesTransferd);

			X_DELETE_ARRAY(static_cast<uint8_t*>(pAsyncReq->pBuf), pAsyncReq->arena);
			// we didnt not read the whole file, pretend we failed to open.
			pAsyncReq->pBuf = nullptr;
			pAsyncReq->dataSize = 0;
			pAsyncReq->callback.Invoke(*this, pAsyncReq, nullptr, 0);
		}
		else
		{
			pAsyncReq->callback.Invoke(*this, pAsyncReq, pReqFile, bytesTransferd);
		}

		// close it.
		closeFileAsync(pReqFile);
	}
	else if (asyncOp.getType() == IoRequest::OPEN_WRITE_ALL)
	{
		IoRequestOpenWrite* pAsyncReq = asyncOp.as<IoRequestOpenWrite>();
		XFileAsync* pReqFile = pAsyncReq->pFile;

		if (vars_.queueDebug_)
		{
			uint32_t threadId = core::Thread::GetCurrentID();

			X_LOG0("FileSys", "IoRequest(0x%x) '%s' async open-write request complete. "
				"bytesTrans: 0x%x pBuf: %p",
				threadId, IoRequest::ToString(pAsyncReq->getType()),
				bytesTransferd, pAsyncReq->data.ptr());
		}

		if (pAsyncReq->data.size() != bytesTransferd)
		{
			X_ERROR("FileSys", "Failed to write whole file contents. requested: %" PRIu32 " got %" PRIu32,
				pAsyncReq->data.size(), bytesTransferd);

			// we didnt not write the whole file, pretend we failed to open.
			pAsyncReq->callback.Invoke(*this, pAsyncReq, nullptr, 0);
		}
		else
		{
			pAsyncReq->callback.Invoke(*this, pAsyncReq, pReqFile, bytesTransferd);
		}

		// deconstruct.
		static_assert(!core::compileTime::IsTrivialDestruct<IoRequestOpenWrite>::Value, "no need to call destructor");

		core::Mem::Destruct(pAsyncReq);

		// close it.
		closeFileAsync(pReqFile);
	}
	else
	{
		X_ASSERT_UNREACHABLE();
	}
}


void xFileSys::AsyncIoCompletetionRoutine(XOsFileAsyncOperation::AsyncOp* pOperation, uint32_t bytesTransferd)
{
	X_ASSERT(pendingOps_.isNotEmpty(), "Recived a unexpected Async complition")(pOperation, bytesTransferd);

	for (size_t i = 0; i < pendingOps_.size(); i++)
	{
		PendingOp& asyncOp = pendingOps_[i];

		if (asyncOp.op.ownsAsyncOp(pOperation))
		{
			onOpFinsihed(asyncOp, bytesTransferd);

			X_DELETE(asyncOp.pRequest, ioQueueDataArena_);
			pendingOps_.removeIndex(i);
			return;
		}
	}

	// failed to fine the op ;(
	X_ASSERT_UNREACHABLE();
}

Thread::ReturnValue xFileSys::ThreadRun(const Thread& thread)
{
	gEnv->pJobSys->CreateQueForCurrentThread();

	xFileSys& fileSys = *this;

	IoRequestBase* pRequest = nullptr;

	XOsFileAsyncOperation::ComplitionRotinue compRoutine;
	compRoutine.Bind<xFileSys, &xFileSys::AsyncIoCompletetionRoutine>(this);

	while (thread.ShouldRun())
	{
		if (pendingOps_.isEmpty())
		{
			pRequest = popRequest();
		}
		else
		{
			// we have pending requests, lets quickly enter a alertable state.
			// then we will try get more requests, untill we get more requests
			// we just stay in a alertable state, so that any pending requests can handled.
			requestSignal_.wait(0, true);

			while((pRequest = tryPopRequest()) == nullptr)
			{
				requestSignal_.wait(core::Signal::WAIT_INFINITE, true);

#if X_ENABLE_FILE_STATS
				stats_.PendingOps = pendingOps_.size();
#endif // !X_ENABLE_FILE_STATS
			}

			// we have a request.
		}

		core::UniquePointer<IoRequestBase> requestPtr(ioQueueDataArena_, pRequest);
		const auto type = pRequest->getType();

#if X_ENABLE_FILE_STATS
		auto start = core::StopWatch::GetTimeNow();
#endif // !X_ENABLE_FILE_STATS
		

		if (type == IoRequest::OPEN)
		{
			IoRequestOpen* pOpen = static_cast<IoRequestOpen*>(pRequest);
			XFileAsync* pFile =	openFileAsync(pOpen->path.c_str(), pOpen->mode);

			pOpen->callback.Invoke(fileSys, pOpen, pFile, 0);
		}
		else if (type == IoRequest::OPEN_READ_ALL)
		{
			IoRequestOpenRead* pOpenRead = static_cast<IoRequestOpenRead*>(pRequest);
			XFileAsync* pFile = openFileAsync(pOpenRead->path.c_str(), pOpenRead->mode);

			// make sure it's safe to allocate the buffer in this thread.
			X_ASSERT_NOT_NULL(pOpenRead->arena);
			X_ASSERT(pOpenRead->arena->isThreadSafe(), "Async OpenRead requests require thread safe arena")(pOpenRead->arena->isThreadSafe());

			if (!pFile)
			{
				pOpenRead->callback.Invoke(fileSys, pOpenRead, pFile, 0);
				goto nextRequest;
			}
			
			const uint64_t fileSize = pFile->fileSize();

			// we don't support open and read for files over >2gb
			// if you are trying todo that you are retarded.
			// instead open the file and submit smaller read requests.
			if (fileSize > std::numeric_limits<int32_t>::max())
			{
				X_ERROR("FileSys", "A request was made to read a entire file which is >2GB, ignoring request. File: \"%s\"",
					pOpenRead->path.c_str());

				closeFileAsync(pFile);
				pOpenRead->callback.Invoke(fileSys, pOpenRead, nullptr, 0);
			}
			else
			{
#if X_ENABLE_FILE_STATS
				stats_.NumBytesRead += fileSize;
#endif // !X_ENABLE_FILE_STATS

				uint8_t* pData = X_NEW_ARRAY_ALIGNED(uint8_t, safe_static_cast<size_t>(fileSize), pOpenRead->arena, "AsyncIOReadAll", 16);

				XFileAsyncOperationCompiltion operation = pFile->readAsync(
					pData,
					safe_static_cast<size_t>(fileSize),
					0,
					compRoutine
				);

				// now we need to wait for the read to finish, then close the file and call the callback.
				// just need to work out a nice way to close the file post read.
				pOpenRead->pFile = pFile;
				pOpenRead->pBuf = pData;
				pOpenRead->dataSize = safe_static_cast<uint32_t>(fileSize);

				pendingOps_.emplace_back(requestPtr.release(), operation);
			}
			
		}
		else if (type == IoRequest::OPEN_WRITE_ALL)
		{
			IoRequestOpenWrite* pOpenWrite = static_cast<IoRequestOpenWrite*>(pRequest);
			XFileAsync* pFile = openFileAsync(pOpenWrite->path.c_str(), core::fileModeFlags::RECREATE | core::fileModeFlags::WRITE);

			X_ASSERT(pOpenWrite->data.getArena()->isThreadSafe(), "Async OpenWrite requests require thread safe arena")();
			X_ASSERT(pOpenWrite->data.size() > 0, "WriteAll called with data size 0")(pOpenWrite->data.size());

			if (!pFile)
			{
				pOpenWrite->callback.Invoke(fileSys, pOpenWrite, pFile, 0);
				goto nextRequest;
			}

			pOpenWrite->pFile = pFile;

#if X_ENABLE_FILE_STATS
			stats_.NumBytesWrite += pOpenWrite->data.size();
#endif // !X_ENABLE_FILE_STATS

			XFileAsyncOperationCompiltion operation = pFile->writeAsync(
				pOpenWrite->data.ptr(),
				pOpenWrite->data.size(),
				0,
				compRoutine
			);

			pendingOps_.emplace_back(requestPtr.release(), operation);
		}
		else if (type == IoRequest::CLOSE)
		{
			// if the close job has a callback, we are shutting down.
			if (pRequest->callback) {
				continue;
			}

			// normal close request.
			IoRequestClose* pClose = static_cast<IoRequestClose*>(pRequest);

			closeFileAsync(pClose->pFile);
		}
		else if (type == IoRequest::READ)
		{
			IoRequestRead* pRead = static_cast<IoRequestRead*>(pRequest);
			XFileAsync* pFile = pRead->pFile;

#if X_ENABLE_FILE_STATS
			stats_.NumBytesRead += pRead->dataSize;
#endif // !X_ENABLE_FILE_STATS

			XFileAsyncOperationCompiltion operation = pFile->readAsync(
				pRead->pBuf,
				pRead->dataSize,
				pRead->offset,
				compRoutine
			);

			operation.hasFinished();

			pendingOps_.emplace_back(requestPtr.release(), operation);
		}
		else if (type == IoRequest::WRITE)
		{
			IoRequestWrite* pWrite = static_cast<IoRequestWrite*>(pRequest);
			XFileAsync* pFile = pWrite->pFile;

#if X_ENABLE_FILE_STATS
			stats_.NumBytesWrite += pWrite->dataSize;
#endif // !X_ENABLE_FILE_STATS

			XFileAsyncOperationCompiltion operation = pFile->writeAsync(
				pWrite->pBuf,
				pWrite->dataSize,
				pWrite->offset,
				compRoutine
			);

			pendingOps_.emplace_back(requestPtr.release(), operation);
		}

	nextRequest:;
#if X_ENABLE_FILE_STATS
		auto end = core::StopWatch::GetTimeNow();

		++stats_.RequestCounts[type];
		stats_.RequestTime[type] += (end - start).GetValue();
#endif // !X_ENABLE_FILE_STATS
	}

	return Thread::ReturnValue(0);
}

bool xFileSys::openPak(const char* pName)
{
	// you can only open pak's from inside the virtual filesystem.
	// so file is opened as normal.
	fileModeFlags mode;
	mode.Set(fileMode::READ);
	mode.Set(fileMode::RANDOM_ACCESS);
	// I'm not sharing, fuck you!

	auto* pFile = openFileAsync(pName, mode);
	if (!pFile) {

		return false;
	}

	// we need the header Wagg man.
	AssetPak::APakHeader hdr;

	auto op = pFile->readAsync(&hdr, sizeof(hdr), 0);
	if (op.waitUntilFinished() != sizeof(hdr)) {
		X_ERROR("AssetPak", "Failed to open file for saving");
		return false;
	}

	if (!hdr.isValid()) {
		X_ERROR("AssetPak", "Invalid header");
		return false;
	}

	if (hdr.version != AssetPak::PAK_VERSION) {
		X_ERROR("AssetPak", "Version incorrect. got %" PRIu8 " require %" PRIu8, hdr.version, AssetPak::PAK_VERSION);
		return false;
	}

	if (hdr.numAssets == 0) {
		X_ERROR("AssetPak", "Pak is empty.");
		return false;
	}

	// Strings
	// Entries
	// Dict
	// Data

	const size_t metaBlockSize = hdr.dataOffset - hdr.stringDataOffset;
	const size_t stringBlockSize = hdr.entryTableOffset - hdr.stringDataOffset;
	const size_t entryDataOffset = hdr.entryTableOffset - hdr.stringDataOffset;

	auto pPak = core::makeUnique<Pak>(g_coreArena, g_coreArena);
	pPak->name.set(pName);
	pPak->pFile = pFile;
	pPak->numAssets = hdr.numAssets;
	pPak->dataOffset = hdr.dataOffset;
	pPak->data.resize(metaBlockSize);
	pPak->strings.reserve(hdr.numAssets);

	op = pFile->readAsync(pPak->data.data(), pPak->data.size(), hdr.stringDataOffset);
	if (op.waitUntilFinished() != pPak->data.size()) {
		X_ERROR("AssetPak", "Error reading pak data");
		return false;
	}

	core::MemCursor cursor(pPak->data.data(), stringBlockSize);

	pPak->strings.push_back(cursor.getPtr<const char>());

	for (auto* pData = cursor.begin(); pData < cursor.end(); ++pData)
	{
		if (*pData == '\0')
		{
			pPak->strings.push_back(pData + 1);
			if (pPak->strings.size() == hdr.numAssets) {
				break;
			}
		}
	}

	if (pPak->strings.size() != hdr.numAssets) {
		X_ERROR("AssetPak", "Error loading pak");
		return false;
	}

	auto numassetsPow2 = core::bitUtil::NextPowerOfTwo(hdr.numAssets);

	pPak->pEntires = reinterpret_cast<const AssetPak::APakEntry*>(pPak->data.data() + entryDataOffset);
	pPak->hash.setGranularity(numassetsPow2);
	pPak->hash.clear(numassetsPow2 * 4, numassetsPow2);

	// we need to build a hash table of the goat.
	for (size_t i = 0; i < pPak->strings.size(); ++i)
	{
		const char* pString = pPak->strings[i];
		core::StrHash hash(pString, core::strUtil::strlen(pString));
		
		pPak->hash.add(hash, safe_static_cast<int32_t>(i));
	}

	// all done?
	Search* pSearch = X_NEW(Search, g_coreArena, "FileSysSearch");
	pSearch->pDir = nullptr;
	pSearch->pPak = pPak.release();
	pSearch->pNext = searchPaths_;
	searchPaths_ = pSearch;

	return true;
}

void xFileSys::listPaks(const char* pSearchPatten) const
{
	X_UNUSED(pSearchPatten);

	size_t numPacks = 0;

	X_LOG0("FileSys", "-------------- ^8Paks(%" PRIuS ")^7 ---------------", numPacks);
	
	for (Search* pSearch = searchPaths_; pSearch; pSearch = pSearch->pNext)
	{
		if (!pSearch->pPak) {
			continue;
		}

		auto* pPak = pSearch->pPak;

		X_LOG0("FileSys", "^2%-32s ^7assets: ^2%" PRIu32 " ^7mode: ^2%s", pPak->name.c_str(), pPak->numAssets, PakMode::ToString(pPak->mode));

	}

	X_LOG0("FileSys", "-------------- ^8Paks End^7 --------------");
}

void xFileSys::Cmd_ListPaks(IConsoleCmdArgs* pCmd)
{
	const char* pSearchPattern = nullptr;

	if (pCmd->GetArgCount() > 1) {
		pSearchPattern = pCmd->GetArg(1);
	}

	listPaks(pSearchPattern);
}


X_NAMESPACE_END