#pragma once


#ifndef _X_FILE_SYSTEM_I_H_
#define _X_FILE_SYSTEM_I_H_

#include <io.h>

// i need the definition :|
#include X_INCLUDE(../Core/X_PLATFORM/OsFileAsyncOperation.h)

X_NAMESPACE_BEGIN(core)

X_DECLARE_FLAGS(fileMode) (READ, WRITE, APPEND, WRITE_FLUSH, RECREATE, SHARE, RANDOM_ACCESS);
X_DECLARE_FLAGS(SeekMode) (CUR, END, SET);
X_DECLARE_ENUM(VirtualDirectory)(GAME, MOD);

typedef Flags<fileMode> fileModeFlags;


struct XFileStats
{
	size_t NumBytesRead;
	size_t NumBytesWrite;
	size_t NumFilesOpened;
	size_t NumSeeks;
	size_t NumTells;
	size_t NumByteLeftChecks;
};

struct XFileAsync;


struct XFileAsyncOperation
{
	friend struct XFileAsync;

	inline XFileAsyncOperation(const XOsFileAsyncOperation& operation, void* pBuffer) :
		operation_(operation),
		pReadBuffer_(pBuffer),
	//	pFileData_(nullptr),
		isReadOperation_(true)
	{

	}
	inline XFileAsyncOperation(const XOsFileAsyncOperation& operation, const void* pBuffer) :
		operation_(operation),
		pWriteBuffer_(pBuffer),
	//	pFileData_(nullptr),
		isReadOperation_(false)
	{

	}


	inline bool hasFinished(void) const {
		return operation_.hasFinished();
	}
	inline uint32_t waitUntilFinished(void) const {
		return operation_.waitUntilFinished();
	}
	inline void cancel(void) {
		operation_.cancel();
	}

	inline bool isReadOperation(void) const {
		return isReadOperation_;
	}

private:
	inline void* getReadBuffer(void) const {
		X_ASSERT(isReadOperation(), "can't get read buffer from a read operation")(isReadOperation());
		return pReadBuffer_;
	}
	inline const void* getWriteBuffer(void) const {
		X_ASSERT(!isReadOperation(), "can't get write buffer from a read operation")(isReadOperation());
		return pWriteBuffer_;
	}


private:
	core::XOsFileAsyncOperation operation_;

	// an operation can either be a read operation, or a write operation, but not both
	union
	{
		void* pReadBuffer_;
		const void* pWriteBuffer_;
	};

//	void* pFileData_;
	bool isReadOperation_;
};


struct XFileAsync
{
	virtual ~XFileAsync() {};

	virtual XFileAsyncOperation readAsync(void* pBuffer, uint32_t length, uint32_t position) X_ABSTRACT;

	/// Asynchronously writes from a buffer into the file.
	virtual XFileAsyncOperation writeAsync(const void* pBuffer, uint32_t length, uint32_t position) X_ABSTRACT;

	/// Waits until the asynchronous operation has finished, and returns the number of transferred bytes.
	virtual uint32_t WaitUntilFinished(const XFileAsyncOperation& operation) X_ABSTRACT;

	// always returns total bytes.
	// you can't seek.
	virtual size_t remainingBytes(void) const X_ABSTRACT;
	virtual void setSize(size_t numBytes) X_ABSTRACT;
};


struct XFile
{
	virtual ~XFile() {};
	virtual uint32_t read(void* pBuf, uint32_t Len) X_ABSTRACT;
	virtual uint32_t write(const void* pBuf, uint32_t Len) X_ABSTRACT;

	virtual void seek(size_t position, SeekMode::Enum origin) X_ABSTRACT;

	template <typename T>
	inline uint32_t readObj(T& object) {
		return read(&object, sizeof(T));
	}

	template <typename T>
	inline uint32_t readObj(T* objects, size_t num) {
		return read(objects, safe_static_cast<uint32_t, size_t>(sizeof(T)* num));
	}

	inline uint32_t readString(core::string& str) {
		// uggh
		char Char;
		size_t pos = tell();
		str.clear();
		while (read(&Char, 1))
		{
			if (Char == '\0')
				break;
			str += Char;
		}
		return safe_static_cast<uint32_t, size_t>(str.length());
	}

	template <typename T>
	inline uint32_t writeObj(const T& object) {
		return write(&object, sizeof(T));
	}

	template <typename T>
	inline uint32_t writeObj(const T* objects, size_t num) {
		return write(objects, safe_static_cast<uint32_t, size_t>(sizeof(T)* num));
	}
	inline uint32_t writeString(core::string& str) {
		return write(str.c_str(), safe_static_cast<uint32_t, size_t>(str.length() + 1));
	}
	inline uint32_t writeString(const char* str) {
		return write(str, safe_static_cast<uint32_t, size_t>(strlen(str) + 1));
	}
	inline uint32_t writeString(const char* str, uint32_t Length) {
		return write(str, Length);
	}
	inline uint32_t printf(const char *fmt, ...) {
		char buf[2048]; // more? i think not!
		int length;

		va_list argptr;

		va_start(argptr, fmt);
		length = vsnprintf(buf, sizeof(buf)-1, fmt, argptr);
		va_end(argptr);


		return write(buf, length);
	}


	virtual size_t remainingBytes(void) const X_ABSTRACT;
	virtual size_t tell(void) const X_ABSTRACT;
	virtual void setSize(size_t numBytes) X_ABSTRACT;
};

struct XFileMem : public XFile
{
	XFileMem(char* begin, char* end, core::MemoryArenaBase* arena) : 
	begin_(begin), current_(begin), end_(end), arena_(arena)
	{
		X_ASSERT_NOT_NULL(begin);
		X_ASSERT_NOT_NULL(end);
		X_ASSERT_NOT_NULL(arena);
		X_ASSERT(end >= begin, "invalid buffer")(begin,end);
	}
	~XFileMem() {
		X_DELETE_ARRAY(begin_,arena_);
	}

	virtual uint32_t read(void* pBuf, uint32_t Len) X_FINAL{
		size_t size = core::Min<size_t>(Len, remainingBytes());

		memcpy(pBuf, current_, size);
		current_ += size;

		return safe_static_cast<uint32_t,size_t>(size);
	}

	virtual uint32_t write(const void* pBuf, uint32_t Len) X_FINAL{
		X_ASSERT_NOT_IMPLEMENTED();
		return 0;
	}

	virtual void seek(size_t position, SeekMode::Enum origin) X_FINAL{
		switch (origin)
		{
			case SeekMode::CUR:
			current_ += core::Min<size_t>(position, remainingBytes());
			break;
			case SeekMode::SET:
			current_ = begin_ + core::Min<size_t>(position, getSize());
			break;
			case SeekMode::END:
				X_ASSERT_NOT_IMPLEMENTED();
			break;
		}
	}
	virtual size_t remainingBytes(void) const X_FINAL{
		return end_ - current_;
	}
	virtual size_t tell(void) const X_FINAL{
		return current_ - begin_;
	}
	virtual void setSize(size_t numBytes) X_FINAL{
		X_UNUSED(numBytes);
		X_ASSERT_UNREACHABLE();
	}

	inline char* getBufferStart(void) { return begin_; }
	inline const char* getBufferStart(void) const { return begin_; }

	inline char* getBufferEnd(void) { return end_; }
	inline const char* getBufferEnd(void) const { return end_; }

	inline size_t getSize(void) const {
		return end_ - begin_;
	}

	inline MemoryArenaBase* getMemoryArena(void) {
		return arena_;
	}

private:
	core::MemoryArenaBase* arena_;
	char* begin_;
	char* current_;
	char* end_;
};



struct IFileSys
{
	typedef fileMode fileMode;
	typedef Flags<fileMode> fileModeFlags;
	typedef SeekMode SeekMode;
	typedef const char* pathType;
	typedef _finddatai64_t findData;

	static const uintptr_t INVALID_HANDLE = (uintptr_t)-1;

	virtual ~IFileSys(){}

	virtual bool Init(void) X_ABSTRACT;
	virtual void ShutDown(void) X_ABSTRACT;
	virtual void CreateVars(void) X_ABSTRACT;

	// folders - there is only one game dirtory.
	// but other folders can be added with 'addModDir' to add to the virtual directory.
	virtual bool setGameDir(pathType path) X_ABSTRACT;
	virtual void addModDir(pathType path) X_ABSTRACT;

	// Open Files
	virtual XFile* openFile(pathType path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_ABSTRACT;
	virtual void closeFile(XFile* file) X_ABSTRACT;

	// async
	virtual XFileAsync* openFileAsync(pathType path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_ABSTRACT;
	virtual void closeFileAsync(XFileAsync* file) X_ABSTRACT;

	// loads the whole file into memory.
	virtual XFileMem* openFileMem(pathType path, fileModeFlags mode) X_ABSTRACT;
	virtual void closeFileMem(XFileMem* file) X_ABSTRACT;

	// Find util
	virtual uintptr_t findFirst(pathType path, _finddatai64_t* findinfo) X_ABSTRACT;
	virtual bool findnext(uintptr_t handle, _finddatai64_t* findinfo) X_ABSTRACT;
	virtual void findClose(uintptr_t handle) X_ABSTRACT;

	// Delete
	virtual bool deleteFile(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
	virtual bool deleteDirectory(pathType path, bool recursive = false) const X_ABSTRACT;

	// Create
	virtual bool createDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
	virtual bool createDirectoryTree(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;

	// exsists.
	virtual bool fileExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
	virtual bool directoryExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;

	// does not error, when it's a file or not exsist.
	virtual bool isDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;

	// stats
	virtual XFileStats& getStats(void) const X_ABSTRACT;
};

class XFileMemScoped
{
public:
	XFileMemScoped() : pFile_(nullptr)
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pFileSys);
		pFileSys_ = gEnv->pFileSys;
	}


	~XFileMemScoped() {
		close();
	}

	inline bool openFile(const char* path, IFileSys::fileModeFlags mode) {
		pFile_ = pFileSys_->openFileMem(path, mode);
		return pFile_ != nullptr;
	}

	inline void close(void) {
		if (pFile_) {
			pFileSys_->closeFileMem(pFile_);
			pFile_ = nullptr;
		}
	}

	inline operator bool() const {
		return pFile_ != nullptr;
	}

	inline bool IsOpen(void) const {
		return pFile_ != nullptr;
	}

	X_INLINE XFileMem* operator->(void) {
		return pFile_;
	}
	X_INLINE const XFileMem* operator->(void) const {
		return pFile_;
	}
	X_INLINE operator XFileMem*(void) {
		return pFile_;
	}
	X_INLINE operator const XFileMem*(void) const {
		return pFile_;
	}

private:
	XFileMem* pFile_;
	IFileSys* pFileSys_;
};

// makes using the file more easy as you don't have to worrie about closing it.
class XFileScoped
{
public:
	XFileScoped() : pFile_(nullptr) 
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pFileSys);
		pFileSys_ = gEnv->pFileSys;
	}

	~XFileScoped() {
		close();
	}

	inline bool openFile(const char* path, IFileSys::fileModeFlags mode) {
		pFile_ = pFileSys_->openFile(path, mode);
		return pFile_ != nullptr;
	}

	inline void close(void) {
		if (pFile_) {
			pFileSys_->closeFile(pFile_);
			pFile_ = nullptr;
		}
	}

	inline operator bool() const {
		return pFile_ != nullptr;
	}

	inline uint32_t read(void* pBuf, uint32_t Len) {
		X_ASSERT_NOT_NULL(pFile_); // catch bad use of this class. "not checking open return val"
		return pFile_->read(pBuf, Len);
	}
#if X_64
	inline uint32_t read(void* pBuf, size_t Len) {
		X_ASSERT_NOT_NULL(pFile_); // catch bad use of this class. "not checking open return val"
		return pFile_->read(pBuf, safe_static_cast<uint32_t,size_t>(Len));
	}
#endif

	template <typename T>
	inline uint32_t read(T& object) {
		return read(&object, sizeof(T));
	}

	template <typename T>
	inline uint32_t readObj(T& object) {
		return read(&object, sizeof(T));
	}

	template <typename T>
	inline uint32_t readObjs(T* objects, uint32_t num) {
		return read(objects, sizeof(T)* num) / sizeof(T);
	}


	inline uint32_t readString(core::string& str) {
		return pFile_->readString(str);
	}


#if X_64
	inline uint32_t write(const void* pBuf, size_t Len) {
		X_ASSERT_NOT_NULL(pFile_);
		return pFile_->write(pBuf, safe_static_cast<uint32_t, size_t>(Len));
	}
#endif // !X_64
	inline uint32_t write(const void* pBuf, uint32_t Len) {
		X_ASSERT_NOT_NULL(pFile_);
		return pFile_->write(pBuf, Len);
	}

	inline uint32_t writeString(core::string& str) {
		return pFile_->writeString(str);
	}
	inline uint32_t writeString(const char* str) {
		return pFile_->writeString(str);
	}
	inline uint32_t writeString(const char* str, uint32_t Length) {
		return pFile_->writeString(str, Length);
	}

	template <typename T>
	inline uint32_t writeObj(T& object) {
		return write(&object, sizeof(T));
	}

	template <typename T>
	inline uint32_t writeObjs(T* objects, uint32_t num) {
		return write(objects, sizeof(T)* num) / sizeof(T);
	}

	template <typename T>
	inline uint32_t write(const T& object) {
		return write(&object, sizeof(T));
	}

	uint32_t printf(const char *fmt, ...) {
		char buf[2048];
		uint32_t length;

		va_list argptr;

		va_start(argptr, fmt);
		length = vsnprintf_s(buf, 2048 - 1, fmt, argptr);
		va_end(argptr);

		return write(buf, length);
	}

	inline void seek(size_t position, SeekMode::Enum origin) {
		X_ASSERT_NOT_NULL(pFile_);
		pFile_->seek(position, origin);
	}

	inline size_t tell(void) const {
		X_ASSERT_NOT_NULL(pFile_);
		return pFile_->tell();
	}

	inline size_t remainingBytes(void) const {
		X_ASSERT_NOT_NULL(pFile_);
		return pFile_->remainingBytes();
	}

	inline XFile* GetFile() const {
		return pFile_;
	}

private:
	XFile*    pFile_;
	IFileSys* pFileSys_;
};



X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_I_H_
