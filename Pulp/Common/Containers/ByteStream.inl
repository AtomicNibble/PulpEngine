#include "ByteStream.h"

ByteStream::ByteStream(MemoryArenaBase* arena) :
    read_(nullptr),
    write_(nullptr),
    start_(nullptr),
    end_(nullptr),
    arena_(X_ASSERT_NOT_NULL(arena))
{
}

ByteStream::ByteStream(const ByteStream& oth)
{
    read_ = nullptr;
    write_ = nullptr;
    start_ = nullptr;
    end_ = nullptr;
    arena_ = oth.arena_;

    reallocate(oth.capacity());

    ::memcpy(start_, oth.start_, oth.size());

    read_ = start_ + (oth.read_ - oth.start_);
    write_ = start_ + (oth.write_ - oth.start_);
}

ByteStream::ByteStream(ByteStream&& oth)
{
    arena_ = oth.arena_;
    read_ = oth.read_;
    write_ = oth.write_;
    start_ = oth.start_;
    end_ = oth.end_;

    // clear other.
    oth.read_ = nullptr;
    oth.write_ = nullptr;
    oth.start_ = nullptr;
    oth.end_ = nullptr;
}

ByteStream::~ByteStream()
{
    free();
}

ByteStream& ByteStream::operator=(const ByteStream& oth)
{
    if (this != &oth) {
        free();

        arena_ = oth.arena_;

        reallocate(oth.capacity());

        ::memcpy(start_, oth.start_, oth.size());

        read_ = start_ + (oth.read_ - oth.start_);
        write_ = start_ + (oth.write_ - oth.start_);
    }
    return *this;
}

ByteStream& ByteStream::operator=(ByteStream&& oth)
{
    if (this != &oth) {
        free();

        // steal buffer.
        arena_ = oth.arena_;
        read_ = oth.read_;
        write_ = oth.write_;
        start_ = oth.start_;
        end_ = oth.end_;

        // clear other.
        oth.read_ = nullptr;
        oth.write_ = nullptr;
        oth.start_ = nullptr;
        oth.end_ = nullptr;
    }
    return *this;
}

template<typename T>
inline void ByteStream::write(const T& val)
{
    write(reinterpret_cast<const Type*>(&val), sizeof(T));
}

template<typename T>
inline void ByteStream::write(const T* pVal, size_type num)
{
    write(reinterpret_cast<const Type*>(pVal), num * sizeof(T));
}

inline void ByteStream::write(const Type* pBuf, size_type numBytes)
{
    ensureSpace(numBytes);

    X_ASSERT(numBytes <= freeSpace(), "Not enougth space")(numBytes, freeSpace());

    ::memcpy(write_, pBuf, numBytes);
    write_ += numBytes;
}

inline void ByteStream::write(const ByteStream& stream)
{
    size_type streamSize = stream.size();
    if (streamSize > 0) {
        write(stream.read_, streamSize);
    }
}

template<typename T>
inline T ByteStream::read(void)
{
    T val;
    read(reinterpret_cast<Type*>(&val), sizeof(T));
    return val;
}

template<typename T>
inline void ByteStream::read(T& val)
{
    read(reinterpret_cast<Type*>(&val), sizeof(T));
}

template<typename T>
inline void ByteStream::read(T* pVal, size_type num)
{
    read(reinterpret_cast<Type*>(pVal), num * sizeof(T));
}

inline void ByteStream::read(Type* pBuf, size_type numBytes)
{
    X_ASSERT(read_ + numBytes <= write_, "can't read buffer of size: %" PRIuS, numBytes)(numBytes, read_, write_);

    ::memcpy(pBuf, read_, numBytes);
    read_ += numBytes;
}

template<typename T>
inline T ByteStream::peek(void) const
{
    X_ASSERT(sizeof(T) <= size(), "can't peek a object of size: %" PRIuS, sizeof(T))(sizeof(T), size());

    return *union_cast<T*, Type*>(read_);
}

inline void ByteStream::alignWrite(size_t alignment)
{
    const size_t currentSize = size();
    if ((currentSize % alignment) != 0) {
        const size_t pad = core::bitUtil::RoundUpToMultiple(currentSize, alignment) - currentSize;

        ensureSpace(pad);

        ::memset(write_, 0xFF, pad);
        write_ += pad;
    }
}

inline void ByteStream::seek(size_type pos)
{
    X_ASSERT(pos < size(), "can't seek that far")(pos, size());
    read_ = (start_ + pos);
}

// resizes the object
inline void ByteStream::reserve(size_type numBytes)
{
    reallocate(numBytes);
}

// clears the stream setting the cursor back to the start.
// no memory is freed
inline void ByteStream::reset(void)
{
    read_ = write_ = start_;
}

// resets the cursor and clears all memory.
inline void ByteStream::free(void)
{
    if (start_) { // memory to free.
        Delete(start_);
    }
    read_ = write_ = start_ = end_ = nullptr;
}

// returns how many bytes are currently stored in the stream.
inline typename ByteStream::size_type ByteStream::size(void) const
{
    return union_cast<size_type>(write_ - read_);
}

// returns the capacity of the byte stream.
inline typename ByteStream::size_type ByteStream::capacity(void) const
{
    return union_cast<size_type>(end_ - start_);
}

// returns the amount of bytes that can be added.
inline typename ByteStream::size_type ByteStream::freeSpace(void) const
{
    return union_cast<size_type>(end_ - write_);
}

inline typename ByteStream::size_type ByteStream::tell(void) const
{
    return union_cast<size_type>(read_ - start_);
}

inline typename ByteStream::size_type ByteStream::tellWrite(void) const
{
    return union_cast<size_type>(write_ - start_);
}

// returns true if the stream is full.
inline bool ByteStream::isEos(void) const
{
    return write_ == end_;
}

inline typename ByteStream::TypePtr ByteStream::ptr(void)
{
    return read_;
}

inline typename ByteStream::ConstTypePtr ByteStream::ptr(void) const
{
    return read_;
}

inline typename ByteStream::TypePtr ByteStream::data(void)
{
    return read_;
}

inline typename ByteStream::ConstTypePtr ByteStream::data(void) const
{
    return read_;
}

inline typename ByteStream::Iterator ByteStream::begin(void)
{
    return read_;
}

inline typename ByteStream::ConstIterator ByteStream::begin(void) const
{
    return read_;
}

inline typename ByteStream::Iterator ByteStream::end(void)
{
    return write_;
}

inline typename ByteStream::ConstIterator ByteStream::end(void) const
{
    return write_;
}

inline typename ByteStream::Reference ByteStream::front(void)
{
    return *begin();
}

inline typename ByteStream::ConstReference ByteStream::front(void) const
{
    return *begin();
}

inline typename ByteStream::Reference ByteStream::back(void)
{
    return *end();
}

inline typename ByteStream::ConstReference ByteStream::back(void) const
{
    return *end();
}

inline void ByteStream::ensureSpace(size_type desiredSpace)
{
    const size_type space = freeSpace();
    // num is how much space we want.
    if (desiredSpace > space) {
        // which ever bigger.
        const size_type cap = capacity();
        const size_type newSize = core::Max(cap * 2, cap + (desiredSpace - space));

        reallocate(newSize);
    }
}

inline void ByteStream::reallocate(size_type newSize)
{
    if (newSize < capacity() || newSize == 0) {
        return;
    }

    const size_type readOffset = union_cast<size_type>(read_ - start_);
    const size_type writeOffset = union_cast<size_type>(write_ - start_);

    Type* pOld = start_;
    start_ = Allocate(newSize);

    // copy old over.
    if (pOld) {
        ::memcpy(start_, pOld, writeOffset); // copy from base to write offset.
        Delete(pOld);

        read_ = start_ + readOffset;
        write_ = start_ + writeOffset;
    }
    else {
        write_ = read_ = start_;
    }

    end_ = start_ + newSize;
}

// for easy memory allocation changes later.
inline void ByteStream::Delete(Type* pData) const
{
    X_DELETE_ARRAY(pData, arena_);
}

inline typename ByteStream::Type* ByteStream::Allocate(size_type num) const
{
    return X_NEW_ARRAY(Type, num, arena_, "ByteStream");
}