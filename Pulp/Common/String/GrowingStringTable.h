#pragma once


#ifndef X_GROWING_STRING_TABLE_H_
#define X_GROWING_STRING_TABLE_H_


#include "Containers\Array.h"

X_NAMESPACE_BEGIN(core)

template<typename IdType>
class GrowingStringTable
{
	X_PRAGMA(pack(push, 4))
	struct Header_t
	{
		uint8_t pad[3]; // i use these later
		uint8_t Length; // 255 max
	};
	X_PRAGMA(pack(pop))

public:
	typedef IdType Id;
	static IdType InvalidId = (IdType)-1;

	GrowingStringTable(core::MemoryArenaBase* arena);
	~GrowingStringTable();

	void setSize(size_t blockGranularity, size_t BlockSize, size_t Alignment);
	void free(void);

	X_INLINE IdType addString(const char* str);
	IdType addString(const char* str, size_t Len);

	const char* getString(IdType ID) const;

	X_INLINE size_t numStrings(void) const;
	X_INLINE size_t wastedBytes(void) const;

private:
	void ensureFreeBlocks(size_t numBlocks);

	Header_t* getCurrentAlignedHeader();

private:
	size_t CurrentBlock_;
	size_t currentBlockSpace_;
	size_t blockGranularity_; // how many blocks we grow by.
	size_t BlockSize_;	// the size of each block.
	size_t Alignment_; // pointers return are aligned to this.

	// stats
	size_t NumStrings_;
	size_t WasteSize_;

	core::Array<uint8_t> buffer_;
	core::MemoryArenaBase* arena_;
};

template<typename IdType>
IdType GrowingStringTable<IdType>::addString(const char* str)
{
	return addString(str, strlen(str));
}

X_NAMESPACE_END


#endif // !X_GROWING_STRING_TABLE_H_