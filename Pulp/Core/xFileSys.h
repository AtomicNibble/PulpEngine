#pragma once

#include <IFileSys.h>

#include "String\Path.h"
#include "FileSysVars.h"

#include <Containers\HashMap.h>

X_DISABLE_WARNING(4702)
#include <set>
X_ENABLE_WARNING(4702)

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\HeapArea.h>

#include "Threading\ThreadQue.h"
#include "Threading\Thread.h"

X_NAMESPACE_BEGIN(core)



// we want multiple search path's so we can add folders into the search.
// we will need the folder name.
// a search can also be a PAK since that is like a directory of files.

struct pak_s
{
	StackString<64> name;
};

struct directory_s
{
	Path<wchar_t> path;

};

struct search_s
{
	directory_s* dir;
	pak_s* pak;
	struct search_s* next_;
};


struct XFindData;




class xFileSys : public IFileSys, private core::ThreadAbstract
{
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

	static const size_t IO_REQUEST_BUF_SIZE = MAX_REQ_SIZE * 0x100;
	static const size_t PENDING_IO_QUE_SIZE = 0x100;

	typedef std::array<uint8_t, MAX_REQ_SIZE> RequestBuffer;

public:
	xFileSys();
	~xFileSys() X_FINAL;

	void registerVars(void) X_FINAL;
	void registerCmds(void) X_FINAL;

	bool init(const SCoreInitParams& params) X_FINAL;
	bool initWorker(void) X_FINAL;
	void shutDown(void) X_FINAL;


	bool InitDirectorys(bool workingDir);

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
	bool createDirectoryTree(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;

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
	const XFileSysVars* getVars(void) const { return &vars_; }

	// stats
	virtual XFileStats& getStats(void) const X_FINAL;


	// IoRequest que.

	void AddIoRequestToQue(const IoRequestBase& request) X_FINAL;
	bool StartRequestWorker(void);
	void ShutDownRequestWorker(void);

	void popRequest(RequestBuffer& buf);
	bool tryPopRequest(RequestBuffer& buf);
	// ~

	// ThreadAbstract
	virtual Thread::ReturnValue ThreadRun(const Thread& thread) X_FINAL;
	// ~ThreadAbstract

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
	const wchar_t* createOSPath(directory_s* dir, pathType path, Path<wchar_t>& buffer) const;
	const wchar_t* createOSPath(directory_s* dir, pathTypeW path, Path<wchar_t>& buffer) const;
	bool isAbsolute(pathType path) const;
	bool isAbsolute(pathTypeW path) const;
	
	bool isDebug(void) const;

private:
#if X_DEBUG == 1
	typedef std::set<XFindData*> findDataSet;
	findDataSet findData_;
#endif // !X_DEBUG

	directory_s* gameDir_;
	search_s* searchPaths_;

	XFileSysVars vars_;

	core::HeapArea      filePoolHeap_;
	core::PoolAllocator filePoolAllocator_;
	FilePoolArena		filePoolArena_;

	core::MallocFreeAllocator memfileAllocator_;
	MemfileArena		memFileArena_;

private:

	
	// i want some sort of FIFO byte stream to store the diffrent sized requests.
	// typedef ThreadQueBlocking<IoRequestBase*, core::CriticalSection> IoQue;
	// IoQue ioQue_;

	core::CriticalSection requestLock_;
	core::ConditionVariable requestCond_;
	core::ByteStreamFifo requestData_;
};


X_NAMESPACE_END