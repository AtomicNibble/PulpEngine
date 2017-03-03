

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