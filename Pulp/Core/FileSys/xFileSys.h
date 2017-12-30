#pragma once

#include <IFileSys.h>

#include <String\Path.h>
#include <String\StringHash.h>

#include "Vars\FileSysVars.h"

#include <Containers\HashMap.h>
#include <Containers\HashIndex.h>
#include <Containers\PriorityQueue.h>
#include <Containers\FixedFifo.h>

X_DISABLE_WARNING(4702)
#include <unordered_set>
X_ENABLE_WARNING(4702)

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
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
	Search() {
		core::zero_this(this);
	}

	Directory* pDir;
	Pak* pPak;
	Search* pNext;
};


struct XFindData;
struct PendingCompiltionOp;

class xFileSys : public IFileSys, private core::ThreadAbstract
{
	struct PendingOpBase
	{
		PendingOpBase(core::UniquePointer<IoRequestBase>&& req);
		PendingOpBase(PendingOpBase&& oth);

		PendingOpBase& operator=(PendingOpBase&& oth);

		IoRequest::Enum getType(void) const;

		template<typename T>
		X_INLINE T* as(void) const {
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
		core::Array<DelayedPendingCompiltionOp>, pendingop_less> PendingComplitionOpPriorityQueue;

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
	> FilePoolArena;

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
	> AsyncOpPoolArena;


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
	> MemfileArena;

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

	friend struct XFindData;

	static const size_t MAX_REQ_SIZE = core::Max(sizeof(IoRequestOpen), 
		core::Max(sizeof(IoRequestOpenRead),
		core::Max(sizeof(IoRequestClose),
		core::Max(sizeof(IoRequestRead), 
		sizeof(IoRequestWrite))))
	);

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

	bool init(const SCoreInitParams& params) X_FINAL;
	bool initWorker(void) X_FINAL;
	void shutDown(void) X_FINAL;


	bool InitDirectorys(bool workingDir);

	core::Path<wchar_t> getWorkingDirectory(void) const X_FINAL;

	// Open / Close
	XFile* openFile(pathType path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_FINAL;
	XFile* openFile(pathTypeW path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_FINAL;
	void closeFile(XFile* file) X_FINAL;

	// async
	XFileAsync* openFileAsync(pathType path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_FINAL;
	XFileAsync* openFileAsync(pathTypeW path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_FINAL;
	void closeFileAsync(XFileAsync* file) X_FINAL;

	// Mem
	XFileMem* openFileMem(pathType path, fileModeFlags mode) X_FINAL;
	XFileMem* openFileMem(pathTypeW path, fileModeFlags mode) X_FINAL;
	void closeFileMem(XFileMem* file) X_FINAL;

	// folders
	bool setGameDir(pathTypeW path) X_FINAL;
	void addModDir(pathTypeW path) X_FINAL;

	// Find util
	uintptr_t findFirst(pathType path, findData* findinfo) X_FINAL;
	bool findnext(uintptr_t handle, findData* findinfo) X_FINAL;
	void findClose(uintptr_t handle) X_FINAL;

	uintptr_t findFirst2(pathType path, findData& findinfo) X_FINAL;
	bool findnext2(uintptr_t handle, findData& findinfo) X_FINAL;
	void findClose2(uintptr_t handle) X_FINAL;

	// Delete
	bool deleteFile(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	bool deleteDirectory(pathType path, bool recursive = true) const X_FINAL;
	bool deleteDirectoryContents(pathType path) X_FINAL;

	// Create
	bool createDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	bool createDirectory(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	bool createDirectoryTree(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	bool createDirectoryTree(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;

	// exsists.
	bool fileExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	bool fileExists(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	bool directoryExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	bool directoryExists(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;


	// does not error, when it's a file or not exsist.
	bool isDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	bool isDirectory(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;

	// rename
	bool moveFile(pathType path, pathType newPath) const X_FINAL;
	bool moveFile(pathTypeW path, pathTypeW newPath) const X_FINAL;


	size_t getMinimumSectorSize(void) const X_FINAL;

	// settings baby
	X_INLINE const XFileSysVars& getVars(void) const { return vars_; }

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
	void CancelRequest(RequestHandle handle);
	bool StartRequestWorker(void);
	void ShutDownRequestWorker(void);

	void listPaks(const char* pSearchPatten = nullptr) const;

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

	OsFileAsync* openOsFileAsync(pathType path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME);


	bool openPak(const char* pName);


private:

	bool fileExistsOS(const core::Path<wchar_t>& fullPath) const;
	bool directoryExistsOS(const core::Path<wchar_t>& fullPath) const;
	bool isDirectoryOS(const core::Path<wchar_t>& fullPath) const;
	bool moveFileOS(const core::Path<wchar_t>& fullPath, const core::Path<wchar_t>& fullPathNew) const;

	bool fileExistsOS(const wchar_t* pFullPath) const;
	bool directoryExistsOS(const wchar_t* pFullPath) const;
	bool isDirectoryOS(const wchar_t* pFullPath) const;
	bool moveFileOS(const wchar_t* pFullPath, const wchar_t* pFullPathNew) const;


	// Ajust path
	const wchar_t* createOSPath(const Directory* dir, pathType path, VirtualDirectory::Enum location, Path<wchar_t>& buffer) const;
	const wchar_t* createOSPath(const Directory* dir, pathTypeW path, VirtualDirectory::Enum location, Path<wchar_t>& buffer) const;
	
	const wchar_t* createOSPath(const Directory* dir, pathType path, Path<wchar_t>& buffer) const;
	const wchar_t* createOSPath(const Directory* dir, pathTypeW path, Path<wchar_t>& buffer) const;

	bool isAbsolute(pathType path) const;
	bool isAbsolute(pathTypeW path) const;
	
	bool isDebug(void) const;

	void updatePendingOpsStats(void);

private:
	void Cmd_ListPaks(IConsoleCmdArgs* pCmd);

private:
#if X_DEBUG == 1
	typedef std::unordered_set<XFindData*> findDataSet;
	findDataSet findData_;
#endif // !X_DEBUG

	Directory* gameDir_;
	Search* searchPaths_;

	XFileSysVars vars_;

	core::HeapArea      filePoolHeap_;
	core::PoolAllocator filePoolAllocator_;
	FilePoolArena		filePoolArena_;

	// pool for async ops.
	core::HeapArea      asyncOpPoolHeap_;
	core::PoolAllocator asyncOpPoolAllocator_;
	AsyncOpPoolArena	asyncOpPoolArena_;

	core::MallocFreeAllocator memfileAllocator_;
	MemfileArena		memFileArena_;

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