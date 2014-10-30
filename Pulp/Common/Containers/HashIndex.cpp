#include <EngineCommon.h>
#include "HashIndex.h"

X_NAMESPACE_BEGIN(core)


int XHashIndex::INVALID_INDEX[1] = { -1 };


void XHashIndex::init(const int initialHashSize, const int initialIndexSize)
{
	X_ASSERT(core::bitUtil::IsPowerOfTwo(initialHashSize), "size must be power of 2")(initialHashSize);

	hashSize_ = initialHashSize;
	hash_ = INVALID_INDEX;
	indexSize_ = initialIndexSize;
	indexChain_ = INVALID_INDEX;
	granularity_ = DEFAULT_HASH_GRANULARITY;
	hashMask_ = hashSize_ - 1;
	lookupMask_ = 0;
}


void XHashIndex::allocate(const int newHashSize, const int newIndexSize)
{
	X_ASSERT_NOT_NULL(arena_);
	X_ASSERT(core::bitUtil::IsPowerOfTwo(newHashSize), "size must be power of 2")(newHashSize);

	free();
	hashSize_ = newHashSize;
	hash_ = X_NEW_ARRAY(int,hashSize_,arena_, "hash");
	memset(hash_, 0xff, hashSize_ * sizeof(hash_[0]));
	indexSize_ = newIndexSize;
	indexChain_ = X_NEW_ARRAY(int,indexSize_,arena_,"index");
	memset(indexChain_, 0xff, indexSize_ * sizeof(indexChain_[0]));
	hashMask_ = hashSize_ - 1;
	lookupMask_ = -1;
}


void XHashIndex::free(void)
{
	X_ASSERT_NOT_NULL(arena_);

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


void XHashIndex::resizeIndex(const int newIndexSize)
{
	X_ASSERT_NOT_NULL(arena_);

	int* oldIndexChain, mod, newSize;

	if (newIndexSize <= indexSize_) {
		return;
	}

	mod = newIndexSize % granularity_;
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

	oldIndexChain = indexChain_;
	indexChain_ = X_NEW_ARRAY(int,newSize,arena_,"index");
	memcpy(indexChain_, oldIndexChain, indexSize_ * sizeof(int));
	memset(indexChain_ + indexSize_, 0xff, (newSize - indexSize_) * sizeof(int));
	X_DELETE_ARRAY(oldIndexChain, arena_);
	indexSize_ = newSize;
}


int XHashIndex::getSpread(void) const 
{
	X_ASSERT_NOT_NULL(arena_);
	int i, index, totalItems, *numHashItems, average, error, e;

	if (hash_ == INVALID_INDEX) {
		return 100;
	}

	totalItems = 0;
	numHashItems = X_NEW_ARRAY(int, hashSize_, arena_, "hash");
	for (i = 0; i < hashSize_; i++) {
		numHashItems[i] = 0;
		for (index = hash_[i]; index >= 0; index = indexChain_[index]) {
			numHashItems[i]++;
		}
		totalItems += numHashItems[i];
	}
	// if no items in hash
	if (totalItems <= 1) {
		X_DELETE_ARRAY(numHashItems,arena_);
		return 100;
	}
	average = totalItems / hashSize_;
	error = 0;
	for (i = 0; i < hashSize_; i++) {
		e = abs(numHashItems[i] - average);
		if (e > 1) {
			error += e - 1;
		}
	}
	X_DELETE_ARRAY(numHashItems,arena_);
	return 100 - (error * 100 / totalItems);
}




X_NAMESPACE_END