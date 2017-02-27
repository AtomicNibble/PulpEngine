#pragma once


#ifndef _X_CON_BYTESTREAM_H_
#define _X_CON_BYTESTREAM_H_

X_NAMESPACE_BEGIN(core)

// can stream memory in and out.
// not just byte's it supports any kind of data.
class ByteStream
{
public:
	typedef char Type;
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
	inline ByteStream(MemoryArenaBase* arena);
	inline ByteStream(MemoryArenaBase* arena, size_t numBytes);
	inline ByteStream(const ByteStream& oth);
	inline ByteStream(ByteStream&& oth);
	inline ~ByteStream();

	inline ByteStream& operator=(const ByteStream& oth);
	inline ByteStream& operator=(ByteStream&& oth);

	// writes the type to the stream.
	template<typename T>
	inline void write(const T& val);
	// writes the type * num to the stream.
	template<typename T>
	inline void write(const T* val, size_t num);
	// removes and returns the top object off the stream.
	template<typename T>
	inline T read(void);
	// returns the top object but dose not remove it.
	template<typename T>
	inline T peek(void) const;
	// sets the absolute position in the stream.
	inline void seek(size_t pos);

	// resizes the object
	inline void resize(size_t numBytes);
	// clears the stream setting the cursor back to the start.
	// no memory is freed
	inline void reset(void);
	// resets the cursor and clears all memory.
	inline void free(void);

	// returns how many bytes are currently stored in the stream.
	inline size_t size(void) const;
	// returns the capacity of the byte stream.
	inline size_t capacity(void) const;
	// returns the amount of bytes that can be added.
	inline size_t freeSpace(void) const;
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
	inline Reference front(void);
	inline ConstReference front(void) const;
	inline Reference back(void);
	inline ConstReference back(void) const;

protected:

	// for easy memory allocation changes later.
	inline void Delete(char* pData) const;
	inline char* Allocate(size_t num) const;

	Type* current_;
	Type* start_;
	Type* end_;

	MemoryArenaBase* arena_;
};

#include "ByteStream.inl"

X_NAMESPACE_END

#endif // !_X_CON_BYTESTREAM_H_
