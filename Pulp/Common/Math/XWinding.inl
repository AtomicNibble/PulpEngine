



template<class Allocator>
X_INLINE const Vec5f& XWindingT<Allocator>::operator[](const size_t idx) const
{
	X_ASSERT(static_cast<int32_t>(idx) < numPoints_, "index out of range")(idx, getNumPoints());
	return pPoints_[idx];
}

template<class Allocator>
X_INLINE Vec5f&	XWindingT<Allocator>::operator[](const size_t idx)
{
	X_ASSERT(static_cast<int32_t>(idx) < numPoints_, "index out of range")(idx, getNumPoints());
	return pPoints_[idx];
}


// add a point to the end of the winding point array
template<class Allocator>
X_INLINE XWindingT<Allocator>& XWindingT<Allocator>::operator+=(const Vec5f& v)
{
	addPoint(v);
	return *this;
}

template<class Allocator>
X_INLINE XWindingT<Allocator>& XWindingT<Allocator>::operator+=(const Vec3f& v)
{
	addPoint(v);
	return *this;
}

template<class Allocator>
X_INLINE void XWindingT<Allocator>::addPoint(const Vec5f& v)
{
	EnsureAlloced(numPoints_ + 1, true);
	pPoints_[numPoints_] = v;
	numPoints_++;
}

template<class Allocator>
X_INLINE void XWindingT<Allocator>::addPoint(const Vec3f& v)
{
	EnsureAlloced(numPoints_ + 1, true);
	pPoints_[numPoints_] = v;
	numPoints_++;
}

template<class Allocator>
X_INLINE size_t XWindingT<Allocator>::getNumPoints(void) const
{
	return numPoints_;
}

template<class Allocator>
X_INLINE size_t XWindingT<Allocator>::getAllocatedSize(void) const
{
	return allocedSize_;
}


template<class Allocator>
X_INLINE void XWindingT<Allocator>::EnsureAlloced(size_t n, bool keep)
{
	int32_t num = safe_static_cast<int32_t, size_t>(n);
	if (num > allocedSize_) {
		ReAllocate(num, keep);
	}
}

template<class Allocator>
X_INLINE void XWindingT<Allocator>::ReAllocate(int32_t num, bool keep)
{
	Vec5f* pOldPoints = pPoints_;
	
	num = core::bitUtil::RoundUpToMultiple(num, 4);
	pPoints_ = allocator_.alloc(num);

	if (pOldPoints) {
		if (keep) {
			memcpy(pPoints_, pOldPoints, numPoints_ * sizeof(pPoints_[0]));
		}
		X_DELETE_ARRAY(pOldPoints, gEnv->pArena);
	}

	allocedSize_ = num;
}