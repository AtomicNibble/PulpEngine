

X_NAMESPACE_BEGIN(core)

// -------------------------------

inline FixedBitStreamNoneOwningPolicy::FixedBitStreamNoneOwningPolicy(Type* pBegin, Type* pEnd, bool dataInit) :
    FixedBitStreamBase(pBegin, core::bitUtil::bytesToBits(union_cast<size_type>(pEnd - pBegin)))
{
    if (dataInit) {
        bitIdx_ = numBits_;
    }
}

inline FixedBitStreamNoneOwningPolicy::~FixedBitStreamNoneOwningPolicy()
{
}

inline FixedBitStreamOwningPolicy::FixedBitStreamOwningPolicy(core::MemoryArenaBase* arena, size_type numBits) :
    FixedBitStreamBase(numBits),
    arena_(arena)
{
    size_type numBytes = core::bitUtil::RoundUpToMultiple(numBits, 8_sz);
    pBegin_ = X_NEW_ARRAY(Type, numBytes, arena_, "FixedBitStream");
}

inline FixedBitStreamOwningPolicy::~FixedBitStreamOwningPolicy()
{
    X_DELETE_ARRAY(pBegin_, arena_);
}

template<size_t N>
inline FixedBitStreamStackPolicy<N>::FixedBitStreamStackPolicy() :
    FixedBitStreamBase(buf_, core::bitUtil::bytesToBits(N))
{
}

template<size_t N>
inline FixedBitStreamStackPolicy<N>::~FixedBitStreamStackPolicy()
{
}

// -------------------------------

inline FixedBitStreamBase::FixedBitStreamBase(size_type numBits) :
    numBits_(numBits),
    readBitIdx_(0),
    bitIdx_(0),
    pBegin_(nullptr)
{
}

inline FixedBitStreamBase::FixedBitStreamBase(TypePtr pBegin, size_type numBits) :
    numBits_(numBits),
    readBitIdx_(0),
    bitIdx_(0),
    pBegin_(pBegin)
{
}

// -------------------------------------------------------------------------

inline void FixedBitStreamBase::write(bool bit)
{
    if (bit) {
        write1();
    }
    else {
        write0();
    }
}

inline void FixedBitStreamBase::write0(void)
{
    uint8_t val = 0;
    writeBits(reinterpret_cast<Type*>(&val), 1);
}

inline void FixedBitStreamBase::write1(void)
{
    uint8_t val = 1;
    writeBits(reinterpret_cast<Type*>(&val), 1);
}

template<typename T>
inline void FixedBitStreamBase::write(const T& val)
{
    writeBits(reinterpret_cast<const Type*>(&val), sizeof(T) << 3);
}

template<>
inline void FixedBitStreamBase::write(const bool& val)
{
    write(val);
}

template<size_t N, typename TChar>
inline void FixedBitStreamBase::write(const StackString<N,TChar>& val)
{
    write(safe_static_cast<uint32_t>(val.length()));
    write(val.c_str(), val.length());
}

// read the type * num from the stream.
template<typename T>
inline void FixedBitStreamBase::write(const T* pVal, size_type num)
{
    writeBits(reinterpret_cast<const Type*>(pVal), (sizeof(T) * num) << 3);
}

inline void FixedBitStreamBase::write(const Type* pBuf, size_type numBytes)
{
    writeBits(pBuf, numBitsForBytes(numBytes));
}

// pads the stream to byte boundry before read.
template<typename T>
inline void FixedBitStreamBase::writeAligned(const T& val)
{
    alignWriteToByteBoundry();
    write<T>(val);
}

template<typename T>
inline void FixedBitStreamBase::writeAligned(const T* pVal, size_type num)
{
    alignWriteToByteBoundry();
    write<T>(pVal, num);
}

inline void FixedBitStreamBase::writeAligned(const Type* pBuf, size_type numBytes)
{
    alignWriteToByteBoundry();
    write(pBuf, numBytes);
}

inline void FixedBitStreamBase::writeBitsAligned(const Type* pBuf, size_type numBits)
{
    alignWriteToByteBoundry();
    writeBits(pBuf, numBits);
}

// -------------------------------

template<typename T>
inline T FixedBitStreamBase::read(void)
{
    T val;
    readBits(reinterpret_cast<Type*>(&val), sizeof(T) << 3);
    return val;
}

template<>
inline bool FixedBitStreamBase::read<bool>(void)
{
    return readBool();
}

inline bool FixedBitStreamBase::readBool(void)
{
    uint8_t val = 0;
    readBits(reinterpret_cast<Type*>(&val), 1);
    return val != 0;
}

template<typename T>
inline void FixedBitStreamBase::read(T& val)
{
    readBits(reinterpret_cast<Type*>(&val), sizeof(T) << 3);
}

template<>
inline void FixedBitStreamBase::read(bool& val)
{
    val = readBool();
}

template<size_t N, typename TChar>
inline void FixedBitStreamBase::read(StackString<N, TChar>& val)
{
    val.clear();

    auto len = read<uint32_t>();
    if (len > 0)
    {
        TChar tmp[N];
        read(tmp, len);
        val.set(tmp, tmp + len);
    }
}

// read the type * num from the stream.
template<typename T>
inline void FixedBitStreamBase::read(T* pVal, size_type num)
{
    readBits(reinterpret_cast<Type*>(pVal), (sizeof(T) * num) << 3);
}

inline void FixedBitStreamBase::read(Type* pBuf, size_type numBytes)
{
    readBits(pBuf, numBitsForBytes(numBytes));
}

// pads the stream to byte boundry before read.
// pads the stream to byte boundry before read.
template<typename T>
inline void FixedBitStreamBase::readAligned(T& val)
{
    alignReadToByteBoundry();
    read<T>(val);
}

template<typename T>
inline void FixedBitStreamBase::readAligned(T* pVal, size_type num)
{
    alignReadToByteBoundry();
    read<T>(pVal, num);
}

inline void FixedBitStreamBase::readAligned(Type* pBuf, size_type numBytes)
{
    alignReadToByteBoundry();
    read(pBuf, numBytes);
}

inline void FixedBitStreamBase::readBitsAligned(Type* pBuf, size_type numBits)
{
    alignReadToByteBoundry();
    readBits(pBuf, numBits);
}

// -------------------------------

inline void FixedBitStreamBase::alignWriteToByteBoundry(void)
{
    bitIdx_ = core::bitUtil::RoundUpToMultiple(bitIdx_, 8_sz);
}

inline void FixedBitStreamBase::alignReadToByteBoundry(void)
{
    readBitIdx_ = core::bitUtil::RoundUpToMultiple(readBitIdx_, 8_sz);
}

// -------------------------------

inline void FixedBitStreamBase::skipBytes(size_type numBytes)
{
    readBitIdx_ += numBitsForBytes(numBytes);
}

inline void FixedBitStreamBase::skipBits(size_type numBits)
{
    readBitIdx_ += numBits;
}

inline void FixedBitStreamBase::zeroPadToLength(size_type numBytes)
{
    if (sizeInBytes() < numBytes) {
        alignWriteToByteBoundry();
        // careful we must handle readbitIdx > 0
        size_t bytesToAdd = numBytes - sizeInBytes();

        std::memset(pBegin_ + byteIndex(), 0, bytesToAdd);
        bitIdx_ += numBitsForBytes(bytesToAdd);

        X_ASSERT(sizeInBytes() == numBytes, "Failed to pad correct")(sizeInBytes(), numBytes);
    }
}

// -------------------------------------------

inline void FixedBitStreamBase::reset(void)
{
    bitIdx_ = 0;
    readBitIdx_ = 0;
}

inline typename FixedBitStreamBase::size_type FixedBitStreamBase::capacity(void) const
{
    return numBits_;
}

inline typename FixedBitStreamBase::size_type FixedBitStreamBase::size(void) const
{
    return bitIdx_ - readBitIdx_;
}

inline typename FixedBitStreamBase::size_type FixedBitStreamBase::sizeInBytes(void) const
{
    return numBytesForBits(size());
}

inline typename FixedBitStreamBase::size_type FixedBitStreamBase::freeSpace(void) const
{
    return capacity() - bitIdx_;
}

inline bool FixedBitStreamBase::isEos(void) const
{
    return size() == 0;
}

inline bool FixedBitStreamBase::isStartOfStream(void) const
{
    return readBitIdx_ == 0;
}

// --------------------

inline typename FixedBitStreamBase::TypePtr FixedBitStreamBase::ptr(void)
{
    return dataBegin();
}

inline typename FixedBitStreamBase::ConstTypePtr FixedBitStreamBase::ptr(void) const
{
    return dataBegin();
}

inline typename FixedBitStreamBase::TypePtr FixedBitStreamBase::data(void)
{
    return dataBegin();
}

inline typename FixedBitStreamBase::ConstTypePtr FixedBitStreamBase::data(void) const
{
    return dataBegin();
}

inline typename FixedBitStreamBase::Iterator FixedBitStreamBase::begin(void)
{
    return dataBegin();
}

inline typename FixedBitStreamBase::ConstIterator FixedBitStreamBase::begin(void) const
{
    return dataBegin();
}

inline typename FixedBitStreamBase::Iterator FixedBitStreamBase::end(void)
{
    return dataBegin() + size();
}

inline typename FixedBitStreamBase::ConstIterator FixedBitStreamBase::end(void) const
{
    return dataBegin() + size();
}

// --------------------

inline typename FixedBitStreamBase::size_type FixedBitStreamBase::byteIndex(void) const
{
    return bitIdx_ >> 3;
}

inline typename FixedBitStreamBase::size_type FixedBitStreamBase::readByteIndex(void) const
{
    return readBitIdx_ >> 3;
}

inline typename FixedBitStreamBase::size_type FixedBitStreamBase::numBytesForBits(size_type numBits)
{
    // align so that all the bits fit.
    return (numBits + 7) >> 3;
}

inline typename FixedBitStreamBase::size_type FixedBitStreamBase::numBitsForBytes(size_type numBytes)
{
    return numBytes << 3;
}

inline typename FixedBitStreamBase::TypePtr FixedBitStreamBase::dataBegin(void)
{
    return pBegin_;
}

inline typename FixedBitStreamBase::ConstTypePtr FixedBitStreamBase::dataBegin(void) const
{
    return pBegin_;
}

// -------------------------------

// -------------------------------

X_NAMESPACE_END