#pragma once


#ifndef _X_FILE_SYSTEM_I_H_
#define _X_FILE_SYSTEM_I_H_

#include <io.h>
#include <Util\Delegate.h>

// i need the definition :|
#include X_INCLUDE(../Core/X_PLATFORM/OsFileAsyncOperation.h)

X_NAMESPACE_BEGIN(core)

static const size_t FS_MAX_VIRTUAL_DIR = 10;


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
		isReadOperation_(true)
	{

	}
	inline XFileAsyncOperation(const XOsFileAsyncOperation& operation, const void* pBuffer) :
		operation_(operation),
		pWriteBuffer_(pBuffer),
		isReadOperation_(false)
	{

	}

#if X_64
	inline bool hasFinished(size_t* pNumBytes = nullptr) const {
		return operation_.hasFinished(pNumBytes);
	}
#endif // !X_64

	inline bool hasFinished(uint32_t* pNumBytes = nullptr) const {
		return operation_.hasFinished(pNumBytes);
	}
	inline size_t waitUntilFinished(void) const {
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

	virtual XFileAsyncOperation readAsync(void* pBuffer, size_t length, uint64_t position) X_ABSTRACT;

	/// Asynchronously writes from a buffer into the file.
	virtual XFileAsyncOperation writeAsync(const void* pBuffer, size_t length, uint64_t position) X_ABSTRACT;

	/// Waits until the asynchronous operation has finished, and returns the number of transferred bytes.
	virtual size_t WaitUntilFinished(const XFileAsyncOperation& operation) X_ABSTRACT;

	// always returns total bytes.
	// you can't seek.
	virtual uint64_t remainingBytes(void) const X_ABSTRACT;
	virtual void setSize(int64_t numBytes) X_ABSTRACT;
};


struct XFile
{
	virtual ~XFile() {};
	virtual size_t read(void* pBuf, size_t Len) X_ABSTRACT;
	virtual size_t write(const void* pBuf, size_t Len) X_ABSTRACT;

	virtual void seek(int64_t position, SeekMode::Enum origin) X_ABSTRACT;

	template <typename T>
	inline size_t readObj(T& object) {
		return read(&object, sizeof(T));
	}

	template <typename T>
	inline size_t readObj(T* objects, size_t num) {
		return read(objects, sizeof(T)* num);
	}

	inline size_t readString(core::string& str) {
		// uggh
		char Char;
		str.clear();
		while (read(&Char, 1))
		{
			if (Char == '\0')
				break;
			str += Char;
		}
		return str.length();
	}

	template <typename T>
	inline size_t writeObj(const T& object) {
		return write(&object, sizeof(T));
	}

	template <typename T>
	inline size_t writeObj(const T* objects, size_t num) {
		return write(objects,(sizeof(T)* num));
	}
	inline size_t writeString(core::string& str) {
		return write(str.c_str(), str.length() + 1);
	}
	inline size_t writeString(const char* str) {
		return write(str, (strlen(str) + 1));
	}
	inline size_t writeString(const char* str, size_t Length) {
		return write(str, Length);
	}

	inline size_t writeStringNNT(core::string& str) {
		return write(str.c_str(), (str.length()));
	}
	inline size_t writeStringNNT(const char* str) {
		return write(str, (strlen(str)));
	}


	inline size_t printf(const char *fmt, ...) {
		char buf[2048]; // more? i think not!
		int length;

		va_list argptr;

		va_start(argptr, fmt);
		length = vsnprintf(buf, sizeof(buf)-1, fmt, argptr);
		va_end(argptr);

		if (length < 0) {
			return 0;
		}

		return write(buf, length);
	}

	virtual inline bool isEof(void) const {
		return remainingBytes() == 0;
	}

	virtual uint64_t remainingBytes(void) const X_ABSTRACT;
	virtual uint64_t tell(void) const X_ABSTRACT;
	virtual void setSize(int64_t numBytes) X_ABSTRACT;
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
	~XFileMem() X_OVERRIDE {
		X_DELETE_ARRAY(begin_,arena_);
	}

	virtual size_t read(void* pBuf, size_t Len) X_FINAL{
		size_t size = core::Min<size_t>(Len, safe_static_cast<size_t, uint64_t>(remainingBytes()));

		memcpy(pBuf, current_, size);
		current_ += size;

		return size;
	}

	virtual size_t write(const void* pBuf, size_t Len) X_FINAL{
		X_UNUSED(pBuf);
		X_UNUSED(Len);
		X_ASSERT_NOT_IMPLEMENTED();
		return 0;
	}

	virtual void seek(int64_t position, SeekMode::Enum origin) X_FINAL{
		switch (origin)
		{
			case SeekMode::CUR:
			current_ += core::Min<int64_t>(position, remainingBytes());
			if (current_ < begin_) {
				current_ = begin_;
			}
			break;
			case SeekMode::SET:
			current_ = begin_ + core::Min<int64_t>(position, getSize());
			if (current_ < begin_) {
				current_ = begin_;
			}
			if (current_ > end_) {
				current_ = end_;
			}
			break;
			case SeekMode::END:
				X_ASSERT_NOT_IMPLEMENTED();
			break;
		}
	}
	virtual uint64_t remainingBytes(void) const X_FINAL {
		return static_cast<size_t>(end_ - current_);
	}
	virtual uint64_t tell(void) const X_FINAL {
		return static_cast<size_t>(current_ - begin_);
	}
	virtual void setSize(int64_t numBytes) X_FINAL {
		X_UNUSED(numBytes);
		X_ASSERT_UNREACHABLE();
	}

	inline char* getBufferStart(void) { return begin_; }
	inline const char* getBufferStart(void) const { return begin_; }

	inline char* getBufferEnd(void) { return end_; }
	inline const char* getBufferEnd(void) const { return end_; }

	inline uint64_t getSize(void) const {
		return static_cast<size_t>(end_ - begin_);
	}

	inline MemoryArenaBase* getMemoryArena(void) {
		return arena_;
	}

	inline bool isEof(void) const X_FINAL {
		return remainingBytes() == 0;
	}

private:
	core::MemoryArenaBase* arena_;
	char* begin_;
	char* current_;
	char* end_;
};



struct XFileBuf : public XFile
{
	XFileBuf(uint8_t* begin, uint8_t* end) :
	begin_(begin), current_(begin), end_(end)
	{
		X_ASSERT_NOT_NULL(begin);
		X_ASSERT_NOT_NULL(end);
		X_ASSERT(end >= begin, "invalid buffer")(begin, end);
	}
	~XFileBuf() X_OVERRIDE {
	}

	virtual size_t read(void* pBuf, size_t Len) X_FINAL{
		size_t size = core::Min<size_t>(Len, safe_static_cast<size_t, uint64_t>(remainingBytes()));

		memcpy(pBuf, current_, size);
		current_ += size;

		return size;
	}

	virtual size_t write(const void* pBuf, size_t Len) X_FINAL {
		X_UNUSED(pBuf);
		X_UNUSED(Len);
		X_ASSERT_NOT_IMPLEMENTED();
		return 0;
	}

	virtual void seek(int64_t position, SeekMode::Enum origin) X_FINAL{
		switch (origin)
		{
			case SeekMode::CUR:
				current_ += core::Min<int64_t>(position, remainingBytes());
				if (current_ < begin_) {
					current_ = begin_;
				}
				break;
			case SeekMode::SET:
				current_ = begin_ + core::Min<int64_t>(position, getSize());
				if (current_ < begin_) {
					current_ = begin_;
				}
				if (current_ > end_) {
					current_ = end_;
				}
				break;
			case SeekMode::END:
				X_ASSERT_NOT_IMPLEMENTED();
				break;
		}
	}
	virtual uint64_t remainingBytes(void) const X_FINAL{
		return static_cast<size_t>(end_ - current_);
	}
	virtual uint64_t tell(void) const X_FINAL{
		return static_cast<size_t>(current_ - begin_);
	}
	virtual void setSize(int64_t numBytes) X_FINAL{
		X_UNUSED(numBytes);
		X_ASSERT_UNREACHABLE();
	}

	inline uint8_t* getBufferStart(void) { return begin_; }
	inline const uint8_t* getBufferStart(void) const { return begin_; }

	inline uint8_t* getBufferEnd(void) { return end_; }
	inline const uint8_t* getBufferEnd(void) const { return end_; }

	inline uint64_t getSize(void) const {
		return static_cast<uint64_t>(end_ - begin_);
	}

	inline bool isEof(void) const X_FINAL{
		return remainingBytes() == 0;
	}

private:
	uint8_t* begin_;
	uint8_t* current_;
	uint8_t* end_;
};



// stuff for io requests
X_DECLARE_ENUM(IoRequest)(
	// open a file gor a given name and mode.
	// OnSuccess: vaild async file handle.
	OPEN,
	OPEN_READ_ALL,
	CLOSE,
	READ,
	WRITE
);

/*
typedef core::traits::Function<void(core::IFileSys*,IoRequest::Enum, bool, XFileAsync*)> IoRequestCallback;
typedef core::traits::Function<void(core::IFileSys*,IoRequest::Enum, bool, XFileMem*)> IoRequestMemCallback;


struct IIoRequestHandler
{
	virtual void IoRequestCallback(core::IFileSys* pFileSys, core::IoRequest::Enum requestType,
		core::XFileAsync* pFile, bool result) X_ABSTRACT;
	virtual void IoRequestCallbackMem(core::IFileSys* pFileSys, core::IoRequest::Enum requestType,
		core::XFileMem* pFile, bool result) X_ABSTRACT;
}; 
*/

struct IoRequestOpen
{
	core::string name;
	fileModeFlags mode;
};

struct IoRequestClose
{
	XFileAsync* pFile;
};

struct IoRequestRead
{
	XFileAsync* pFile;
	void* pBuf;
	uint64_t offset;	// support files >4gb.
	uint32_t dataSize; // don't support reading >4gb at once.
	void* pUserData;
};

typedef IoRequestRead IoRequestWrite;

X_DISABLE_WARNING(4624)
class IoRequestData
{
public:
	X_INLINE IoRequestData() {
		type = IoRequest::CLOSE;
	}
	X_INLINE ~IoRequestData()
	{
		if (type == IoRequest::OPEN || type == IoRequest::OPEN_READ_ALL)
		{
			core::Mem::Destruct(&openInfo.name);
		}
	}

	X_INLINE IoRequestData(const IoRequestData& oth) {
		type = oth.type;
		callback = oth.callback;
		
		if (type == IoRequest::OPEN || type == IoRequest::OPEN_READ_ALL)
		{
			core::Mem::Construct(&openInfo.name, oth.openInfo.name);
			openInfo.mode = oth.openInfo.mode;
		}
		else if (type == IoRequest::CLOSE) {
			closeInfo = oth.closeInfo;
		}
		else if (type == IoRequest::READ) {
			readInfo = oth.readInfo;
		}
		else if (type == IoRequest::WRITE) {
			writeInfo = oth.writeInfo;
		}
	}

	X_INLINE IoRequestData& operator=(const IoRequestData& oth) {
		type = oth.type;
		callback = oth.callback;

		if (type == IoRequest::OPEN || type == IoRequest::OPEN_READ_ALL)
		{
			core::Mem::Construct(&openInfo.name, oth.openInfo.name);
			openInfo.mode = oth.openInfo.mode;
		}	
		else if (type == IoRequest::CLOSE) {
			closeInfo = oth.closeInfo;
		}
		else if (type == IoRequest::READ) {
			readInfo = oth.readInfo;
		}
		else if (type == IoRequest::WRITE) {
			writeInfo = oth.writeInfo;
		}
		return *this;
	}

	X_INLINE IoRequest::Enum getType(void) const {
		return type;
	}

	X_INLINE void setType(IoRequest::Enum type_)
	{
		if (type_ == IoRequest::OPEN || type == IoRequest::OPEN_READ_ALL)
		{
			core::Mem::Construct<core::string>(&openInfo.name);
		}
		this->type = type_;
	}

private:
	IoRequest::Enum type;

public:
	core::Delegate<void(core::IFileSys*, IoRequestData&,
		core::XFileAsync*, uint32_t)> callback;

	union
	{
		IoRequestOpen openInfo;
		IoRequestClose closeInfo;
		IoRequestRead readInfo;
		IoRequestWrite writeInfo;
	};
};
X_ENABLE_WARNING(4624)

X_ENSURE_LE(sizeof(IoRequestData), 64, "IoRequest data should be 64 bytes or less");


struct IFileSys
{
	typedef fileMode fileMode;
	typedef Flags<fileMode> fileModeFlags;
	typedef SeekMode SeekMode;
	typedef const char* pathType;
	typedef const wchar_t* pathTypeW;
	typedef _wfinddatai64_t findData;

	static const uintptr_t INVALID_HANDLE = (uintptr_t)-1;

	virtual ~IFileSys(){}

	virtual bool Init(void) X_ABSTRACT;
	virtual bool InitWorker(void) X_ABSTRACT;
	virtual void ShutDown(void) X_ABSTRACT;
	virtual void CreateVars(void) X_ABSTRACT;

	// folders - there is only one game dirtory.
	// but other folders can be added with 'addModDir' to add to the virtual directory.
	virtual bool setGameDir(pathTypeW path) X_ABSTRACT;
	virtual void addModDir(pathTypeW path) X_ABSTRACT;

	// Open Files
	virtual XFile* openFile(pathType path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_ABSTRACT;
	virtual XFile* openFile(pathTypeW path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_ABSTRACT;
	virtual void closeFile(XFile* file) X_ABSTRACT;

	// async
	virtual XFileAsync* openFileAsync(pathType path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_ABSTRACT;
	virtual XFileAsync* openFileAsync(pathTypeW path, fileModeFlags mode, VirtualDirectory::Enum location = VirtualDirectory::GAME) X_ABSTRACT;
	virtual void closeFileAsync(XFileAsync* file) X_ABSTRACT;

	// loads the whole file into memory.
	virtual XFileMem* openFileMem(pathType path, fileModeFlags mode) X_ABSTRACT;
	virtual XFileMem* openFileMem(pathTypeW path, fileModeFlags mode) X_ABSTRACT;
	virtual void closeFileMem(XFileMem* file) X_ABSTRACT;

	// Find util
	virtual uintptr_t findFirst(pathType path, _wfinddatai64_t* findinfo) X_ABSTRACT;
	virtual bool findnext(uintptr_t handle, _wfinddatai64_t* findinfo) X_ABSTRACT;
	virtual void findClose(uintptr_t handle) X_ABSTRACT;

	// Delete
	virtual bool deleteFile(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
	virtual bool deleteDirectory(pathType path, bool recursive = false) const X_ABSTRACT;

	// Create
	virtual bool createDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
	virtual bool createDirectoryTree(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;

	// exsists.
	virtual bool fileExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
	virtual bool fileExists(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
	virtual bool directoryExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
	virtual bool directoryExists(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;

	// does not error, when it's a file or not exsist.
	virtual bool isDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
	virtual bool isDirectory(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;

	// returns the min sector size for all virtual directories.
	// so if game folder is drive A and mod is drive B
	// yet drive B has a larger sector size, it will return the largest.
	virtual size_t getMinimumSectorSize(void) const X_ABSTRACT;

	// stats
	virtual XFileStats& getStats(void) const X_ABSTRACT;

	virtual void AddIoRequestToQue(const IoRequestData& request) X_ABSTRACT;
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
	inline bool openFile(const wchar_t* path, IFileSys::fileModeFlags mode) {
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

	inline size_t read(void* pBuf, size_t Len) {
		X_ASSERT_NOT_NULL(pFile_); // catch bad use of this class. "not checking open return val"
		return pFile_->read(pBuf, Len);
	}

	template <typename T>
	inline size_t read(T& object) {
		return read(&object, sizeof(T));
	}

	template <typename T>
	inline size_t readObj(T& object) {
		return read(&object, sizeof(T));
	}

	template <typename T>
	inline size_t readObjs(T* objects, size_t num) {
		return read(objects, sizeof(T)* num) / sizeof(T);
	}


	inline size_t readString(core::string& str) {
		return pFile_->readString(str);
	}

	inline size_t write(const void* pBuf, size_t Len) {
		X_ASSERT_NOT_NULL(pFile_);
		return pFile_->write(pBuf, Len);
	}

	inline size_t writeString(core::string& str) {
		return pFile_->writeString(str);
	}
	inline size_t writeString(const char* str) {
		return pFile_->writeString(str);
	}
	inline size_t writeString(const char* str, size_t Length) {
		return pFile_->writeString(str, Length);
	}

	inline size_t writeStringNNT(core::string& str) {
		return pFile_->writeStringNNT(str);
	}
	inline size_t writeStringNNT(const char* str) {
		return pFile_->writeStringNNT(str);
	}

	template <typename T>
	inline size_t writeObj(T& object) {
		return write(&object, sizeof(T));
	}
	template <typename T>
	inline size_t writeObj(const T* objects, size_t num) {
		return write(objects, (sizeof(T)* num));
	}

	template <typename T>
	inline size_t writeObjs(T* objects, size_t num) {
		return write(objects, sizeof(T)* num) / sizeof(T);
	}

	template <typename T>
	inline size_t write(const T& object) {
		return write(&object, sizeof(T));
	}

	size_t printf(const char *fmt, ...) {
		char buf[2048];
		int32_t length;

		va_list argptr;

		va_start(argptr, fmt);
		length = vsnprintf_s(buf, 2048 - 1, fmt, argptr);
		va_end(argptr);

		if (length < 0) {
			return 0;
		}

		return write(buf, length);
	}

	inline void seek(int64_t position, SeekMode::Enum origin) {
		X_ASSERT_NOT_NULL(pFile_);
		pFile_->seek(position, origin);
	}

	inline uint64_t tell(void) const {
		X_ASSERT_NOT_NULL(pFile_);
		return pFile_->tell();
	}

	inline uint64_t remainingBytes(void) const {
		X_ASSERT_NOT_NULL(pFile_);
		return pFile_->remainingBytes();
	}

	inline XFile* GetFile(void) const {
		return pFile_;
	}

private:
	XFile*    pFile_;
	IFileSys* pFileSys_;
};



X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_I_H_
