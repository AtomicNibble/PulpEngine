#pragma once


X_NAMESPACE_BEGIN(core)


class FixedBitStreamNoneOwningPolicy
{
public:
	typedef uint8_t Type;
	typedef Type* TypePtr;
	typedef Type value_type;
	typedef size_t size_type;
	typedef const Type* ConstTypePtr;
	typedef Type* Iterator;
	typedef const Type* ConstIterator;
	typedef Type& Reference;
	typedef Type& reference;
	typedef const Type& ConstReference;
	typedef const Type& const_reference;

public:
	FixedBitStreamNoneOwningPolicy(Type* pBegin, Type* pEnd, bool dataInit) :
		pBegin_(pBegin),
		pEnd_(pEnd),
		readBitIdx_(0),
		bitIdx_(0)
	{
		numBits_ = core::bitUtil::bytesToBits(union_cast<size_type>(pEnd - pBegin));

		if (dataInit) {
			bitIdx_ = numBits_;
		}
	}
	~FixedBitStreamNoneOwningPolicy() {

	}

	size_type capacity(void) const {
		return numBits_;
	}

protected:
	TypePtr dataBegin(void) {
		return pBegin_;
	}

protected:
	size_type numBits_;
	size_type readBitIdx_;
	size_type bitIdx_;
	Type* pBegin_;
	Type* pEnd_;
};

class FixedBitStreamOwningPolicy
{
public:
	typedef uint8_t Type;
	typedef Type* TypePtr;
	typedef Type value_type;
	typedef size_t size_type;
	typedef const Type* ConstTypePtr;
	typedef Type* Iterator;
	typedef const Type* ConstIterator;
	typedef Type& Reference;
	typedef Type& reference;
	typedef const Type& ConstReference;
	typedef const Type& const_reference;

public:
	FixedBitStreamOwningPolicy(core::MemoryArenaBase* arena, size_type numBits) :
		arena_(arena),
		numBits_(0),
		readBitIdx_(0),
		bitIdx_(0)
	{
		size_type numBytes = core::bitUtil::RoundUpToMultiple(numBits, 8_sz);
		pBegin_ = X_NEW_ARRAY(Type, numBytes, arena_, "FixedBitStream");
		pEnd_ = pBegin_ + numBytes;
	}

	~FixedBitStreamOwningPolicy() {
		X_DELETE_ARRAY(pBegin_, arena_);
	}

	size_type capacity(void) const {
		return numBits_;
	}

protected:
	TypePtr dataBegin(void) {
		return pBegin_;
	}

protected:
	size_type numBits_;
	size_type readBitIdx_;
	size_type bitIdx_;
	Type* pBegin_;
	Type* pEnd_;
private:
	core::MemoryArenaBase* arena_;
};

template<size_t N>
class FixedBitStreamStackPolicy
{
public:
	typedef uint8_t Type;
	typedef Type* TypePtr;
	typedef Type value_type;
	typedef size_t size_type;
	typedef const Type* ConstTypePtr;
	typedef Type* Iterator;
	typedef const Type* ConstIterator;
	typedef Type& Reference;
	typedef Type& reference;
	typedef const Type& ConstReference;
	typedef const Type& const_reference;

public:
	FixedBitStreamStackPolicy() :
		numBits_(0),
		readBitIdx_(0),
		bitIdx_(0)
	{
	}

	~FixedBitStreamStackPolicy() {

	}

	size_type capacity(void) const {
		return numBits_;
	}

protected:
	TypePtr dataBegin(void) {
		return buf_;
	}

protected:
	size_type numBits_;
	size_type readBitIdx_;
	size_type bitIdx_;

	Type buf_[N];
};

template<class StorageType>
class FixedBitStream : public StorageType
{
public:
	typedef FixedBitStreamNoneOwningPolicy NoneOwningPolicy;
	typedef FixedBitStreamOwningPolicy OwningPolicy;

public:
	typedef typename StorageType::Type Type;
	typedef typename StorageType::TypePtr TypePtr;
	typedef typename StorageType::value_type value_type;
	typedef typename StorageType::size_type size_type;
	typedef typename StorageType::ConstTypePtr ConstTypePtr;
	typedef typename StorageType::Iterator Iterator;
	typedef typename StorageType::ConstIterator ConstIterator;
	typedef typename StorageType::Reference Reference;
	typedef typename StorageType::reference reference;
	typedef typename StorageType::ConstReference ConstReference;
	typedef typename StorageType::const_reference const_reference;

public:
	using StorageType::StorageType;

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


	// convience helper.
	template<typename T>
	T read(void);

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

	void alignWriteToByteBoundry(void);
	void alignReadToByteBoundry(void);

	// skips forwward in read pointer.
	void skipBytes(size_type numBytes);
	void skipBits(size_type numBits);

	// pads the bit stream until the stream length is equal to length.
	// will not trucate.
	void zeroPadToLength(size_type numBytes);

	// clears the stream setting the cursor back to the start.
	void reset(void);

	size_type size(void) const;
	size_type sizeInBytes(void) const;

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
	size_type byteIndex(void) const;
	size_type readByteIndex(void) const;

	size_type numBytesForBits(size_type numBits) const;
	size_type numBitsForBytes(size_type numBytes) const;
};

X_NAMESPACE_END


#include "FixedBitStream.inl"