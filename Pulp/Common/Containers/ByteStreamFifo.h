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
	inline ~ByteStreamFifo(void);

	inline void setArena(MemoryArenaBase* arena);

	// writes the type to the stream.
	template<typename T>
	inline void write(const T& val);

	inline void write(const void* pVal, size_t size);

	// reads from the start.
	template<typename T>
	inline T read();

	// returns the current object but dose not remove it.
	template<typename T>
	inline T& peek();

	template<typename T>
	inline const T peek() const;

	// sets the absolute position in the stream.
	inline void seek(size_t pos);

	// skips number of items
	template<typename T>
	inline void skip(size_t num);

	// resizes the object
	inline void resize(size_t size);
	// clears the stream setting the write / read back to the start.
	// no memory is freed
	inline void reset(void);
	// resets the cursor and clears all memory.
	inline void free(void);

	inline bool isEmpty(void) const;
	// returns how many bytes are currently stored in the stream.
	inline size_t size() const;
	// returns the capacity of the byte stream.
	inline size_t capacity() const;
	// returns the amount of bytes that can be added.
	inline size_t freeSpace() const;
	// returns true if the stream is full.
	inline bool isEos() const;

	inline char* begin() const {
		return start_;
	}

protected:
	X_NO_COPY(ByteStreamFifo);
	X_NO_ASSIGN(ByteStreamFifo);

	// for easy memory allocation changes later.
	inline void Delete(char* pData);
	inline char* Allocate(size_t num);

	char* read_;
	char* write_;
	char* start_;
	char* end_;

	MemoryArenaBase* arena_;
};

#include "ByteStreamFifo.inl"

X_NAMESPACE_END

#endif // !_X_CON_BYTESTREAM_FIFO_H_
