#pragma once

#ifndef _X_FILE_SYSTEM_I_H_
#define _X_FILE_SYSTEM_I_H_

// why the fuck is this here?
// #include <io.h>

#include <Util\Delegate.h>
#include <Containers\Array.h>

#if X_ENABLE_FILE_STATS
#include <Time\TimeVal.h>
#endif // !X_ENABLE_FILE_STATS

// i need the definition :|
#include X_INCLUDE(../Core/FileSys/X_PLATFORM/OsFileAsyncOperation.h)

X_NAMESPACE_BEGIN(core)

static const size_t FS_MAX_VIRTUAL_DIR = 10;

X_DECLARE_FLAGS(fileMode)
(
    READ,
    WRITE,
    APPEND,
    WRITE_FLUSH,
    RECREATE,
    SHARE,
    RANDOM_ACCESS,
    NOBUFFER);

X_DECLARE_FLAGS(SeekMode)
(
    CUR,
    END,
    SET);

X_DECLARE_ENUM(VirtualDirectory)
(
    GAME,
    MOD);

typedef Flags<fileMode> fileModeFlags;

X_DECLARE_FLAG_OPERATORS(fileModeFlags);

typedef core::XOsFileAsyncOperation XFileAsyncOperation;
typedef core::XOsFileAsyncOperationCompiltion XFileAsyncOperationCompiltion;

struct XFileAsync
{
    X_DECLARE_ENUM(Type)
    (DISK, VIRTUAL);

    typedef XOsFileAsyncOperation::ComplitionRotinue ComplitionRotinue;

    virtual ~XFileAsync(){};

    virtual Type::Enum getType(void) const X_ABSTRACT;

    virtual XFileAsyncOperation readAsync(void* pBuffer, size_t length, uint64_t position) X_ABSTRACT;
    virtual XFileAsyncOperation writeAsync(const void* pBuffer, size_t length, uint64_t position) X_ABSTRACT;

    virtual void cancelAll(void) const X_ABSTRACT;

    /// Waits until the asynchronous operation has finished, and returns the number of transferred bytes.
    virtual size_t waitUntilFinished(const XFileAsyncOperation& operation) X_ABSTRACT;

    virtual uint64_t fileSize(void) const X_ABSTRACT;
    virtual void setSize(int64_t numBytes) X_ABSTRACT;
};

struct XFile
{
    virtual ~XFile() = default;
    virtual size_t read(void* pBuf, size_t Len) X_ABSTRACT;
    virtual size_t write(const void* pBuf, size_t Len) X_ABSTRACT;

    virtual void seek(int64_t position, SeekMode::Enum origin) X_ABSTRACT;

    template<typename T>
    inline size_t readObj(T& object)
    {
        return read(&object, sizeof(T));
    }

    template<typename T>
    inline size_t readObj(T* objects, size_t num)
    {
        return read(objects, sizeof(T) * num);
    }

    inline size_t readString(core::string& str)
    {
        // uggh
        char Char;
        str.clear();
        while (read(&Char, 1)) {
            if (Char == '\0') {
                break;
            }
            str += Char;
        }
        return str.length();
    }

    template<typename T>
    inline size_t writeObj(const T& object)
    {
        return write(&object, sizeof(T));
    }
    template<>
    inline size_t writeObj(const core::string& str)
    {
        return writeString(str);
    }

    template<typename T>
    inline size_t writeObj(const T* objects, size_t num)
    {
        return write(objects, (sizeof(T) * num));
    }
    inline size_t writeString(const core::string& str)
    {
        return write(str.c_str(), str.length() + 1);
    }
    inline size_t writeString(const char* str)
    {
        return write(str, (strlen(str) + 1));
    }
    inline size_t writeString(const char* str, size_t Length)
    {
        return write(str, Length);
    }

    inline size_t writeStringNNT(const core::string& str)
    {
        return write(str.c_str(), (str.length()));
    }
    inline size_t writeStringNNT(const char* str)
    {
        return write(str, (strlen(str)));
    }

    inline size_t printf(const char* fmt, ...)
    {
        char buf[2048]; // more? i think not!
        int length;

        va_list argptr;

        va_start(argptr, fmt);
        length = vsnprintf(buf, sizeof(buf) - 1, fmt, argptr);
        va_end(argptr);

        if (length < 0) {
            return 0;
        }

        return write(buf, length);
    }

    virtual inline bool isEof(void) const
    {
        return remainingBytes() == 0;
    }

    virtual uint64_t remainingBytes(void) const X_ABSTRACT;
    virtual uint64_t tell(void) const X_ABSTRACT;
    virtual void setSize(int64_t numBytes) X_ABSTRACT;
};

// I don't like this.
// as it's taking ownership of buffer.
// and is simular functionaloty to XFileFixedBuf otherwise.
struct XFileMem : public XFile
{
    XFileMem(char* begin, char* end, core::MemoryArenaBase* arena) :
        arena_(arena),
        begin_(begin),
        current_(begin),
        end_(end)
    {
        X_ASSERT_NOT_NULL(begin);
        X_ASSERT_NOT_NULL(end);
        X_ASSERT_NOT_NULL(arena);
        X_ASSERT(end >= begin, "invalid buffer")
        (begin, end);
    }
    ~XFileMem() X_OVERRIDE
    {
        X_DELETE_ARRAY(begin_, arena_);
    }

    virtual size_t read(void* pBuf, size_t Len) X_FINAL
    {
        size_t size = core::Min<size_t>(Len, safe_static_cast<size_t, uint64_t>(remainingBytes()));

        memcpy(pBuf, current_, size);
        current_ += size;

        return size;
    }

    virtual size_t write(const void* pBuf, size_t Len) X_FINAL
    {
        X_UNUSED(pBuf);
        X_UNUSED(Len);
        X_ASSERT_NOT_IMPLEMENTED();
        return 0;
    }

    virtual void seek(int64_t position, SeekMode::Enum origin) X_FINAL
    {
        switch (origin) {
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
    virtual uint64_t remainingBytes(void) const X_FINAL
    {
        return static_cast<size_t>(end_ - current_);
    }
    virtual uint64_t tell(void) const X_FINAL
    {
        return static_cast<size_t>(current_ - begin_);
    }
    virtual void setSize(int64_t numBytes) X_FINAL
    {
        X_UNUSED(numBytes);
        X_ASSERT_UNREACHABLE();
    }

    inline char* getBufferStart(void)
    {
        return begin_;
    }
    inline const char* getBufferStart(void) const
    {
        return begin_;
    }

    inline char* getBufferEnd(void)
    {
        return end_;
    }
    inline const char* getBufferEnd(void) const
    {
        return end_;
    }

    inline uint64_t getSize(void) const
    {
        return static_cast<size_t>(end_ - begin_);
    }

    inline MemoryArenaBase* getMemoryArena(void)
    {
        return arena_;
    }

    inline bool isEof(void) const X_FINAL
    {
        return remainingBytes() == 0;
    }

private:
    core::MemoryArenaBase* arena_;
    char* begin_;
    char* current_;
    char* end_;
};

struct XFileFixedBuf : public XFile
{
    XFileFixedBuf(const uint8_t* begin, const uint8_t* end) :
        begin_(begin),
        current_(begin),
        end_(end)
    {
        X_ASSERT_NOT_NULL(begin);
        X_ASSERT_NOT_NULL(end);
        X_ASSERT(end >= begin, "invalid buffer")
        (begin, end);
    }

    XFileFixedBuf(const char* begin, const char* end) :
        XFileFixedBuf(reinterpret_cast<const uint8_t*>(begin), reinterpret_cast<const uint8_t*>(end))
    {
    }

    ~XFileFixedBuf() X_OVERRIDE
    {
    }

    virtual size_t read(void* pBuf, size_t Len) X_FINAL
    {
        size_t size = core::Min<size_t>(Len, safe_static_cast<size_t, uint64_t>(remainingBytes()));

        memcpy(pBuf, current_, size);
        current_ += size;

        return size;
    }

    virtual size_t write(const void* pBuf, size_t Len) X_FINAL
    {
        X_UNUSED(pBuf);
        X_UNUSED(Len);
        X_ASSERT_NOT_IMPLEMENTED();
        return 0;
    }

    virtual void seek(int64_t position, SeekMode::Enum origin) X_FINAL
    {
        switch (origin) {
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
    virtual uint64_t remainingBytes(void) const X_FINAL
    {
        return static_cast<size_t>(end_ - current_);
    }
    virtual uint64_t tell(void) const X_FINAL
    {
        return static_cast<size_t>(current_ - begin_);
    }
    virtual void setSize(int64_t numBytes) X_FINAL
    {
        X_UNUSED(numBytes);
        X_ASSERT_UNREACHABLE();
    }

    inline const uint8_t* getBufferStart(void) const
    {
        return begin_;
    }

    inline const uint8_t* getBufferEnd(void) const
    {
        return end_;
    }

    inline uint64_t getSize(void) const
    {
        return static_cast<uint64_t>(end_ - begin_);
    }

    inline bool isEof(void) const X_FINAL
    {
        return remainingBytes() == 0;
    }

private:
    const uint8_t* begin_;
    const uint8_t* current_;
    const uint8_t* end_;
};

struct XFileStream : public XFile
{
    typedef core::Array<uint8_t> DataVec;

    XFileStream(core::MemoryArenaBase* arena) :
        buf_(arena)
    {
        X_ASSERT_NOT_NULL(arena);

        buf_.setGranularity(1024 * 16);
    }
    ~XFileStream() X_OVERRIDE
    {
    }

    virtual size_t read(void* pBuf, size_t len) X_FINAL
    {
        X_UNUSED(pBuf);
        X_UNUSED(len);
        X_ASSERT_NOT_IMPLEMENTED();
        return 0;
    }

    virtual size_t write(const void* pBuf, size_t len) X_FINAL
    {
        const size_t offset = buf_.size();

        buf_.resize(offset + len);

        std::memcpy(&buf_[offset], pBuf, len);
        return len;
    }

    virtual void seek(int64_t position, SeekMode::Enum origin) X_FINAL
    {
        X_UNUSED(position);
        X_UNUSED(origin);
        X_ASSERT_NOT_IMPLEMENTED();
    }

    virtual uint64_t remainingBytes(void) const X_FINAL
    {
        return 0;
    }
    virtual uint64_t tell(void) const X_FINAL
    {
        return buf_.size();
    }
    virtual void setSize(int64_t numBytes) X_FINAL
    {
        X_UNUSED(numBytes);
        X_ASSERT_UNREACHABLE();
    }

    inline uint64_t getSize(void) const
    {
        return buf_.size();
    }

    inline bool isEof(void) const X_FINAL
    {
        return remainingBytes() == 0;
    }

    inline const DataVec& buffer(void) const
    {
        return buf_;
    }

private:
    DataVec buf_;
};

struct XFileByteStream : public XFile
{
    XFileByteStream(core::ByteStream& stream) :
        stream_(stream)
    {
    }
    ~XFileByteStream() X_OVERRIDE
    {
    }

    virtual size_t read(void* pBuf, size_t len) X_FINAL
    {
        stream_.read(reinterpret_cast<char*>(pBuf), len);
        return len;
    }

    virtual size_t write(const void* pBuf, size_t len) X_FINAL
    {
        stream_.write(reinterpret_cast<const char*>(pBuf), len);
        return len;
    }

    virtual void seek(int64_t position, SeekMode::Enum origin) X_FINAL
    {
        X_UNUSED(position);
        X_UNUSED(origin);

        X_ASSERT_NOT_IMPLEMENTED();
    }
    virtual uint64_t remainingBytes(void) const X_FINAL
    {
        return stream_.size();
    }
    virtual uint64_t tell(void) const X_FINAL
    {
        return stream_.tell();
    }
    virtual void setSize(int64_t numBytes) X_FINAL
    {
        X_UNUSED(numBytes);
        X_ASSERT_NOT_IMPLEMENTED();
    }

    inline uint64_t getSize(void) const
    {
        return stream_.tellWrite();
    }

    inline bool isEof(void) const X_FINAL
    {
        return stream_.isEos();
    }

private:
    core::ByteStream& stream_;
};

// stuff for io requests
X_DECLARE_ENUM(IoRequest)
(
    // these are ordered based on priority.
    // aka READ requests are higest priority.
    CLOSE,
    OPEN,
    OPEN_WRITE_ALL,
    OPEN_READ_ALL,
    WRITE,
    READ);

typedef uint32_t RequestHandle;
static const RequestHandle INVALID_IO_REQ_HANDLE = 0;

// I want to pass these into filesystem with one function call.
// but support diffrent types.
// so many i should have a base type that contains the mode
// and we copy it into a internal buffer

struct IoRequestBase;

typedef core::Delegate<void(core::IFileSys&, const IoRequestBase*, core::XFileAsync*, uint32_t)> IoCallBack;

struct IoRequestBase
{
    X_INLINE IoRequest::Enum getType(void) const
    {
        return type;
    }
    template<typename T>
    X_INLINE T* getUserData(void) const
    {
        return reinterpret_cast<T*>(pUserData);
    }

#if X_ENABLE_FILE_STATS
    X_INLINE core::TimeVal getAddTime(void) const
    {
        return addTime;
    }

    X_INLINE void setAddTime(core::TimeVal time)
    {
        addTime = time;
    }
#endif // !X_ENABLE_FILE_STATS

    IoCallBack callback; // 8 bytes
    void* pUserData;

protected:
    IoRequest::Enum type; // 4 bytes

#if X_ENABLE_FILE_STATS
    core::TimeVal addTime;
#endif // !X_ENABLE_FILE_STATS
};

struct IoRequestOpen : public IoRequestBase
{
    IoRequestOpen()
    {
        pUserData = nullptr;
        type = IoRequest::OPEN;
    }

    fileModeFlags mode;
    core::Path<char> path;
};

struct IoRequestOpenRead : public IoRequestOpen
{
    IoRequestOpenRead()
    {
        pUserData = nullptr;
        type = IoRequest::OPEN_READ_ALL;
        arena = nullptr;
        pFile = nullptr;
        pBuf = nullptr;
        dataSize = 0;
    }

    core::MemoryArenaBase* arena; // the arena the buffer to read the whole file into is allocated from.
    // these are only valid if read completed.
    XFileAsync* pFile;
    uint8_t* pBuf;
    uint32_t dataSize;
};

// Takes a buffer and file, and attemps to write it all to a file.
// will then free the data after.
// used for dispatch and forget style writes
// mode is: RECREATE | WRITE
struct IoRequestOpenWrite : public IoRequestBase
{
    IoRequestOpenWrite(core::Array<uint8_t>&& arr) :
        data(std::move(arr))
    {
        pUserData = nullptr;
        type = IoRequest::OPEN_WRITE_ALL;
        pFile = nullptr;
    }

    core::Path<char> path;
    XFileAsync* pFile;
    core::Array<uint8_t> data;
};

struct IoRequestClose : public IoRequestBase
{
    IoRequestClose()
    {
        core::zero_this(this);
        type = IoRequest::CLOSE;
    }

    XFileAsync* pFile;
};

struct IoRequestRead : public IoRequestBase
{
    IoRequestRead()
    {
        core::zero_this(this);
        type = IoRequest::READ;
    }

    XFileAsync* pFile;
    void* pBuf;
    uint64_t offset;   // support files >4gb.
    uint32_t dataSize; // don't support reading >4gb at once.
};

struct IoRequestWrite : public IoRequestBase
{
    IoRequestWrite()
    {
        core::zero_this(this);
        type = IoRequest::WRITE;
    }

    XFileAsync* pFile;
    void* pBuf;
    uint64_t offset;   // support files >4gb.
    uint32_t dataSize; // don't support reading >4gb at once.
};

struct IFileSys
{
    typedef fileMode fileMode;
    typedef Flags<fileMode> fileModeFlags;
    typedef SeekMode SeekMode;
    typedef const char* pathType;
    typedef const wchar_t* pathTypeW;
    typedef _wfinddatai64_t findData;

    static const uintptr_t INVALID_HANDLE = (uintptr_t)-1;

    virtual ~IFileSys() = default;

    virtual void registerVars(void) X_ABSTRACT;
    virtual void registerCmds(void) X_ABSTRACT;

    virtual bool init(const SCoreInitParams& params) X_ABSTRACT;
    virtual bool initWorker(void) X_ABSTRACT;
    virtual void shutDown(void) X_ABSTRACT;

    virtual core::Path<wchar_t> getWorkingDirectory(void) const X_ABSTRACT;

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
    virtual uintptr_t findFirst(pathType path, findData* findinfo) X_ABSTRACT;
    virtual bool findnext(uintptr_t handle, findData* findinfo) X_ABSTRACT;
    virtual void findClose(uintptr_t handle) X_ABSTRACT;

    virtual uintptr_t findFirst2(pathType path, findData& findinfo) X_ABSTRACT;
    virtual bool findnext2(uintptr_t handle, findData& findinfo) X_ABSTRACT;
    virtual void findClose2(uintptr_t handle) X_ABSTRACT;

    // Delete
    virtual bool deleteFile(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
    virtual bool deleteDirectory(pathType path, bool recursive = true) const X_ABSTRACT;
    virtual bool deleteDirectoryContents(pathType path) X_ABSTRACT;

    // Create
    virtual bool createDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
    virtual bool createDirectory(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
    virtual bool createDirectoryTree(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
    virtual bool createDirectoryTree(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;

    // exsists.
    virtual bool fileExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
    virtual bool fileExists(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
    virtual bool directoryExists(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
    virtual bool directoryExists(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;

    // does not error, when it's a file or not exsist.
    virtual bool isDirectory(pathType path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;
    virtual bool isDirectory(pathTypeW path, VirtualDirectory::Enum location = VirtualDirectory::GAME) const X_ABSTRACT;

    // rename
    virtual bool moveFile(pathType path, pathType newPath) const X_ABSTRACT;
    virtual bool moveFile(pathTypeW path, pathTypeW newPath) const X_ABSTRACT;

    // returns the min sector size for all virtual directories.
    // so if game folder is drive A and mod is drive B
    // yet drive B has a larger sector size, it will return the largest.
    virtual size_t getMinimumSectorSize(void) const X_ABSTRACT;

    virtual RequestHandle AddCloseRequestToQue(core::XFileAsync* pFile) X_ABSTRACT;
    virtual RequestHandle AddIoRequestToQue(IoRequestBase& request) X_ABSTRACT;
};

class XFileMemScoped
{
public:
    XFileMemScoped() :
        pFile_(nullptr)
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pFileSys);
        pFileSys_ = gEnv->pFileSys;
    }

    ~XFileMemScoped()
    {
        close();
    }

    inline bool openFile(const char* path, IFileSys::fileModeFlags mode)
    {
        pFile_ = pFileSys_->openFileMem(path, mode);
        return pFile_ != nullptr;
    }

    inline void close(void)
    {
        if (pFile_) {
            pFileSys_->closeFileMem(pFile_);
            pFile_ = nullptr;
        }
    }

    inline operator bool() const
    {
        return pFile_ != nullptr;
    }

    inline bool IsOpen(void) const
    {
        return pFile_ != nullptr;
    }

    inline size_t read(void* pBuf, size_t Len)
    {
        X_ASSERT_NOT_NULL(pFile_); // catch bad use of this class. "not checking open return val"
        return pFile_->read(pBuf, Len);
    }

    template<typename T>
    inline size_t read(T& object)
    {
        return read(&object, sizeof(T));
    }

    template<typename T>
    inline size_t readObj(T& object)
    {
        return read(&object, sizeof(T));
    }

    template<typename T>
    inline size_t readObjs(T* objects, size_t num)
    {
        return read(objects, sizeof(T) * num) / sizeof(T);
    }

    inline size_t readString(core::string& str)
    {
        return pFile_->readString(str);
    }

    inline size_t write(const void* pBuf, size_t Len)
    {
        X_ASSERT_NOT_NULL(pFile_);
        return pFile_->write(pBuf, Len);
    }

    inline size_t writeString(const core::string& str)
    {
        return pFile_->writeString(str);
    }
    inline size_t writeString(const char* str)
    {
        return pFile_->writeString(str);
    }
    inline size_t writeString(const char* str, size_t Length)
    {
        return pFile_->writeString(str, Length);
    }

    inline size_t writeStringNNT(const core::string& str)
    {
        return pFile_->writeStringNNT(str);
    }
    inline size_t writeStringNNT(const char* str)
    {
        return pFile_->writeStringNNT(str);
    }

    template<typename T>
    inline size_t writeObj(T& object)
    {
        return write(&object, sizeof(T));
    }
    template<>
    inline size_t writeObj(const core::string& str)
    {
        return writeString(str);
    }

    template<typename T>
    inline size_t writeObj(const T* objects, size_t num)
    {
        return write(objects, (sizeof(T) * num));
    }

    template<typename T>
    inline size_t writeObjs(T* objects, size_t num)
    {
        return write(objects, sizeof(T) * num) / sizeof(T);
    }

    template<typename T>
    inline size_t write(const T& object)
    {
        return write(&object, sizeof(T));
    }

    size_t printf(const char* fmt, ...)
    {
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

    inline void seek(int64_t position, SeekMode::Enum origin)
    {
        X_ASSERT_NOT_NULL(pFile_);
        pFile_->seek(position, origin);
    }

    inline uint64_t tell(void) const
    {
        X_ASSERT_NOT_NULL(pFile_);
        return pFile_->tell();
    }

    inline void setSize(int64_t numBytes)
    {
        X_ASSERT_NOT_NULL(pFile_);
        return pFile_->setSize(numBytes);
    }

    inline uint64_t remainingBytes(void) const
    {
        X_ASSERT_NOT_NULL(pFile_);
        return pFile_->remainingBytes();
    }

    inline XFileMem* GetFile(void) const
    {
        return pFile_;
    }

    X_INLINE XFileMem* operator->(void)
    {
        return pFile_;
    }
    X_INLINE const XFileMem* operator->(void)const
    {
        return pFile_;
    }
    X_INLINE operator XFileMem*(void)
    {
        return pFile_;
    }
    X_INLINE operator const XFileMem*(void)const
    {
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
    XFileScoped() :
        pFile_(nullptr)
    {
        X_ASSERT_NOT_NULL(gEnv);
        X_ASSERT_NOT_NULL(gEnv->pFileSys);
        pFileSys_ = gEnv->pFileSys;
    }

    ~XFileScoped()
    {
        close();
    }

    inline bool openFile(const char* path, IFileSys::fileModeFlags mode)
    {
        X_ASSERT(pFile_ == nullptr, "File already open")
        ();
        pFile_ = pFileSys_->openFile(path, mode);
        return pFile_ != nullptr;
    }
    inline bool openFile(const wchar_t* path, IFileSys::fileModeFlags mode)
    {
        X_ASSERT(pFile_ == nullptr, "File already open")
        ();
        pFile_ = pFileSys_->openFile(path, mode);
        return pFile_ != nullptr;
    }

    inline void close(void)
    {
        if (pFile_) {
            pFileSys_->closeFile(pFile_);
            pFile_ = nullptr;
        }
    }

    inline operator bool() const
    {
        return pFile_ != nullptr;
    }

    inline bool IsOpen(void) const
    {
        return pFile_ != nullptr;
    }

    inline size_t read(void* pBuf, size_t Len)
    {
        X_ASSERT_NOT_NULL(pFile_); // catch bad use of this class. "not checking open return val"
        return pFile_->read(pBuf, Len);
    }

    template<typename T>
    inline size_t read(T& object)
    {
        return read(&object, sizeof(T));
    }

    template<typename T>
    inline size_t readObj(T& object)
    {
        return read(&object, sizeof(T));
    }

    template<typename T>
    inline size_t readObjs(T* objects, size_t num)
    {
        return read(objects, sizeof(T) * num) / sizeof(T);
    }

    inline size_t readString(core::string& str)
    {
        return pFile_->readString(str);
    }

    inline size_t write(const void* pBuf, size_t Len)
    {
        X_ASSERT_NOT_NULL(pFile_);
        return pFile_->write(pBuf, Len);
    }

    inline size_t writeString(const core::string& str)
    {
        return pFile_->writeString(str);
    }
    inline size_t writeString(const char* str)
    {
        return pFile_->writeString(str);
    }
    inline size_t writeString(const char* str, size_t Length)
    {
        return pFile_->writeString(str, Length);
    }

    inline size_t writeStringNNT(const core::string& str)
    {
        return pFile_->writeStringNNT(str);
    }
    inline size_t writeStringNNT(const char* str)
    {
        return pFile_->writeStringNNT(str);
    }

    template<typename T>
    inline size_t writeObj(T& object)
    {
        return write(&object, sizeof(T));
    }
    template<>
    inline size_t writeObj(const core::string& str)
    {
        return writeString(str);
    }

    template<typename T>
    inline size_t writeObj(const T* objects, size_t num)
    {
        return write(objects, (sizeof(T) * num));
    }

    template<typename T>
    inline size_t writeObjs(T* objects, size_t num)
    {
        return write(objects, sizeof(T) * num) / sizeof(T);
    }

    template<typename T>
    inline size_t write(const T& object)
    {
        return write(&object, sizeof(T));
    }

    size_t printf(const char* fmt, ...)
    {
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

    inline void seek(int64_t position, SeekMode::Enum origin)
    {
        X_ASSERT_NOT_NULL(pFile_);
        pFile_->seek(position, origin);
    }

    inline uint64_t tell(void) const
    {
        X_ASSERT_NOT_NULL(pFile_);
        return pFile_->tell();
    }

    inline void setSize(int64_t numBytes)
    {
        X_ASSERT_NOT_NULL(pFile_);
        return pFile_->setSize(numBytes);
    }

    inline uint64_t remainingBytes(void) const
    {
        X_ASSERT_NOT_NULL(pFile_);
        return pFile_->remainingBytes();
    }

    inline XFile* GetFile(void) const
    {
        return pFile_;
    }

private:
    XFile* pFile_;
    IFileSys* pFileSys_;
};

class FindFirstScoped
{
public:
    X_INLINE FindFirstScoped() :
        handle_(core::IFileSys::INVALID_HANDLE)
    {
        pFileSys_ = gEnv->pFileSys;
    }

    X_INLINE ~FindFirstScoped()
    {
        if (handle_ != core::IFileSys::INVALID_HANDLE) {
            pFileSys_->findClose2(handle_);
        }
    }

    X_INLINE bool findfirst(const char* pPath)
    {
        handle_ = pFileSys_->findFirst2(pPath, fd_);
        return handle_ != core::IFileSys::INVALID_HANDLE;
    }

    X_INLINE bool findNext(void)
    {
        X_ASSERT(handle_ != core::IFileSys::INVALID_HANDLE, "handle is invalid")
        ();
        return pFileSys_->findnext2(handle_, fd_);
    }

    X_INLINE core::IFileSys::findData& fileData(void)
    {
        return fd_;
    }
    X_INLINE const core::IFileSys::findData& fileData(void) const
    {
        return fd_;
    }

    X_INLINE operator bool() const
    {
        return handle_ != core::IFileSys::INVALID_HANDLE;
    }

private:
    core::IFileSys::findData fd_;
    core::IFileSys* pFileSys_;
    uintptr_t handle_;
};

X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_I_H_
