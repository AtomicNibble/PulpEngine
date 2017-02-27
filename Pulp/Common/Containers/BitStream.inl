#include "BitStream.h"

BitStream::BitStream(MemoryArenaBase* arena) :
	bitIdx_(0),
	capacity_(0),
	start_(0),
	arena_(arena)
{
	X_ASSERT_NOT_NULL(arena);
}

BitStream::BitStream(MemoryArenaBase* arena, size_type numBits) :
	bitIdx_(0),
	capacity_(0),
	start_(0),
	arena_(arena)
{
	X_ASSERT_NOT_NULL(arena);
	resize(numBits);
}

BitStream::BitStream(const BitStream& oth)
{
	// use it's arena.
	arena_ = oth.arena_;

	resize(oth.capacity_);

	// copy stored bits.
	size_type numBytes = numBytesForBits(oth.bitIdx_);
	::memcpy(start_, oth.start_, numBytes);

	bitIdx_ = oth.bitIdx_;
}

BitStream::BitStream(BitStream&& oth)
{
	capacity_ = oth.capacity_;
	bitIdx_ = oth.bitIdx_;
	start_ = oth.start_;
	arena_ = oth.arena_;

	// clear oth.
	oth.capacity_ = 0;
	oth.bitIdx_ = 0;
	oth.start_ = nullptr;
}

BitStream::~BitStream()
{
	free();
}

BitStream & BitStream::operator=(const BitStream & oth)
{
	if (this != &oth)
	{
		// free and re allocat using oth's arena.
		free();
		arena_ = oth.arena_;

		resize(oth.capacity_);

		// copy stored bits.
		size_type numBytes = numBytesForBits(oth.bitIdx_);
		::memcpy(start_, oth.start_, numBytes);

		bitIdx_ = oth.bitIdx_;
	}
	return *this;
}

BitStream & BitStream::operator=(BitStream && oth)
{
	if (this != &oth)
	{
		free();

		capacity_ = oth.capacity_;
		bitIdx_ = oth.bitIdx_;
		start_ = oth.start_;
		arena_ = oth.arena_;

		// clear oth.
		oth.capacity_ = 0;
		oth.bitIdx_ = 0;
		oth.start_ = nullptr;
	}
	return *this;
}

// writes a bit to the stream
inline void BitStream::write(bool bit)
{
	X_ASSERT(freeSpace() > 0, "can't write bit to stream.")(freeSpace(), capacity());

	size_type byteIdx = bitIdx_ >> 3; // val / 8
	int32 bitpos = safe_static_cast<int32>(7 - (bitIdx_ & 7));
	int32 bitval = 1 << bitpos;

	if (bit)
		start_[byteIdx] = (start_[byteIdx] | bitval) & 0xFF;
	else
		start_[byteIdx] = (start_[byteIdx] & ~bitval) & 0xFF;

	bitIdx_++;
}

// removes and returns the top bit off the stream.
inline bool BitStream::read(void)
{
	X_ASSERT(size() > 0, "can't read bit from stream.")(size());

	bitIdx_--; // take of a bit.

	size_type byteIdx = bitIdx_ >> 3; // val / 8
	uint32 bitpos = (7 - (bitIdx_ & 7));

	return (start_[byteIdx] & (1 << bitpos)) != 0;
}

// returns the top bit but dose not remove it.
inline bool BitStream::peek(void) const
{
	X_ASSERT(size() > 0, "can't peek bit from stream.")(size());

	size_type idx = bitIdx_ - 1;

	size_type byteIdx = (idx) >> 3; // val / 8
	uint32 bitpos = (7 - (idx & 7));

	return (start_[byteIdx] & (1 << bitpos)) != 0;;
}

// sets the absolute position in the stream.
inline void BitStream::seek(size_type pos)
{
	X_ASSERT(pos <= capacity(), "can't skeep to that position")(pos, capacity());
	bitIdx_ = pos;
}



// resizes the object
inline void BitStream::resize(size_type numBits)
{
	if (numBits > capacity())
	{
		// save local copy of old array and it's size.
		Type* pOld = start_;
		size_type bytesAllocated = numBytesForBits(capacity_);

		// allocate the new one.
		start_ = Allocate(bytesAllocated);

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
inline typename BitStream::size_type BitStream::size(void) const
{
	return bitIdx_;
}

// returns the capacity
inline typename BitStream::size_type BitStream::capacity(void) const
{
	return capacity_;
}

// returns the amount of bits that can be added.
inline typename BitStream::size_type BitStream::freeSpace(void) const
{
	return capacity_ - bitIdx_;
}

// returns true if the stream is full.
inline bool BitStream::isEos(void) const
{
	return bitIdx_ == capacity_;
}


inline typename BitStream::TypePtr BitStream::ptr(void)
{
	return start_;
}

inline typename BitStream::ConstTypePtr BitStream::ptr(void) const
{
	return start_;
}

inline typename BitStream::TypePtr BitStream::data(void)
{
	return start_;
}

inline typename BitStream::ConstTypePtr BitStream::data(void) const
{
	return start_;
}


inline typename BitStream::Iterator BitStream::begin(void)
{
	return start_;
}

inline typename BitStream::ConstIterator BitStream::begin(void) const
{
	return start_;
}

inline typename BitStream::Iterator BitStream::end(void)
{
	return start_ + numBytesForBits(bitIdx_);
}

inline typename BitStream::ConstIterator BitStream::end(void) const
{
	return start_ + numBytesForBits(bitIdx_);
}



// ----------------------------

inline typename BitStream::size_type BitStream::numBytesForBits(size_t numBits) const
{
	// align so that all the bits fit.
	return (numBits + 7) >> 3;
}


inline typename BitStream::size_type BitStream::numBitsForBytes(size_t numBytes) const
{
	return numBytes << 3;
}


// for easy memory allocation changes later.
inline void BitStream::Delete(Type* pData) const
{
	X_DELETE_ARRAY(pData,arena_);
}

inline typename BitStream::Type* BitStream::Allocate(size_type numBytes) const
{
	return X_NEW_ARRAY(Type, numBytes, arena_, "BitStream");
}
