#include "BitStream.h"

template<typename T>
inline void BitStream::write(const T& val)
{
    writeBits(reinterpret_cast<const Type*>(&val), sizeof(T) << 3);
}

// writes the type * num to the stream.
template<typename T>
inline void BitStream::write(const T* pVal, size_type num)
{
    // we call writeBits as write() calls writeBits internally.
    // and writeBits has byte copy optermistations
    writeBits(reinterpret_cast<const Type*>(pVal), (sizeof(T) * num) << 3);
}

inline void BitStream::writeAligned(const Type* pBuf, size_type numBytes)
{
    alignWriteToByteBoundry();
    write(pBuf, numBytes);
}

inline void BitStream::writeBitsAligned(const Type* pBuf, size_type numBits)
{
    alignWriteToByteBoundry();
    writeBits(pBuf, numBits);
}

// --------------------------------------------------------

template<typename T>
T BitStream::read(void)
{
    T val;
    readBits(reinterpret_cast<Type*>(&val), sizeof(T) << 3);
    return val;
}

template<typename T>
inline void BitStream::read(T& val)
{
    readBits(reinterpret_cast<Type*>(&val), sizeof(T) << 3);
}

template<typename T>
inline void BitStream::read(T* pVal, size_type num)
{
    readBits(reinterpret_cast<Type*>(pVal), (sizeof(T) * num) << 3);
}

inline void BitStream::readAligned(Type* pBuf, size_type numBytes)
{
    alignReadToByteBoundry();
    read(pBuf, numBytes);
}

inline void BitStream::readBitsAligned(Type* pBuf, size_type numBits)
{
    alignReadToByteBoundry();
    readBits(pBuf, numBits);
}

// --------------------------------------------------------

inline void BitStream::alignWriteToByteBoundry(void)
{
    bitIdx_ = core::bitUtil::RoundUpToMultiple(bitIdx_, 8_sz);
}

inline void BitStream::alignReadToByteBoundry(void)
{
    readBitIdx_ = core::bitUtil::RoundUpToMultiple(readBitIdx_, 8_sz);
}

// --------------------------------------------------------

// returns how many bits are currently stored in the stream.
inline typename BitStream::size_type BitStream::size(void) const
{
    return bitIdx_ - readBitIdx_;
}

inline typename BitStream::size_type BitStream::sizeInBytes(void) const
{
    return numBytesForBits(size());
}

// returns the capacit

// returns the capacity
inline typename BitStream::size_type BitStream::capacity(void) const
{
    return capacity_;
}

// returns the amount of bits that can be added.
inline typename BitStream::size_type BitStream::freeSpace(void) const
{
    return capacity() - size();
}

// returns true if the stream is full.
inline bool BitStream::isEos(void) const
{
    return size() == capacity();
}

inline typename BitStream::TypePtr BitStream::ptr(void)
{
    return start_;
}

inline typename BitStream::ConstTypePtr BitStream::ptr(void) const
{
    return start_;
}

inline typename BitStream::TypePtr BitStream::data(void)
{
    return start_;
}

inline typename BitStream::ConstTypePtr BitStream::data(void) const
{
    return start_;
}

inline typename BitStream::Iterator BitStream::begin(void)
{
    return start_;
}

inline typename BitStream::ConstIterator BitStream::begin(void) const
{
    return start_;
}

inline typename BitStream::Iterator BitStream::end(void)
{
    return start_ + numBytesForBits(bitIdx_);
}

inline typename BitStream::ConstIterator BitStream::end(void) const
{
    return start_ + numBytesForBits(bitIdx_);
}

// ----------------------------

inline void BitStream::ensureSpace(size_type numBits)
{
    resize(size() + numBits);
}

inline bool BitStream::currentlyOnByteBoundary(void) const
{
    return (bitIdx_ & 7) == 0;
}

inline bool BitStream::currentlyOnReadByteBoundary(void) const
{
    return (readBitIdx_ & 7) == 0;
}

// ----------------------------

inline typename BitStream::size_type BitStream::byteIndex(void) const
{
    return bitIdx_ >> 3;
}

inline typename BitStream::size_type BitStream::readByteIndex(void) const
{
    return readBitIdx_ >> 3;
}

inline typename BitStream::size_type BitStream::numBytesForBits(size_t numBits) const
{
    // align so that all the bits fit.
    return (numBits + 7) >> 3;
}

inline typename BitStream::size_type BitStream::numBitsForBytes(size_t numBytes) const
{
    return numBytes << 3;
}

// for easy memory allocation changes later.
inline void BitStream::Delete(Type* pData) const
{
    X_DELETE_ARRAY(pData, arena_);
}

inline typename BitStream::Type* BitStream::Allocate(size_type numBytes) const
{
    return X_NEW_ARRAY(Type, numBytes, arena_, "BitStream");
}
