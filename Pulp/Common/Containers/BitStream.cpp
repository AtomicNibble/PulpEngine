#include <EngineCommon.h>
#include "BitStream.h"

X_NAMESPACE_BEGIN(core)

BitStream::BitStream(MemoryArenaBase* arena) :
    bitIdx_(0),
    readBitIdx_(0),
    capacity_(0),
    start_(0),
    arena_(arena)
{
    X_ASSERT_NOT_NULL(arena);
}

BitStream::BitStream(MemoryArenaBase* arena, size_type numBits) :
    bitIdx_(0),
    readBitIdx_(0),
    capacity_(0),
    start_(0),
    arena_(arena)
{
    X_ASSERT_NOT_NULL(arena);
    resize(numBits);
}

BitStream::BitStream(const BitStream& oth)
{
    // use it's arena.
    arena_ = oth.arena_;

    resize(oth.capacity_);

    // copy stored bits.
    size_type numBytes = numBytesForBits(oth.bitIdx_);
    ::memcpy(start_, oth.start_, numBytes);

    bitIdx_ = oth.bitIdx_;
    readBitIdx_ = oth.readBitIdx_;
}

BitStream::BitStream(BitStream&& oth)
{
    capacity_ = oth.capacity_;
    bitIdx_ = oth.bitIdx_;
    readBitIdx_ = oth.readBitIdx_;
    start_ = oth.start_;
    arena_ = oth.arena_;

    // clear oth.
    oth.capacity_ = 0;
    oth.bitIdx_ = 0;
    oth.readBitIdx_ = 0;
    oth.start_ = nullptr;
}

BitStream::~BitStream()
{
    free();
}

BitStream& BitStream::operator=(const BitStream& oth)
{
    if (this != &oth) {
        // free and re allocat using oth's arena.
        free();
        arena_ = oth.arena_;

        resize(oth.capacity_);

        // copy stored bits.
        size_type numBytes = numBytesForBits(oth.bitIdx_);
        ::memcpy(start_, oth.start_, numBytes);

        readBitIdx_ = oth.readBitIdx_;
        bitIdx_ = oth.bitIdx_;
    }
    return *this;
}

BitStream& BitStream::operator=(BitStream&& oth)
{
    if (this != &oth) {
        free();

        capacity_ = oth.capacity_;
        readBitIdx_ = oth.readBitIdx_;
        bitIdx_ = oth.bitIdx_;
        start_ = oth.start_;
        arena_ = oth.arena_;

        // clear oth.
        oth.capacity_ = 0;
        oth.readBitIdx_ = 0;
        oth.bitIdx_ = 0;
        oth.start_ = nullptr;
    }
    return *this;
}

// writes a bit to the stream
void BitStream::write(bool bit)
{
    if (bit) {
        write0();
    }
    else {
        write1();
    }
}

void BitStream::write0(void)
{
    ensureSpace(1);

    size_type byteIdx = bitIdx_ >> 3; // val / 8
    size_type bitsMod8 = bitIdx_ & 7;

    if (bitsMod8) {
        start_[byteIdx] = 0x0;
    }
    else {
        // if we zero new bytes always we can actually skip this.
        // if we don't allow seeking to specific bits :/
        start_[byteIdx] &= ~(0x80 >> bitsMod8);
    }

    bitIdx_++;
}

void BitStream::write1(void)
{
    ensureSpace(1);

    size_type byteIdx = bitIdx_ >> 3; // val / 8
    size_type bitsMod8 = bitIdx_ & 7;

    if (bitsMod8) {
        start_[byteIdx] = 0x80;
    }
    else {
        start_[byteIdx] |= 0x80 >> bitsMod8;
    }

    bitIdx_++;
}

void BitStream::write(const Type* pBuf, size_type numBytes)
{
    if (!numBytes) {
        return;
    }

    // if we on byte boundary we can just memcpy!
    // potentially it might be faster to directly call writeBits, save a branch. :| (if inlined)
    if (currentlyOnByteBoundary()) {
        size_type numBits = numBitsForBytes(numBytes);

        ensureSpace(numBits);
        std::memcpy(start_ + byteIndex(), pBuf, numBytes);

        bitIdx_ += numBits;
    }
    else {
        // rip.
        writeBits(pBuf, numBitsForBytes(numBytes));
    }
}

void BitStream::writeBits(const Type* pBuf, size_type numBits)
{
    if (!numBits) {
        return;
    }

    ensureSpace(numBits);

    size_type bitsMod8 = bitIdx_ & 7;
    size_type srcTrailingBits = numBits & 7;

    // copy full bytes if we can.
    // trailing bits are handled after.
    if (bitsMod8 == 0 && numBits >= 8) {
        size_type bitsToCopy = numBits - srcTrailingBits;
        std::memcpy(start_ + byteIndex(), pBuf, numBytesForBits(bitsToCopy));
        bitIdx_ += bitsToCopy;

        if (srcTrailingBits == 0) {
            return; // early out on whole byte src.
        }

        numBits -= bitsToCopy;
        pBuf += bitsToCopy;
    }

    // okay now we are left with data we can't do simple byte copy on.
    // shift away!
    while (numBits >= 8) {
        Type* pDst = start_ + byteIndex();
        Type srcByte = *(pBuf++);

        // we place first part of srcbyte, is last bit of dst byte.
        *(pDst) |= (srcByte >> bitsMod8) & 0xff;

        // we shift lower bits up to start of dst byte.
        *(pDst + 1) |= (srcByte << (8 - bitsMod8)) & 0xff;

        numBits -= 8;
        bitIdx_ += 8;
    }

    if (numBits) {
        // now we only have a few bits left.
        // we need to shift them up
        Type* pDst = start_ + byteIndex();
        Type srcByte = *pBuf;

        *(pDst) |= (srcByte << (8 - bitsMod8)) & 0xff;

        bitIdx_ += numBits;
    }
}

// removes and returns the top bit off the stream.
bool BitStream::read(void)
{
    // YOU WANT A BIT?
    // you can have a slap!
    uint8_t bit;
    readBits(&bit, 1);

    return bit != 0;
}

void BitStream::read(Type* pBuf, size_type numBytes)
{
    if (!numBytes) {
        return;
    }

    // if we on byte boundary we can just memcpy!
    if (currentlyOnReadByteBoundary()) {
        size_type numBits = numBitsForBytes(numBytes);

        ensureSpace(numBits);
        std::memcpy(start_ + readByteIndex(), pBuf, numBytes);

        readBitIdx_ += numBits;
    }
    else {
        readBits(pBuf, numBitsForBytes(numBytes));
    }
}

// read bits from stream
void BitStream::readBits(Type* pBuf, size_type numBits)
{
    // kill yourself.
    if (!numBits) {
        return;
    }

    size_type bitsMod8 = readBitIdx_ & 7;
    size_type srcTrailingBits = numBits & 7;

    // copy full bytes if we can.
    // trailing bits are handled after.
    if (bitsMod8 == 0 && numBits >= 8) {
        size_type bitsToCopy = numBits - srcTrailingBits;
        std::memcpy(pBuf, start_ + readByteIndex(), numBytesForBits(bitsToCopy));
        readBitIdx_ += bitsToCopy;

        if (srcTrailingBits == 0) {
            return;
        }

        numBits -= bitsToCopy;
        pBuf += bitsToCopy;
    }

    while (numBits >= 8) {
        Type srcByte = *(start_ + readByteIndex());
        Type* pDst = pBuf++;

        *(pDst) |= (srcByte >> bitsMod8) & 0xff;
        *(pDst + 1) |= (srcByte << (8 - bitsMod8)) & 0xff;

        numBits -= 8;
        readBitIdx_ += 8;
    }

    if (numBits) {
        Type srcByte = *(start_ + readByteIndex());
        Type* pDst = pBuf;

        *(pDst) |= (srcByte << (8 - bitsMod8)) & 0xff;

        readBitIdx_ += numBits;
    }
}

void BitStream::zeroPadToLength(size_type numBytes)
{
    if (sizeInBytes() < numBytes) {
        const size_t numBits = numBitsForBytes(numBytes);
        alignWriteToByteBoundry();
        ensureSpace(numBits);
        std::memset(start_ + byteIndex(), 0, numBytes);
        bitIdx_ += numBits;
    }
}

// sets the absolute position in the stream.
void BitStream::skipBytes(size_type numBytes)
{
    readBitIdx_ += numBitsForBytes(numBytes);
}

void BitStream::skipBits(size_type numBits)
{
    readBitIdx_ += numBits;
}

// resizes the object
void BitStream::resize(size_type numBits)
{
    if (numBits > capacity()) {
        // save local copy of old array and it's size.
        Type* pOld = start_;
        size_type bytesAllocated = numBytesForBits(numBits);

        // allocate the new one.
        start_ = Allocate(bytesAllocated);

        // copy over.
        if (pOld) {
            ::memcpy(start_, pOld, bytesAllocated);
            Delete(pOld);
        }

        // update capacity.
        capacity_ = numBits;
    }
}

void BitStream::reset(void)
{
    bitIdx_ = 0;
    readBitIdx_ = 0;
}
// resets the cursor and clears all memory.
void BitStream::free(void)
{
    if (start_) {
        Delete(start_);
    }
    start_ = nullptr;
    bitIdx_ = readBitIdx_ = capacity_ = 0;
}

X_NAMESPACE_END