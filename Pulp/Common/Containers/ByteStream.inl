#include "ByteStream.h"



ByteStream::ByteStream(MemoryArenaBase* arena) :
	current_(nullptr),
	start_(nullptr),
	end_(nullptr),
	arena_(X_ASSERT_NOT_NULL(arena))
{
}

ByteStream::ByteStream(MemoryArenaBase* arena, size_type numBytes) :
	current_(nullptr),
	start_(nullptr),
	end_(nullptr),
	arena_(X_ASSERT_NOT_NULL(arena))
{
	resize(numBytes);
}

ByteStream::ByteStream(const ByteStream& oth)
{
	// make null incase oth is empty.
	current_ = nullptr;
	start_ = nullptr;
	end_ = nullptr;

	arena_ = oth.arena_;

	resize(oth.capacity());

	::memcpy(start_, oth.start_, oth.size());

	current_ = start_ + oth.size();
}

ByteStream::ByteStream(ByteStream&& oth)
{
	arena_ = oth.arena_;
	current_ = oth.current_;
	start_ = oth.start_;
	end_ = oth.end_;

	// clear other.
	oth.current_ = nullptr;
	oth.start_ = nullptr;
	oth.end_ = nullptr;
}

ByteStream::~ByteStream()
{
	free();
}

ByteStream& ByteStream::operator=(const ByteStream& oth)
{
	if (this != &oth)
	{
		free();

		arena_ = oth.arena_;

		resize(oth.capacity());

		::memcpy(start_, oth.start_, oth.size());

		current_ = start_ + oth.size();
	}
	return *this;
}

ByteStream& ByteStream::operator=(ByteStream&& oth)
{
	if (this != &oth)
	{
		free();

		// steal buffer.
		arena_ = oth.arena_;
		current_ = oth.current_;
		start_ = oth.start_;
		end_ = oth.end_;

		// clear other.
		oth.current_ = nullptr;
		oth.start_ = nullptr;
		oth.end_ = nullptr;
	}
	return *this;
}

template<typename T>
inline void ByteStream::write(const T& val)
{
	write(reinterpret_cast<const Type*>(&val), sizeof(T));
}


template<typename T>
inline void ByteStream::write(const T* pVal, size_type num)
{
	write(reinterpret_cast<const Type*>(pVal), num * sizeof(T));
}

inline void ByteStream::write(const Type* pBuf, size_type numBytes)
{
	ensureSpace(numBytes);

	X_ASSERT(numBytes <= freeSpace(), "Not enougth space")(numBytes, freeSpace());

	::memcpy(current_, pBuf, numBytes);
	current_ += numBytes;
}


template<typename T>
inline T ByteStream::read(void)
{
	T val;
	read(reinterpret_cast<Type*>(&val), sizeof(T));
	return val;
}

template<typename T>
inline void ByteStream::read(T& val)
{
	read(reinterpret_cast<Type*>(&val), sizeof(T));
}

template<typename T>
inline void ByteStream::read(T* pVal, size_type num)
{
	read(reinterpret_cast<Type*>(pVal), num * sizeof(T));
}

inline void ByteStream::read(Type* pBuf, size_type numBytes)
{
	X_ASSERT(numBytes <= size(), "can't read buffer of size: %" PRIuS, numBytes)(numBytes, size());

	::memcpy(pBuf, current_ - numBytes, numBytes);
	current_ -= numBytes;
}

template<typename T>
inline T ByteStream::peek(void) const
{
	X_ASSERT(sizeof(T) <= size(), "can't peek a object of size: %" PRIuS, sizeof(T))(sizeof(T), size());

	return union_cast<T*, Type*>(current_)[-1];
}

inline void ByteStream::seek(size_type pos)
{
	X_ASSERT(pos < size(), "can't seek that far")(pos, size());
	current_ = (start_ + pos);
}


// resizes the object
inline void ByteStream::resize(size_type numBytes)
{
	if (numBytes > capacity()) 
	{
		Type* pOld = start_;
		const size_type currentBytes = size();

		start_ = Allocate(numBytes);

		if (pOld)
		{
			::memcpy(start_, pOld, currentBytes);
			Delete(pOld); 
			current_ = start_ + currentBytes;
		}
		else
		{
			current_ = start_;
		}

		end_ = start_ + numBytes;
	}
}


// clears the stream setting the cursor back to the start.
// no memory is freed
inline void ByteStream::reset(void)
{
	current_ = start_;
}

// resets the cursor and clears all memory.
inline void ByteStream::free(void)
{
	if (start_) { // memory to free.
		Delete(start_);
	}
	current_ = start_ = end_ = nullptr;
}




// returns how many bytes are currently stored in the stream.
inline typename ByteStream::size_type ByteStream::size(void) const
{
	return union_cast<size_type>(current_ - start_);
}

// returns the capacity of the byte stream.
inline typename ByteStream::size_type ByteStream::capacity(void) const
{
	return union_cast<size_type>(end_ - start_);
}

// returns the amount of bytes that can be added.
inline typename ByteStream::size_type ByteStream::freeSpace(void) const
{
	return union_cast<size_type>(end_ - current_);
}

// returns true if the stream is full.
inline bool ByteStream::isEos(void) const
{
	return current_ == end_;
}


inline typename ByteStream::TypePtr ByteStream::ptr(void)
{
	return start_;
}

inline typename ByteStream::ConstTypePtr ByteStream::ptr(void) const
{
	return start_;
}

inline typename ByteStream::TypePtr ByteStream::data(void)
{
	return start_;
}

inline typename ByteStream::ConstTypePtr ByteStream::data(void) const
{
	return start_;
}


inline typename ByteStream::Iterator ByteStream::begin(void)
{
	return start_;
}

inline typename ByteStream::ConstIterator ByteStream::begin(void) const
{
	return start_;
}

inline typename ByteStream::Iterator ByteStream::end(void)
{
	return current_;
}

inline typename ByteStream::ConstIterator ByteStream::end(void) const
{
	return current_;
}

inline typename ByteStream::Reference ByteStream::front(void)
{
	return *start_;
}

inline typename ByteStream::ConstReference ByteStream::front(void) const
{
	return *start_;
}

inline typename ByteStream::Reference ByteStream::back(void)
{
	return *end();
}

inline typename ByteStream::ConstReference ByteStream::back(void) const
{
	return *end();
}

inline void ByteStream::ensureSpace(size_type num)
{
	if (num > freeSpace())
	{
		// copy of old memory.
		Type* pOld = start_;
		const size_type currentBytes = size();
		const size_type currentCapacity = capacity();
		const size_type newSize = currentCapacity * 2;

		// allocate new
		start_ = Allocate(newSize);

		// copy old over.
		if (pOld)
		{
			::memcpy(start_, pOld, currentBytes);
			Delete(pOld);
			current_ = start_ + currentBytes;
		}
		else
		{
			current_ = start_;
		}

		end_ = start_ + newSize;
	} 
}

// for easy memory allocation changes later.
inline void ByteStream::Delete(Type* pData) const
{
	X_DELETE_ARRAY(pData, arena_);
}

inline typename ByteStream::Type* ByteStream::Allocate(size_type num) const
{
	return X_NEW_ARRAY(Type, num, arena_, "ByteStream");
}