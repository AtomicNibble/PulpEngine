#pragma once


#ifndef X_GROWING_STRING_TABLE_H_
#define X_GROWING_STRING_TABLE_H_


#include "Containers\Array.h"

X_NAMESPACE_BEGIN(core)

// valid IdTypes: uint16_t, uint32_t

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
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
	typedef IdType IdType;
	static const IdType InvalidId = (IdType)-1;

	GrowingStringTable(core::MemoryArenaBase* arena);
	~GrowingStringTable();

	void reserve(size_t numBlocks);
	void free(void);

	X_INLINE IdType addString(const char* str);
	IdType addString(const char* str, size_t Len);

	const char* getString(IdType ID) const;

	X_INLINE size_t numStrings(void) const;
	X_INLINE size_t wastedBytes(void) const;
	X_INLINE size_t allocatedBytes(void) const;

private:
	void ensureFreeBlocks(size_t numBlocks);
	Header_t* getCurrentAlignedHeader(void);

private:
	X_NO_COPY(GrowingStringTable);
	X_NO_ASSIGN(GrowingStringTable);

	IdType CurrentBlock_;
	IdType currentBlockSpace_;

	// stats
	size_t NumStrings_;
	size_t WasteSize_;

	core::Array<uint8_t> buffer_;
	core::MemoryArenaBase* arena_;
};


#include "GrowingStringTable.inl"

X_NAMESPACE_END


#endif // !X_GROWING_STRING_TABLE_H_