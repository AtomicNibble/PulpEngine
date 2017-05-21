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
	X_ASSERT(sizeof(T) <= freeSpace(), "can't write a object of size: %" PRIuS, sizeof(T))(sizeof(T), freeSpace());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = current_;
	*as_type = val;
	current_ += sizeof(T);
}


template<typename T>
inline void ByteStream::write(const T* val, size_type num)
{
	X_ASSERT(((sizeof(T) * num) <= freeSpace()), "can't write %" PRIuS " objects of size: %" PRIuS,
		num, sizeof(T)) (sizeof(T), freeSpace());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = current_;

	size_type i;
	for (i = 0; i < num; i++)
	{
		*as_type = val[i];
		++as_type;
	}

	current_ += (sizeof(T) * num);
}

template<typename T>
inline T ByteStream::read(void)
{
	X_ASSERT(sizeof(T) <= size(), "can't read a object of size: %" PRIuS, sizeof(T))(sizeof(T), size());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = current_;
	current_ -= sizeof(T);
	return as_type[-1];
}

template<typename T>
inline T ByteStream::peek(void) const
{
	X_ASSERT(sizeof(T) <= size(), "can't peek a object of size: %" PRIuS, sizeof(T))(sizeof(T), size());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = current_;
	return as_type[-1];
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
		// copy of old memory.
		char* pOld = start_;
		size_type currentBytes = this->size();

		// allocate new
		start_ = Allocate(numBytes);

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


// for easy memory allocation changes later.
inline void ByteStream::Delete(char* pData) const
{
	X_DELETE_ARRAY(pData, arena_);
}

inline char* ByteStream::Allocate(size_type num) const
{
	return X_NEW_ARRAY(char, num, arena_, "ByteStream");
}