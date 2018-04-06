X_NAMESPACE_BEGIN(core)

// -------------------------------

inline FixedByteStreamRingNoneOwningPolicy::FixedByteStreamRingNoneOwningPolicy(Type* pBegin, Type* pEnd, bool dataInit) :
    FixedByteStreamRingBase(pBegin, union_cast<size_type>(pEnd - pBegin))
{
    if (dataInit) {
        byteIdx_ = numBytes_;
    }
}

inline FixedByteStreamRingNoneOwningPolicy::~FixedByteStreamRingNoneOwningPolicy()
{
}

inline FixedByteStreamRingOwningPolicy::FixedByteStreamRingOwningPolicy(core::MemoryArenaBase* arena, size_type numBytes) :
    FixedByteStreamRingBase(numBytes),
    arena_(arena)
{
    pBegin_ = X_NEW_ARRAY(Type, numBytes, arena_, "FixedByteStream");
}

inline FixedByteStreamRingOwningPolicy::~FixedByteStreamRingOwningPolicy()
{
    X_DELETE_ARRAY(pBegin_, arena_);
}

template<size_t N>
inline FixedByteStreamRingStackPolicy<N>::FixedByteStreamRingStackPolicy() :
    FixedByteStreamBase(buf_, core::ByteUtil::bytesToBytes(N))
{
}

template<size_t N>
inline FixedByteStreamRingStackPolicy<N>::~FixedByteStreamRingStackPolicy()
{
}

// -------------------------------

inline FixedByteStreamRingBase::FixedByteStreamRingBase(size_type numBytes) :
    FixedByteStreamRingBase(nullptr, numBytes)
{
}

inline FixedByteStreamRingBase::FixedByteStreamRingBase(TypePtr pBegin, size_type numBytes) :
    numBytes_(numBytes),
    readByteIdx_(0),
    byteIdx_(0),
    pBegin_(pBegin)
{
    X_ASSERT(core::bitUtil::IsPowerOfTwo(numBytes), "Size must be pow2")
    (numBytes);

    mask_ = numBytes - 1;
}

// -------------------------------------------------------------------------

// -------------------------------------------------------------------------

template<typename T>
inline void FixedByteStreamRingBase::write(const T& val)
{
    write(reinterpret_cast<const Type*>(&val), sizeof(T));
}

// read the type * num from the stream.
template<typename T>
inline void FixedByteStreamRingBase::write(const T* pVal, size_type num)
{
    write(reinterpret_cast<const Type*>(pVal), (sizeof(T) * num));
}

inline void FixedByteStreamRingBase::write(const Type* pBuf, size_type numBytes)
{
    X_ASSERT(numBytes <= freeSpace(), "Tried to write more bytes than avalible space")
    (numBytes, size(), freeSpace(), isEos());

    // so we need to handle wrap around :Z
    // which basically means we might need two copy.
    size_type writeIdx = byteIdx_ & mask_;
    size_type bytesToCopy = core::Min(numBytes, numBytes_ - writeIdx);

    std::memcpy(pBegin_ + writeIdx, pBuf, bytesToCopy);

    if (bytesToCopy < numBytes) {
        size_type trailing = numBytes - bytesToCopy;
        std::memcpy(pBegin_, pBuf + bytesToCopy, trailing);
    }

    byteIdx_ += numBytes;
}

// -------------------------------

template<typename T>
inline T FixedByteStreamRingBase::read(void)
{
    T val;
    read(reinterpret_cast<Type*>(&val), sizeof(T));
    return val;
}

template<typename T>
inline void FixedByteStreamRingBase::read(T& val)
{
    read(reinterpret_cast<Type*>(&val), sizeof(T));
}

// read the type * num from the stream.
template<typename T>
inline void FixedByteStreamRingBase::read(T* pVal, size_type num)
{
    read(reinterpret_cast<Type*>(pVal), (sizeof(T) * num));
}

inline void FixedByteStreamRingBase::read(Type* pBuf, size_type numBytes)
{
    X_ASSERT(numBytes <= size(), "Tried to read more bytes than avalible")
    (numBytes, size(), freeSpace(), isEos());

    size_type readIdx = readByteIdx_ & mask_;
    size_type bytesToCopy = core::Min(numBytes, numBytes_ - readIdx);

    std::memcpy(pBuf, pBegin_ + readIdx, bytesToCopy);

    if (bytesToCopy < numBytes) {
        size_type trailing = numBytes - bytesToCopy;
        std::memcpy(pBuf + bytesToCopy, pBegin_, trailing);
    }

    readByteIdx_ += numBytes;
}

// -------------------------------

template<typename T>
inline typename std::enable_if<std::is_trivially_copyable<T>::value && !std::is_reference<T>::value, T>::type
    FixedByteStreamRingBase::peek(void) const
{
    X_ASSERT(sizeof(T) <= size(), "Tried to peek a type bigger than avalible bytes")
    (sizeof(T), size(), freeSpace(), isEos());

    size_type readIdx = readByteIdx_ & mask_;
    size_type bytesToCopy = core::Min(sizeof(T), numBytes_ - readIdx);

    union
    {
        T t;
        Type bytes[sizeof(T)];
    };

    std::memcpy(bytes, pBegin_ + readIdx, bytesToCopy);

    if (bytesToCopy < sizeof(T)) {
        size_type trailing = sizeof(T) - bytesToCopy;
        std::memcpy(bytes + bytesToCopy, pBegin_, trailing);
    }

    return t;
}

// -------------------------------

inline void FixedByteStreamRingBase::skip(size_type numBytes)
{
    X_ASSERT(numBytes <= size(), "Tried to skip more bytes than avalible")
    (numBytes, size(), freeSpace(), isEos());

    readByteIdx_ += numBytes;
}

inline void FixedByteStreamRingBase::zeroPadToLength(size_type numBytes)
{
    if (size() < numBytes) {
        X_ASSERT(numBytes <= capacity(), "Tried to pad more than avalible space")
        (numBytes, size(), freeSpace(), capacity());

        const size_t diff = numBytes - size();

        size_type writeIdx = byteIdx_ & mask_;
        size_type bytesToZero = core::Min(diff, numBytes_ - writeIdx);

        std::memset(pBegin_ + writeIdx, 0, bytesToZero);

        if (bytesToZero < diff) {
            size_type trailing = diff - bytesToZero;
            std::memset(pBegin_, 0, trailing);
        }

        byteIdx_ += numBytes;

        X_ASSERT(size() == numBytes, "Failed to pad corect")
        (size(), numBytes);
    }
}

// clears the stream setting the cursor back to the start.
inline void FixedByteStreamRingBase::reset(void)
{
    readByteIdx_ = byteIdx_ = 0;
}

inline typename FixedByteStreamRingBase::size_type FixedByteStreamRingBase::capacity(void) const
{
    return numBytes_;
}

inline typename FixedByteStreamRingBase::size_type FixedByteStreamRingBase::size(void) const
{
    return byteIdx_ - readByteIdx_;
}

inline typename FixedByteStreamRingBase::size_type FixedByteStreamRingBase::freeSpace(void) const
{
    return capacity() - size();
}

inline bool FixedByteStreamRingBase::isEos(void) const
{
    return byteIdx_ == readByteIdx_;
}

X_NAMESPACE_END