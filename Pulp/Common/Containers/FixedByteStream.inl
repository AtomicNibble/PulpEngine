

X_NAMESPACE_BEGIN(core)

// -------------------------------

inline FixedByteStreamNoneOwningPolicy::FixedByteStreamNoneOwningPolicy(Type* pBegin, Type* pEnd, bool dataInit) :
    FixedByteStreamBase(pBegin, union_cast<size_type>(pEnd - pBegin))
{
    if (dataInit) {
        byteIdx_ = numBytes_;
    }
}

inline FixedByteStreamNoneOwningPolicy::~FixedByteStreamNoneOwningPolicy()
{
}

inline FixedByteStreamOwningPolicy::FixedByteStreamOwningPolicy(core::MemoryArenaBase* arena, size_type numBytes) :
    FixedByteStreamBase(numBytes),
    arena_(arena)
{
    pBegin_ = X_NEW_ARRAY(Type, numBytes, arena_, "FixedByteStream");
}

inline FixedByteStreamOwningPolicy::~FixedByteStreamOwningPolicy()
{
    X_DELETE_ARRAY(pBegin_, arena_);
}

template<size_t N>
inline FixedByteStreamStackPolicy<N>::FixedByteStreamStackPolicy() :
    FixedByteStreamBase(buf_, core::bitUtil::bytesToBits(N))
{
}

template<size_t N>
inline FixedByteStreamStackPolicy<N>::~FixedByteStreamStackPolicy()
{
}

// -------------------------------

inline FixedByteStreamBase::FixedByteStreamBase(size_type numBytes) :
    FixedByteStreamBase(nullptr, numBytes)
{
}

inline FixedByteStreamBase::FixedByteStreamBase(TypePtr pBegin, size_type numBytes) :
    numBytes_(numBytes),
    readByteIdx_(0),
    byteIdx_(0),
    pBegin_(pBegin)
{
}

// -------------------------------------------------------------------------

template<typename T>
inline void FixedByteStreamBase::write(const T& val)
{
    write(reinterpret_cast<const Type*>(&val), sizeof(T));
}

// read the type * num from the stream.
template<typename T>
inline void FixedByteStreamBase::write(const T* pVal, size_type num)
{
    write(reinterpret_cast<const Type*>(pVal), (sizeof(T) * num));
}

inline void FixedByteStreamBase::write(const Type* pBuf, size_type numBytes)
{
    X_ASSERT(numBytes <= freeSpace(), "Tried to write more bytes than avalible space")(numBytes, size(), freeSpace(), isEos());

    std::memcpy(pBegin_ + byteIdx_, pBuf, numBytes);
    byteIdx_ += numBytes;
}

// -------------------------------

template<typename T>
inline T FixedByteStreamBase::read(void)
{
    T val;
    read(reinterpret_cast<Type*>(&val), sizeof(T));
    return val;
}

template<typename T>
inline void FixedByteStreamBase::read(T& val)
{
    read(reinterpret_cast<Type*>(&val), sizeof(T));
}

// read the type * num from the stream.
template<typename T>
inline void FixedByteStreamBase::read(T* pVal, size_type num)
{
    read(reinterpret_cast<Type*>(pVal), (sizeof(T) * num));
}

inline void FixedByteStreamBase::read(Type* pBuf, size_type numBytes)
{
    X_ASSERT(numBytes <= size(), "Tried to read more bytes than avalible")(numBytes, size(), freeSpace(), isEos());

    std::memcpy(pBuf, pBegin_ + readByteIdx_, numBytes);
    readByteIdx_ += numBytes;
}

// -------------------------------

inline void FixedByteStreamBase::skip(size_type numBytes)
{
    X_ASSERT(numBytes <= size(), "Tried to skip more bytes than avalible")(numBytes, size(), freeSpace(), isEos());

    readByteIdx_ += numBytes;
}

inline void FixedByteStreamBase::zeroPadToLength(size_type numBytes)
{
    if (size() < numBytes) {
        X_ASSERT(numBytes <= capacity(), "Tried to pad more than avalible space")(numBytes, size(), freeSpace(), capacity());
        const size_t diff = numBytes - size();

        std::memset(pBegin_ + byteIdx_, 0, diff);
        byteIdx_ += diff;

        X_ASSERT(size() == numBytes, "Failed to pad corect")(size(), numBytes);
    }
}

// -------------------------------------------

inline void FixedByteStreamBase::reset(void)
{
    readByteIdx_ = 0;
    byteIdx_ = 0;
}

inline typename FixedByteStreamBase::size_type FixedByteStreamBase::capacity(void) const
{
    return numBytes_;
}

inline typename FixedByteStreamBase::size_type FixedByteStreamBase::size(void) const
{
    return byteIdx_ - readByteIdx_;
}

inline typename FixedByteStreamBase::size_type FixedByteStreamBase::freeSpace(void) const
{
    return capacity() - byteIdx_;
}

inline bool FixedByteStreamBase::isEos(void) const
{
    return size() == 0;
}

inline bool FixedByteStreamBase::isStartOfStream(void) const
{
    return readByteIdx_ == 0;
}

// --------------------

inline typename FixedByteStreamBase::TypePtr FixedByteStreamBase::ptr(void)
{
    return dataBegin();
}

inline typename FixedByteStreamBase::ConstTypePtr FixedByteStreamBase::ptr(void) const
{
    return dataBegin();
}

inline typename FixedByteStreamBase::TypePtr FixedByteStreamBase::data(void)
{
    return dataBegin();
}

inline typename FixedByteStreamBase::ConstTypePtr FixedByteStreamBase::data(void) const
{
    return dataBegin();
}

inline typename FixedByteStreamBase::Iterator FixedByteStreamBase::begin(void)
{
    return dataBegin();
}

inline typename FixedByteStreamBase::ConstIterator FixedByteStreamBase::begin(void) const
{
    return dataBegin();
}

inline typename FixedByteStreamBase::Iterator FixedByteStreamBase::end(void)
{
    return dataBegin() + size();
}

inline typename FixedByteStreamBase::ConstIterator FixedByteStreamBase::end(void) const
{
    return dataBegin() + size();
}

// --------------------

inline typename FixedByteStreamBase::TypePtr FixedByteStreamBase::dataBegin(void)
{
    return pBegin_;
}

inline typename FixedByteStreamBase::ConstTypePtr FixedByteStreamBase::dataBegin(void) const
{
    return pBegin_;
}

X_NAMESPACE_END