


ByteStream::ByteStream(MemoryArenaBase* arena) :
	current_(nullptr),
	start_(nullptr),
	end_(nullptr),
	arena_(arena)
{
	X_ASSERT_NOT_NULL(arena);
}

template<typename T>
inline void ByteStream::write(const T& val)
{
	X_ASSERT(sizeof(T) <= freeSpace(), "can't write a object of size: %i", sizeof(T))(sizeof(T), freeSpace());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = current_;
	*as_type = val;
	current_ += sizeof(T);
}

template<typename T>
inline T ByteStream::read()
{
	X_ASSERT(sizeof(T) <= size(), "can't read a object of size: %i", sizeof(T))(sizeof(T), size());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = current_;
	current_ -= sizeof(T);
	return as_type[-1];
}

template<typename T>
inline T ByteStream::peek() const
{
	X_ASSERT(sizeof(T) <= size(), "can't peek a object of size: %i", sizeof(T))(sizeof(T), size());

	union {
		char* as_char;
		T* as_type;
	};

	as_char = current_;
	return as_type[-1];
}

inline void ByteStream::seek(size_t pos)
{
	X_ASSERT(pos < size(), "can't seek that far")(pos, size());
	current_ = (start_ + pos);
}


// resizes the object
inline void ByteStream::resize(size_t size)
{
	if (size > capacity()) 
	{
		// copy of old memory.
		char* pOld = start_;
		size_t numBytes = this->size();

		// allocate new
		start_ = Allocate(size);

		// copy old over.
		if (pOld)
		{
			::memcpy(start_, pOld, numBytes);
			Delete(pOld); 
			current_ = start_ + numBytes;
		}
		else
		{
			current_ = start_;
		}

		end_ = start_ + size;
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
inline size_t ByteStream::size() const
{
	return static_cast<size_t>(current_ - start_);
}

// returns the capacity of the byte stream.
inline size_t ByteStream::capacity() const
{
	return static_cast<size_t>(end_ - start_);
}

// returns the amount of bytes that can be added.
inline size_t ByteStream::freeSpace() const
{
	return static_cast<size_t>(end_ - current_);
}

// returns true if the stream is full.
inline bool ByteStream::isEos() const
{
	return current_ == end_;
}




// for easy memory allocation changes later.
inline void ByteStream::Delete(char* pData)
{
	// ::free((void*)pData);
	X_DELETE_ARRAY(pData, arena_);
}

inline char* ByteStream::Allocate(size_t num)
{
	return X_NEW_ARRAY(char, num, arena_, "ByteStream");
//	return (char*)malloc(num);
}