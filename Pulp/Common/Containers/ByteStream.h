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
    inline ByteStream(MemoryArenaBase* arena);
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
    inline void write(const T* val, size_type num);

    inline void write(const Type* pBuf, size_type numBytes);

    inline void write(const ByteStream& stream);

    // removes and returns the top object off the stream.
    template<typename T>
    inline T read(void);

    template<typename T>
    inline void read(T& val);
    // read the type * num from the stream.
    template<typename T>
    inline void read(T* pVal, size_type num);

    inline void read(Type* pBuf, size_type numBytes);

    // returns the top object but dose not remove it.
    template<typename T>
    inline T peek(void) const;

    void alignWrite(size_t alignment);

    // sets the absolute position in the stream.
    inline void seek(size_type pos);
    // reserves memory, the size is unchanged.
    inline void reserve(size_type numBytes);
    // clears the stream setting the cursor back to the start.
    // no memory is freed
    inline void reset(void);
    // resets the cursor and clears all memory.
    inline void free(void);

    // returns how many bytes are currently stored in the stream.
    inline size_type size(void) const;
    // returns the capacity of the byte stream.
    inline size_type capacity(void) const;
    // returns the amount of bytes that can be added.
    inline size_type freeSpace(void) const;
    // returns the read offset from base.
    inline size_type tell(void) const;
    // returns the write offset from base
    inline size_type tellWrite(void) const;
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
    void ensureSpace(size_type desiredSpace);
    void reallocate(size_type newSize);

    // for easy memory allocation changes later.
    inline void Delete(Type* pData) const;
    inline Type* Allocate(size_type num) const;

    Type* read_;
    Type* write_;
    Type* start_;
    Type* end_;

    MemoryArenaBase* arena_;
};

#include "ByteStream.inl"

X_NAMESPACE_END

#endif // !_X_CON_BYTESTREAM_H_
