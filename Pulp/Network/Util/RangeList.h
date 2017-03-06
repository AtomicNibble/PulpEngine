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
		RangeNode(T min, T max) : 
			min(min), max(max)
		{
		}

		typedef T Type;
		T min;
		T max;
	};

	typedef T RangeType;
	typedef core::Array<RangeNode> RangeArr;


public:
	RangeList(core::MemoryArenaBase* arena);


	void add(RangeType val);

	X_INLINE const T& operator[](typename RangeArr::size_type idx) const;
	X_INLINE T& operator[](typename RangeArr::size_type idx);

	// exposing some of the arr members.
	X_INLINE const bool isEmpty(void) const;
	X_INLINE const bool isNotEmpty(void) const;

	X_INLINE void reserve(typename RangeArr::size_type size);

	X_INLINE void clear(void);
	X_INLINE void free(void);

	X_INLINE typename RangeArr::size_type size(void);
	X_INLINE typename RangeArr::size_type capacity(void);


private:
	RangeArr ranges_;
};



X_NAMESPACE_END

#include "RangeList.inl"