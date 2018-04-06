#include <EngineCommon.h>
#include "FixedBitStream.h"

X_NAMESPACE_BEGIN(core)

void FixedBitStreamBase::writeBits(const Type* pBuf, size_type numBits)
{
    if (!numBits) {
        return;
    }

    X_ASSERT(numBits <= freeSpace(), "Tried to write more bits than avalible space")(numBits, size(), freeSpace(), isEos());

    size_type bitsMod8 = bitIdx_ & 7;
    size_type srcTrailingBits = numBits & 7;

    // copy full bytes if we can.
    // trailing bits are handled after.
    if (bitsMod8 == 0) {
        if (numBits >= 8) {
            size_type bitsToCopy = numBits - srcTrailingBits;
            size_type bytesToCopy = numBytesForBits(bitsToCopy);
            std::memcpy(dataBegin() + byteIndex(), pBuf, bytesToCopy);
            bitIdx_ += bitsToCopy;

            if (srcTrailingBits == 0) {
                return; // early out on whole byte src.
            }

            numBits -= bitsToCopy;
            pBuf += bytesToCopy;
        }

        // any trailing bits.
        X_ASSERT(numBits > 0, "SHould early out before if no bits")(numBits);

        // okay so just one byte left and we on a boundary.
        // just flip the byte to be left aligned.
        // and assign.
        Type* pDst = dataBegin() + byteIndex();
        Type srcByte = *pBuf;

        srcByte <<= 8 - numBits;
        *(pDst) = srcByte & 0xff;

        bitIdx_ += numBits;
        return;
    }

    X_ASSERT(bitsMod8 != 0, "Logic here is for none byte aligned writes")(bitsMod8);

    // okay now we are left with data we can't do simple byte copy on.
    // shift away!
    while (numBits >= 8) {
        Type* pDst = dataBegin() + byteIndex();
        Type srcByte = *(pBuf++);

        // we place first part of srcbyte, is last bit of dst byte.
        *(pDst) |= (srcByte >> bitsMod8) & 0xff;

        // we shift lower bits up to start of dst byte.
        // assign tho, as this byte is currently not init.
        *(pDst + 1) = (srcByte << (8 - bitsMod8)) & 0xff;

        numBits -= 8;
        bitIdx_ += 8;
    }

    // handle last few bits
    if (numBits) {
        Type* pDst = dataBegin() + byteIndex();
        Type srcByte = *pBuf;

        // shift them over.
        srcByte <<= 8 - numBits;

        // now we need to split what ever is left across the boundry.
        // it may not actually coross the boundry tho.
        *(pDst) |= (srcByte >> bitsMod8) & 0xff;

        if ((8 - bitsMod8) < numBits) {
            *(pDst + 1) = (srcByte << (8 - bitsMod8)) & 0xff;
        }

        bitIdx_ += numBits;
    }
}

void FixedBitStreamBase::readBits(Type* pBuf, size_type numBits)
{
    if (!numBits) {
        return;
    }

    X_ASSERT(numBits <= size(), "Tried to read more bits than avalible")(numBits, size(), freeSpace(), isEos());

    size_type bitsMod8 = readBitIdx_ & 7;
    size_type srcTrailingBits = numBits & 7;

    // copy full bytes if we can.
    // trailing bits are handled after.
    if (bitsMod8 == 0) {
        if (numBits >= 8) {
            size_type bitsToCopy = numBits - srcTrailingBits;
            size_type bytesToCopy = numBytesForBits(bitsToCopy);

            std::memcpy(pBuf, pBegin_ + readByteIndex(), bytesToCopy);
            readBitIdx_ += bitsToCopy;

            if (srcTrailingBits == 0) {
                return;
            }

            numBits -= bitsToCopy;
            pBuf += bytesToCopy;
        }

        X_ASSERT(numBits > 0, "SHould early out before if no bits")(numBits);

        Type srcByte = *(pBegin_ + readByteIndex());

        // un flip it.
        srcByte >>= 8 - numBits;
        *(pBuf) = srcByte & 0xff;

        readBitIdx_ += numBits;
        return;
    }

    X_ASSERT(bitsMod8 != 0, "Logic here is for none byte aligned writes")(bitsMod8);

    while (numBits >= 8) {
        Type* pSrc = (pBegin_ + readByteIndex());
        Type* pDst = pBuf++;

        // this is basically a sliding window across a byte boundry.
        // and we take trailng bits from current byte and leading from next.
        Type trailing = (*pSrc << bitsMod8) & 0xFF;
        Type leading = (*(pSrc + 1) >> (8 - bitsMod8));
        Type constructed = trailing | leading;

        *pDst = constructed;

        numBits -= 8;
        readBitIdx_ += 8;
    }

    if (numBits) {
        Type* pSrc = (pBegin_ + readByteIndex());
        Type* pDst = pBuf;

        // bits from current byte
        *(pDst) |= (*pSrc << (bitsMod8)) & 0xff;

        // do we need to get some from next byte?
        if (numBits > (8 - bitsMod8)) {
            // shift them down
            *(pDst) |= (*(pSrc + 1) >> (8 - bitsMod8));
        }

        *(pDst) >>= 8 - numBits;

        readBitIdx_ += numBits;
    }
}

X_NAMESPACE_END