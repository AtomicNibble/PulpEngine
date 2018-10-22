

X_INLINE size_t XHashIndex::allocated(void) const
{
    return hashSize_ * sizeof(int) + indexSize_ * sizeof(int);
}

X_INLINE void XHashIndex::add(const uint32_t key, const index_type index)
{
    X_ASSERT(index >= 0, "invalid index")(index);

    if (hash_ == INVALID_INDEX_BLOCK) {
        allocate(hashSize_, index >= indexSize_ ? index + 1 : indexSize_);
    }
    else if (index >= indexSize_) {
        resizeIndex(index + 1);
    }
    index_type h = key & hashMask_;
    indexChain_[index] = hash_[h];
    hash_[h] = index;
}

X_INLINE int XHashIndex::first(const uint32_t key) const
{
    return hash_[key & hashMask_ & lookupMask_];
}

X_INLINE int XHashIndex::next(const index_type index) const
{
    X_ASSERT(index >= 0 && index < indexSize_, "invalid index")(index, indexSize_);
    return indexChain_[index & lookupMask_];
}

X_INLINE XHashIndex::size_type XHashIndex::getHashSize(void) const
{
    return hashSize_;
}

X_INLINE XHashIndex::size_type XHashIndex::getIndexSize(void) const
{
    return indexSize_;
}

X_INLINE void XHashIndex::setGranularity(const size_type newGranularity)
{
    X_ASSERT(newGranularity > 0, "granularity must be positive")(newGranularity);
    granularity_ = newGranularity;
}

X_INLINE uint32_t XHashIndex::generateKey(const char* startInclusive, const char* endExclusive, bool caseSensitive) const
{
    auto len = static_cast<size_t>(endExclusive - startInclusive);

    if (caseSensitive) {
        return core::Hash::Fnv1aHash(startInclusive, len);
    }
    else {
        X_ASSERT_NOT_IMPLEMENTED();
        return core::Hash::Fnv1aHash(startInclusive, len);
    }
}

X_INLINE uint32_t XHashIndex::generateKey(const Vec3f& v) const
{
    return ((((int)v[0]) + ((int)v[1]) + ((int)v[2])) & hashMask_);
}

