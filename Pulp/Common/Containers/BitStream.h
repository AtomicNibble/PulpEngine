#pragma once

#ifndef _X_CON_BITSTREAM_H_
#define _X_CON_BITSTREAM_H_

X_NAMESPACE_BEGIN(core)

#ifdef GetFreeSpace
#undef GetFreeSpace
#endif

// can stream memory in and out.
class BitStream
{
public:
    typedef uint8_t Type;
    typedef Type value_type;
    typedef Type* TypePtr;
    typedef const Type* ConstTypePtr;
    typedef Type* Iterator;
    typedef const Type* ConstIterator;
    typedef size_t size_type;
    typedef Type& Reference;
    typedef Type& reference;
    typedef const Type& ConstReference;
    typedef const Type& const_reference;

public:
    // constructs the stream no memory is allocated.
    BitStream(MemoryArenaBase* arena);
    BitStream(MemoryArenaBase* arena, size_type numBits);
    BitStream(const BitStream& oth);
    BitStream(BitStream&& oth);
    ~BitStream();

    BitStream& operator=(const BitStream& oth);
    BitStream& operator=(BitStream&& oth);

    // writes a bit to the stream
    void write(bool bit);
    void write0(void);
    void write1(void);

    template<typename T>
    void write(const T& val);
    // writes the type * num to the stream.
    template<typename T>
    void write(const T* pVal, size_type num);

    // writes bytes to stream, will stich in if not currently on byte boundary, unlike writeAligned.
    void write(const Type* pBuf, size_type numBytes);
    // write bits to stream
    void writeBits(const Type* pBuf, size_type numBits);

    // pads the stream to byte boundry before writing.
    void writeAligned(const Type* pBuf, size_type numBytes);
    void writeBitsAligned(const Type* pBuf, size_type numBits);

    // read api
    template<typename T>
    T read(void);

    bool read(void);

    template<typename T>
    void read(T& val);
    // read the type * num from the stream.
    template<typename T>
    void read(T* pVal, size_type num);

    void read(Type* pBuf, size_type numBytes);
    // read bits from stream
    void readBits(Type* pBuf, size_type numBits);

    // pads the stream to byte boundry before read.
    void readAligned(Type* pBuf, size_type numBytes);
    void readBitsAligned(Type* pBuf, size_type numBits);

    // moves forward to next byte boundry, data in remanings bits is undefined.
    void alignWriteToByteBoundry(void);
    void alignReadToByteBoundry(void);

    // sets the absolute bit position in the stream.
    void skipBytes(size_type numBytes);
    void skipBits(size_type numBits);

    // pads the bit stream until the stream length is equal to length.
    // will not trucate.
    void zeroPadToLength(size_type numBytes);

    // resizes the object to holx X bits
    void resize(size_type numBits);
    // clears the stream setting the cursor back to the start.
    // no memory is freed
    void reset(void);
    // resets the cursor and clears all memory.
    void free(void);

    // returns how many bits are currently stored in the stream.
    size_type size(void) const;
    size_type sizeInBytes(void) const;
    // returns how many bits it can store.
    size_type capacity(void) const;
    // returns the amount of bits that can be added.
    size_type freeSpace(void) const;
    // returns true if the stream is full.
    bool isEos(void) const;

    TypePtr ptr(void);
    ConstTypePtr ptr(void) const;
    TypePtr data(void);
    ConstTypePtr data(void) const;

    Iterator begin(void);
    ConstIterator begin(void) const;
    Iterator end(void);
    ConstIterator end(void) const;

private:
    void ensureSpace(size_type numBits);
    bool currentlyOnByteBoundary(void) const;
    bool currentlyOnReadByteBoundary(void) const;

    // for easy memory allocation changes later.
    size_type byteIndex(void) const;
    size_type readByteIndex(void) const;
    size_type numBytesForBits(size_type numBits) const;
    size_type numBitsForBytes(size_type numBytes) const;
    void Delete(Type* pData) const;
    Type* Allocate(size_type numBytes) const;

    size_type capacity_;   // capacity in bits.
    size_type readBitIdx_; // the current read bit index.
    size_type bitIdx_;     // the current bit index.
    Type* start_;

    MemoryArenaBase* arena_;
};

#include "BitStream.inl"

X_NAMESPACE_END

#endif // !_X_CON_BITSTREAM_H_
