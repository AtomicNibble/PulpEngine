#pragma once


#ifndef _X_CON_BYTESTREAM_FIFO_H_
#define _X_CON_BYTESTREAM_FIFO_H_

X_NAMESPACE_BEGIN(core)

// can stream memory in and out.
// not just byte's it supports any kind of data.
class ByteStreamFifo
{
public:
	// constructs the stream no memory is allocated.
	inline ByteStreamFifo(MemoryArenaBase* arena);
	inline ByteStreamFifo(MemoryArenaBase* arena, size_t numBytes);
	inline ByteStreamFifo(const ByteStreamFifo& oth);
	inline ByteStreamFifo(ByteStreamFifo&& oth);
	inline ~ByteStreamFifo(void);

	inline ByteStreamFifo& operator=(const ByteStreamFifo& oth);
	inline ByteStreamFifo& operator=(ByteStreamFifo&& oth);

	// writes the type to the stream.
	template<typename T>
	inline void write(const T& val);

	inline void write(const void* pVal, size_t size);

	// reads from the start.
	template<typename T>
	inline T read(void);

	// returns the current object but dose not remove it.
	template<typename T>
	inline T& peek(void);

	template<typename T>
	inline const T peek(void) const;

	// sets the absolute position in the stream.
	inline void seekSet(size_t pos);

	// skips number of items
	template<typename T>
	inline void skip(size_t num);

	// resizes the object
	inline void resize(size_t numBytes);
	// clears the stream setting the write / read back to the start.
	// no memory is freed
	inline void reset(void);
	// resets the cursor and clears all memory.
	inline void free(void);

	inline bool isEmpty(void) const;
	// returns how many bytes are currently stored in the stream.
	inline size_t size(void) const;
	// returns the capacity of the byte stream.
	inline size_t capacity(void) const;
	// returns the amount of bytes that can be added.
	inline size_t freeSpace(void) const;
	// returns true if the stream is full.
	inline bool isEos(void) const;

	inline char* begin(void) const {
		return start_;
	}

protected:

	// for easy memory allocation changes later.
	inline void Delete(char* pData) const;
	inline char* Allocate(size_t num) const;

	char* read_;
	char* write_;
	char* start_;
	char* end_;

	MemoryArenaBase* arena_;
};

#include "ByteStreamFifo.inl"

X_NAMESPACE_END

#endif // !_X_CON_BYTESTREAM_FIFO_H_
