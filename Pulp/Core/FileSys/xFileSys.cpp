#include "stdafx.h"
#include "xFileSys.h"
#include "DiskFile.h"
#include "DiskFileAsync.h"
#include "PakFileAsync.h"

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
            core::Max(sizeof(XFileMem), sizeof(XPakFileAsync))));

    const size_t FILE_ALLOCATION_ALIGN = core::Max(X_ALIGN_OF(XDiskFile),
        core::Max(X_ALIGN_OF(XDiskFileAsync),
            core::Max(X_ALIGN_OF(XFileMem), X_ALIGN_OF(XPakFileAsync))));

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
    gameDir_(nullptr),
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
    memFileArena_(&memfileAllocator_, "MemFileData"),
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

    for (Search* s = searchPaths_; s;) {
        Search* cur = s;
        s = cur->pNext;
        if (cur->pDir) {
            X_DELETE(cur->pDir, g_coreArena);
        }
        else {
            if (cur->pPak->pFile) {
                X_DELETE_AND_NULL(cur->pPak->pFile, &filePoolArena_);
            }

            X_DELETE(cur->pPak, g_coreArena);
        }
        X_DELETE(cur, g_coreArena);
    }
}

bool xFileSys::initDirectorys(bool working)
{
    loadPacks_ = !working; // TODO: work out somethign better? basically only want packs in game and maybe some tools?

    core::Path<wchar_t> workingDir;
    if (!PathUtil::GetCurrentDirectory(workingDir)) {
        return false;
    }

    if (working) {
        // working dir added.
        return setGameDir(workingDir);
    }
    else {
        
        const wchar_t* pGameDir = gEnv->pCore->GetCommandLineArgForVarW(L"fs_basepath");
        if (pGameDir) {
            workingDir = pGameDir;
        }

        core::Path<wchar_t> base(workingDir);
        base.ensureSlash();

        core::Path<wchar_t> core(base);
        core /= L"core_assets\\";

        core::Path<wchar_t> testAssets(base);
        testAssets /= L"test_assets\\";

        if (setGameDir(core)) {
            addModDir(testAssets);
            return true;
        }
    }

    return false;
}

bool xFileSys::getWorkingDirectory(core::Path<wchar_t>& pathOut) const
{
    return PathUtil::GetCurrentDirectory(pathOut);
}

// --------------------- Open / Close ---------------------

XFile* xFileSys::openFile(pathType path, FileFlags mode)
{
    core::Path<wchar_t> real_path;
    
    if (mode.IsSet(FileFlag::READ) && !mode.IsSet(FileFlag::WRITE) && !isAbsolute(path)) {
        FindData findinfo;
        XFindData findData(path, this);

        if (!findData.findnext(findinfo)) {
            FileFlags::Description Dsc;
            X_WARNING("FileSys", "Failed to find file: %s, Flags: %s", path, mode.ToString(Dsc));
            return nullptr;
        }

        findData.getOSPath(real_path, findinfo);
    }
    else {
        createOSPath(gameDir_, path, real_path);
    }

    if (isDebug()) {
        X_LOG0("FileSys", "openFile: \"%ls\"", real_path.c_str());
    }

    XDiskFile* pFile = X_NEW(XDiskFile, &filePoolArena_, "Diskfile")(real_path, mode);
    if (pFile->valid()) {
        return pFile;
    }

    closeFile(pFile);
    return nullptr;
}

XFile* xFileSys::openFile(const PathWT& path, FileFlags mode)
{
    // This is only for opening a a absolute path really.
    core::Path<wchar_t> osPath;

    createOSPath(gameDir_, path, osPath);

    if (isDebug()) {
        X_LOG0("FileSys", "openFile: \"%ls\"", osPath.c_str());
    }

    XDiskFile* pFile = X_NEW(XDiskFile, &filePoolArena_, "Diskfile")(osPath, mode);
    if (pFile->valid()) {
        return pFile;
    }

    closeFile(pFile);
    return nullptr;
}

void xFileSys::closeFile(XFile* file)
{
    X_ASSERT_NOT_NULL(file);
    X_DELETE(file, &filePoolArena_);
}

// --------------------------------------------------

// async
XFileAsync* xFileSys::openFileAsync(pathType path, FileFlags mode)
{
    core::Path<wchar_t> fullPath;

    // so this needs to handle opening both disk files from the virtual dir's
    // or opening files from a pak.
    // the pak file don't need any disk paths.
    // you can only open a pak file if no write access tho.
    // we have search priority also.
    // so specific dir's / pak's are checked first.
    // if we have lots of packs and they all have same priority, i want a quicker lookup.
    // like a pak group that has a hash of all files and pak index.
    // but we do want override pak's like ones from mod dir so the file is taken from that first.
    // i think the linked list layout is still appropriate, just might add a pak group node.
    XDiskFileAsync* pFile = nullptr;

    if (mode.IsSet(FileFlag::READ) && !mode.IsSet(FileFlag::WRITE)) {
        // so we are going to look in the search list, till we find a file.
        // findData can't deal with files in pak's correcly, it has the wrong api.
        core::StrHash hash(path, strUtil::strlen(path));

        for (const Search* pSearch = searchPaths_; pSearch; pSearch = pSearch->pNext) {
            if (pSearch->pDir) {
                const auto* pDir = pSearch->pDir;

                createOSPath(pDir, path, fullPath);

                if (PathUtil::DoesFileExist(fullPath, true)) {
                    if (isDebug()) {
                        X_LOG0("FileSys", "openFileAsync: \"%ls\"", fullPath.c_str());
                    }

                    pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(fullPath, mode, &asyncOpPoolArena_);
                    break;
                }
            }
            else if (pSearch->pPak) {
                auto* pPak = pSearch->pPak;

                auto idx = pPak->find(hash, path);
                if (idx != -1) {
                    if (isDebug()) {
                        X_LOG0("FileSys", "openFileAsync: \"%s\" fnd in pak: \"%s\"", path, pPak->name.c_str());
                    }

                    auto& entry = pPak->pEntires[idx];
                    return X_NEW(XPakFileAsync, &filePoolArena_, "PakFileAsync")(pPak, entry, &asyncOpPoolArena_);
                }
            }
            else {
                X_ASSERT_UNREACHABLE();
            }
        }

        if (!pFile) {
            return nullptr;
        }
    }
    else {
        createOSPath(gameDir_, path, fullPath);

        if (isDebug()) {
            X_LOG0("FileSys", "openFileAsync: \"%ls\"", fullPath.c_str());
        }

        pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(fullPath, mode, &asyncOpPoolArena_);
    }

    if (pFile->valid()) {
        return pFile;
    }

    closeFileAsync(pFile);
    return nullptr;
}

XFileAsync* xFileSys::openFileAsync(pathTypeW path, FileFlags mode)
{
    core::Path<wchar_t> real_path;

    if (mode.IsSet(FileFlag::READ) && !mode.IsSet(FileFlag::WRITE)) {
        FindData findinfo;
        XFindData findData(path, this);

        if (!findData.findnext(findinfo)) {
            FileFlags::Description Dsc;
            X_WARNING("FileSys", "Failed to find file: %ls, Flags: %s", path, mode.ToString(Dsc));
            return nullptr;
        }

        findData.getOSPath(real_path, findinfo);
    }
    else {
        createOSPath(gameDir_, path, real_path);
    }

    if (isDebug()) {
        X_LOG0("FileSys", "openFileAsync: \"%ls\"", real_path.c_str());
    }

    XDiskFileAsync* pFile = X_NEW(XDiskFileAsync, &filePoolArena_, "DiskFileAsync")(real_path, mode, &asyncOpPoolArena_);
    if (pFile->valid()) {
        return pFile;
    }

    closeFileAsync(pFile);
    return nullptr;
}

void xFileSys::closeFileAsync(XFileAsync* file)
{
    X_ASSERT_NOT_NULL(file);
    X_DELETE(file, &filePoolArena_);
}

// --------------------------------------------------

XFileMem* xFileSys::openFileMem(pathType path, FileFlags mode)
{
    if (mode.IsSet(FileFlag::WRITE)) {
        X_ERROR("FileSys", "can't open a memory file for writing.");
        return nullptr;
    }

    FindData findinfo;
    XFindData findData(path, this);
    if (!findData.findnext(findinfo)) {
        FileFlags::Description Dsc;
        X_WARNING("FileSys", "Failed to find file: %s, Flags: %s", path, mode.ToString(Dsc));
        return nullptr;
    }

    core::Path<wchar_t> real_path;
    findData.getOSPath(real_path, findinfo);

    if (isDebug()) {
        X_LOG0("FileSys", "openFileMem: \"%ls\"", real_path.c_str());
    }

    OsFile file(real_path, mode);
    if (!file.valid()) {
        return nullptr;
    }

    size_t size = safe_static_cast<size_t, int64_t>(file.remainingBytes());
    char* pBuf = X_NEW_ARRAY(char, size, &memFileArena_, "MemBuffer");

    if (file.read(pBuf, size) != size) {
        X_DELETE_ARRAY(pBuf, &memFileArena_);
        return nullptr;
    }

    XFileMem* pFile = X_NEW(XFileMem, &filePoolArena_, "MemFile")(pBuf, pBuf + size, &memFileArena_);

    return pFile;
}

XFileMem* xFileSys::openFileMem(pathTypeW path, FileFlags mode)
{
    if (mode.IsSet(FileFlag::WRITE)) {
        X_ERROR("FileSys", "can't open a memory file for writing.");
        return nullptr;
    }

    FindData findinfo;
    XFindData findData(path, this);
    if (!findData.findnext(findinfo)) {
        FileFlags::Description Dsc;
        X_WARNING("FileSys", "Failed to find file: %s, Flags: %s", path, mode.ToString(Dsc));
        return nullptr;
    }

    core::Path<wchar_t> real_path;
    findData.getOSPath(real_path, findinfo);

    if (isDebug()) {
        X_LOG0("FileSys", "openFileMem: \"%ls\"", real_path.c_str());
    }

    OsFile file(real_path, mode);
    if (!file.valid()) {
        return nullptr;
    }

    size_t size = safe_static_cast<size_t, int64_t>(file.remainingBytes());
    char* pBuf = X_NEW_ARRAY(char, size, &memFileArena_, "MemBuffer");

    if (file.read(pBuf, size) != size) {
        X_DELETE_ARRAY(pBuf, &memFileArena_);
        return nullptr;
    }

    XFileMem* pFile = X_NEW(XFileMem, &filePoolArena_, "MemFile")(pBuf, pBuf + size, &memFileArena_);

    return pFile;
}

void xFileSys::closeFileMem(XFileMem* file)
{
    X_ASSERT_NOT_NULL(file);
    // class free's the buffer.
    X_DELETE(file, &filePoolArena_);
}

// --------------------- folders ---------------------

bool xFileSys::setGameDir(const PathWT& path)
{
    X_ASSERT(gameDir_ == nullptr, "can only set one game directoy")(path.c_str(), gameDir_);

    // check if the irectory is even valid.
    if (!directoryExistsOS(path)) {
        core::Path<wchar_t> fullPath;
        PathUtil::GetFullPath(path, fullPath);
        X_ERROR("FileSys", "Faled to set game directory, path does not exsists: \"%ls\"", fullPath.c_str());
        return false;
    }

    if (!addDirInteral(path, true)) {
        return false;
    }

    X_ASSERT_NOT_NULL(gameDir_);
    return true;
}

bool xFileSys::addModDir(const PathWT& path)
{
    return addDirInteral(path, false);
}


bool xFileSys::addDirInteral(const PathWT& path, bool isGame)
{
    if (isDebug()) {
        X_LOG0("FileSys", "addModDir: \"%ls\"", path);
    }

    if (!directoryExistsOS(path)) {
        X_ERROR("FileSys", "Faled to add mod drectory, the directory does not exsists: \"%ls\"", path.c_str());
        return false;
    }

    // ok remove any ..//
    core::Path<wchar_t> fixedPath;
    if (!PathUtil::GetFullPath(path, fixedPath)) {
        X_ERROR("FileSys", "addModDir full path name creation failed");
        return false;
    }

    if (!directoryExistsOS(fixedPath)) {
        X_ERROR("FileSys", "Fixed path does not exsists: \"%ls\"", fixedPath.c_str());
        return false;
    }

    // work out if this directory is a sub directory of any of the current
    // searxh paths.
    for (Search* s = searchPaths_; s; s = s->pNext) {
        if (!s->pDir) {
            continue;
        }

        if (strUtil::FindCaseInsensitive(fixedPath.c_str(), s->pDir->path.c_str()) != nullptr) {
            X_ERROR("FileSys", "mod dir is identical or inside a current mod dir: \"%ls\" -> \"%ls\"",
                fixedPath.c_str(), s->pDir->path.c_str());
            return false;
        }
    }

    // add it to virtual file system.
    Search* search = X_NEW(Search, g_coreArena, "FileSysSearch");
    search->pDir = X_NEW(Directory, g_coreArena, "FileSysDir");
    search->pDir->path = fixedPath;
    search->pDir->path.ensureSlash();
    search->pPak = nullptr;
    search->pNext = searchPaths_;
    searchPaths_ = search;

    if (isGame) {
        gameDir_ = searchPaths_->pDir;
    }

    // add hotreload dir.
    gEnv->pDirWatcher->addDirectory(fixedPath);

    if (!loadPacks_) {
        return true;
    }

    // Load packs.
    auto searchPath = fixedPath;
    searchPath.appendFmt(L"*.%S", AssetPak::PAK_FILE_EXTENSION);

    FindData findInfo;
    uintptr_t handle = PathUtil::findFirst(searchPath.c_str(), findInfo);

    if (handle != PathUtil::INVALID_FIND_HANDLE)
    {
        do
        {
            core::Path<char> name(findInfo.name);

            if (!openPak(name.c_str()))
            {
                X_ERROR("FileSys", "Failed to add pak: \"%s\"", name.c_str());
            }
        }
        while(PathUtil::findNext(handle, findInfo));

        PathUtil::findClose(handle);
    }

    return true;
}

// --------------------- Find util ---------------------

uintptr_t xFileSys::findFirst(pathType path, FindData& findinfo)
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

    // TODO: just have one const?
    static_assert(INVALID_HANDLE == PathUtil::INVALID_FIND_HANDLE, "Invalid handles don't match");

    return handle;
}

uintptr_t xFileSys::findFirst(pathTypeW path, FindData& findinfo)
{
    Path<wchar_t> buf;
    createOSPath(gameDir_, path, buf);

    uintptr_t handle = PathUtil::findFirst(buf.c_str(), findinfo);

    static_assert(INVALID_HANDLE == PathUtil::INVALID_FIND_HANDLE, "Invalid handles don't match");

    return handle;
}

bool xFileSys::findnext(uintptr_t handle, FindData& findinfo)
{
    X_ASSERT(handle != PathUtil::INVALID_FIND_HANDLE, "FindNext called with invalid handle")(handle);

    return PathUtil::findNext(handle, findinfo);
}

void xFileSys::findClose(uintptr_t handle)
{
    if (handle != PathUtil::INVALID_FIND_HANDLE) {
        PathUtil::findClose(handle);
    }
}

// --------------------- Delete ---------------------

bool xFileSys::deleteFile(pathType path) const
{
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

    FindData fd;
    uintptr_t handle = PathUtil::findFirst(searchPath.c_str(), fd);
    if (handle != PathUtil::INVALID_FIND_HANDLE) {
        do {

            // build a OS Path.
            core::Path<wchar_t> dirItem(buf);
            dirItem.ensureSlash();
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

        } while (PathUtil::findNext(handle, fd));

        PathUtil::findClose(handle);
    }

    return true;
}

// --------------------- Create ---------------------

bool xFileSys::createDirectory(pathType path) const
{
    X_ASSERT_NOT_NULL(path);

    Path<wchar_t> buf;
    createOSPath(gameDir_, path, buf);

    buf.removeFileName();

    if (isDebug()) {
        X_LOG0("FileSys", "createDirectory: \"%ls\"", buf.c_str());
    }

    return PathUtil::CreateDirectory(buf);
}

bool xFileSys::createDirectory(pathTypeW path) const
{
    X_ASSERT_NOT_NULL(path);

    Path<wchar_t> buf;
    createOSPath(gameDir_, path, buf);

    buf.removeFileName();

    if (isDebug()) {
        X_LOG0("FileSys", "createDirectory: \"%ls\"", buf.c_str());
    }

    return PathUtil::CreateDirectory(buf);
}

bool xFileSys::createDirectoryTree(pathType _path) const
{
    X_ASSERT_NOT_NULL(_path);

    // we want to just loop and create like a goat.
    Path<wchar_t> buf;
    createOSPath(gameDir_, _path, buf);

    buf.removeFileName();

    if (isDebug()) {
        X_LOG0("FileSys", "CreateDirectoryTree: \"%ls\"", buf.c_str());
    }

    return PathUtil::CreateDirectoryTree(buf);
}

bool xFileSys::createDirectoryTree(pathTypeW _path) const
{
    X_ASSERT_NOT_NULL(_path);

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

bool xFileSys::fileExists(pathType path) const
{
    Path<wchar_t> buf;
    createOSPath(gameDir_, path, buf);

    return fileExistsOS(buf);
}

bool xFileSys::fileExists(pathTypeW path) const
{
    Path<wchar_t> buf;
    createOSPath(gameDir_, path, buf);

    return fileExistsOS(buf);
}

bool xFileSys::directoryExists(pathType path) const
{
    Path<wchar_t> buf;
    createOSPath(gameDir_, path, buf);

    return directoryExistsOS(buf);
}

bool xFileSys::directoryExists(pathTypeW path) const
{
    Path<wchar_t> buf;
    createOSPath(gameDir_, path, buf);

    return directoryExistsOS(buf);
}

bool xFileSys::isDirectory(pathType path) const
{
    Path<wchar_t> buf;
    createOSPath(gameDir_, path, buf);

    return isDirectoryOS(buf);
}

bool xFileSys::isDirectory(pathTypeW path) const
{
    Path<wchar_t> buf;
    createOSPath(gameDir_, path, buf);

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
const wchar_t* xFileSys::createOSPath(const Directory* dir, pathType path, PathWT& buffer) const
{
    wchar_t pathW[core::Path<wchar_t>::BUF_SIZE];
    strUtil::Convert(path, pathW, sizeof(pathW));

    return createOSPath(dir, pathW, buffer);
}

const wchar_t* xFileSys::createOSPath(const Directory* dir, pathTypeW path, PathWT& buffer) const
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

const wchar_t* xFileSys::createOSPath(const Directory* dir, const PathWT& path, PathWT& buffer) const
{
    // is it absolute?
    if (!isAbsolute(path)) {
        buffer = dir->path / path;
    }
    else {
        buffer = path;
    }

    buffer.replaceSeprators();
    return buffer.c_str();
}

bool xFileSys::isAbsolute(pathType path) const
{
    return path[0] == NATIVE_SLASH || path[0] == NON_NATIVE_SLASH || path[1] == ':';
}

bool xFileSys::isAbsolute(pathTypeW path) const
{
    return path[0] == NATIVE_SLASH_W || path[0] == NON_NATIVE_SLASH_W || path[1] == L':';
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
            XFileAsync* pFile = openFileAsync(pOpen->path.c_str(), pOpen->mode);

            pOpen->callback.Invoke(fileSys, pOpen, pFile, 0);
        }
        else if (type == IoRequest::OPEN_READ_ALL) {
            IoRequestOpenRead* pOpenRead = static_cast<IoRequestOpenRead*>(pRequest);
            XFileAsync* pFileAsync = openFileAsync(pOpenRead->path.c_str(), pOpenRead->mode);

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
            XDiskFileAsync* pFile = static_cast<XDiskFileAsync*>(openFileAsync(pOpenWrite->path.c_str(), core::FileFlags::RECREATE | core::FileFlags::WRITE));

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

OsFileAsync* xFileSys::openOsFileAsync(pathType path, FileFlags mode)
{
    core::Path<wchar_t> real_path;

    if (mode.IsSet(FileFlag::READ) && !mode.IsSet(FileFlag::WRITE)) {
        FindData findinfo;
        XFindData findData(path, this);

        if (!findData.findnext(findinfo)) {
            FileFlags::Description Dsc;
            X_WARNING("FileSys", "Failed to find file: %ls, Flags: %s", path, mode.ToString(Dsc));
            return nullptr;
        }

        findData.getOSPath(real_path, findinfo);
    }
    else {
        createOSPath(gameDir_, path, real_path);
    }

    if (isDebug()) {
        X_LOG0("FileSys", "openFileAsync: \"%ls\"", real_path.c_str());
    }

    OsFileAsync* pFile = X_NEW(OsFileAsync, &filePoolArena_, "DiskFileAsync")(real_path, mode, &asyncOpPoolArena_);
    if (pFile->valid()) {
        return pFile;
    }

    X_DELETE(pFile, &filePoolArena_);
    return nullptr;
}

bool xFileSys::openPak(const char* pName)
{
    X_LOG1("FileSys", "Mounting pak: \"%s\"", pName);

    // you can only open pak's from inside the virtual filesystem.
    // so file is opened as normal.
    FileFlags mode;
    mode.Set(FileFlag::READ);
    mode.Set(FileFlag::RANDOM_ACCESS);
    // I'm not sharing, fuck you!

    auto* pFile = openOsFileAsync(pName, mode);
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
    pPak->name.set(pName);
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
    Search* pSearch = X_NEW(Search, g_coreArena, "FileSysSearch");
    pSearch->pDir = nullptr;
    pSearch->pPak = pPak.release();
    pSearch->pNext = searchPaths_;
    searchPaths_ = pSearch;

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
            pPak->name.c_str(), core::HumanSize::toString(sizeStr, pPak->fileSize), pPak->numAssets, PakMode::ToString(pPak->mode), pPak->openHandles);
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