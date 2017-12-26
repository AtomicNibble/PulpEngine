#include <EngineCommon.h>
#include "HashIndex.h"

X_NAMESPACE_BEGIN(core)


XHashIndex::size_type XHashIndex::INVALID_INDEX[1] = { -1 };

XHashIndex::XHashIndex(core::MemoryArenaBase* arena) :
	XHashIndex(arena, DEFAULT_HASH_SIZE, DEFAULT_HASH_SIZE)
{

}

XHashIndex::XHashIndex(core::MemoryArenaBase* arena, const size_type initialHashSize, const size_type initialIndexSize)
{
	arena_ = X_ASSERT_NOT_NULL(arena);

	X_ASSERT(core::bitUtil::IsPowerOfTwo(initialHashSize), "size must be power of 2")(initialHashSize);

	hashSize_ = initialHashSize;
	hash_ = INVALID_INDEX;
	indexSize_ = initialIndexSize;
	indexChain_ = INVALID_INDEX;
	granularity_ = DEFAULT_HASH_GRANULARITY;
	hashMask_ = hashSize_ - 1;
	lookupMask_ = 0;
}


XHashIndex::XHashIndex(XHashIndex&& oth) 
{
	hashSize_ = oth.hashSize_;
	hash_ = oth.hash_;
	indexSize_ = oth.indexSize_;
	indexChain_ = oth.indexChain_;
	granularity_ = oth.granularity_;
	hashMask_ = oth.hashMask_;
	lookupMask_ = oth.lookupMask_;
	arena_ = oth.arena_;


	oth.hash_ = INVALID_INDEX;
	oth.indexChain_ = INVALID_INDEX;
	oth.lookupMask_ = 0;
}


XHashIndex::~XHashIndex(void)
{
	free();
}


XHashIndex& XHashIndex::operator=(const XHashIndex& oth)
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
			indexChain_ = X_NEW_ARRAY(index_type, indexSize_, arena_, "HashIndex:Chain");
		}

		memcpy(hash_, oth.hash_, hashSize_ * sizeof(hash_[0]));
		memcpy(indexChain_, oth.indexChain_, indexSize_ * sizeof(indexChain_[0]));
	}

	return *this;
}

XHashIndex& XHashIndex::operator=(XHashIndex&& oth)
{
	hashSize_ = oth.hashSize_;
	hash_ = oth.hash_;
	indexSize_ = oth.indexSize_;
	indexChain_ = oth.indexChain_;
	granularity_ = oth.granularity_;
	hashMask_ = oth.hashMask_;
	lookupMask_ = oth.lookupMask_;
	arena_ = oth.arena_;


	oth.hash_ = INVALID_INDEX;
	oth.indexChain_ = INVALID_INDEX;
	oth.lookupMask_ = 0;
	return *this;
}

void XHashIndex::clear(void)
{
	// only clear the hash table because clearing the indexChain is not really needed
	if (hash_ != INVALID_INDEX) {
		std::memset(hash_, 0xff, hashSize_ * sizeof(hash_[0]));
	}
}

void XHashIndex::clear(const size_type newHashSize, const size_type newIndexSize)
{
	free();
	hashSize_ = newHashSize;
	indexSize_ = newIndexSize;
}


void XHashIndex::free(void)
{
	if (hash_ != INVALID_INDEX) {
		X_DELETE_ARRAY(hash_,arena_);
		hash_ = INVALID_INDEX;
	}
	if (indexChain_ != INVALID_INDEX) {
		X_DELETE_ARRAY(indexChain_, arena_);
		indexChain_ = INVALID_INDEX;
	}

	lookupMask_ = 0;
}


void XHashIndex::insertIndex(const uint32_t key, const index_type index)
{
	if (hash_ != INVALID_INDEX) {
		index_type max = index;

		index_type i;
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


void XHashIndex::removeIndex(const uint32_t key, const index_type index)
{
	remove(key, index);

	if (hash_ != INVALID_INDEX) {

		index_type i, max = index;
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


void XHashIndex::resizeIndex(const size_type newIndexSize)
{
	if (newIndexSize <= indexSize_) {
		return;
	}

	int mod = newIndexSize % granularity_;
	int newSize;
	if (!mod) {
		newSize = newIndexSize;
	}
	else {
		newSize = newIndexSize + granularity_ - mod;
	}

	if (indexChain_ == INVALID_INDEX) {
		indexSize_ = newSize;
		return;
	}

	index_type* oldIndexChain = indexChain_;
	indexChain_ = X_NEW_ARRAY(index_type, newSize, arena_, "index");
	memcpy(indexChain_, oldIndexChain, indexSize_ * sizeof(index_type));
	memset(indexChain_ + indexSize_, 0xff, (newSize - indexSize_) * sizeof(index_type));
	X_DELETE_ARRAY(oldIndexChain, arena_);
	indexSize_ = newSize;
}


int XHashIndex::getSpread(void) const
{
	if (hash_ == INVALID_INDEX) {
		return 100;
	}

	int totalItems = 0;
	int* numHashItems = X_NEW_ARRAY(int, hashSize_, arena_, "hash");

	for (int i = 0; i < hashSize_; i++) {
		numHashItems[i] = 0;
		for (int index = hash_[i]; index >= 0; index = indexChain_[index]) {
			numHashItems[i]++;
		}
		totalItems += numHashItems[i];
	}

	// if no items in hash
	if (totalItems <= 1) {
		X_DELETE_ARRAY(numHashItems, arena_);
		return 100;
	}

	int average = totalItems / hashSize_;
	int error = 0;

	for (int i = 0; i < hashSize_; i++) {
		int e = abs(numHashItems[i] - average);
		if (e > 1) {
			error += e - 1;
		}
	}

	X_DELETE_ARRAY(numHashItems, arena_);
	return 100 - (error * 100 / totalItems);
}


void XHashIndex::allocate(const size_type newHashSize, const size_type newIndexSize)
{
	X_ASSERT_NOT_NULL(arena_);
	X_ASSERT(core::bitUtil::IsPowerOfTwo(newHashSize), "size must be power of 2")(newHashSize);

	free();
	hashSize_ = newHashSize;
	hash_ = X_NEW_ARRAY(index_type, hashSize_, arena_, "hash");
	memset(hash_, 0xff, hashSize_ * sizeof(hash_[0]));
	indexSize_ = newIndexSize;
	indexChain_ = X_NEW_ARRAY(index_type, indexSize_, arena_, "index");
	memset(indexChain_, 0xff, indexSize_ * sizeof(indexChain_[0]));
	hashMask_ = hashSize_ - 1;
	lookupMask_ = -1;
}



X_NAMESPACE_END