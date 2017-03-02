

X_NAMESPACE_BEGIN(core)


// -------------------------------

template<class StorageType>
void FixedBitStream<StorageType>::write(bool bit)
{
	if (bit) {
		write0();
	}
	else {
		write1();
	}
}

template<class StorageType>
void FixedBitStream<StorageType>::write0(void)
{
	uint8_t val = 0;
	readBits(reinterpret_cast<Type*>(&val), 1);
}

template<class StorageType>
void FixedBitStream<StorageType>::write1(void)
{
	uint8_t val = 1;
	readBits(reinterpret_cast<Type*>(&val), 1);
}

template<class StorageType>
template<typename T>
void FixedBitStream<StorageType>::write(const T& val)
{
	writeBits(reinterpret_cast<const Type*>(&val), sizeof(T) << 3);
}

// read the type * num from the stream.
template<class StorageType>
template<typename T>
void FixedBitStream<StorageType>::write(const T* pVal, size_type num)
{
	writeBits(reinterpret_cast<const Type*>(pVal), (sizeof(T) * num) << 3);
}

template<class StorageType>
void FixedBitStream<StorageType>::write(const Type* pBuf, size_type numBytes)
{
	writeBits(pBuf, numBitsForBytes(numBytes));
}

// read bits from stream
template<class StorageType>
void FixedBitStream<StorageType>::writeBits(const Type* pBuf, size_type numBits)
{
	if (!numBits) {
		return;
	}

	size_type bitsMod8 = bitIdx_ & 7;
	size_type srcTrailingBits = numBits & 7;

	// copy full bytes if we can.
	// trailing bits are handled after.
	if (bitsMod8 == 0 && numBits >= 8)
	{
		size_type bitsToCopy = numBits - srcTrailingBits;
		std::memcpy(pBegin_ + byteIndex(), pBuf, numBytesForBits(bitsToCopy));
		bitIdx_ += bitsToCopy;

		if (srcTrailingBits == 0) {
			return; // early out on whole byte src.
		}

		numBits -= bitsToCopy;
		pBuf += bitsToCopy;
	}

	// okay now we are left with data we can't do simple byte copy on.
	// shift away!
	while (numBits >= 8)
	{
		Type* pDst = pBegin_ + byteIndex();
		Type srcByte = *(pBuf++);

		// we place first part of srcbyte, is last bit of dst byte.
		*(pDst) |= srcByte >> bitsMod8;

		// we shift lower bits up to start of dst byte.
		*(pDst + 1) |= (srcByte << (8 - bitsMod8));

		numBits -= 8;
		bitIdx_ += 8;
	}

	if (numBits)
	{
		// now we only have a few bits left.
		// we need to shift them up
		Type* pDst = pBegin_ + byteIndex();
		Type srcByte = *pBuf;

		*(pDst) |= (srcByte << (8 - bitsMod8));

		bitIdx_ += numBits;
	}
}

// pads the stream to byte boundry before read.
template<class StorageType>
void FixedBitStream<StorageType>::writeAligned(const Type* pBuf, size_type numBytes)
{
	alignReadToByteBoundry();
	write(pBuf, numBytes);
}

template<class StorageType>
void FixedBitStream<StorageType>::writeBitsAligned(const Type* pBuf, size_type numBits)
{
	alignReadToByteBoundry();
	writeBits(pBuf, numBits);
}


// -------------------------------

template<class StorageType>
template<typename T>
T FixedBitStream<StorageType>::read(void)
{
	T val;
	readBits(reinterpret_cast<Type*>(&val), sizeof(T) << 3);
	return val;
}

template<class StorageType>
template<typename T>
void FixedBitStream<StorageType>::read(T& val)
{
	readBits(reinterpret_cast<Type*>(&val), sizeof(T) << 3);
}

// read the type * num from the stream.
template<class StorageType>
template<typename T>
void FixedBitStream<StorageType>::read(T* pVal, size_type num)
{
	readBits(reinterpret_cast<Type*>(pVal), (sizeof(T) * num) << 3);
}

template<class StorageType>
void FixedBitStream<StorageType>::read(Type* pBuf, size_type numBytes)
{
	readBits(pBuf, numBitsForBytes(numBytes));
}

// read bits from stream
template<class StorageType>
void FixedBitStream<StorageType>::readBits(Type* pBuf, size_type numBits)
{
	if (!numBits) {
		return;
	}

	size_type bitsMod8 = readBitIdx_ & 7;
	size_type srcTrailingBits = numBits & 7;

	// copy full bytes if we can.
	// trailing bits are handled after.
	if (bitsMod8 == 0 && numBits >= 8)
	{
		size_type bitsToCopy = numBits - srcTrailingBits;
		std::memcpy(pBuf, pBegin_ + readByteIndex(), numBytesForBits(bitsToCopy));
		readBitIdx_ += bitsToCopy;

		if (srcTrailingBits == 0) {
			return;
		}

		numBits -= bitsToCopy;
		pBuf += bitsToCopy;
	}

	while (numBits >= 8)
	{
		Type srcByte = *(pBegin_ + readByteIndex());
		Type* pDst = pBuf++;

		*(pDst) |= srcByte >> bitsMod8;
		*(pDst + 1) |= (srcByte << (8 - bitsMod8));

		numBits -= 8;
		readBitIdx_ += 8;
	}

	if (numBits)
	{
		Type srcByte = *(pBegin_ + readByteIndex());
		Type* pDst = pBuf;

		*(pDst) |= (srcByte << (8 - bitsMod8));

		readBitIdx_ += numBits;
	}
}

// pads the stream to byte boundry before read.
template<class StorageType>
void FixedBitStream<StorageType>::readAligned(Type* pBuf, size_type numBytes)
{
	alignReadToByteBoundry();
	read(pBuf, numBytes);
}

template<class StorageType>
void FixedBitStream<StorageType>::readBitsAligned(Type* pBuf, size_type numBits)
{
	alignReadToByteBoundry();
	readBits(pBuf, numBits);
}


// -------------------------------

template<class StorageType>
void FixedBitStream<StorageType>::alignWriteToByteBoundry(void)
{
	bitIdx_ = core::bitUtil::RoundUpToMultiple(bitIdx_, 8_sz);
}


template<class StorageType>
void FixedBitStream<StorageType>::alignReadToByteBoundry(void)
{
	readBitIdx_ = core::bitUtil::RoundUpToMultiple(readBitIdx_, 8_sz);
}

// -------------------------------

template<class StorageType>
void FixedBitStream<StorageType>::skipBytes(size_type numBytes)
{
	readBitIdx_ += numBitsForBytes(numBytes);
}

template<class StorageType>
void FixedBitStream<StorageType>::skipBits(size_type numBits)
{
	readBitIdx_ += numBits;
}

template<class StorageType>
void FixedBitStream<StorageType>::reset(void)
{
	bitIdx_ = 0;
	readBitIdx_ = 0;
}

// -------------------------------
template<class StorageType>
typename FixedBitStream<StorageType>::size_type typename FixedBitStream<StorageType>::size(void) const
{
	return bitIdx_ - readBitIdx_;
}

template<class StorageType>
typename FixedBitStream<StorageType>::size_type typename FixedBitStream<StorageType>::sizeInBytes(void) const
{
	return numBytesForBits(size());
}

template<class StorageType>
inline typename FixedBitStream<StorageType>::size_type FixedBitStream<StorageType>::freeSpace(void) const
{
	return capacity() - size();
}

template<class StorageType>
inline bool FixedBitStream<StorageType>::isEos(void) const
{
	return size() == capacity();
}


// -------------------------------

template<class StorageType>
typename FixedBitStream<StorageType>::TypePtr FixedBitStream<StorageType>::ptr(void)
{
	return pBegin_;
}

template<class StorageType>
typename FixedBitStream<StorageType>::ConstTypePtr FixedBitStream<StorageType>::ptr(void) const
{
	return pBegin_;
}

template<class StorageType>
typename FixedBitStream<StorageType>::TypePtr FixedBitStream<StorageType>::data(void)
{
	return ptr();
}

template<class StorageType>
typename FixedBitStream<StorageType>::ConstTypePtr FixedBitStream<StorageType>::data(void) const
{
	return data();
}

template<class StorageType>
typename FixedBitStream<StorageType>::Iterator FixedBitStream<StorageType>::begin(void)
{
	return data();
}

template<class StorageType>
typename FixedBitStream<StorageType>::ConstIterator FixedBitStream<StorageType>::begin(void) const
{
	return begin();
}

template<class StorageType>
typename FixedBitStream<StorageType>::Iterator FixedBitStream<StorageType>::end(void)
{
	return pBegin_ + size();
}

template<class StorageType>
typename FixedBitStream<StorageType>::ConstIterator FixedBitStream<StorageType>::end(void) const
{
	return pBegin_ + size();
}

// -------------------------------

template<class StorageType>
inline typename FixedBitStream<StorageType>::size_type FixedBitStream<StorageType>::byteIndex(void) const
{
	return bitIdx_ >> 3;
}

template<class StorageType>
inline typename FixedBitStream<StorageType>::size_type FixedBitStream<StorageType>::readByteIndex(void) const
{
	return readBitIdx_ >> 3;
}

template<class StorageType>
inline typename FixedBitStream<StorageType>::size_type FixedBitStream<StorageType>::numBytesForBits(size_type numBits) const
{
	// align so that all the bits fit.
	return (numBits + 7) >> 3;
}

template<class StorageType>
inline typename FixedBitStream<StorageType>::size_type FixedBitStream<StorageType>::numBitsForBytes(size_type numBytes) const
{
	return numBytes << 3;
}



X_NAMESPACE_END