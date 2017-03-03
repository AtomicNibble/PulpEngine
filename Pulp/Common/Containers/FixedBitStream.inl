

X_NAMESPACE_BEGIN(core)


// -------------------------------


inline FixedBitStreamNoneOwningPolicy::FixedBitStreamNoneOwningPolicy(Type* pBegin, Type* pEnd, bool dataInit) :
	FixedStreamBase(pBegin, core::bitUtil::bytesToBits(union_cast<size_type>(pEnd - pBegin)))
{
	if (dataInit) {
		bitIdx_ = numBits_;
	}
}

inline FixedBitStreamNoneOwningPolicy::~FixedBitStreamNoneOwningPolicy()
{

}

inline FixedBitStreamOwningPolicy::FixedBitStreamOwningPolicy(core::MemoryArenaBase* arena, size_type numBits) :
	FixedStreamBase(numBits),
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
	FixedStreamBase(buf_, core::bitUtil::bytesToBits(N))
{
}

template<size_t N>
inline FixedBitStreamStackPolicy<N>::~FixedBitStreamStackPolicy() 
{

}


// -------------------------------


inline FixedStreamBase::FixedStreamBase(size_type numBits) :
	pBegin_(nullptr),
	numBits_(numBits),
	readBitIdx_(0),
	bitIdx_(0)
{

}

inline FixedStreamBase::FixedStreamBase(TypePtr pBegin, size_type numBits) :
	pBegin_(pBegin),
	numBits_(numBits),
	readBitIdx_(0),
	bitIdx_(0)
{

}

// -------------------------------------------------------------------------

inline void FixedStreamBase::write(bool bit)
{
	if (bit) {
		write0();
	}
	else {
		write1();
	}
}

inline void FixedStreamBase::write0(void)
{
	uint8_t val = 0;
	readBits(reinterpret_cast<Type*>(&val), 1);
}

inline void FixedStreamBase::write1(void)
{
	uint8_t val = 1;
	readBits(reinterpret_cast<Type*>(&val), 1);
}

template<typename T>
inline void FixedStreamBase::write(const T& val)
{
	writeBits(reinterpret_cast<const Type*>(&val), sizeof(T) << 3);
}

// read the type * num from the stream.
template<typename T>
inline void FixedStreamBase::write(const T* pVal, size_type num)
{
	writeBits(reinterpret_cast<const Type*>(pVal), (sizeof(T) * num) << 3);
}

inline void FixedStreamBase::write(const Type* pBuf, size_type numBytes)
{
	writeBits(pBuf, numBitsForBytes(numBytes));
}

// read bits from stream
inline void FixedStreamBase::writeBits(const Type* pBuf, size_type numBits)
{
	if (!numBits) {
		return;
	}

	X_ASSERT(numBits <= freeSpace(), "Tried to write more bits than avalible space")(numBits, size(), freeSpace(), isEos());


	size_type bitsMod8 = bitIdx_ & 7;
	size_type srcTrailingBits = numBits & 7;

	// copy full bytes if we can.
	// trailing bits are handled after.
	if (bitsMod8 == 0)
	{
		if (numBits >= 8)
		{
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
	while (numBits >= 8)
	{
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
	if (numBits)
	{
		Type* pDst = dataBegin() + byteIndex();
		Type srcByte = *pBuf;

		// shift them over.
		srcByte <<= 8 - numBits;

		// now we need to split what ever is left across the boundry.
		// it may not actually coross the boundry tho.
		*(pDst) |= (srcByte >> bitsMod8) & 0xff;

		if ((8 - bitsMod8) < numBits)
		{
			*(pDst + 1) = (srcByte << (8 - bitsMod8)) & 0xff;
		}

		bitIdx_ += numBits;
	}
}

// pads the stream to byte boundry before read.
inline void FixedStreamBase::writeAligned(const Type* pBuf, size_type numBytes)
{
	alignWriteToByteBoundry();
	write(pBuf, numBytes);
}

inline void FixedStreamBase::writeBitsAligned(const Type* pBuf, size_type numBits)
{
	alignWriteToByteBoundry();
	writeBits(pBuf, numBits);
}


// -------------------------------

template<typename T>
inline T FixedStreamBase::read(void)
{
	T val;
	readBits(reinterpret_cast<Type*>(&val), sizeof(T) << 3);
	return val;
}

template<typename T>
inline void FixedStreamBase::read(T& val)
{
	readBits(reinterpret_cast<Type*>(&val), sizeof(T) << 3);
}

// read the type * num from the stream.
template<typename T>
inline void FixedStreamBase::read(T* pVal, size_type num)
{
	readBits(reinterpret_cast<Type*>(pVal), (sizeof(T) * num) << 3);
}

inline void FixedStreamBase::read(Type* pBuf, size_type numBytes)
{
	readBits(pBuf, numBitsForBytes(numBytes));
}

// read bits from stream
inline void FixedStreamBase::readBits(Type* pBuf, size_type numBits)
{
	if (!numBits) {
		return;
	}

	X_ASSERT(numBits <= size(), "Tried to read more bits than avalible")(numBits, size(), freeSpace(), isEos());

	size_type bitsMod8 = readBitIdx_ & 7;
	size_type srcTrailingBits = numBits & 7;

	// copy full bytes if we can.
	// trailing bits are handled after.
	if (bitsMod8 == 0)
	{
		if (numBits >= 8)
		{
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

	while (numBits >= 8)
	{
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

	if (numBits)
	{
		Type* pSrc = (pBegin_ + readByteIndex());
		Type* pDst = pBuf;

		// bits from current byte
		*(pDst) |= (*pSrc << (bitsMod8)) & 0xff;

		// do we need to get some from next byte?
		if (numBits > (8 - bitsMod8))
		{
			// shift them down
			*(pDst) |= (*(pSrc + 1) >> (8 - bitsMod8));
		}

		*(pDst) >>= 8 - numBits;

		readBitIdx_ += numBits;
	}
}

// pads the stream to byte boundry before read.
inline void FixedStreamBase::readAligned(Type* pBuf, size_type numBytes)
{
	alignReadToByteBoundry();
	read(pBuf, numBytes);
}

inline void FixedStreamBase::readBitsAligned(Type* pBuf, size_type numBits)
{
	alignReadToByteBoundry();
	readBits(pBuf, numBits);
}


// -------------------------------

inline void FixedStreamBase::alignWriteToByteBoundry(void)
{
	bitIdx_ = core::bitUtil::RoundUpToMultiple(bitIdx_, 8_sz);
}


inline void FixedStreamBase::alignReadToByteBoundry(void)
{
	readBitIdx_ = core::bitUtil::RoundUpToMultiple(readBitIdx_, 8_sz);
}

// -------------------------------

inline void FixedStreamBase::skipBytes(size_type numBytes)
{
	readBitIdx_ += numBitsForBytes(numBytes);
}

inline void FixedStreamBase::skipBits(size_type numBits)
{
	readBitIdx_ += numBits;
}

inline void FixedStreamBase::zeroPadToLength(size_type numBytes)
{
	if (sizeInBytes() < numBytes)
	{
		const size_t numBits = numBitsForBytes(numBytes);
		alignWriteToByteBoundry();
		std::memset(pBegin_ + byteIndex(), 0, numBytes);
		bitIdx_ += numBits;
	}
}

// -------------------------------------------

inline void FixedStreamBase::reset(void)
{
	bitIdx_ = 0;
	readBitIdx_ = 0;
}

inline typename FixedStreamBase::size_type typename FixedStreamBase::capacity(void) const
{
	return numBits_;
}


inline typename FixedStreamBase::size_type typename FixedStreamBase::size(void) const
{
	return bitIdx_ - readBitIdx_;
}

inline typename FixedStreamBase::size_type typename FixedStreamBase::sizeInBytes(void) const
{
	return numBytesForBits(size());
}

inline typename FixedStreamBase::size_type FixedStreamBase::freeSpace(void) const
{
	return capacity() - bitIdx_;
}

inline bool FixedStreamBase::isEos(void) const
{
	return size() == 0;
}


// --------------------

inline typename FixedStreamBase::TypePtr FixedStreamBase::ptr(void)
{
	return dataBegin();
}

inline typename FixedStreamBase::ConstTypePtr FixedStreamBase::ptr(void) const
{
	return dataBegin();
}

inline typename FixedStreamBase::TypePtr FixedStreamBase::data(void)
{
	return dataBegin();
}

inline typename FixedStreamBase::ConstTypePtr FixedStreamBase::data(void) const
{
	return dataBegin();
}

inline typename FixedStreamBase::Iterator FixedStreamBase::begin(void)
{
	return dataBegin();
}

inline typename FixedStreamBase::ConstIterator FixedStreamBase::begin(void) const
{
	return dataBegin();
}

inline typename FixedStreamBase::Iterator FixedStreamBase::end(void)
{
	return dataBegin() + size();
}

inline typename FixedStreamBase::ConstIterator FixedStreamBase::end(void) const
{
	return dataBegin() + size();
}

// --------------------

inline typename FixedStreamBase::size_type FixedStreamBase::byteIndex(void) const
{
	return bitIdx_ >> 3;
}

inline typename FixedStreamBase::size_type FixedStreamBase::readByteIndex(void) const
{
	return readBitIdx_ >> 3;
}

inline typename FixedStreamBase::size_type FixedStreamBase::numBytesForBits(size_type numBits)
{
	// align so that all the bits fit.
	return (numBits + 7) >> 3;
}

inline typename FixedStreamBase::size_type FixedStreamBase::numBitsForBytes(size_type numBytes) 
{
	return numBytes << 3;
}

inline typename FixedStreamBase::TypePtr FixedStreamBase::dataBegin(void)
{
	return pBegin_;
}

inline typename FixedStreamBase::ConstTypePtr FixedStreamBase::dataBegin(void) const
{
	return pBegin_;
}



// -------------------------------


// -------------------------------




X_NAMESPACE_END