#pragma once

#ifndef X_CONTAINER_HASH_INDEX_H_
#define X_CONTAINER_HASH_INDEX_H_

#include <Hashing\Fnva1Hash.h>

X_NAMESPACE_BEGIN(core)

class XHashIndex
{
	static const uint32_t DEFAULT_HASH_SIZE = 1024;
	static const uint32_t DEFAULT_HASH_GRANULARITY = 1024;

	typedef int32_t index_type;
	typedef int32_t size_type;
	typedef uint32_t key_type;

public:
	XHashIndex(core::MemoryArenaBase* arena);
	XHashIndex(core::MemoryArenaBase* arena, const size_type initialHashSize, const size_type initialIndexSize);
	XHashIndex(XHashIndex&& oth);
	~XHashIndex(void);

	XHashIndex& operator=(const XHashIndex& oth);
	XHashIndex& operator=(XHashIndex&& oth);

	// clear the hash
	void clear(void);
	// clear and resize
	void clear(const size_type newHashSize, const size_type newIndexSize);
	// free allocated memory
	void free(void);

	// add an index to the hash, assumes the index has not yet been added to the hash
	void add(const uint32_t key, const index_type index);
	// remove an index from the hash
	void remove(const uint32_t key, const index_type index);
	// get the first index from the hash, returns -1 if empty hash entry
	X_INLINE index_type first(const uint32_t key) const;
	// get the next index from the hash, returns -1 if at the end of the hash chain
	X_INLINE index_type next(const index_type index) const;
	// insert an entry into the index and add it to the hash, increasing all indexes >= index
	void insertIndex(const uint32_t key, const index_type index);
	// remove an entry from the index and remove it from the hash, decreasing all indexes >= index
	void removeIndex(const uint32_t key, const index_type index);
	// get size of hash table
	X_INLINE size_type getHashSize(void) const;
	// get size of the index
	X_INLINE size_type getIndexSize(void) const;
	// set granularity
	X_INLINE void setGranularity(const size_type newGranularity);
	// force resizing the index, current hash table stays intact
	void resizeIndex(const size_type newIndexSize);
	// returns number in the range [0-100] representing the spread over the hash table
	size_type getSpread(void) const;
	// returns total size of allocated memory
	X_INLINE size_t allocated(void) const;
	// returns a key for a string
	X_INLINE uint32_t generateKey(const char* string, bool caseSensitive = true) const;
	// returns a key for a vector
	X_INLINE uint32_t generateKey(const Vec3f& v) const;
	// returns a key for two integers
	X_INLINE uint32_t generateKey(const int n1, const int n2) const;

private:
	void allocate(const size_type newHashSize, const size_type newIndexSize);

private:
	static size_type INVALID_INDEX[1];

	size_type		hashSize_;
	index_type*		hash_;
	size_type       indexSize_;
	index_type*		indexChain_;
	size_type		granularity_;
	int				hashMask_;
	int				lookupMask_;

	core::MemoryArenaBase* arena_;
};

#include "HashIndex.inl"

X_NAMESPACE_END

#endif // !X_CONTAINER_HASH_INDEX_H_