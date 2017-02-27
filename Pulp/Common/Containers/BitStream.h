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
	typedef uint8_t Type;
	typedef Type value_type;
	typedef Type* TypePtr;
	typedef const Type* ConstTypePtr;
	typedef Type* Iterator;
	typedef const Type* ConstIterator;
	typedef size_t size_type;
	typedef Type& Reference;
	typedef Type& reference;
	typedef const Type& ConstReference;
	typedef const Type& const_reference;


public:
	// constructs the stream no memory is allocated.
	inline BitStream(MemoryArenaBase* arena);
	inline BitStream(MemoryArenaBase* arena, size_type numBits);
	inline BitStream(const BitStream& oth);
	inline BitStream(BitStream&& oth);
	inline ~BitStream();

	inline BitStream& operator=(const BitStream& oth);
	inline BitStream& operator=(BitStream&& oth);

	// writes a bit to the stream
	inline void write(bool bit);
	// removes and returns the top bit off the stream.
	inline bool read(void);
	// returns the top bit but dose not remove it.
	inline bool peek(void) const;
	// sets the absolute bit position in the stream.
	inline void seek(size_type pos);


	// resizes the object to holx X bits
	inline void resize(size_type numBits);
	// clears the stream setting the cursor back to the start.
	// no memory is freed
	inline void reset(void);
	// resets the cursor and clears all memory.
	inline void free(void);

	// returns how many bits are currently stored in the stream.
	inline size_type size(void) const;
	// returns how many bits it can store.
	inline size_type capacity(void) const;
	// returns the amount of bits that can be added.
	inline size_type freeSpace(void) const;
	// returns true if the stream is full.
	inline bool isEos(void) const;


	inline TypePtr ptr(void);
	inline ConstTypePtr ptr(void) const;
	inline TypePtr data(void);
	inline ConstTypePtr data(void) const;

	inline Iterator begin(void);
	inline ConstIterator begin(void) const;
	inline Iterator end(void);
	inline ConstIterator end(void) const;
	
private:

	// for easy memory allocation changes later.
	inline size_type numBytesForBits(size_type numBits) const;
	inline size_type numBitsForBytes(size_type numBytes) const;
	inline void Delete(Type* pData) const;
	inline Type* Allocate(size_type numBytes) const;

	size_type capacity_;
	size_type bitIdx_;		// the current bit index.
	Type* start_;

	MemoryArenaBase* arena_;
};

#include "BitStream.inl"

X_NAMESPACE_END

#endif // !_X_CON_BITSTREAM_H_
