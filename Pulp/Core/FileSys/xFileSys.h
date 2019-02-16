#pragma once

#include <IFileSys.h>

#include <String\Path.h>
#include <String\StringHash.h>

#include "Vars\FileSysVars.h"

#include <Containers\HashMap.h>
#include <Containers\HashIndex.h>
#include <Containers\PriorityQueue.h>
#include <Containers\FixedFifo.h>

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\AllocationPolicies\LinearAllocator.h>
#include <Memory\HeapArea.h>
#include <Util\UniquePointer.h>

#include <Threading\ThreadQue.h>
#include <Threading\Thread.h>

#include <IFileSysStats.h>
#include <IAssetPak.h>

X_NAMESPACE_BEGIN(core)

class OsFileAsync;

struct IConsoleCmdArgs;

// we want multiple search path's so we can add folders into the search.
// we will need the folder name.
// a search can also be a PAK since that is like a directory of files.

struct Pak;

struct Directory
{
    Path<wchar_t> path;
};

struct Search
{
    Search()
    {
        core::zero_this(this);
    }

    Directory* pDir;
    Pak* pPak;
    Search* pNext;
};

struct XFindData;
struct PendingCompiltionOp;

class xFileSys : public IFileSys
    , private core::ThreadAbstract
{
    struct PendingOpBase
    {
        PendingOpBase(core::UniquePointer<IoRequestBase>&& req);
        PendingOpBase(PendingOpBase&& oth);

        PendingOpBase& operator=(PendingOpBase&& oth);

        IoRequest::Enum getType(void) const;

        template<typename T>
        X_INLINE T* as(void) const
        {
            return reinterpret_cast<T*>(pRequest.get());
        }

    private:
        core::UniquePointer<IoRequestBase> pRequest;
    };

    struct PendingCompiltionOp : public PendingOpBase
    {
        PendingCompiltionOp(core::UniquePointer<IoRequestBase>&& req, XFileAsyncOperationCompiltion&& op);
        PendingCompiltionOp(PendingCompiltionOp&& oth);

        PendingCompiltionOp& operator=(PendingCompiltionOp&& oth);
        XFileAsyncOperationCompiltion op;
    };

    struct PendingOp : public PendingOpBase
    {
        PendingOp(core::UniquePointer<IoRequestBase>&& req, XFileAsyncOperation&& op);
        PendingOp(PendingOp&& oth);

        PendingOp& operator=(PendingOp&& oth);

        XFileAsyncOperation op;
    };

    struct iorequest_less
    {
        bool operator()(const IoRequestBase* lhs, const IoRequestBase* rhs) const
        {
#if 0 // sort reads by offset?
			if (lhs->getType() == IoRequest::READ && lhs->getType() == IoRequest::READ) {
				auto* pReadLhs = static_cast<const IoRequestRead*>(lhs);
				auto* pReadRhs = static_cast<const IoRequestRead*>(rhs);
				return pReadLhs->offset < pReadRhs->offset;
			}
#endif
            return (lhs->getType() < rhs->getType());
        }
    };

#if X_ENABLE_FILE_ARTIFICAIL_DELAY

    struct DelayedPendingCompiltionOp
    {
        DelayedPendingCompiltionOp(PendingCompiltionOp&& op, core::TimeVal time) :
            op(std::move(op)),
            time(time)
        {
        }

        DelayedPendingCompiltionOp(DelayedPendingCompiltionOp&& oth) = default;

        DelayedPendingCompiltionOp& operator=(DelayedPendingCompiltionOp&& oth) = default;

        mutable PendingCompiltionOp op;
        core::TimeVal time;
    };

    struct pendingop_less
    {
        bool operator()(const DelayedPendingCompiltionOp& lhs, const DelayedPendingCompiltionOp& rhs) const
        {
            return (lhs.time < rhs.time);
        }
    };

    typedef core::PriorityQueue<DelayedPendingCompiltionOp,
        core::Array<DelayedPendingCompiltionOp>, pendingop_less>
        PendingComplitionOpPriorityQueue;

#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

    typedef core::MemoryArena<
        core::PoolAllocator,
        core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
        >
        FilePoolArena;

    typedef core::MemoryArena<
        core::PoolAllocator,
        core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
        >
        AsyncOpPoolArena;

    typedef core::MemoryArena<
        core::MallocFreeAllocator,
        core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
    >
        MemfileArena;

    typedef core::MemoryArena<
        core::LinearAllocator,
        core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
        >
        VirtualDirArena;

public:
#ifdef X_PLATFORM_WIN32
    static const char NATIVE_SLASH = '\\';
    static const char NON_NATIVE_SLASH = '/';
    static const wchar_t NATIVE_SLASH_W = L'\\';
    static const wchar_t NON_NATIVE_SLASH_W = L'/';
#else
    static const char NATIVE_SLASH = '/';
    static const char NON_NATIVE_SLASH = '\\';
    static const wchar_t NATIVE_SLASH_W = L'/';
    static const wchar_t NON_NATIVE_SLASH_W = L'\\';
#endif

    static const size_t MAX_VIRTUAL_DIR = FS_MAX_VIRTUAL_DIR;
    static const size_t MAX_PAK = FS_MAX_PAK;

    friend struct XFindData;

    static const size_t MAX_REQ_SIZE = core::Max(sizeof(IoRequestOpen),
        core::Max(sizeof(IoRequestOpenRead),
            core::Max(sizeof(IoRequestClose),
                core::Max(sizeof(IoRequestRead),
                    sizeof(IoRequestWrite)))));

    static const size_t PENDING_IO_QUE_SIZE = 0x100;

    typedef std::array<uint8_t, MAX_REQ_SIZE> RequestBuffer;
    typedef core::FixedArray<PendingCompiltionOp, PENDING_IO_QUE_SIZE> AsyncComplitionOps;
    typedef core::FixedArray<PendingOp, PENDING_IO_QUE_SIZE> AsyncOps;

    typedef core::PriorityQueue<IoRequestBase*, core::Array<IoRequestBase*>, iorequest_less> IoRequestPriorityQueue;

public:
    xFileSys(core::MemoryArenaBase* arena);
    ~xFileSys() X_FINAL;

    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    bool init(const CoreInitParams& params) X_FINAL;
    bool initWorker(void) X_FINAL;
    void shutDown(void) X_FINAL;

    bool initDirectorys(bool workingDir);

    bool getWorkingDirectory(PathT& pathOut) const X_FINAL;
    bool getWorkingDirectory(PathWT& pathOut) const X_FINAL;

    // Open / Close
    XFile* openFileOS(const PathWT& osPath, FileFlags mode) X_FINAL;
    XFile* openFile(const PathT& relPath, FileFlags mode, VirtualDirectory::Enum writeDir) X_FINAL;
    void closeFile(XFile* file) X_FINAL;

    // async
    XFileAsync* openFileAsync(const PathT& relPath, FileFlags mode, VirtualDirectory::Enum writeDir) X_FINAL;
    void closeFileAsync(XFileAsync* file) X_FINAL;

    // Mem
    XFileMem* openFileMem(const PathT& relPath, FileFlags mode, VirtualDirectory::Enum writeDir) X_FINAL;
    void closeFileMem(XFileMem* file) X_FINAL;

    // folders
    bool setBaseDir(const PathWT& osPath) X_FINAL;
    bool setSaveDir(const PathWT& osPath) X_FINAL;
    bool addModDir(const PathWT& osPath) X_FINAL;

    // Find util
    FindPair findFirst(const PathT& relPath, FindData& findinfo) X_FINAL;
    FindPair findFirstOS(const PathWT& osPath, FindData& findinfo) X_FINAL;
    bool findnext(findhandle handle, FindData& findinfo) X_FINAL;
    void findClose(findhandle handle) X_FINAL;

    // Delete
    bool deleteFile(const PathT& relPath, VirtualDirectory::Enum dir) const X_FINAL;
    bool deleteDirectory(const PathT& relPath, VirtualDirectory::Enum dir, bool recursive) const X_FINAL;
    bool deleteDirectoryContents(const PathT& relPath, VirtualDirectory::Enum dir) X_FINAL;

    // Create
    bool createDirectory(const PathT& relPath, VirtualDirectory::Enum dir) const X_FINAL;
    bool createDirectoryOS(const PathWT& osPath) const X_FINAL;
    bool createDirectoryTree(const PathT& relPath, VirtualDirectory::Enum dir) const X_FINAL;
    bool createDirectoryTreeOS(const PathWT& osPath) const X_FINAL;

    // exsists.
    bool fileExists(const PathT& relPath) const X_FINAL;
    bool fileExists(const PathT& relPath, VirtualDirectory::Enum dir) const X_FINAL;
    bool fileExistsOS(const PathWT& osPath) const X_FINAL;
    bool directoryExists(const PathT& relPath) const X_FINAL;
    bool directoryExists(const PathT& relPath, VirtualDirectory::Enum dir) const X_FINAL;
    bool directoryExistsOS(const PathWT& osPath) const X_FINAL;

    // does not error, when it's a file or not exsist.
    bool isDirectory(const PathT& relPath, VirtualDirectory::Enum dir) const X_FINAL;
    bool isDirectoryOS(const PathWT& osPath) const X_FINAL;

    // rename
    bool moveFile(const PathT& relPath, const PathT& newPathRel, VirtualDirectory::Enum dir) const X_FINAL;
    bool moveFileOS(const PathWT& osPath, const PathWT& osPathNew) const X_FINAL;

    size_t getMinimumSectorSize(void) const X_FINAL;

    // settings baby
    X_INLINE const XFileSysVars& getVars(void) const
    {
        return vars_;
    }

#if X_ENABLE_FILE_STATS
    // stats
    XFileStats getStats(void) const;
    XFileStats getStatsAsync(void) const;
    IOQueueStats getIOQueueStats(void) const;
#endif // !X_ENABLE_FILE_STATS

    // IoRequest que.
    RequestHandle AddCloseRequestToQue(core::XFileAsync* pFile) X_FINAL;
    RequestHandle AddIoRequestToQue(IoRequestBase& request) X_FINAL;
    RequestHandle AddIoRequestToQue(IoRequestBase* pRequest);
    void cancelRequest(RequestHandle handle);
    void waitForRequest(RequestHandle handle) X_FINAL;
    bool startRequestWorker(void);
    void shutDownRequestWorker(void);

    void listPaks(core::string_view searchPattern) const;
    void listSearchPaths(core::string_view searchPattern) const;

private:
    template<typename FileT, typename PakFuncT, typename FuncT>
    FileT* findFile(const PathT& relPath, FileFlags mode, VirtualDirectory::Enum writeDir, PakFuncT pakFunc, FuncT func);

    Directory* addDirInteral(const PathWT& path);

private:
    IoRequestBase* popRequest(void);
    IoRequestBase* tryPopRequest(void);
    void onOpFinsihed(PendingOpBase& asyncOp, uint32_t bytesTransferd);

    void AsyncIoCompletetionRoutine(XOsFileAsyncOperation::AsyncOp* pOperation, uint32_t bytesTransferd);

    RequestHandle getNextRequestHandle(void);

    // ~

    // ThreadAbstract
    virtual Thread::ReturnValue ThreadRun(const Thread& thread) X_FINAL;
    // ~ThreadAbstract

private:
    OsFileAsync* openPakFile(const PathT& relPath);

    bool openPak(const PathT& relPath);

private:

    // Ajust path
    const wchar_t* createOSPath(const VirtualDirectory::Enum dir, const PathT& path, PathWT& buffer) const;
    const wchar_t* createOSPath(const Directory* dir, const PathT& path, PathWT& buffer) const;
    const wchar_t* createOSPath(const Directory* dir, const PathWT& path, PathWT& buffer) const;

    bool isAbsolute(const PathT& path) const;
    bool isAbsolute(const PathWT& path) const;

    bool isDebug(void) const;

    void updatePendingOpsStats(void);

private:
    void Cmd_ListPaks(IConsoleCmdArgs* pCmd);
    void Cmd_ListSearchPaths(IConsoleCmdArgs* pCmd);

private:
    PathWT basePath_;

    Directory* baseDir_;
    Directory* saveDir_;
    Search* searchPaths_;
    bool loadPacks_;

    XFileSysVars vars_;

    core::MemoryArenaBase* arena_;

    core::HeapArea filePoolHeap_;
    core::PoolAllocator filePoolAllocator_;
    FilePoolArena filePoolArena_;

    // pool for async ops.
    core::HeapArea asyncOpPoolHeap_;
    core::PoolAllocator asyncOpPoolAllocator_;
    AsyncOpPoolArena asyncOpPoolArena_;

    core::HeapArea virtualDirHeap_;
    core::LinearAllocator virtualDirAllocator_;
    VirtualDirArena virtualDirArena_;

    core::MallocFreeAllocator memfileAllocator_;
    MemfileArena memFileArena_;

    int32_t numDir_;
    int32_t numPak_;

private:
    RequestHandle currentRequestIdx_;

    core::CriticalSection requestLock_;
    core::Signal requestSignal_;

    IoRequestPriorityQueue requests_;
    core::MemoryArenaBase* ioQueueDataArena_;

    AsyncComplitionOps pendingCompOps_;
    AsyncOps pendingOps_;

#if X_ENABLE_FILE_ARTIFICAIL_DELAY
    PendingComplitionOpPriorityQueue delayedOps_;
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

#if X_ENABLE_FILE_STATS
    IOQueueStats stats_;
#endif // !X_ENABLE_FILE_STATS
};

X_NAMESPACE_END