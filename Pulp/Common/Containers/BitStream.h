#pragma once


#ifndef _X_CON_BITSTREAM_H_
#define _X_CON_BITSTREAM_H_


X_NAMESPACE_BEGIN(core)

#ifdef GetFreeSpace
#undef GetFreeSpace
#endif

// can stream memory in and out.
class BitStream
{
public:
	// constructs the stream no memory is allocated.
	inline BitStream(MemoryArenaBase* arena);

	// writes a bit to the stream
	inline void write(bool bit);
	// removes and returns the top bit off the stream.
	inline bool read();
	// returns the top bit but dose not remove it.
	inline bool peek() const;
	// sets the absolute bit position in the stream.
	inline void seek(size_t pos);


	// resizes the object to holx X bits
	inline void resize(size_t numBits);
	// clears the stream setting the cursor back to the start.
	// no memory is freed
	inline void reset(void);
	// resets the cursor and clears all memory.
	inline void free(void);

	// returns how many bits are currently stored in the stream.
	inline size_t size() const;
	// returns how many bits it can store.
	inline size_t capacity() const;
	// returns the amount of bits that can be added.
	inline size_t freeSpace() const;
	// returns true if the stream is full.
	inline bool isEos() const;
	
private:
	X_NO_COPY(BitStream);
	X_NO_ASSIGN(BitStream);

	// for easy memory allocation changes later.
	inline void Delete(char* pData);
	inline char* Allocate(size_t numbits);

	size_t capacity_;
	size_t bitIdx_;		// the current bit index.
	char* start_;

	MemoryArenaBase* arena_;
};

#include "BitStream.inl"

X_NAMESPACE_END

#endif // !_X_CON_BITSTREAM_H_
