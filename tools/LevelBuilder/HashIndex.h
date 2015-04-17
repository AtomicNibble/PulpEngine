#pragma once

#include <Hashing\Fnva1Hash.h>

X_USING_NAMESPACE;

#define DEFAULT_HASH_SIZE			1024
#define DEFAULT_HASH_GRANULARITY	1024

class XHashIndex 
{
public:
	XHashIndex(void);
	XHashIndex(const int initialHashSize, const int initialIndexSize);
	~XHashIndex(void);

	// returns total size of allocated memory
	size_t			Allocated(void) const;
	// returns total size of allocated memory including size of hash index type
	size_t			Size(void) const;

	XHashIndex &	operator=(const XHashIndex &other);
	// add an index to the hash, assumes the index has not yet been added to the hash
	void			Add(const int key, const int index);
	// remove an index from the hash
	void			Remove(const int key, const int index);
	// get the first index from the hash, returns -1 if empty hash entry
	int				First(const int key) const;
	// get the next index from the hash, returns -1 if at the end of the hash chain
	int				Next(const int index) const;
	// insert an entry into the index and add it to the hash, increasing all indexes >= index
	void			InsertIndex(const int key, const int index);
	// remove an entry from the index and remove it from the hash, decreasing all indexes >= index
	void			RemoveIndex(const int key, const int index);
	// clear the hash
	void			Clear(void);
	// clear and resize
	void			Clear(const int newHashSize, const int newIndexSize);
	// free allocated memory
	void			Free(void);
	// get size of hash table
	int				GetHashSize(void) const;
	// get size of the index
	int				GetIndexSize(void) const;
	// set granularity
	void			SetGranularity(const int newGranularity);
	// force resizing the index, current hash table stays intact
	void			ResizeIndex(const int newIndexSize);
	// returns number in the range [0-100] representing the spread over the hash table
	int				GetSpread(void) const;
	// returns a key for a string
	int				GenerateKey(const char *string, bool caseSensitive = true) const;
	// returns a key for a vector
	int				GenerateKey(const Vec3f &v) const;
	// returns a key for two integers
	int				GenerateKey(const int n1, const int n2) const;

private:
	int				hashSize;
	int *			hash;
	int				indexSize;
	int *			indexChain;
	int				granularity;
	int				hashMask;
	int				lookupMask;

	static int		INVALID_INDEX[1];

	void			Init(const int initialHashSize, const int initialIndexSize);
	void			Allocate(const int newHashSize, const int newIndexSize);
};

/*
================
XHashIndex::XHashIndex
================
*/
X_INLINE XHashIndex::XHashIndex(void) {
	Init(DEFAULT_HASH_SIZE, DEFAULT_HASH_SIZE);
}

/*
================
XHashIndex::XHashIndex
================
*/
X_INLINE XHashIndex::XHashIndex(const int initialHashSize, const int initialIndexSize) {
	Init(initialHashSize, initialIndexSize);
}

/*
================
XHashIndex::~XHashIndex
================
*/
X_INLINE XHashIndex::~XHashIndex(void) {
	Free();
}

/*
================
XHashIndex::Allocated
================
*/
X_INLINE size_t XHashIndex::Allocated(void) const {
	return hashSize * sizeof(int)+indexSize * sizeof(int);
}

/*
================
XHashIndex::Size
================
*/
X_INLINE size_t XHashIndex::Size(void) const {
	return sizeof(*this) + Allocated();
}

/*
================
XHashIndex::operator=
================
*/
X_INLINE XHashIndex &XHashIndex::operator=(const XHashIndex &other) {
	granularity = other.granularity;
	hashMask = other.hashMask;
	lookupMask = other.lookupMask;

	if (other.lookupMask == 0) {
		hashSize = other.hashSize;
		indexSize = other.indexSize;
		Free();
	}
	else {
		if (other.hashSize != hashSize || hash == INVALID_INDEX) {
			if (hash != INVALID_INDEX) {
				delete[] hash;
			}
			hashSize = other.hashSize;
			hash = new int[hashSize];
		}
		if (other.indexSize != indexSize || indexChain == INVALID_INDEX) {
			if (indexChain != INVALID_INDEX) {
				delete[] indexChain;
			}
			indexSize = other.indexSize;
			indexChain = new int[indexSize];
		}
		memcpy(hash, other.hash, hashSize * sizeof(hash[0]));
		memcpy(indexChain, other.indexChain, indexSize * sizeof(indexChain[0]));
	}

	return *this;
}

/*
================
XHashIndex::Add
================
*/
X_INLINE void XHashIndex::Add(const int key, const int index) {
	int h;

	X_ASSERT(index >= 0, "invalid index")(index);
	if (hash == INVALID_INDEX) {
		Allocate(hashSize, index >= indexSize ? index + 1 : indexSize);
	}
	else if (index >= indexSize) {
		ResizeIndex(index + 1);
	}
	h = key & hashMask;
	indexChain[index] = hash[h];
	hash[h] = index;
}

/*
================
XHashIndex::Remove
================
*/
X_INLINE void XHashIndex::Remove(const int key, const int index) {
	int k = key & hashMask;

	if (hash == INVALID_INDEX) {
		return;
	}
	if (hash[k] == index) {
		hash[k] = indexChain[index];
	}
	else {
		for (int i = hash[k]; i != -1; i = indexChain[i]) {
			if (indexChain[i] == index) {
				indexChain[i] = indexChain[index];
				break;
			}
		}
	}
	indexChain[index] = -1;
}

/*
================
XHashIndex::First
================
*/
X_INLINE int XHashIndex::First(const int key) const {
	return hash[key & hashMask & lookupMask];
}

/*
================
XHashIndex::Next
================
*/
X_INLINE int XHashIndex::Next(const int index) const {
	X_ASSERT(index >= 0 && index < indexSize, "invalid index")(index, indexSize);
	return indexChain[index & lookupMask];
}

/*
================
XHashIndex::InsertIndex
================
*/
X_INLINE void XHashIndex::InsertIndex(const int key, const int index) {
	int i, max;

	if (hash != INVALID_INDEX) {
		max = index;
		for (i = 0; i < hashSize; i++) {
			if (hash[i] >= index) {
				hash[i]++;
				if (hash[i] > max) {
					max = hash[i];
				}
			}
		}
		for (i = 0; i < indexSize; i++) {
			if (indexChain[i] >= index) {
				indexChain[i]++;
				if (indexChain[i] > max) {
					max = indexChain[i];
				}
			}
		}
		if (max >= indexSize) {
			ResizeIndex(max + 1);
		}
		for (i = max; i > index; i--) {
			indexChain[i] = indexChain[i - 1];
		}
		indexChain[index] = -1;
	}
	Add(key, index);
}

/*
================
XHashIndex::RemoveIndex
================
*/
X_INLINE void XHashIndex::RemoveIndex(const int key, const int index) {
	int i, max;

	Remove(key, index);
	if (hash != INVALID_INDEX) {
		max = index;
		for (i = 0; i < hashSize; i++) {
			if (hash[i] >= index) {
				if (hash[i] > max) {
					max = hash[i];
				}
				hash[i]--;
			}
		}
		for (i = 0; i < indexSize; i++) {
			if (indexChain[i] >= index) {
				if (indexChain[i] > max) {
					max = indexChain[i];
				}
				indexChain[i]--;
			}
		}
		for (i = index; i < max; i++) {
			indexChain[i] = indexChain[i + 1];
		}
		indexChain[max] = -1;
	}
}

/*
================
XHashIndex::Clear
================
*/
X_INLINE void XHashIndex::Clear(void) {
	// only clear the hash table because clearing the indexChain is not really needed
	if (hash != INVALID_INDEX) {
		memset(hash, 0xff, hashSize * sizeof(hash[0]));
	}
}

/*
================
XHashIndex::Clear
================
*/
X_INLINE void XHashIndex::Clear(const int newHashSize, const int newIndexSize) {
	Free();
	hashSize = newHashSize;
	indexSize = newIndexSize;
}

/*
================
XHashIndex::GetHashSize
================
*/
X_INLINE int XHashIndex::GetHashSize(void) const {
	return hashSize;
}

/*
================
XHashIndex::GetIndexSize
================
*/
X_INLINE int XHashIndex::GetIndexSize(void) const {
	return indexSize;
}

/*
================
XHashIndex::SetGranularity
================
*/
X_INLINE void XHashIndex::SetGranularity(const int newGranularity) {
	X_ASSERT(newGranularity > 0, "granularity must be positive")(newGranularity);
	granularity = newGranularity;
}

/*
================
XHashIndex::GenerateKey
================
*/
X_INLINE int XHashIndex::GenerateKey(const char *string, bool caseSensitive) const {
	if (caseSensitive) {
	//	return (idStr::Hash(string) & hashMask);
		return core::Hash::Fnv1aHash(string, strlen(string));
	}
	else {
	//	return (idStr::IHash(string) & hashMask);
		return core::Hash::Fnv1aHash(string, strlen(string));
	}
}

/*
================
XHashIndex::GenerateKey
================
*/
X_INLINE int XHashIndex::GenerateKey(const Vec3f &v) const {
	return ((((int)v[0]) + ((int)v[1]) + ((int)v[2])) & hashMask);
}

/*
================
XHashIndex::GenerateKey
================
*/
X_INLINE int XHashIndex::GenerateKey(const int n1, const int n2) const {
	return ((n1 + n2) & hashMask);
}
