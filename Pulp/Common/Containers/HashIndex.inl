

X_INLINE size_t XHashIndex::allocated(void) const
{
    return hashSize_ * sizeof(int) + indexSize_ * sizeof(int);
}

X_INLINE void XHashIndex::add(const uint32_t key, const index_type index)
{
    X_ASSERT(index >= 0, "invalid index")(index);

    if (hash_ == INVALID_INDEX) {
        allocate(hashSize_, index >= indexSize_ ? index + 1 : indexSize_);
    }
    else if (index >= indexSize_) {
        resizeIndex(index + 1);
    }
    index_type h = key & hashMask_;
    indexChain_[index] = hash_[h];
    hash_[h] = index;
}

X_INLINE void XHashIndex::remove(const uint32_t key, const index_type index)
{
    key_type k = key & hashMask_;

    if (hash_ == INVALID_INDEX) {
        return;
    }
    if (hash_[k] == index) {
        hash_[k] = indexChain_[index];
    }
    else {
        for (int i = hash_[k]; i != -1; i = indexChain_[i]) {
            if (indexChain_[i] == index) {
                indexChain_[i] = indexChain_[index];
                break;
            }
        }
    }
    indexChain_[index] = -1;
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

X_INLINE uint32_t XHashIndex::generateKey(const char* string, bool caseSensitive) const
{
    if (caseSensitive) {
        return core::Hash::Fnv1aHash(string, strlen(string));
    }
    else {
        X_ASSERT_NOT_IMPLEMENTED();
        return core::Hash::Fnv1aHash(string, strlen(string));
    }
}

X_INLINE uint32_t XHashIndex::generateKey(const Vec3f& v) const
{
    return ((((int)v[0]) + ((int)v[1]) + ((int)v[2])) & hashMask_);
}

X_INLINE uint32_t XHashIndex::generateKey(const int n1, const int n2) const
{
    return ((n1 + n2) & hashMask_);
}
