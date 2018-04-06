#pragma once

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(net)

// stores a collection of ranges.
// to efficentially store a collection of index's where some are missing.
template<typename T>
class RangeList
{
public:
    struct RangeNode
    {
        RangeNode() :
            min(0),
            max(0)
        {
        }
        RangeNode(T min, T max) :
            min(min),
            max(max)
        {
        }

        typedef T Type;
        T min;
        T max;
    };

    typedef T RangeType;
    typedef RangeNode RangeNodeType;
    typedef core::Array<RangeNode> RangeArr;

    typedef typename RangeArr::Iterator Iterator;
    typedef typename RangeArr::ConstIterator ConstIterator;
    typedef typename RangeArr::Reference Reference;
    typedef typename RangeArr::ConstReference ConstReference;
    typedef typename RangeArr::size_type size_type;

public:
    RangeList(core::MemoryArenaBase* arena);

    size_t writeToBitStream(core::FixedBitStreamBase& bs, BitSizeT maxBits, bool removeAdded);
    bool fromBitStream(core::FixedBitStreamBase& bs);

    void add(RangeType val);

    X_INLINE const T& operator[](typename RangeArr::size_type idx) const;
    X_INLINE T& operator[](typename RangeArr::size_type idx);

    // exposing some of the arr members.
    X_INLINE const bool isEmpty(void) const;
    X_INLINE const bool isNotEmpty(void) const;

    X_INLINE void reserve(typename RangeArr::size_type size);

    X_INLINE void clear(void);
    X_INLINE void free(void);

    X_INLINE size_type size(void) const;
    X_INLINE size_type capacity(void) const;

    X_INLINE Iterator begin(void);
    X_INLINE ConstIterator begin(void) const;
    X_INLINE Iterator end(void);
    X_INLINE ConstIterator end(void) const;
    X_INLINE Reference front(void);
    X_INLINE ConstReference front(void) const;
    X_INLINE Reference back(void);
    X_INLINE ConstReference back(void) const;

private:
    RangeArr ranges_;
};

X_NAMESPACE_END

#include "RangeList.inl"