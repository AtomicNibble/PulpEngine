#pragma once

#include <Util\BitUtil.h>

X_NAMESPACE_BEGIN(core)

// can stream memory in / out.
// but will loop around.
class FixedByteStreamRingBase
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

protected:
    FixedByteStreamRingBase(size_type numBytes);
    FixedByteStreamRingBase(TypePtr pBegin, size_type numBytes);

public:
    // writes the type to the stream.
    template<typename T>
    inline void write(const T& val);
    // writes the type * num to the stream.
    template<typename T>
    inline void write(const T* val, size_type num);

    inline void write(const Type* pBuf, size_type numBytes);

    // removes and returns the top object off the stream.
    template<typename T>
    inline T read(void);

    template<typename T>
    inline void read(T& val);
    // read the type * num from the stream.
    template<typename T>
    inline void read(T* pVal, size_type num);

    inline void read(Type* pBuf, size_type numBytes);

    template<typename T>
    inline void peek(size_type offset, T* pVal, size_type num) const;

    inline void peek(size_type offset, Type* pBuf, size_type numBytes) const;

    // can't take refrence, only value.
    // since the T may be lying on the end of the ring.
    template<typename T>
    inline typename std::enable_if<std::is_trivially_copyable<T>::value && !std::is_reference<T>::value, T>::type
        peek(void) const;

    template<typename T>
    inline typename std::enable_if<std::is_trivially_copyable<T>::value && !std::is_reference<T>::value, T>::type
        peek(size_type offset) const;

    inline size_type absoluteToRelativeOffset(size_type offset) const;

    // skips forwward in read pointer.
    inline void skip(size_type numBytes);

    // pads the bit stream until the stream length is equal to length.
    // will not trucate.
    inline void zeroPadToLength(size_type numBytes);

    // clears the stream setting the cursor back to the start.
    inline void reset(void);

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

protected:
    size_type mask_;
    size_type numBytes_;
    size_type readByteIdx_;
    size_type byteIdx_;
    Type* pBegin_;
};

class FixedByteStreamRingNoneOwningPolicy : public FixedByteStreamRingBase
{
public:
    FixedByteStreamRingNoneOwningPolicy(Type* pBegin, Type* pEnd, bool dataInit);
    ~FixedByteStreamRingNoneOwningPolicy();
};

class FixedByteStreamRingOwningPolicy : public FixedByteStreamRingBase
{
public:
    FixedByteStreamRingOwningPolicy(core::MemoryArenaBase* arena, size_type numBytes);
    ~FixedByteStreamRingOwningPolicy();

private:
    core::MemoryArenaBase* arena_;
};

template<size_t N>
class FixedByteStreamRingStackPolicy : public FixedByteStreamRingBase
{
public:
    FixedByteStreamRingStackPolicy();
    ~FixedByteStreamRingStackPolicy();

protected:
    static_assert(sizeof(Type) == 1, "Read comment :)");
    Type buf_[N]; // this is fine, since will only ever be 8bit type.
};

template<class StorageType>
class FixedByteStreamRing : public StorageType
{
public:
    typedef FixedByteStreamRingNoneOwningPolicy NoneOwningPolicy;
    typedef FixedByteStreamRingOwningPolicy OwningPolicy;

    template<size_t N>
    using StackPolicy = FixedByteStreamRing<FixedByteStreamRingStackPolicy<N>>;

public:
    typedef typename StorageType::Type Type;
    typedef typename StorageType::TypePtr TypePtr;
    typedef typename StorageType::value_type value_type;
    typedef typename StorageType::size_type size_type;
    typedef typename StorageType::ConstTypePtr ConstTypePtr;
    typedef typename StorageType::Iterator Iterator;
    typedef typename StorageType::ConstIterator ConstIterator;
    typedef typename StorageType::Reference Reference;
    typedef typename StorageType::reference reference;
    typedef typename StorageType::ConstReference ConstReference;
    typedef typename StorageType::const_reference const_reference;

public:
    using StorageType::StorageType;
};

typedef FixedByteStreamRing<FixedByteStreamRingOwningPolicy> FixedByteStreamRingOwning;
typedef FixedByteStreamRing<FixedByteStreamRingNoneOwningPolicy> FixedByteStreamRingNoneOwning;

template<size_t N>
using FixedByteStreamRingStack = FixedByteStreamRing<FixedByteStreamRingStackPolicy<N>>;

X_NAMESPACE_END

#include "FixedByteStreamRing.inl"