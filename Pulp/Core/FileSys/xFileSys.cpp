#include "stdafx.h"
#include "xFileSys.h"
#include "DiskFile.h"
#include "DiskFileAsync.h"
#include "PakFileAsync.h"
#include "PakFile.h"

#include "PathUtil.h"
#include "XFindData.h"

#include <Util\LastError.h>
#include <Util\UniquePointer.h>

#include <ITimer.h>
#include <IConsole.h>
#include <IDirectoryWatcher.h>

#include <IAssetPak.h>

#include <Memory\VirtualMem.h>
#include <Memory\MemCursor.h>
#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>
#include <String\StringHash.h>

#include "Pak.h"

X_NAMESPACE_BEGIN(core)

namespace
{
    // might need more since memory files will be from pool.
    const size_t MAX_FILE_HANDLES = 1024;
    const size_t MAX_ASYNC_OP = 1024;

    const size_t FILE_ALLOCATION_SIZE = core::Max(sizeof(XDiskFile),
        core::Max(sizeof(XDiskFileAsync),
            core::Max(sizeof(XFileMem), core::Max(sizeof(XPakFile), sizeof(XPakFileAsync)))));

    const size_t FILE_ALLOCATION_ALIGN = core::Max(X_ALIGN_OF(XDiskFile),
        core::Max(X_ALIGN_OF(XDiskFileAsync),
            core::Max(X_ALIGN_OF(XFileMem), core::Max(X_ALIGN_OF(XPakFile), X_ALIGN_OF(XPakFileAsync)))));

    const size_t ASYNC_OP_ALLOCATION_SIZE = sizeof(XOsFileAsyncOperation::AsyncOp);
    const size_t ASYNC_OP_ALLOCATION_ALIGN = X_ALIGN_OF(XOsFileAsyncOperation::AsyncOp);

    static_assert(IoRequest::ENUM_COUNT == 6, "Enum count changed? this logic needs updating");

    constexpr const size_t ioReqSize[IoRequest::ENUM_COUNT] = {
        sizeof(IoRequestClose),
        sizeof(IoRequestOpen),
        sizeof(IoRequestOpenWrite),
        sizeof(IoRequestOpenRead),
        sizeof(IoRequestWrite),
        sizeof(IoRequestRead)};

    static_assert(ioReqSize[IoRequest::OPEN] == sizeof(IoRequestOpen), "Enum mismtach?");
    static_assert(ioReqSize[IoRequest::OPEN_WRITE_ALL] == sizeof(IoRequestOpenWrite), "Enum mismtach?");
    static_assert(ioReqSize[IoRequest::OPEN_READ_ALL] == sizeof(IoRequestOpenRead), "Enum mismtach?");
    static_assert(ioReqSize[IoRequest::CLOSE] == sizeof(IoRequestClose), "Enum mismtach?");
    static_assert(ioReqSize[IoRequest::READ] == sizeof(IoRequestRead), "Enum mismtach?");
    static_assert(ioReqSize[IoRequest::WRITE] == sizeof(IoRequestWrite), "Enum mismtach?");

    // I know want it so all open requests that reach the file system are basically asset names.
    // unless a OS path.
    static_assert(core::Path<>::SLASH == assetDb::ASSET_NAME_SLASH, "Slash mismatch");
    static_assert(core::Path<>::INVALID_SLASH == assetDb::ASSET_NAME_INVALID_SLASH, "Slash mismatch");

} // namespace

// ------------------------------------

xFileSys::PendingOpBase::PendingOpBase(core::UniquePointer<IoRequestBase>&& req) :
    pRequest(std::forward<core::UniquePointer<IoRequestBase>>(req))
{
}

xFileSys::PendingOpBase::PendingOpBase(PendingOpBase&& oth) :
    pRequest(std::move(oth.pRequest))
{
}

xFileSys::PendingOpBase& xFileSys::PendingOpBase::operator=(PendingOpBase&& oth)
{
    pRequest = std::move(oth.pRequest);
    return *this;
}

X_INLINE IoRequest::Enum xFileSys::PendingOpBase::getType(void) const
{
    return pRequest->getType();
}

// ------------------------------------

xFileSys::PendingCompiltionOp::PendingCompiltionOp(core::UniquePointer<IoRequestBase>&& req, XFileAsyncOperationCompiltion&& op) :
    PendingOpBase(std::forward<core::UniquePointer<IoRequestBase>>(req)),
    op(std::forward<XFileAsyncOperationCompiltion>(op))
{
}

xFileSys::PendingCompiltionOp::PendingCompiltionOp(PendingCompiltionOp&& oth) :
    PendingOpBase(std::move(oth)),
    op(std::move(oth.op))
{
}

xFileSys::PendingCompiltionOp& xFileSys::PendingCompiltionOp::operator=(PendingCompiltionOp&& oth)
{
    PendingOpBase::operator=(std::move(oth));
    op = std::move(oth.op);
    return *this;
}

// ------------------------------------

xFileSys::PendingOp::PendingOp(core::UniquePointer<IoRequestBase>&& req, XFileAsyncOperation&& op) :
    PendingOpBase(std::forward<core::UniquePointer<IoRequestBase>>(req)),
    op(std::forward<XFileAsyncOperation>(op))
{
}

xFileSys::PendingOp::PendingOp(PendingOp&& oth) :
    PendingOpBase(std::move(oth)),
    op(std::move(oth.op))
{
}

xFileSys::PendingOp& xFileSys::PendingOp::operator=(PendingOp&& oth)
{
    PendingOpBase::operator=(std::move(oth));
    op = std::move(oth.op);
    return *this;
}

// --------------------------------------------------------------

xFileSys::xFileSys(core::MemoryArenaBase* arena) :
    baseDir_(nullptr),
    saveDir_(nullptr),
    searchPaths_(nullptr),
    loadPacks_(false),
    // ..
    filePoolHeap_(
        bitUtil::RoundUpToMultiple<size_t>(
            FilePoolArena::getMemoryRequirement(FILE_ALLOCATION_SIZE) * MAX_FILE_HANDLES,
            VirtualMem::GetPageSize())),
    filePoolAllocator_(filePoolHeap_.start(), filePoolHeap_.end(),
        FilePoolArena::getMemoryRequirement(FILE_ALLOCATION_SIZE),
        FilePoolArena::getMemoryAlignmentRequirement(FILE_ALLOCATION_ALIGN),
        FilePoolArena::getMemoryOffsetRequirement()),
    filePoolArena_(&filePoolAllocator_, "FileHandlePool"),
    // ..
    asyncOpPoolHeap_(
        bitUtil::RoundUpToMultiple<size_t>(
            AsyncOpPoolArena::getMemoryRequirement(ASYNC_OP_ALLOCATION_SIZE) * MAX_ASYNC_OP,
            VirtualMem::GetPageSize())),
    asyncOpPoolAllocator_(asyncOpPoolHeap_.start(), asyncOpPoolHeap_.end(),
        AsyncOpPoolArena::getMemoryRequirement(ASYNC_OP_ALLOCATION_SIZE),
        AsyncOpPoolArena::getMemoryAlignmentRequirement(ASYNC_OP_ALLOCATION_ALIGN),
        AsyncOpPoolArena::getMemoryOffsetRequirement()),
    asyncOpPoolArena_(&asyncOpPoolAllocator_, "AsyncOpPool"),
    // ..
    virtualDirHeap_(
        bitUtil::RoundUpToMultiple<size_t>(
            AsyncOpPoolArena::getMemoryRequirement(sizeof(Directory)) * MAX_VIRTUAL_DIR + 
            AsyncOpPoolArena::getMemoryRequirement(sizeof(Search)) * (MAX_PAK + MAX_VIRTUAL_DIR),
            VirtualMem::GetPageSize())),
    virtualDirAllocator_(virtualDirHeap_.start(), virtualDirHeap_.end()),
    virtualDirArena_(&virtualDirAllocator_, "VirtualDirPool"),
    // ..
    memFileArena_(&memfileAllocator_, "MemFileData"),
    numDir_(0),
    numPak_(0),
    currentRequestIdx_(0),
    requestSignal_(true),
    requests_(arena),

#if X_ENABLE_FILE_ARTIFICAIL_DELAY
    delayedOps_(arena),
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

    ioQueueDataArena_(arena)
{
    arena->addChildArena(&filePoolArena_);
    arena->addChildArena(&asyncOpPoolArena_);
    arena->addChildArena(&virtualDirArena_);
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
    ADD_COMMAND_MEMBER("listAssetPaks", this, xFileSys, &xFileSys::Cmd_ListPaks, core::VarFlag::SYSTEM, "List open asset pak's");
    ADD_COMMAND_MEMBER("listFileSystemPaths", this, xFileSys, &xFileSys::Cmd_ListSearchPaths, core::VarFlag::SYSTEM, "List the virtual filesystem paths");
}

bool xFileSys::init(const CoreInitParams& params)
{
    X_ASSERT_NOT_NULL(gEnv->pCore);
    X_LOG0("FileSys", "Starting Filesys..");

    if (!initDirectorys(params.FileSysWorkingDir())) {
        X_ERROR("FileSys", "Failed to set game directories");
        return false;
    }

    return true;
}

bool xFileSys::initWorker(void)
{
    if (!startRequestWorker()) {
        X_ERROR("FileSys", "Failed to start io request worker");
        return false;
    }

    return true;
}

void xFileSys::shutDown(void)
{
    X_LOG0("FileSys", "Shutting Down");

    shutDownRequestWorker();

    for (Search* pSearch = searchPaths_; pSearch;) {
        Search* pCur = pSearch;
        pSearch = pCur->pNext;
        if (pCur->pDir) {
            X_DELETE(pCur->pDir, &virtualDirArena_);
        }
        else {
            if (pCur->pPak->pFile) {
                X_DELETE_AND_NULL(pCur->pPak->pFile, &filePoolArena_);
            }

            X_DELETE(pCur->pPak, g_coreArena);
        }
        X_DELETE(pCur, &virtualDirArena_);
    }
}

bool xFileSys::initDirectorys(bool working)
{
    loadPacks_ = !working; // TODO: work out somethign better? basically only want packs in game and maybe some tools?

    PathWT workingDir;
    if (!PathUtil::GetCurrentDirectory(workingDir)) {
        return false;
    }

    if (working) {
        // working dir added.
        if (!setBaseDir(workingDir)) {
            return false;
        }

        if (!setSaveDir(workingDir)) {
            return false;
        }
    }
    else 
    {
        
        const wchar_t* pGameDir = gEnv->pCore->GetCommandLineArgForVarW(L"fs_basepath");
        if (pGameDir) {
            workingDir = pGameDir;
        }

        const wchar_t* pSaveDir = gEnv->pCore->GetCommandLineArgForVarW(L"fs_savepath");
      

        PathWT base(workingDir);
        PathUtil::replaceSeprators(base);
        PathUtil::ensureSlash(base);

        PathWT core(base);
        core += L"core_assets";

#if 1
        PathWT saveDir(core);

        if (pSaveDir) {
            saveDir = pSaveDir;
            PathUtil::replaceSeprators(saveDir);
            PathUtil::ensureSlash(saveDir);
        }
#else
        PathWT saveDir(base);
        saveDir += L"save_dir";

        if (!PathUtil::DirectoryExist(saveDir)) {
            PathUtil::CreateDirectoryTree(saveDir);
        }
#endif

        PathWT testAssets(base);
        PathUtil::ensureSlash(testAssets);
        testAssets += L"test_assets";

        if (!setBaseDir(core)) {
            return false;
        }
        if (!setSaveDir(saveDir)) {
            return false;
        }
#if 0 // unit tester should just add this.
        if(!addModDir(testAssets)) {
            return false;
        }
#endif
    }

    return true;
}

bool xFileSys::getWorkingDirectory(PathWT& pathOut) const
{
    return PathUtil::GetCurrentDirectory(pathOut);
}

// --------------------- Open / Close ---------------------

XFile* xFileSys::openFileOS(const PathWT& osPath, FileFlags mode)
{
    if (isDebug()) {
        X_LOG0("FileSys", "openFileOS: \"%ls\"", osPath.c_str());
    }

    XDiskFile* pFile = X_NEW(XDiskFile, &filePoolArena_, "Diskfile")(osPath, mode);
    if (pFile->valid()) {
        return pFile;
    }

    closeFile(pFile);
    return nullptr;
}


XFile* xFileSys::openFile(const PathT& relPath, FileFlags mode, VirtualDirectory::Enum writeDir)
{
    X_ASSERT(relPath.find(assetDb::ASSET_NAME_INVALID_SLASH) == nullptr, "Path must use asset slashes")(relPath.c_str());

    return findFile<XFile>(relPath, mode, writeDir,
        [&](Pak* pPak, int32_t idx) -> XFile* {
            if (isDebug()) {
                X_LOG0("FileSys", "openFile: \"%s\" fnd in pak: \"%s\"", relPath.c_str(), pPak->name.c_str());
            }

            auto& entry = pPak->pEntires[idx];
            return X_NEW(XPakFile, &filePoolArena_, "PakFile")(pPak, entry);
        },
        [&](const PathWT& osPath, FileFlags mode) -> XFile* {
            if (isDebug()) {
                X_LOG0("FileSys", "openFile: \"%ls\"", osPath.c_str());
            }

            auto* pFile = X_NEW(XDiskFile, &filePoolArena_, "Diskfile")(osPath, mode);
            if (pFile->valid()) {
                return pFile;
            }

            closeFile(pFile);
            return nullptr;
        }
    );
}

void xFileSys::closeFile(XFile* file)
{
    X_ASSERT_NOT_NULL(file);
    X_DELETE(file, &filePoolArena_);
}

// --------------------------------------------------

// async
XFileAsync* xFileSys::openFileAsync(const PathT& relPath, FileFlags mode, VirtualDirectory::Enum writeDir)
{
    X_ASSERT(relPath.find(assetDb::ASSET_NAME_INVALID_SLASH) == nullptr, "Path must use asset slashes")(relPath.c_str());

    return findFile<XFileAsync>(relPath, mode, writeDir,
        [&](Pak* pPak, int32_t idx) -> XFileAsync* {
            if (isDebug()) {
                X_LOG0("FileSys", "openFileAsync: \"%s\" fnd in pak: \"%s\"", relPath.c_str(), pPak->name.c_str());
            }

            auto& entry = pPak->pEntires[idx];
            return X_NEW(XPakFileAsync, &filePoolArena_, "PakFile")(pPak, entry, &asyncOpPoolArena_);
        },
        [&](const PathWT& osPath, FileFlags mode) -> XFileAsync* {
            if (isDebug()) {
                X_LOG0("FileSys", "openFileAsync: \"%ls\"", osPath.c_str());
            }

            auto* pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(osPath, mode, &asyncOpPoolArena_);
            if (pFile->valid()) {
                return pFile;
            }

            closeFileAsync(pFile);
            return nullptr;
        }
    );
}

void xFileSys::closeFileAsync(XFileAsync* file)
{
    X_ASSERT_NOT_NULL(file);
    X_DELETE(file, &filePoolArena_);
}

// --------------------------------------------------

XFileMem* xFileSys::openFileMem(const PathT& relPath, FileFlags mode, VirtualDirectory::Enum writeDir)
{
    X_ASSERT(relPath.find(assetDb::ASSET_NAME_INVALID_SLASH) == nullptr, "Path must use asset slashes")(relPath.c_str());

    return findFile<XFileMem>(relPath, mode, writeDir,
        [&](Pak* pPak, int32_t idx) -> XFileMem* {
            if (isDebug()) {
                X_LOG0("FileSys", "openFileMem: \"%s\" fnd in pak: \"%s\"", relPath.c_str(), pPak->name.c_str());
            }

            auto& entry = pPak->pEntires[idx];
            
            XPakFile file(pPak, entry);

            size_t size = safe_static_cast<size_t, int64_t>(file.remainingBytes());
            char* pBuf = X_NEW_ARRAY(char, size, &memFileArena_, "MemBuffer");

            if (file.read(pBuf, size) != size) {
                X_DELETE_ARRAY(pBuf, &memFileArena_);
                return nullptr;
            }
            
            return X_NEW(XFileMem, &filePoolArena_, "MemFilePak")(pBuf, pBuf + size, &memFileArena_);
        },
        [&](const PathWT& osPath, FileFlags mode) -> XFileMem* {
            if (isDebug()) {
                X_LOG0("FileSys", "openFileMem: \"%ls\"", osPath.c_str());
            }

            OsFile file(osPath, mode);
            if (!file.valid()) {
                return nullptr;
            }

            size_t size = safe_static_cast<size_t, int64_t>(file.remainingBytes());
            char* pBuf = X_NEW_ARRAY(char, size, &memFileArena_, "MemBuffer");

            if (file.read(pBuf, size) != size) {
                X_DELETE_ARRAY(pBuf, &memFileArena_);
                return nullptr;
            }

            return X_NEW(XFileMem, &filePoolArena_, "MemFile")(pBuf, pBuf + size, &memFileArena_);
        }
    );
}


void xFileSys::closeFileMem(XFileMem* file)
{
    X_ASSERT_NOT_NULL(file);
    // class free's the buffer.
    X_DELETE(file, &filePoolArena_);
}


template<typename FileT, typename PakFuncT, typename FuncT>
FileT* xFileSys::findFile(const PathT& relPath, FileFlags mode, VirtualDirectory::Enum writeDir, PakFuncT pakFunc, FuncT func)
{
    PathWT osPath;

    if (mode.IsSet(FileFlag::READ) && !mode.IsSet(FileFlag::WRITE))
    {
        core::StrHash hash(relPath.data(), relPath.length());

        for (const Search* pSearch = searchPaths_; pSearch; pSearch = pSearch->pNext)
        {
            if (pSearch->pDir)
            {
                const auto* pDir = pSearch->pDir;

                createOSPath(pDir, relPath, osPath);

                if (PathUtil::FileExist(osPath, true)) {
                    return func(osPath, mode);
                }
            }
            else if (pSearch->pPak)
            {
                auto* pPak = pSearch->pPak;

                auto idx = pPak->find(hash, relPath.c_str());
                if (idx != core::Pak::INVALID_INDEX) {
                    return pakFunc(pPak, idx);
                }
            }
        }
    }
    else
    {
        createOSPath(writeDir, relPath, osPath);

        return func(osPath, mode);
    }

    return nullptr;
}


// --------------------- folders ---------------------

bool xFileSys::setBaseDir(const PathWT& osPath)
{
    X_ASSERT(baseDir_ == nullptr, "can only set one game directoy")(osPath.c_str(), baseDir_);

    // check if the directory is even valid.
    if (!directoryExistsOS(osPath)) {
        PathWT fullPath;
        PathUtil::GetFullPath(osPath, fullPath);
        X_ERROR("FileSys", "Faled to set game directory, path does not exsists: \"%ls\"", fullPath.c_str());
        return false;
    }

    auto* pDir = addDirInteral(osPath);
    if (!pDir) {
        return false;
    }

    baseDir_ = pDir;
    return true;
}

bool xFileSys::setSaveDir(const PathWT& osPath)
{
    X_ASSERT(saveDir_ == nullptr, "can only set one game directoy")(osPath.c_str(), saveDir_);

    // check if the directory is even valid.
    if (!directoryExistsOS(osPath)) {
        PathWT fullPath;
        PathUtil::GetFullPath(osPath, fullPath);
        X_ERROR("FileSys", "Faled to set save directory, path does not exsists: \"%ls\"", fullPath.c_str());
        return false;
    }

    auto* pDir = addDirInteral(osPath);
    if(!pDir) {
        return false;
    }

    saveDir_ = pDir;
    return true;
}


bool xFileSys::addModDir(const PathWT& osPath)
{
    return addDirInteral(osPath) != nullptr;
}


Directory* xFileSys::addDirInteral(const PathWT& osPath)
{
    if (isDebug()) {
        X_LOG0("FileSys", "addModDir: \"%ls\"", osPath.c_str());
    }

    if (numDir_ == MAX_VIRTUAL_DIR) {
        X_ERROR("FileSys", "Reached max virtual dir(%" PRIuS ")", MAX_VIRTUAL_DIR);
        return false;
    }

    if (!directoryExistsOS(osPath)) {
        X_ERROR("FileSys", "Faled to add virtual drectory, the directory does not exsists: \"%ls\"", osPath.c_str());
        return nullptr;
    }

    // ok remove any ..//
    PathWT fixedPath;
    if (!PathUtil::GetFullPath(osPath, fixedPath)) {
        X_ERROR("FileSys", "addModDir full path name creation failed");
        return nullptr;
    }

    PathUtil::ensureSlash(fixedPath);

    if (!directoryExistsOS(fixedPath)) {
        X_ERROR("FileSys", "Fixed path does not exsists: \"%ls\"", fixedPath.c_str());
        return nullptr;
    }

    // work out if this directory is a sub directory of any of the current
    // searxh paths.
    for (Search* s = searchPaths_; s; s = s->pNext) {
        if (!s->pDir) {
            continue;
        }

        if (strUtil::FindCaseInsensitive(fixedPath.c_str(), s->pDir->path.c_str()) != nullptr) {
            return s->pDir;
        }
    }

    // add it to virtual file system.
    Search* search = X_NEW(Search, &virtualDirArena_, "FileSysSearch");
    search->pDir = X_NEW(Directory, &virtualDirArena_, "FileSysDir");
    search->pDir->path = fixedPath;
    search->pPak = nullptr;
    search->pNext = searchPaths_;
    searchPaths_ = search;

    ++numDir_;

    // add hotreload dir.
#if X_ENABLE_DIR_WATCHER
    gEnv->pDirWatcher->addDirectory(fixedPath);
#endif // !X_ENABLE_DIR_WATCHER

    if (!loadPacks_) {
        return search->pDir;
    }

    // Load packs.
    auto searchPath = fixedPath;
    searchPath.appendFmt(L"*.%S", AssetPak::PAK_FILE_EXTENSION);

    FindData findInfo;
    auto findPair = PathUtil::findFirst(searchPath, findInfo);

    if (findPair.handle != INVALID_FIND_HANDLE)
    {
        do
        {
            if (!openPak(findInfo.name))
            {
                X_ERROR("FileSys", "Failed to add pak: \"%s\"", findInfo.name.c_str());
            }
        }
        while(PathUtil::findNext(findPair.handle, findInfo));

        PathUtil::findClose(findPair.handle);
    }

    return search->pDir;
}

// --------------------- Find util ---------------------

FindPair xFileSys::findFirst(const PathT& relPath, FindData& findinfo)
{
    // i don't like how the findData shit works currently it's anoying!
    // so this is start of new version but i dunno how i want it to work yet.
    // i want somthing better but don't know what it is :)
    // list of shit:
    // 1. should return relative paths to the requested search dir.
    // 2. should expose some nicer scoped based / maybe iterative design to make nicer to use.
    // 3. <insert idea here>
    //

    PathWT osPath;
    createOSPath(baseDir_, relPath, osPath); // TODO: search all paths?

    return findFirstOS(osPath, findinfo);
}

FindPair xFileSys::findFirstOS(const PathWT& osPath, FindData& findinfo)
{
    auto fp = PathUtil::findFirst(osPath, findinfo);

    return fp;
}

bool xFileSys::findnext(findhandle handle, FindData& findinfo)
{
    X_ASSERT(handle != INVALID_FIND_HANDLE, "FindNext called with invalid handle")(handle);

    return PathUtil::findNext(handle, findinfo);
}

void xFileSys::findClose(findhandle handle)
{
    PathUtil::findClose(handle);
}

// --------------------- Delete ---------------------

bool xFileSys::deleteFile(const PathT& relPath, VirtualDirectory::Enum dir) const
{
    PathWT osPath;
    createOSPath(dir, relPath, osPath);

    if (isDebug()) {
        X_LOG0("FileSys", "deleteFile: \"%ls\"", osPath.c_str());
    }

    return PathUtil::DeleteFile(osPath);
}

bool xFileSys::deleteDirectory(const PathT& relPath, VirtualDirectory::Enum dir, bool recursive) const
{
    PathWT osPath;
    createOSPath(dir, relPath, osPath);

    if (osPath.fillSpaceWithNullTerm() < 1) {
        X_ERROR("FileSys", "Failed to pad puffer for OS operation");
        return false;
    }

    if (isDebug()) {
        X_LOG0("FileSys", "deleteDirectory: \"%ls\"", osPath.c_str());
    }

    return PathUtil::DeleteDirectory(osPath, recursive);
}

bool xFileSys::deleteDirectoryContents(const PathT& path, VirtualDirectory::Enum dir)
{
    PathWT osPath;
    createOSPath(dir, path, osPath);

    if (isDebug()) {
        X_LOG0("FileSys", "deleteDirectoryContents: \"%s\"", path.c_str());
    }

    // check if the dir exsists.
    if (!directoryExistsOS(osPath)) {
        return false;
    }

    // we build a relative search path.
    // as findFirst works on game dir's
    PathWT searchPath(osPath);
    PathUtil::ensureSlash(searchPath);
    searchPath.append(L"*");

    FindData fd;
    auto findPair = PathUtil::findFirst(searchPath, fd);
    if (findPair.handle != INVALID_FIND_HANDLE) {
        return findPair.valid;
    }

    do {
        // build a OS Path.
        PathWT dirItem(osPath);
        PathUtil::ensureSlash(dirItem);
        dirItem.append(fd.name.begin(), fd.name.end());

        if (PathUtil::IsDirectory(fd)) {
            if (dirItem.fillSpaceWithNullTerm() < 1) {
                X_ERROR("FileSys", "Failed to pad puffer for OS operation");
                return false;
            }

            if (!PathUtil::DeleteDirectory(dirItem, true)) {
                return false;
            }
        }
        else {
            if (!PathUtil::DeleteFile(dirItem)) {
                return false;
            }
        }

    } while (PathUtil::findNext(findPair.handle, fd));

    PathUtil::findClose(findPair.handle);
    return true;
}

// --------------------- Create ---------------------

bool xFileSys::createDirectory(const PathT& relPath, VirtualDirectory::Enum dir) const
{
    PathWT osPath;
    createOSPath(dir, relPath, osPath);

    osPath.removeFileName();

    if (isDebug()) {
        X_LOG0("FileSys", "createDirectory: \"%ls\"", osPath.c_str());
    }

    return PathUtil::CreateDirectory(osPath);
}

bool xFileSys::createDirectoryOS(const PathWT& osPath) const
{
    PathWT path(osPath);
    path.removeFileName();

    if (isDebug()) {
        X_LOG0("FileSys", "createDirectory: \"%ls\"", path.c_str());
    }

    return PathUtil::CreateDirectory(path);
}

bool xFileSys::createDirectoryTree(const PathT& relPath, VirtualDirectory::Enum dir) const
{
    // we want to just loop and create like a goat.
    PathWT osPath;
    createOSPath(dir, relPath, osPath);

    osPath.removeFileName();

    if (isDebug()) {
        X_LOG0("FileSys", "CreateDirectoryTree: \"%ls\"", osPath.c_str());
    }

    return PathUtil::CreateDirectoryTree(osPath);
}

bool xFileSys::createDirectoryTreeOS(const PathWT& osPath) const
{
    // we want to just loop and create like a goat.
    PathWT path(osPath);
    path.removeFileName();

    if (isDebug()) {
        X_LOG0("FileSys", "CreateDirectoryTree: \"%ls\"", path.c_str());
    }

    return PathUtil::CreateDirectoryTree(path);
}

// --------------------- exsists ---------------------

bool xFileSys::fileExists(const PathT& relPath) const
{
    PathWT osPath;

    core::StrHash hash(relPath.data(), relPath.length());

    for (const Search* pSearch = searchPaths_; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->pDir)
        {
            const auto* pDir = pSearch->pDir;
            createOSPath(pDir, relPath, osPath);

            if (PathUtil::FileExist(osPath, true)) {
                return true;
            }
        }
        else if (pSearch->pPak)
        {
            auto* pPak = pSearch->pPak;

            auto idx = pPak->find(hash, relPath.c_str());
            if (idx != core::Pak::INVALID_INDEX) {
                return true;
            }
        }
    }

    return false;
}


bool xFileSys::fileExists(const PathT& relPath, VirtualDirectory::Enum dir) const
{
    PathWT osPath;
    createOSPath(dir, relPath, osPath);

    if (PathUtil::FileExist(osPath, true)) {
        return true;
    }

    return false;
}

bool xFileSys::fileExistsOS(const PathWT& osPath) const
{
    return PathUtil::FileExist(osPath, false);
}

bool xFileSys::directoryExists(const PathT& relPath) const
{
    PathWT osPath;

    for (const Search* pSearch = searchPaths_; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->pDir)
        {
            const auto* pDir = pSearch->pDir;
            createOSPath(pDir, relPath, osPath);

            if (PathUtil::DirectoryExist(osPath)) {
                return true;
            }
        }
    }

    return false;
}

bool xFileSys::directoryExists(const PathT& relPath, VirtualDirectory::Enum dir) const
{
    PathWT osPath;
    createOSPath(dir, relPath, osPath);

    if (PathUtil::DirectoryExist(osPath)) {
        return true;
    }

    return false;
}

bool xFileSys::directoryExistsOS(const PathWT& osPath) const
{
    return core::PathUtil::DirectoryExist(osPath);
}

bool xFileSys::isDirectory(const PathT& relPath, VirtualDirectory::Enum dir) const
{
    PathWT osPath;
    createOSPath(dir, relPath, osPath);

    if (PathUtil::IsDirectory(osPath)) {
        return true;
    }

    return false;
}

bool xFileSys::isDirectoryOS(const PathWT& osPath) const
{
    bool result = core::PathUtil::IsDirectory(osPath);

    if (isDebug()) {
        X_LOG0("FileSys", "isDirectory: \"%ls\" res: ^6%s", osPath.c_str(), result ? "TRUE" : "FALSE");
    }

    return result;
}

bool xFileSys::moveFile(const PathT& relPath, const PathT& newPathRel, VirtualDirectory::Enum dir) const
{
    PathWT osPath, osPathNew;

    createOSPath(dir, relPath, osPath);
    createOSPath(dir, newPathRel, osPathNew);

    return moveFileOS(osPath, osPathNew);
}

bool xFileSys::moveFileOS(const PathWT& osPath, const PathWT& osPathNew) const
{
    bool result = core::PathUtil::MoveFile(osPath, osPathNew);

    if (isDebug()) {
        X_LOG0("FileSys", "moveFile: \"%ls\" -> \"%ls\" res: ^6%s",
            osPath.c_str(), osPathNew.c_str(), result ? "TRUE" : "FALSE");
    }

    return result;
}

size_t xFileSys::getMinimumSectorSize(void) const
{
    // get all the driver letters.
    size_t sectorSize = 0;
    FileFlags mode(FileFlag::READ);

    core::FixedArray<wchar_t, 128> driveLettters;

    for (Search* s = searchPaths_; s; s = s->pNext) {
        if (s->pDir) {
            const auto& path = s->pDir->path;
            int8_t driveIdx = path.getDriveNumber(); // watch this be a bug at somepoint.
            if (driveIdx >= 0) {
                wchar_t driveLetter = (L'A' + driveIdx);

                if (std::find(driveLettters.begin(), driveLettters.end(), driveLetter) == driveLettters.end()) {
                    driveLettters.append(driveLetter);
                }
            }
        }
    }

    for (const auto d : driveLettters) {
        core::StackString<64, wchar_t> device(L"\\\\.\\"); // this is OS specific.

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

// Ajust path
const wchar_t* xFileSys::createOSPath(VirtualDirectory::Enum dir, const PathT& path, PathWT& buffer) const
{
    if (dir == VirtualDirectory::BASE) {
        return createOSPath(baseDir_, path, buffer);
    }
    else if (dir == VirtualDirectory::SAVE) {
        return createOSPath(saveDir_, path, buffer);
    } 

    X_ASSERT_UNREACHABLE();
    return createOSPath(baseDir_, path, buffer);
}

const wchar_t* xFileSys::createOSPath(const Directory* dir, const PathT& path, PathWT& buffer) const
{
    if (!isAbsolute(path)) {
        buffer = dir->path; 
        PathUtil::ensureSlash(buffer);
        buffer.append(path.begin(), path.end());
    }
    else {
        buffer.set(path.begin(), path.end());
    }

    PathUtil::replaceSeprators(buffer);
    return buffer.c_str();
}

const wchar_t* xFileSys::createOSPath(const Directory* dir, const PathWT& path, PathWT& buffer) const
{
    // is it absolute?
    if (!isAbsolute(path)) {
        buffer = dir->path / path;
    }
    else {
        buffer = path;
    }

    PathUtil::replaceSeprators(buffer);
    return buffer.c_str();
}

bool xFileSys::isAbsolute(const PathT& path) const
{
    return path.isAbsolute();
}

bool xFileSys::isAbsolute(const PathWT& path) const
{
    return path.isAbsolute();
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

    switch (request.getType()) {
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

    if (vars_.queueDebug_) {
        uint32_t threadId = core::Thread::getCurrentID();
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

void xFileSys::cancelRequest(RequestHandle handle)
{
    X_UNUSED(handle);
    X_ASSERT_NOT_IMPLEMENTED();
}

void xFileSys::waitForRequest(RequestHandle handle)
{
    X_UNUSED(handle);
    X_ASSERT_NOT_IMPLEMENTED(); 
    // i can't think how to implement this, other than many some extra callback you can set on the request.
    // like if you pass supportWaiting = true, \o/
}

bool xFileSys::startRequestWorker(void)
{
    ThreadAbstract::create("IoWorker", 1024 * 128); // small stack
    ThreadAbstract::start();
    return true;
}

void xFileSys::shutDownRequestWorker(void)
{
    if (ThreadAbstract::getState() != core::Thread::State::RUNNING) {
        return;
    }

    ThreadAbstract::stop();

    {
        // post a close job with a none null callback.
        IoRequestClose* pRequest = X_NEW(IoRequestClose, ioQueueDataArena_, "IORequestClose");
        ::memset(&pRequest->callback, 1, sizeof(pRequest->callback));

        CriticalSection::ScopedLock lock(requestLock_);

        requests_.push(pRequest);
    }

    requestSignal_.raise();
    ThreadAbstract::join();
}

IoRequestBase* xFileSys::popRequest(void)
{
    CriticalSection::ScopedLock lock(requestLock_);

    while (requests_.isEmpty()) {
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

void xFileSys::onOpFinsihed(PendingOpBase& asyncOp, uint32_t bytesTransferd)
{
    if (asyncOp.getType() == IoRequest::READ) {
        const IoRequestRead* pAsyncReq = asyncOp.as<const IoRequestRead>();
        XFileAsync* pReqFile = pAsyncReq->pFile;

        static_assert(core::compileTime::IsTrivialDestruct<IoRequestRead>::Value, "Need to call destructor");

        if (vars_.queueDebug_) {
            uint32_t threadId = core::Thread::getCurrentID();

            X_LOG0("FileSys", "IoRequest(0x%x) '%s' async read request complete. "
                              "bytesTrans: 0x%x pBuf: %p",
                threadId, IoRequest::ToString(pAsyncReq->getType()),
                bytesTransferd, pAsyncReq->pBuf);
        }

        pAsyncReq->callback.Invoke(*this, pAsyncReq, pReqFile, bytesTransferd);
    }
    else if (asyncOp.getType() == IoRequest::WRITE) {
        const IoRequestWrite* pAsyncReq = asyncOp.as<const IoRequestWrite>();
        XFileAsync* pReqFile = pAsyncReq->pFile;

        static_assert(core::compileTime::IsTrivialDestruct<IoRequestWrite>::Value, "Need to call destructor");

        if (vars_.queueDebug_) {
            uint32_t threadId = core::Thread::getCurrentID();

            X_LOG0("FileSys", "IoRequest(0x%x) '%s' async write request complete. "
                              "bytesTrans: 0x%x pBuf: %p",
                threadId, IoRequest::ToString(pAsyncReq->getType()),
                bytesTransferd, pAsyncReq->pBuf);
        }

        pAsyncReq->callback.Invoke(*this, pAsyncReq, pReqFile, bytesTransferd);
    }
    else if (asyncOp.getType() == IoRequest::OPEN_READ_ALL) {
        IoRequestOpenRead* pAsyncReq = asyncOp.as<IoRequestOpenRead>();
        XFileAsync* pReqFile = pAsyncReq->pFile;

        static_assert(core::compileTime::IsTrivialDestruct<IoRequestOpenRead>::Value, "Need to call destructor");

        if (vars_.queueDebug_) {
            uint32_t threadId = core::Thread::getCurrentID();

            X_LOG0("FileSys", "IoRequest(0x%x) '%s' async open-read request complete. "
                              "bytesTrans: 0x%x pBuf: %p",
                threadId, IoRequest::ToString(pAsyncReq->getType()),
                bytesTransferd, pAsyncReq->pBuf);
        }

        if (pAsyncReq->dataSize != bytesTransferd) {
            X_ERROR("FileSys", "Failed to read whole file contents. requested: %" PRIu32 " got %" PRIu32,
                pAsyncReq->dataSize, bytesTransferd);

            X_DELETE_ARRAY(static_cast<uint8_t*>(pAsyncReq->pBuf), pAsyncReq->arena);
            // we didnt not read the whole file, pretend we failed to open.
            pAsyncReq->pBuf = nullptr;
            pAsyncReq->dataSize = 0;
            pAsyncReq->callback.Invoke(*this, pAsyncReq, nullptr, 0);
        }
        else {
            pAsyncReq->callback.Invoke(*this, pAsyncReq, pReqFile, bytesTransferd);
        }

        // close it.
        closeFileAsync(pReqFile);
    }
    else if (asyncOp.getType() == IoRequest::OPEN_WRITE_ALL) {
        IoRequestOpenWrite* pAsyncReq = asyncOp.as<IoRequestOpenWrite>();
        XFileAsync* pReqFile = pAsyncReq->pFile;

        if (vars_.queueDebug_) {
            uint32_t threadId = core::Thread::getCurrentID();

            X_LOG0("FileSys", "IoRequest(0x%x) '%s' async open-write request complete. "
                              "bytesTrans: 0x%x pBuf: %p",
                threadId, IoRequest::ToString(pAsyncReq->getType()),
                bytesTransferd, pAsyncReq->data.ptr());
        }

        if (pAsyncReq->data.size() != bytesTransferd) {
            X_ERROR("FileSys", "Failed to write whole file contents. requested: %" PRIu32 " got %" PRIu32,
                pAsyncReq->data.size(), bytesTransferd);

            // we didnt not write the whole file, pretend we failed to open.
            pAsyncReq->callback.Invoke(*this, pAsyncReq, nullptr, 0);
        }
        else {
            pAsyncReq->callback.Invoke(*this, pAsyncReq, pReqFile, bytesTransferd);
        }

        // deconstruct.
        static_assert(!core::compileTime::IsTrivialDestruct<IoRequestOpenWrite>::Value, "no need to call destructor");

        core::Mem::Destruct(pAsyncReq);

        // close it.
        closeFileAsync(pReqFile);
    }
    else {
        X_ASSERT_UNREACHABLE();
    }
}

void xFileSys::AsyncIoCompletetionRoutine(XOsFileAsyncOperation::AsyncOp* pOperation, uint32_t bytesTransferd)
{
    X_ASSERT(pendingCompOps_.isNotEmpty(), "Recived a unexpected Async complition")(pOperation, bytesTransferd);

    for (size_t i = 0; i < pendingCompOps_.size(); i++) {
        PendingCompiltionOp& asyncOp = pendingCompOps_[i];

        if (asyncOp.op.ownsAsyncOp(pOperation)) {
#if X_ENABLE_FILE_ARTIFICAIL_DELAY
            {
                auto type = asyncOp.getType();
                int32_t delayMS = 0;
                if (type == IoRequest::READ) {
                    delayMS = vars_.artReadDelay_;
                }
                else if (type == IoRequest::WRITE) {
                    delayMS = vars_.artWriteDelay_;
                }

                if (delayMS > 0) {
                    const auto delay = core::TimeVal::fromMS(delayMS);
                    auto time = gEnv->pTimer->GetTimeNowReal() + delay;

                    delayedOps_.emplace(DelayedPendingCompiltionOp(std::move(asyncOp), time));
                    pendingCompOps_.removeIndex(i);

                    stats_.DelayedOps = delayedOps_.size();
                    return;
                }
            }
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

            onOpFinsihed(asyncOp, bytesTransferd);
            pendingCompOps_.removeIndex(i);

            updatePendingOpsStats();
            return;
        }
    }

    // failed to fine the op ;(
    X_ASSERT_UNREACHABLE();
}

void xFileSys::updatePendingOpsStats(void)
{
#if X_ENABLE_FILE_STATS
    stats_.PendingOps = pendingCompOps_.size() + pendingOps_.size();
#endif // !X_ENABLE_FILE_STATS
}

Thread::ReturnValue xFileSys::ThreadRun(const Thread& thread)
{
    gEnv->pJobSys->CreateQueForCurrentThread();

    xFileSys& fileSys = *this;

    IoRequestBase* pRequest = nullptr;

    XOsFileAsyncOperation::ComplitionRotinue compRoutine;
    compRoutine.Bind<xFileSys, &xFileSys::AsyncIoCompletetionRoutine>(this);

    auto checkForCompletedOps = [&] {
        if (pendingOps_.isNotEmpty()) {
            for (size_t i = 0; i < pendingOps_.size(); i++) {
                uint32_t bytesTrans = 0;
                if (pendingOps_[i].op.hasFinished(&bytesTrans)) {
                    onOpFinsihed(pendingOps_[i], bytesTrans);
                    pendingOps_.removeIndex(i);

                    updatePendingOpsStats();
                }
            }
        }

#if X_ENABLE_FILE_ARTIFICAIL_DELAY
        if (delayedOps_.isNotEmpty()) {
            auto now = gEnv->pTimer->GetTimeNowReal();

            while (delayedOps_.isNotEmpty() && delayedOps_.peek().time < now) {
                auto& delayedOp = delayedOps_.peek().op;

                uint32_t bytesTrans;
                X_ASSERT(delayedOp.op.hasFinished(&bytesTrans), "Delayed op should be always be finished")();

                onOpFinsihed(delayedOp, bytesTrans);

                delayedOps_.pop();
            }

            stats_.DelayedOps = delayedOps_.size();
        }
#endif
    };

    auto noPendingOps = [&]() -> bool {
        return pendingOps_.isEmpty()
#if X_ENABLE_FILE_ARTIFICAIL_DELAY
               && delayedOps_.isEmpty()
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY
            ;
    };

    while (thread.shouldRun()) {
        if (pendingCompOps_.isEmpty() && noPendingOps()) {
            pRequest = popRequest();
        }
        else {
            // we have pending requests, lets quickly enter a alertable state.
            // then we will try get more requests, untill we get more requests
            // we just stay in a alertable state, so that any pending requests can handled.
            requestSignal_.wait(0, true);

            checkForCompletedOps();

            while ((pRequest = tryPopRequest()) == nullptr) {
                if (noPendingOps()) {
                    requestSignal_.wait(core::Signal::WAIT_INFINITE, true);
                }
                else {
                    // if we have some op's not waiting for APC wake up periodically and check them.
                    requestSignal_.wait(2, true);

                    checkForCompletedOps();
                }
            }

            // we have a request.
        }

        core::UniquePointer<IoRequestBase> requestPtr(ioQueueDataArena_, pRequest);
        const auto type = pRequest->getType();

#if X_ENABLE_FILE_STATS
        auto start = core::StopWatch::GetTimeNow();
#endif // !X_ENABLE_FILE_STATS

        if (type == IoRequest::OPEN) {
            IoRequestOpen* pOpen = static_cast<IoRequestOpen*>(pRequest);
            XFileAsync* pFile = openFileAsync(pOpen->path, pOpen->mode, VirtualDirectory::BASE);

            pOpen->callback.Invoke(fileSys, pOpen, pFile, 0);
        }
        else if (type == IoRequest::OPEN_READ_ALL) {
            IoRequestOpenRead* pOpenRead = static_cast<IoRequestOpenRead*>(pRequest);
            XFileAsync* pFileAsync = openFileAsync(pOpenRead->path, pOpenRead->mode, VirtualDirectory::BASE);

            // make sure it's safe to allocate the buffer in this thread.
            X_ASSERT_NOT_NULL(pOpenRead->arena);
            X_ASSERT(pOpenRead->arena->isThreadSafe(), "Async OpenRead requests require thread safe arena")(pOpenRead->arena->isThreadSafe());

            if (!pFileAsync) {
                pOpenRead->callback.Invoke(fileSys, pOpenRead, pFileAsync, 0);
                goto nextRequest;
            }

            const uint64_t fileSize = pFileAsync->fileSize();

            // we don't support open and read for files over >2gb
            // if you are trying todo that you are retarded.
            // instead open the file and submit smaller read requests.
            if (fileSize > std::numeric_limits<int32_t>::max()) {
                X_ERROR("FileSys", "A request was made to read a entire file which is >2GB, ignoring request. File: \"%s\"",
                    pOpenRead->path.c_str());

                closeFileAsync(pFileAsync);
                pOpenRead->callback.Invoke(fileSys, pOpenRead, nullptr, 0);
            }
            else {
#if X_ENABLE_FILE_STATS
                stats_.NumBytesRead += fileSize;
#endif // !X_ENABLE_FILE_STATS

                uint8_t* pData = X_NEW_ARRAY_ALIGNED(uint8_t, safe_static_cast<size_t>(fileSize), pOpenRead->arena, "AsyncIOReadAll", 16);
                auto fileType = pFileAsync->getType();

                pOpenRead->pFile = pFileAsync;
                pOpenRead->pBuf = pData;
                pOpenRead->dataSize = safe_static_cast<uint32_t>(fileSize);

                if (fileType == XFileAsync::Type::DISK) {
                    XDiskFileAsync* pFile = static_cast<XDiskFileAsync*>(pFileAsync);

                    XFileAsyncOperationCompiltion operation = pFile->readAsync(
                        pData,
                        safe_static_cast<size_t>(fileSize),
                        0,
                        compRoutine);

                    pendingCompOps_.emplace_back(std::move(requestPtr), std::move(operation));
                }
                else if (fileType == XFileAsync::Type::VIRTUAL) {
                    XPakFileAsync* pFile = static_cast<XPakFileAsync*>(pFileAsync);

                    if (pFile->supportsComplitionRoutine()) {
                        pendingCompOps_.emplace_back(std::move(requestPtr), pFile->readAsync(
                            pData,
                            safe_static_cast<size_t>(fileSize),
                            0,
                            compRoutine));
                    }
                    else {
                        PendingOp op(std::move(requestPtr), pFile->readAsync(
                            pData,
                            safe_static_cast<size_t>(fileSize),
                            0));

                        uint32_t bytesTrans = 0;
                        if (op.op.hasFinished(&bytesTrans)) {
                            onOpFinsihed(op, bytesTrans);
                        }
                        else {
                            pendingOps_.emplace_back(std::move(op));
                        }
                    }
                }
            }
        }
        else if (type == IoRequest::OPEN_WRITE_ALL) {
            IoRequestOpenWrite* pOpenWrite = static_cast<IoRequestOpenWrite*>(pRequest);

            auto flags = core::FileFlags::RECREATE | core::FileFlags::WRITE;
            XDiskFileAsync* pFile = static_cast<XDiskFileAsync*>(openFileAsync(pOpenWrite->path, flags, VirtualDirectory::BASE));

            X_ASSERT(pOpenWrite->data.getArena()->isThreadSafe(), "Async OpenWrite requests require thread safe arena")();
            X_ASSERT(pOpenWrite->data.size() > 0, "WriteAll called with data size 0")(pOpenWrite->data.size());

            if (!pFile) {
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
                compRoutine);

            pendingCompOps_.emplace_back(std::move(requestPtr), std::move(operation));
        }
        else if (type == IoRequest::CLOSE) {
            // if the close job has a callback, we are shutting down.
            if (pRequest->callback) {
                continue;
            }

            // normal close request.
            IoRequestClose* pClose = static_cast<IoRequestClose*>(pRequest);

            closeFileAsync(pClose->pFile);
        }
        else if (type == IoRequest::READ) {
            IoRequestRead* pRead = static_cast<IoRequestRead*>(pRequest);
            auto fileType = pRead->pFile->getType();

#if X_ENABLE_FILE_STATS
            stats_.NumBytesRead += pRead->dataSize;
#endif // !X_ENABLE_FILE_STATS

            if (fileType == XFileAsync::Type::DISK) {
                XDiskFileAsync* pFile = static_cast<XDiskFileAsync*>(pRead->pFile);

                XFileAsyncOperationCompiltion operation = pFile->readAsync(
                    pRead->pBuf,
                    pRead->dataSize,
                    pRead->offset,
                    compRoutine);

                pendingCompOps_.emplace_back(std::move(requestPtr), std::move(operation));
            }
            else if (fileType == XFileAsync::Type::VIRTUAL) {
                XPakFileAsync* pFile = static_cast<XPakFileAsync*>(pRead->pFile);

                if (pFile->supportsComplitionRoutine()) {
                    pendingCompOps_.emplace_back(std::move(requestPtr), pFile->readAsync(
                                                                            pRead->pBuf,
                                                                            pRead->dataSize,
                                                                            pRead->offset,
                                                                            compRoutine));
                }
                else {
                    PendingOp op(std::move(requestPtr), pFile->readAsync(
                                                            pRead->pBuf,
                                                            pRead->dataSize,
                                                            pRead->offset));

                    uint32_t bytesTrans = 0;
                    if (op.op.hasFinished(&bytesTrans)) {
                        onOpFinsihed(op, bytesTrans);
                    }
                    else {
                        pendingOps_.emplace_back(std::move(op));
                    }
                }
            }
            else {
                X_ASSERT_UNREACHABLE();
            }
        }
        else if (type == IoRequest::WRITE) {
            IoRequestWrite* pWrite = static_cast<IoRequestWrite*>(pRequest);
            XDiskFileAsync* pFile = static_cast<XDiskFileAsync*>(pWrite->pFile);

            X_ASSERT(pWrite->pFile->getType() != XFileAsync::Type::VIRTUAL, "Tried to write to virtual file")(pWrite->pFile->getType());

#if X_ENABLE_FILE_STATS
            stats_.NumBytesWrite += pWrite->dataSize;
#endif // !X_ENABLE_FILE_STATS

            XFileAsyncOperationCompiltion operation = pFile->writeAsync(
                pWrite->pBuf,
                pWrite->dataSize,
                pWrite->offset,
                compRoutine);

            pendingCompOps_.emplace_back(std::move(requestPtr), std::move(operation));
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

OsFileAsync* xFileSys::openPakFile(const PathT& relPath)
{
    FileFlags mode;
    mode.Set(FileFlag::READ);
    mode.Set(FileFlag::RANDOM_ACCESS);

    PathWT osPath;

    for (const Search* pSearch = searchPaths_; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->pDir)
        {
            const auto* pDir = pSearch->pDir;
            createOSPath(pDir, relPath, osPath);

            if (PathUtil::FileExist(osPath, true)) 
            {
                if (isDebug()) {
                    X_LOG0("FileSys", "openFileAsync: \"%ls\"", osPath.c_str());
                }

                OsFileAsync* pFile = X_NEW(OsFileAsync, &filePoolArena_, "DiskFileAsync")(osPath, mode, &asyncOpPoolArena_);
                if (pFile->valid()) {
                    return pFile;
                }

                X_DELETE(pFile, &filePoolArena_);
            }
        }
    }

    return nullptr;
}

bool xFileSys::openPak(const PathT& relPath)
{
    X_LOG1("FileSys", "Mounting pak: \"%s\"", relPath.c_str());

    if (numPak_ == MAX_PAK) {
        X_ERROR("FileSys", "Reached max open paks(%" PRIuS ")", MAX_PAK);
        return false;
    }

    // you can only open pak's from inside the virtual filesystem.
    auto* pFile = openPakFile(relPath);
    if (!pFile) {
        X_ERROR("FileSys", "Failed to open pak");
        return false;
    }

    // we need the header Wagg man.
    AssetPak::APakHeader hdr;

    auto op = pFile->readAsync(&hdr, sizeof(hdr), 0);
    if (op.waitUntilFinished() != sizeof(hdr)) {
        X_ERROR("FileSys", "Failed to read pak hdr");
        return false;
    }

    if (!hdr.isValid()) {
        X_ERROR("FileSys", "Invalid pak header");
        return false;
    }

    if (hdr.version != AssetPak::PAK_VERSION) {
        X_ERROR("FileSys", "Pak Version incorrect. got %" PRIu8 " require %" PRIu8, hdr.version, AssetPak::PAK_VERSION);
        return false;
    }

    if (hdr.numAssets == 0) {
        X_ERROR("FileSys", "Pak is empty.");
        return false;
    }

    if (hdr.size != hdr.inflatedSize) {
        X_ERROR("FileSys", "Compressed Paks not currently supported");
        return false;
    }

    // for now switch to memory loading if below setting.
    // dunno if want to be more smart about which pak's we load to memory..
    PakMode::Enum pakMode = PakMode::STREAM;

    if (hdr.size <= vars_.pakMemorySizeLimitMB_ * (1024 * 1024)) {
        pakMode = PakMode::MEMORY;
    }

    if (hdr.flags.IsSet(AssetPak::APakFlag::HINT_MEMORY)) {
        pakMode = PakMode::MEMORY;
    }

    if (pakMode == PakMode::MEMORY) {
        if (hdr.size > std::numeric_limits<int32_t>::max()) {
            core::HumanSize::Str str;
            X_WARNING("AssetPak", "Can't load pak in memory mode it's too large, steaming instead. size: %s", core::HumanSize::toString(str, hdr.size));
            pakMode = PakMode::STREAM;
        }
    }

    const size_t stringBlockSize = hdr.entryTableOffset - hdr.stringDataOffset;
    const size_t dataSize = safe_static_cast<size_t>(pakMode == PakMode::MEMORY ? hdr.size : hdr.dataOffset);

    auto pPak = core::makeUnique<Pak>(g_coreArena, g_coreArena);
    pPak->name.set(relPath.fileName());
    pPak->mode = pakMode;
    pPak->pFile = pFile;
    pPak->numAssets = hdr.numAssets;
    pPak->dataOffset = hdr.dataOffset;
    pPak->fileSize = hdr.size;
    pPak->data.resize(dataSize);
    pPak->strings.reserve(hdr.numAssets);

    // read all the data.
    op = pFile->readAsync(pPak->data.data(), pPak->data.size(), 0);
    if (op.waitUntilFinished() != pPak->data.size()) {
        X_ERROR("FileSys", "Error reading pak data");
        return false;
    }

    core::MemCursor cursor(pPak->data.data() + hdr.stringDataOffset, stringBlockSize);

    pPak->strings.push_back(cursor.getPtr<const char>());

    for (auto* pData = cursor.begin(); pData < cursor.end(); ++pData) {
        if (*pData == '\0') {
            pPak->strings.push_back(pData + 1);
            if (pPak->strings.size() == hdr.numAssets) {
                break;
            }
        }
    }

    if (pPak->strings.size() != hdr.numAssets) {
        X_ERROR("FileSys", "Error loading pak");
        return false;
    }

    auto numassetsPow2 = core::bitUtil::NextPowerOfTwo(hdr.numAssets);

    pPak->pEntires = reinterpret_cast<const AssetPak::APakEntry*>(pPak->data.data() + hdr.entryTableOffset);
    pPak->hash.setGranularity(numassetsPow2);
    pPak->hash.clear(numassetsPow2 * 4, numassetsPow2);

    // we need to build a hash table of the goat.
    for (size_t i = 0; i < pPak->strings.size(); ++i) {
        const char* pString = pPak->strings[i];
        core::StrHash hash(pString, core::strUtil::strlen(pString));

        pPak->hash.add(hash, safe_static_cast<int32_t>(i));
    }

    // all done?
    Search* pSearch = X_NEW(Search, &virtualDirArena_, "FileSysSearch");
    pSearch->pDir = nullptr;
    pSearch->pPak = pPak.release();
    pSearch->pNext = searchPaths_;
    searchPaths_ = pSearch;

    ++numPak_;
    return true;
}

void xFileSys::listPaks(const char* pSearchPatten) const
{
    X_UNUSED(pSearchPatten); // TODO

    size_t numPacks = 0;

    X_LOG0("FileSys", "-------------- ^8Paks(%" PRIuS ")^7 ---------------", numPacks);

    for (Search* pSearch = searchPaths_; pSearch; pSearch = pSearch->pNext) {
        if (!pSearch->pPak) {
            continue;
        }

        auto* pPak = pSearch->pPak;

        core::HumanSize::Str sizeStr;
        X_LOG0("FileSys", "^2%-32s ^7size: ^2%s ^7assets: ^2%" PRIu32 " ^7mode: ^2%s ^7openHandles: ^2%" PRIi32,
            pPak->name.c_str(), core::HumanSize::toString(sizeStr, pPak->fileSize), pPak->numAssets, PakMode::ToString(pPak->mode), int32_t(pPak->openHandles));
    }

    X_LOG0("FileSys", "-------------- ^8Paks End^7 --------------");
}

void xFileSys::listSearchPaths(const char* pSearchPatten) const
{
    X_UNUSED(pSearchPatten); // TODO

    size_t numPacks = 0;

    X_LOG0("FileSys", "-------------- ^8Paths(%" PRIuS ")^7 ---------------", numPacks);

    for (Search* pSearch = searchPaths_; pSearch; pSearch = pSearch->pNext) {
        if (pSearch->pPak) {
            continue;
        }

        const auto& path = pSearch->pDir->path;
        X_LOG0("FileSys", "^2%S", path.c_str());
    }

    X_LOG0("FileSys", "-------------- ^8Paths End^7 --------------");
}

void xFileSys::Cmd_ListPaks(IConsoleCmdArgs* pCmd)
{
    const char* pSearchPattern = nullptr;

    if (pCmd->GetArgCount() > 1) {
        pSearchPattern = pCmd->GetArg(1);
    }

    listPaks(pSearchPattern);
}


void xFileSys::Cmd_ListSearchPaths(IConsoleCmdArgs* pCmd)
{
    const char* pSearchPattern = nullptr;

    if (pCmd->GetArgCount() > 1) {
        pSearchPattern = pCmd->GetArg(1);
    }

    listSearchPaths(pSearchPattern);
}

X_NAMESPACE_END