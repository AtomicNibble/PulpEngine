#pragma once

#include <IFileSys.h>

#include "String\Path.h"
#include "FileSysVars.h"

#include <Containers\HashMap.h>

#include <set>

#include <Memory\ThreadPolicies\MultiThreadPolicy.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\HeapArea.h>

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
	Path<char> path;

};

struct search_s
{
	directory_s* dir;
	pak_s* pak;
	struct search_s* next_;
};


struct XFindData;

typedef core::MemoryArena<
	core::PoolAllocator, 
	core::MultiThreadPolicy<core::Spinlock>,
	core::SimpleBoundsChecking, 
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging> FilePoolArena;

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::MultiThreadPolicy<core::Spinlock>,
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging> MemfileArena;



class xFileSys : public IFileSys
{
public:
#ifdef X_PLATFORM_WIN32
	static const char NATIVE_SLASH = '\\';
	static const char NON_NATIVE_SLASH = '/';

#else
	static const char NATIVE_SLASH = '/';
	static const char NON_NATIVE_SLASH = '\\';
#endif


	friend struct XFindData;

	xFileSys();
	~xFileSys() X_FINAL;

	bool Init(void) X_FINAL;
	void ShutDown(void) X_FINAL;
	void CreateVars(void) X_FINAL;

	// Open / Close
	XFile* openFile(pathType path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_FINAL;
	void closeFile(XFile* file) X_FINAL;

	// async
	virtual XFileAsync* openFileAsync(pathType path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_FINAL;
	virtual void closeFileAsync(XFileAsync* file) X_FINAL;

	// Mem
	XFileMem* openFileMem(pathType path, fileModeFlags mode) X_FINAL;
	void closeFileMem(XFileMem* file) X_FINAL;

	// folders
	bool setGameDir(pathType path) X_FINAL;
	void addModDir(pathType path) X_FINAL;

	// Find util
	virtual uintptr_t findFirst(pathType path, _finddatai64_t* findinfo) X_FINAL;
	virtual bool findnext(uintptr_t handle, _finddatai64_t* findinfo) X_FINAL;
	virtual void findClose(uintptr_t handle) X_FINAL;

	// Delete
	virtual bool deleteFile(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	virtual bool deleteDirectory(pathType path, bool recursive = false) const X_FINAL;

	// Create
	virtual bool createDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	virtual bool createDirectoryTree(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;

	// exsists.
	virtual bool fileExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;
	virtual bool directoryExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;

	// does not error, when it's a file or not exsist.
	virtual bool isDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_FINAL;

	// settings baby
	const XFileSysVars* getVars() const { return &vars_; }

	// stats
	virtual XFileStats& getStats(void) const X_FINAL;

private:

	// Ajust path
	const char* createOSPath(directory_s* dir, pathType path, Path<char>& buffer) const;
	bool isAbsolute(pathType path) const;

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

};


X_NAMESPACE_END