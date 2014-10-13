#pragma once

#ifndef X_STRING_TABLE_H_
#define X_STRING_TABLE_H_

#include "StringUtil.h"
#include "Memory\MemCursor.h"

X_NAMESPACE_BEGIN(core)

/// \ingroup String
/// \class StrTable
/// \brief A class that stores strings with low memory indentifiers.
/// \details this class stores a collection of strings in congious memory and returns a indentifier for that string
/// this identifyier can be used to get the string in O(1) but has the advantage of been much smaller than a pointer
/// so storing them takes much less space.
/// the table also makes use of blocking so the max string data than can be store is min(255,(blockSize * numeric_limits<T>::max()))
/// this means a 16 bit interger can refrence upto 65535 strings if every string & header was to fit inside a single block.
/// and up to 1mb of data.
/// 
/// Large block size don't increase the maximum potential strings a type can hold, but make a string more likley to use less blocks.
/// Allowing more strings to fit in.
/// but is more prone to waste due to the block alignment.
template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
class StringTable
{
X_PRAGMA(pack(push, 4))
	struct Header_t
	{
		BYTE pad[3]; // i use these later
		BYTE Length; // 255 max
	};
X_PRAGMA(pack(pop))

public:
	typedef IdType Id;

	StringTable();

	X_INLINE IdType addString(const char* str);
	X_INLINE IdType addString(const char* str, int Len);

	X_INLINE const char* getString(IdType ID) const;

	X_INLINE size_t numStrings(void) const {
		return NumStrings_;
	}

	X_INLINE size_t wastedBytes(void) const {
		return WasteSize_;
	}

protected:
	IdType MaxBlocks_;
	IdType CurrentBlock_;
	size_t NumStrings_;
	size_t WasteSize_;
	core::MemCursor<BYTE*> Cursor_;
	BYTE Buffer_[NumBlocks * BlockSize];
};


template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
StringTable<NumBlocks, BlockSize, Alignment, IdType>::StringTable() :
MaxBlocks_(NumBlocks),
CurrentBlock_(0),
NumStrings_(0),
WasteSize_(0),
Cursor_(Buffer_, NumBlocks * BlockSize)
{
	Cursor_.SeekBytes(4);
}


template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTable<NumBlocks, BlockSize, Alignment, IdType>::addString(const char* Str)
{
	return addString(Str, strUtil::strlen(Str));
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTable<NumBlocks, BlockSize, Alignment, IdType>::addString(const char* Str, int Len)
{
	X_ASSERT(Len < 255, "string is longer than the 255 max")(Len);
	X_ASSERT(CurrentBlock_ < MaxBlocks_, "string table is full")(CurrentBlock_,MaxBlocks_);

	IdType Block = CurrentBlock_;

	// ensure space for null terminator as we return pointers to this memory.
	int NumBlocks = (Len + sizeof(Header_t)+BlockSize) / BlockSize;

	// set the header
	Header_t* head = Cursor_.getSeekPtr<Header_t>();
	head->Length = Len;

	// copy the string.
	memcpy(Cursor_.getPtr<void>(), Str, Len);

	// ensusre null
	memset(Cursor_.getPtr<BYTE>() + Len, '\0', 1);

	Cursor_.SeekBytes((NumBlocks * BlockSize) - sizeof(Header_t));
	CurrentBlock_ += NumBlocks;

	NumStrings_++;
	WasteSize_ += (NumBlocks * BlockSize) - (Len + sizeof(Header_t) + 1);
	return Block;
}


template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
const char* StringTable<NumBlocks, BlockSize, Alignment, IdType>::getString(IdType ID) const
{
	X_ASSERT(ID < MaxBlocks_, "Strid out of range")(ID);
	return reinterpret_cast<const char*>(&Buffer_[ 4 + (BlockSize * ID) + sizeof(Header_t)]);
}


// ---------------------------------

/// \ingroup String
/// \class StringTableUnique
/// \brief A class that stores strings with low memory indentifiers, prevents duplicates
/// \details Extends the default StringTable but pools strings, if a string already exsists
/// in the table it will return the id to that string instead of appending it.
/// 
/// Very useful for joint names since there will be many models with simular bone names.
///
/// InsertUnique() is O(log n)
template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
class StringTableUnique :
	public StringTable<NumBlocks, BlockSize, Alignment, IdType>
{
public:
	StringTableUnique();
	~StringTableUnique();

	X_INLINE IdType addStringUnqiue(const char* Str);
	X_INLINE IdType addStringUnqiue(const char* Str, int Len);

protected:

	static const int CHAR_TRIE_SIZE = 128; // support Ascii set

	typedef struct Node {
		Node() {
			core::zero_this(this);
		}
		IdType id;
		union {
			void *sentinel;
			union {
				Node* chars[CHAR_TRIE_SIZE];
			};
		};
	} Node;


	void AddStringToTrie(const char* Str, IdType id);
	bool FindString(const char* Str, int Len, IdType& id);

	int   LongestStr_;
	int   NumNodes_;
	Node  searchTree_;
};


template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::StringTableUnique() :
	StringTable(),
	LongestStr_(0), 
	NumNodes_(0)
{

}


template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::~StringTableUnique()
{
	Node* node = &searchTree_;
	for (int i = /*skip sentinel*/ 1; i < CHAR_TRIE_SIZE; i++) {
		if (node->chars[i] != nullptr) {
			delete node->chars[i];
		}
	}
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::addStringUnqiue(const char* Str)
{
	return addStringUnqiue(Str, strUtil::strlen(Str));
}


template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::addStringUnqiue(const char* Str, int Len)
{
	IdType id = 0;
	if (this->FindString(Str, Len, id))
		return id;

	// Update longest string
	LongestStr_ = core::Max(LongestStr_, Len);

	id = addString(Str, Len);

	AddStringToTrie(Str, id); // add to search Trie

	return id;
}


template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
void StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::AddStringToTrie(const char* Str, IdType id)
{
	Node* node = &searchTree_;
	const char* Txt = Str;
	int c;
	while ((c = *Txt++)) {
		if (node->chars[c] == nullptr) {
			node->chars[c] = new Node;
			NumNodes_++;
		}
		node = node->chars[c];
	}
	node->id = id;
	node->sentinel = (void*)!nullptr;
}


template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
bool StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::FindString(const char* Str, int Len, IdType& id)
{
	if (Len > LongestStr_) // we can skip checking for strings longer then any in the table.
		return false;

	Node* node = &searchTree_;
	const char* Txt = Str;
	int c;
	while ((c = *Txt++)) {
		if (node->chars[c] == nullptr) {
			return false;
		}
		node = node->chars[c];
	}
	if (node->sentinel != nullptr) {
		id = node->id;
		return true;
	}
	return false;
}

X_NAMESPACE_END

#endif // X_STRING_TABLE_H_