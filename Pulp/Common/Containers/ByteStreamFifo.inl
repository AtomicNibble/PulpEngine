#include "ByteStreamFifo.h"



ByteStreamFifo::ByteStreamFifo(MemoryArenaBase* arena) :
	read_(nullptr),
	write_(nullptr),
	start_(nullptr),
	end_(nullptr),
	arena_(X_ASSERT_NOT_NULL(arena))
{
}

ByteStreamFifo::ByteStreamFifo(MemoryArenaBase* arena, size_t numBytes) :
	read_(nullptr),
	write_(nullptr),
	start_(nullptr),
	end_(nullptr),
	arena_(X_ASSERT_NOT_NULL(arena))
{
	resize(numBytes);
}

ByteStreamFifo::ByteStreamFifo(const ByteStreamFifo & oth)
{
	arena_ = oth.arena_;

	resize(oth.capacity());

	// copy it all.
	::memcpy(start_, oth.start_, oth.capacity());

	// offset read/write
	read_ = start_ + (oth.read_ - oth.start_);
	write_ = start_ + (oth.write_ - oth.start_);
}

ByteStreamFifo::ByteStreamFifo(ByteStreamFifo&& oth)
{
	read_ = oth.read_;
	write_ = oth.write_;
	start_ = oth.start_;
	end_ = oth.end_;
	arena_ = oth.arena_;

	oth.read_ = nullptr;
	oth.write_ = nullptr;
	oth.start_ = nullptr;
	oth.end_ = nullptr;
}



ByteStreamFifo::~ByteStreamFifo(void)
{
	free();
}

inline ByteStreamFifo& ByteStreamFifo::operator = (const ByteStreamFifo& oth)
{
	if (this != &oth)
	{
		free();

		arena_ = oth.arena_;

		resize(oth.capacity());

		// copy it all.
		::memcpy(start_, oth.start_, oth.capacity());

		// offset read/write
		read_ = start_ + (oth.read_ - oth.start_);
		write_ = start_ + (oth.write_ - oth.start_);
	}
	return *this;
}

inline ByteStreamFifo& ByteStreamFifo::operator = (ByteStreamFifo&& oth)
{
	if (this != &oth)
	{
		free();

		read_ = oth.read_;
		write_ = oth.write_;
		start_ = oth.start_;
		end_ = oth.end_;
		arena_ = oth.arena_;

		oth.read_ = nullptr;
		oth.write_ = nullptr;
		oth.start_ = nullptr;
		oth.end_ = nullptr;
	}
	return *this;
}

/*
inline void ByteStreamFifo::setArena(MemoryArenaBase* arena)
{
	X_ASSERT_NOT_NULL(arena);

	arena_ = arena;
}
*/

template<typename T>
inline void ByteStreamFifo::write(const T& val)
{
	X_ASSERT(sizeof(T) <= freeSpace(), "can't write a object of size: %i", sizeof(T))(sizeof(T), freeSpace());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = write_;
	*as_type = val;
	write_ += sizeof(T);
}


inline void ByteStreamFifo::write(const void* pval, size_t  num)
{
	X_ASSERT(num <= freeSpace(), "can't write a object of size: %i", num)(num, freeSpace());

	memcpy(write_, pval, num);
	write_ += num;
}



template<typename T>
inline T ByteStreamFifo::read(void)
{
	X_ASSERT(sizeof(T) <= size(), "can't read a object of size: %i", sizeof(T))(sizeof(T), size());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = read_;
	read_ += sizeof(T);
	return as_type[0];
}

template<typename T>
inline T& ByteStreamFifo::peek(void)
{
	X_ASSERT(sizeof(T) <= size(), "can't peek a object of size: %i", sizeof(T))(sizeof(T), size());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = read_;
	return *as_type;
}

template<typename T>
inline const T ByteStreamFifo::peek(void) const
{
	X_ASSERT(sizeof(T) <= size(), "can't peek a object of size: %i", sizeof(T))(sizeof(T), size());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = read_;
	return *as_type;
}

inline void ByteStreamFifo::seekSet(size_t pos)
{
	X_ASSERT(pos < size(), "can't seek that far")(pos, size());
	read_ = (start_ + pos);
}

template<typename T>
inline void ByteStreamFifo::skip(size_t num)
{
	X_ASSERT((sizeof(T)* num) <= size(), "can't skip %i of size: %i", num, sizeof(T))(num, sizeof(T), size());
	read_ += (sizeof(T) * num);
}

// resizes the object
inline void ByteStreamFifo::resize(size_t numBytes)
{
	if (numBytes > capacity()) {
		Delete(start_); // free any current memory

		write_ = read_ = start_ = Allocate(numBytes);
		end_ = start_ + numBytes;
	}
	reset();
}


// clears the stream setting the cursor back to the start.
// no memory is freed
inline void ByteStreamFifo::reset(void)
{
	write_ = start_;
	read_ = start_;
}

// resets the cursor and clears all memory.
inline void ByteStreamFifo::free(void)
{
	if (start_) { // memory to free.
		Delete(start_);
	}
	read_ = write_ = start_ = end_ = nullptr;
}


inline bool ByteStreamFifo::isEmpty(void) const
{
	return size() == 0;
}

// returns how many bytes are currently stored in the stream.
inline size_t ByteStreamFifo::size(void) const
{
	return static_cast<size_t>(write_ - read_);
}

// returns the capacity of the byte stream.
inline size_t ByteStreamFifo::capacity(void) const
{
	return static_cast<size_t>(end_ - start_);
}

// returns the amount of bytes that can be added.
inline size_t ByteStreamFifo::freeSpace(void) const
{
	return static_cast<size_t>(end_ - write_);
}

// returns true if the stream is full.
inline bool ByteStreamFifo::isEos(void) const
{
	return write_ == end_;
}



// for easy memory allocation changes later.
inline void ByteStreamFifo::Delete(char* pData) const
{
	X_DELETE_ARRAY(pData, arena_);
}

inline char* ByteStreamFifo::Allocate(size_t num) const
{
	return X_NEW_ARRAY(char, num, arena_, "ByteStreamFifi");
}