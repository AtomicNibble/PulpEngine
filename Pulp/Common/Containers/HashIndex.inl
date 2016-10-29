

X_INLINE XHashIndex::XHashIndex(core::MemoryArenaBase* arena)
{
	arena_ = arena;
	init(DEFAULT_HASH_SIZE, DEFAULT_HASH_SIZE);
}

X_INLINE XHashIndex::XHashIndex(core::MemoryArenaBase* arena, const int initialHashSize, const int initialIndexSize) 
{
	arena_ = arena;
	init(initialHashSize, initialIndexSize);
}


X_INLINE XHashIndex::~XHashIndex(void) 
{
	free();
}


X_INLINE size_t XHashIndex::allocated(void) const 
{
	return hashSize_ * sizeof(int) + indexSize_ * sizeof(int);
}


X_INLINE size_t XHashIndex::size(void) const 
{
	return sizeof(*this) + allocated();
}

X_INLINE void XHashIndex::setArena(core::MemoryArenaBase* arena)
{
	arena_ = arena;
}

X_INLINE XHashIndex& XHashIndex::operator=(const XHashIndex& oth)
{
	granularity_ = oth.granularity_;
	hashMask_ = oth.hashMask_;
	lookupMask_ = oth.lookupMask_;

	if (oth.lookupMask_ == 0) {
		hashSize_ = oth.hashSize_;
		indexSize_ = oth.indexSize_;
		free();
	}
	else 
	{
		if (oth.hashSize_ != hashSize_ || hash_ == INVALID_INDEX)
		{
			if (hash_ != INVALID_INDEX) {
				X_DELETE_ARRAY(hash_, arena_);
			}
			hashSize_ = oth.hashSize_;
			hash_ = X_NEW_ARRAY(int, hashSize_, arena_, "HashIndex:Hash");
		}

		if (oth.indexSize_ != indexSize_ || indexChain_ == INVALID_INDEX)
		{
			if (indexChain_ != INVALID_INDEX) {
				X_DELETE_ARRAY(indexChain_, arena_);
			}
			indexSize_ = oth.indexSize_;
			indexChain_ = X_NEW_ARRAY(int, indexSize_, arena_, "HashIndex:Chain");
		}

		memcpy(hash_, oth.hash_, hashSize_ * sizeof(hash_[0]));
		memcpy(indexChain_, oth.indexChain_, indexSize_ * sizeof(indexChain_[0]));
	}

	return *this;
}


X_INLINE void XHashIndex::add(const uint32_t key, const index_type index)
{
	int h;

	X_ASSERT(index >= 0, "invalid index")(index);
	if (hash_ == INVALID_INDEX) {
		allocate(hashSize_, index >= indexSize_ ? index + 1 : indexSize_);
	}
	else if (index >= indexSize_) {
		resizeIndex(index + 1);
	}
	h = key & hashMask_;
	indexChain_[index] = hash_[h];
	hash_[h] = index;
}


X_INLINE void XHashIndex::remove(const uint32_t key, const index_type index)
{
	int k = key & hashMask_;

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

X_INLINE void XHashIndex::insertIndex(const uint32_t key, const index_type index)
{
	int i, max;

	if (hash_ != INVALID_INDEX) {
		max = index;
		for (i = 0; i < hashSize_; i++) {
			if (hash_[i] >= index) {
				hash_[i]++;
				if (hash_[i] > max) {
					max = hash_[i];
				}
			}
		}
		for (i = 0; i < indexSize_; i++) {
			if (indexChain_[i] >= index) {
				indexChain_[i]++;
				if (indexChain_[i] > max) {
					max = indexChain_[i];
				}
			}
		}
		if (max >= indexSize_) {
			resizeIndex(max + 1);
		}
		for (i = max; i > index; i--) {
			indexChain_[i] = indexChain_[i - 1];
		}
		indexChain_[index] = -1;
	}
	add(key, index);
}


X_INLINE void XHashIndex::removeIndex(const uint32_t key, const index_type index)
{
	int i, max;

	remove(key, index);
	if (hash_ != INVALID_INDEX) {
		max = index;
		for (i = 0; i < hashSize_; i++) {
			if (hash_[i] >= index) {
				if (hash_[i] > max) {
					max = hash_[i];
				}
				hash_[i]--;
			}
		}
		for (i = 0; i < indexSize_; i++) {
			if (indexChain_[i] >= index) {
				if (indexChain_[i] > max) {
					max = indexChain_[i];
				}
				indexChain_[i]--;
			}
		}
		for (i = index; i < max; i++) {
			indexChain_[i] = indexChain_[i + 1];
		}
		indexChain_[max] = -1;
	}
}


X_INLINE void XHashIndex::clear(void) 
{
	// only clear the hash table because clearing the indexChain is not really needed
	if (hash_ != INVALID_INDEX) {
		memset(hash_, 0xff, hashSize_ * sizeof(hash_[0]));
	}
}

X_INLINE void XHashIndex::clear(const size_type newHashSize, const size_type newIndexSize)
{
	free();
	hashSize_ = newHashSize;
	indexSize_ = newIndexSize;
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
