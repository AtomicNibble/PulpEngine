
BitStream::BitStream(MemoryArenaBase* arena) :
bitIdx_(0),
capacity_(0),
start_(0),
arena_(arena)
{
	X_ASSERT_NOT_NULL(arena);


}


// writes a bit to the stream
inline void BitStream::write(bool bit)
{
	X_ASSERT(freeSpace() > 0, "can't write bit to stream.")(freeSpace(), capacity());

	size_t byteIdx = bitIdx_ >> 3; // val / 8
	int32 bitpos = safe_static_cast<int32,size_t>(7 - (bitIdx_ & 7));
	int32 bitval = 1 << bitpos;

	if (bit)
		start_[byteIdx] = (start_[byteIdx] | bitval) & 0xFF;
	else
		start_[byteIdx] = (start_[byteIdx] & ~bitval) & 0xFF;

	bitIdx_++;
}

// removes and returns the top bit off the stream.
inline bool BitStream::read()
{
	X_ASSERT(size() > 0, "can't read bit from stream.")(size());

	bitIdx_--; // take of a bit.

	size_t byteIdx = bitIdx_ >> 3; // val / 8
	uint32 bitpos = (7 - (bitIdx_ & 7));

	return (start_[byteIdx] & (1 << bitpos)) != 0;
}

// returns the top bit but dose not remove it.
inline bool BitStream::peek() const
{
	X_ASSERT(size() > 0, "can't peek bit from stream.")(size());

	size_t idx = bitIdx_ - 1;

	size_t byteIdx = (idx) >> 3; // val / 8
	uint32 bitpos = (7 - (idx & 7));

	return (start_[byteIdx] & (1 << bitpos)) != 0;;
}

// sets the absolute position in the stream.
inline void BitStream::seek(size_t pos)
{
	X_ASSERT(pos <= capacity(), "can't skeep to that position")(pos, capacity());
	bitIdx_ = pos;
}



// resizes the object
inline void BitStream::resize(size_t numBits) 
{
	if (numBits > capacity())
	{
		// save local copy of old array and it's size.
		char* pOld = start_;
		size_t bytesAllocated = bytesRequired(capacity_);

		// allocate the new one.
		start_ = Allocate(numBits);

		// copy over.
		if(pOld)
		{
			::memcpy(start_,pOld, bytesAllocated);
			Delete(pOld);
		}

		// update capacity.		
		capacity_ = numBits;
	}
}

inline void BitStream::reset(void)
{
	bitIdx_ = 0;
}
// resets the cursor and clears all memory.
inline void BitStream::free(void)
{
	if (start_) {
		Delete(start_);
	}
	start_ = nullptr;
	bitIdx_ = capacity_ = 0;
}




// returns how many bits are currently stored in the stream.
inline size_t BitStream::size() const
{
	return bitIdx_;
}

// returns the capacity
inline size_t BitStream::capacity() const
{
	return capacity_;
}

// returns the amount of bits that can be added.
inline size_t BitStream::freeSpace() const
{
	return capacity_ - bitIdx_;
}

// returns true if the stream is full.
inline bool BitStream::isEos() const
{
	return bitIdx_ == capacity_;
}

inline size_t BitStream::bytesRequired(size_t numBits) const
{
	// align so that all the bits fit.
	numBits = bitUtil::RoundUpToMultiple<size_t>(numBits, 8);
	numBits >>= 3; // to bytes
	return numBits;
}

// for easy memory allocation changes later.
inline void BitStream::Delete(char* pData)
{
	X_DELETE_ARRAY(pData,arena_);
}

inline char* BitStream::Allocate(size_t numbits)
{
	size_t numBytes = bytesRequired(numbits);

	return X_NEW_ARRAY(char, numBytes, arena_, "BitStream");
}
