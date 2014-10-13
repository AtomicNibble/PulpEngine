#pragma once


#ifndef _X_CON_BYTESTREAM_H_
#define _X_CON_BYTESTREAM_H_

X_NAMESPACE_BEGIN(core)

// can stream memory in and out.
// not just byte's it supports any kind of data.
class ByteStream
{
public:
	// constructs the stream no memory is allocated.
	inline ByteStream(MemoryArenaBase* arena);

	// writes the type to the stream.
	template<typename T>
	inline void write(const T& val);
	// removes and returns the top object off the stream.
	template<typename T>
	inline T read();
	// returns the top object but dose not remove it.
	template<typename T>
	inline T peek() const;
	// sets the absolute position in the stream.
	inline void seek(size_t pos);

	// resizes the object
	inline void resize(size_t size);
	// clears the stream setting the cursor back to the start.
	// no memory is freed
	inline void reset(void);
	// resets the cursor and clears all memory.
	inline void free(void);

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
	X_NO_COPY(ByteStream);
	X_NO_ASSIGN(ByteStream);

	// for easy memory allocation changes later.
	inline void Delete(char* pData);
	inline char* Allocate(size_t num);

	char* current_;
	char* start_;
	char* end_;

	MemoryArenaBase* arena_;
};

#include "ByteStream.inl"

X_NAMESPACE_END

#endif // !_X_CON_BYTESTREAM_H_
