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
	const size_t MAX_FILE_HANDLES = 1024;
	const size_t MAX_ASYNC_OP = 1024;

	const size_t FILE_ALLOCATION_SIZE = core::Max(sizeof(XDiskFile),
		core::Max(sizeof(XDiskFileAsync), sizeof(XFileMem)));

	const size_t FILE_ALLOCATION_ALIGN = core::Max(X_ALIGN_OF(XDiskFile),
		core::Max(X_ALIGN_OF(XDiskFileAsync), X_ALIGN_OF(XFileMem)));


	const size_t ASYNC_OP_ALLOCATION_SIZE = sizeof(XOsFileAsyncOperation::AsyncOp);
	const size_t ASYNC_OP_ALLOCATION_ALIGN = X_ALIGN_OF(XOsFileAsyncOperation::AsyncOp);


	constexpr const size_t ioReqSize[IoRequest::ENUM_COUNT] = {
		sizeof(IoRequestOpen),
		sizeof(IoRequestOpenRead),
		sizeof(IoRequestClose),
		sizeof(IoRequestRead),
		sizeof(IoRequestWrite)
	};

	static_assert(ioReqSize[IoRequest::OPEN] == sizeof(IoRequestOpen), "Enum mismtach?");
	static_assert(ioReqSize[IoRequest::OPEN_READ_ALL] == sizeof(IoRequestOpenRead), "Enum mismtach?");
	static_assert(ioReqSize[IoRequest::CLOSE] == sizeof(IoRequestClose), "Enum mismtach?");
	static_assert(ioReqSize[IoRequest::READ] == sizeof(IoRequestRead), "Enum mismtach?");
	static_assert(ioReqSize[IoRequest::WRITE] == sizeof(IoRequestWrite), "Enum mismtach?");

} // namespace



xFileSys::PendingOp::PendingOp(const IoRequestRead& req, const XFileAsyncOperationCompiltion& op) :
	readReq(req),
	op(op)
{
}

xFileSys::PendingOp::PendingOp(const IoRequestWrite& req, const XFileAsyncOperationCompiltion& op) :
	writeReq(req),
	op(op)
{
}

xFileSys::PendingOp::PendingOp(const IoRequestOpenRead& req, const XFileAsyncOperationCompiltion& op) :
	openReadReq(req),
	op(op)
{
	X_ASSERT_NOT_NULL(req.arena);
	X_ASSERT_NOT_NULL(req.pFile);
	X_ASSERT_NOT_NULL(req.pBuf);
}

X_INLINE IoRequest::Enum xFileSys::PendingOp::getType(void) const
{
	return readReq.getType();
}

xFileSys::PendingOp& xFileSys::PendingOp::operator=(PendingOp&& oth)
{
	op = std::move(oth.op);

	switch (getType())
	{
		case IoRequest::READ:
			readReq = std::move(oth.readReq);
			break;
		case IoRequest::WRITE:
			writeReq = std::move(oth.writeReq);
			break;
		case IoRequest::OPEN_READ_ALL:
			openReadReq = std::move(oth.openReadReq);
			break;
		default:
			X_ASSERT_UNREACHABLE();
			break;
	}

	return *this;
}

// --------------------------------------------------------------


xFileSys::xFileSys() :
	searchPaths_(nullptr),
	gameDir_(nullptr),
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
	requestData_(gEnv->pArena, IO_REQUEST_BUF_SIZE),
	currentRequestIdx_(0),
	requestSignal_(true)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pArena);

	gEnv->pArena->addChildArena(&filePoolArena_);
	gEnv->pArena->addChildArena(&asyncOpPoolArena_);
	gEnv->pArena->addChildArena(&memFileArena_);
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

	for (search_s* s = searchPaths_; s; ) {
		search_s* cur = s;
		s = cur->next_;
		if (cur->dir) {
			X_DELETE(cur->dir, g_coreArena);
		}
		else {
			X_DELETE(cur->pak, g_coreArena);
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
			pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode, &asyncOpPoolArena_);

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
		{
			createOSPath(gameDir_, path, real_path);
		}
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode, &asyncOpPoolArena_);

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
			pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode, &asyncOpPoolArena_);

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
		{
			createOSPath(gameDir_, path, real_path);
		}
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}

		pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path.c_str(), mode, &asyncOpPoolArena_);

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

RequestHandle xFileSys::AddIoRequestToQue(const IoRequestBase& request)
{
	static_assert(std::is_unsigned<RequestHandle>::value, "Logic makes use of wrap around, so must be unsigned");

	if (request.getType() == IoRequest::CLOSE) {
		if (request.callback) {
			const IoRequestClose& closeReq = static_cast<const IoRequestClose&>(request);
			X_ERROR("FileSys", "Close request can't have a callback. pfile: %p", closeReq.pFile);
			return INVALID_IO_REQ_HANDLE;
		}
	}
	else {
		X_ASSERT(request.callback, "Callback can't be null")(IoRequest::ToString(request.getType()), request.callback);
	}

	RequestHandle reqHandle = getNextRequestHandle();

	if (vars_.QueDebug)
	{
		uint32_t threadId = core::Thread::GetCurrentID();

		X_LOG0("FileSys","IoRequest(0x%x) type: %s", threadId,
			IoRequest::ToString(request.getType()));
	}

	const size_t reqSize = ioReqSize[request.getType()];

	CriticalSection::ScopedLock lock(requestLock_);

	while (reqSize > requestData_.freeSpace())
	{
		requestLock_.Leave();

		X_WARNING("FileSys", "IO que full, stalling util free space");

		core::Thread::Sleep(1);
		requestLock_.Enter();
	}

	// POD it like it's hot.
	requestData_.write(&request, reqSize);
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
	ThreadAbstract::Stop();

	// post a close job with a none null callback.
	IoRequestClose closeRequest;
	::memset(&closeRequest.callback, 1, sizeof(closeRequest.callback));

	{
		CriticalSection::ScopedLock lock(requestLock_);

		// push a few close jobs on to give chance for thread should run to get updated.
		for (size_t i = 0; i < 10; i++) {
			requestData_.write(&closeRequest, sizeof(closeRequest));
		}
	}
		
	requestSignal_.raise();
	ThreadAbstract::Join();
}

void xFileSys::popRequest(RequestBuffer& buf)
{
	CriticalSection::ScopedLock lock(requestLock_);

	while (requestData_.isEmpty()) 
	{
		requestLock_.Leave();
		requestSignal_.wait();
		requestLock_.Enter();
	}

	// read the request you dirty little grass hoper.
	IoRequestBase& base = requestData_.peek<IoRequestBase>();
	const size_t reqSize = ioReqSize[base.getType()];

	std::memcpy(buf.data(), &base, reqSize);

	requestData_.skip<uint8_t>(reqSize);
}

bool xFileSys::tryPopRequest(RequestBuffer& buf)
{
	CriticalSection::ScopedLock lock(requestLock_);

	if (requestData_.isEmpty()) {
		return false;
	}

	// if we got one make sure signal is cleared.
	requestSignal_.clear();

	IoRequestBase& base = requestData_.peek<IoRequestBase>();
	const size_t reqSize = ioReqSize[base.getType()];

	std::memcpy(buf.data(), &base, reqSize);

	requestData_.skip<uint8_t>(reqSize);
	return true;
}

void xFileSys::onOpFinsihed(PendingOp& asyncOp, uint32_t bytesTransferd)
{
	if (asyncOp.getType() == IoRequest::READ)
	{
		const IoRequestRead& asyncReq = asyncOp.readReq;
		XFileAsync* pReqFile = asyncReq.pFile;

		if (vars_.QueDebug)
		{
			uint32_t threadId = core::Thread::GetCurrentID();

			X_LOG0("FileSys", "IoRequest(0x%x) '%s' async read request complete. "
				"bytesTrans: 0x%x pBuf: %p",
				threadId, IoRequest::ToString(asyncReq.getType()),
				bytesTransferd, asyncReq.pBuf);
		}

		asyncReq.callback.Invoke(*this, &asyncReq, pReqFile, bytesTransferd);
	}
	else if (asyncOp.getType() == IoRequest::WRITE)
	{
		const IoRequestWrite& asyncReq = asyncOp.writeReq;
		XFileAsync* pReqFile = asyncReq.pFile;

		if (vars_.QueDebug)
		{
			uint32_t threadId = core::Thread::GetCurrentID();

			X_LOG0("FileSys", "IoRequest(0x%x) '%s' async write request complete. "
				"bytesTrans: 0x%x pBuf: %p",
				threadId, IoRequest::ToString(asyncReq.getType()),
				bytesTransferd, asyncReq.pBuf);
		}

		asyncReq.callback.Invoke(*this, &asyncReq, pReqFile, bytesTransferd);
	}
	else if (asyncOp.getType() == IoRequest::OPEN_READ_ALL)
	{
		IoRequestOpenRead& asyncReq = asyncOp.openReadReq;
		XFileAsync* pReqFile = asyncReq.pFile;

		if (vars_.QueDebug)
		{
			uint32_t threadId = core::Thread::GetCurrentID();

			X_LOG0("FileSys", "IoRequest(0x%x) '%s' async open-read request complete. "
				"bytesTrans: 0x%x pBuf: %p",
				threadId, IoRequest::ToString(asyncReq.getType()),
				bytesTransferd, asyncReq.pBuf);
		}

		if (asyncReq.dataSize != bytesTransferd)
		{
			X_ERROR("FileSys", "Failed to read whole file contents. requested: %" PRIu32 " got %" PRIu32,
				asyncReq.dataSize, bytesTransferd);

			X_DELETE_ARRAY(static_cast<uint8_t*>(asyncReq.pBuf), asyncReq.arena);
			// we didnt not read the whole file, pretend we failed to open.
			asyncReq.pBuf = nullptr;
			asyncReq.dataSize = 0;
			asyncReq.callback.Invoke(*this, &asyncReq, nullptr, 0);
		}
		else
		{
			asyncReq.callback.Invoke(*this, &asyncReq, pReqFile, bytesTransferd);
		}

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

	RequestBuffer X_ALIGNED_SYMBOL(requestBuffer, 16);
	IoRequestBase* pRequest = reinterpret_cast<IoRequestBase*>(&requestBuffer);

	XOsFileAsyncOperation::ComplitionRotinue compRoutine;
	compRoutine.Bind<xFileSys, &xFileSys::AsyncIoCompletetionRoutine>(this);


	while (thread.ShouldRun())
	{
		if (pendingOps_.isEmpty())
		{
			popRequest(requestBuffer);
		}
		else
		{
			// wait for any requests in a alertable state.
			// so we wake for either async op completing or new requests need processing
			while(!tryPopRequest(requestBuffer)) 
			{
				requestSignal_.wait(core::Signal::WAIT_INFINITE, true);

#if X_ENABLE_FILE_STATS
				stats_.PendingOps = pendingOps_.size();
#endif // !X_ENABLE_FILE_STATS
			}

			// we have a request.
		}

		const auto type = pRequest->getType();

#if X_ENABLE_FILE_STATS
		++stats_.RequestCounts[type];
#endif // !X_ENABLE_FILE_STATS

		if (type == IoRequest::OPEN)
		{
#if X_ENABLE_FILE_STATS
			++stats_.NumFilesOpened;
#endif // !X_ENABLE_FILE_STATS

			IoRequestOpen* pOpen = static_cast<IoRequestOpen*>(pRequest);
			XFileAsync* pFile =	openFileAsync(pOpen->path.c_str(), pOpen->mode);

			pOpen->callback.Invoke(fileSys, pOpen, pFile, 0);
		}
		else if (type == IoRequest::OPEN_READ_ALL)
		{
#if X_ENABLE_FILE_STATS
			++stats_.NumFilesOpened;
#endif // !X_ENABLE_FILE_STATS

			IoRequestOpenRead* pOpenRead = static_cast<IoRequestOpenRead*>(pRequest);
			XFileAsync* pFile = openFileAsync(pOpenRead->path.c_str(), pOpenRead->mode);

			// make sure it's safe to allocate the buffer in this thread.
			X_ASSERT_NOT_NULL(pOpenRead->arena);
			X_ASSERT(pOpenRead->arena->isThreadSafe(), "Async OpenRead requests require thread safe arena")(pOpenRead->arena->isThreadSafe());

			if (!pFile)
			{
				pOpenRead->callback.Invoke(fileSys, pOpenRead, pFile, 0);
				continue;
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

				pendingOps_.emplace_back(*pOpenRead, operation);
			}
			
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

			pendingOps_.emplace_back(*pRead,operation);
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

			pendingOps_.emplace_back(*pWrite, operation);
		}
	}

	return Thread::ReturnValue(0);
}



X_NAMESPACE_END