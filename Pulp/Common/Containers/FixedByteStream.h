#pragma once


X_NAMESPACE_BEGIN(core)


class FixedByteStreamBase
{
public:
	typedef size_t size_type;
	typedef uint8_t Type;
	typedef Type* TypePtr;
	typedef Type value_type;
	typedef const Type* ConstTypePtr;
	typedef Type* Iterator;
	typedef const Type* ConstIterator;
	typedef Type& Reference;
	typedef Type& reference;
	typedef const Type& ConstReference;
	typedef const Type& const_reference;

protected:
	FixedByteStreamBase(size_type numBytes);
	FixedByteStreamBase(TypePtr pBegin, size_type numBytes);

public:

	template<typename T>
	void write(const T& val);
	// writes the type * num to the stream.
	template<typename T>
	void write(const T* pVal, size_type num);

	// writes bytes to stream, will stich in if not currently on byte boundary, unlike writeAligned.
	void write(const Type* pBuf, size_type numBytes);

	// convience helper.
	template<typename T>
	T read(void);

	template<typename T>
	void read(T& val);
	// read the type * num from the stream.
	template<typename T>
	void read(T* pVal, size_type num);

	void read(Type* pBuf, size_type numBytes);

	// skips forwward in read pointer.
	void skip(size_type numBytes);

	// pads the bit stream until the stream length is equal to length.
	// will not trucate.
	void zeroPadToLength(size_type numBytes);

	// clears the stream setting the cursor back to the start.
	void reset(void);

	size_type size(void) const;
	
	size_type capacity(void) const;
	// returns the amount of bits that can be added.
	size_type freeSpace(void) const;
	// returns true if the stream is full.
	bool isEos(void) const;
	// returns true if never read from stream.
	bool isStartOfStream(void) const;

	TypePtr ptr(void);
	ConstTypePtr ptr(void) const;
	TypePtr data(void);
	ConstTypePtr data(void) const;

	Iterator begin(void);
	ConstIterator begin(void) const;
	Iterator end(void);
	ConstIterator end(void) const;

protected:
	TypePtr dataBegin(void);
	ConstTypePtr dataBegin(void) const;

protected:
	size_type numBytes_;
	size_type readByteIdx_;
	size_type byteIdx_;
	Type* pBegin_;
};

class FixedByteStreamNoneOwningPolicy : public FixedByteStreamBase
{

public:
	FixedByteStreamNoneOwningPolicy(Type* pBegin, Type* pEnd, bool dataInit);
	~FixedByteStreamNoneOwningPolicy();
};

class FixedByteStreamOwningPolicy : public FixedByteStreamBase
{
public:
	FixedByteStreamOwningPolicy(core::MemoryArenaBase* arena, size_type numBytes);
	~FixedByteStreamOwningPolicy();

private:
	core::MemoryArenaBase* arena_;
};

template<size_t N>
class FixedByteStreamStackPolicy : public FixedByteStreamBase
{
public:
	FixedByteStreamStackPolicy();
	~FixedByteStreamStackPolicy();

protected:
	Type buf_[N]; // this is fine, since will only ever be 8bit type.
};

template<class StorageType>
class FixedByteStream : public StorageType
{
public:
	typedef FixedByteStreamNoneOwningPolicy NoneOwningPolicy;
	typedef FixedByteStreamOwningPolicy OwningPolicy;

	template<size_t N>
	using StackPolicy = FixedByteStream<FixedByteStreamStackPolicy<N>>;

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


};

typedef FixedByteStream<FixedByteStreamOwningPolicy> FixedByteStreamOwning;
typedef FixedByteStream<FixedByteStreamNoneOwningPolicy> FixedByteStreamNoneOwning;

template<size_t N>
using FixedByteStreamStack = FixedByteStream<FixedByteStreamStackPolicy<N>>;


X_NAMESPACE_END


#include "FixedByteStream.inl"