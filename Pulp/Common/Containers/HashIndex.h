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

public:
	XHashIndex(core::MemoryArenaBase* arena);
	XHashIndex(core::MemoryArenaBase* arena, const int initialHashSize, const int initialIndexSize);
	~XHashIndex(void);

	// returns total size of allocated memory
	size_t allocated(void) const;
	// returns total size of allocated memory including size of hash index type
	size_t size(void) const;

	void setArena(core::MemoryArenaBase* arena);


	XHashIndex& operator=(const XHashIndex& oth);
	// add an index to the hash, assumes the index has not yet been added to the hash
	void add(const uint32_t key, const index_type index);
	// remove an index from the hash
	void remove(const uint32_t key, const index_type index);
	// get the first index from the hash, returns -1 if empty hash entry
	index_type first(const uint32_t key) const;
	// get the next index from the hash, returns -1 if at the end of the hash chain
	index_type next(const index_type index) const;
	// insert an entry into the index and add it to the hash, increasing all indexes >= index
	void insertIndex(const uint32_t key, const index_type index);
	// remove an entry from the index and remove it from the hash, decreasing all indexes >= index
	void removeIndex(const uint32_t key, const index_type index);
	// clear the hash
	void clear(void);
	// clear and resize
	void clear(const size_type newHashSize, const size_type newIndexSize);
	// free allocated memory
	void free(void);
	// get size of hash table
	size_type getHashSize(void) const;
	// get size of the index
	size_type getIndexSize(void) const;
	// set granularity
	void setGranularity(const size_type newGranularity);
	// force resizing the index, current hash table stays intact
	void resizeIndex(const size_type newIndexSize);
	// returns number in the range [0-100] representing the spread over the hash table
	size_type getSpread(void) const;
	// returns a key for a string
	uint32_t generateKey(const char* string, bool caseSensitive = true) const;
	// returns a key for a vector
	uint32_t generateKey(const Vec3f& v) const;
	// returns a key for two integers
	uint32_t generateKey(const int n1, const int n2) const;

private:
	static int		INVALID_INDEX[1];

	int				hashSize_;
	int *			hash_;
	int				indexSize_;
	int *			indexChain_;
	int				granularity_;
	int				hashMask_;
	int				lookupMask_;

	core::MemoryArenaBase* arena_;

	void			init(const int initialHashSize, const int initialIndexSize);
	void			allocate(const int newHashSize, const int newIndexSize);
};

#include "HashIndex.inl"

X_NAMESPACE_END

#endif // !X_CONTAINER_HASH_INDEX_H_