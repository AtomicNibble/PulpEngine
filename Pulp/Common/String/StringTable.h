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

    X_INLINE StringTable();

    X_INLINE IdType addString(const char* str);
    X_INLINE IdType addString(const char* str, size_t Len);

    X_INLINE const char* getString(IdType ID) const;

    X_INLINE size_t numStrings(void) const
    {
        return NumStrings_;
    }

    X_INLINE size_t wastedBytes(void) const
    {
        return WasteSize_;
    }

protected:
    IdType MaxBlocks_;
    IdType CurrentBlock_;
    size_t NumStrings_;
    size_t WasteSize_;
    core::MemCursor Cursor_;
    BYTE Buffer_[NumBlocks * BlockSize];
};

/// \ingroup String
/// \class StringTableUnique
/// \brief A class that stores strings with low memory indentifiers, prevents duplicates
/// \details Extends the default StringTable but pools strings, if a string already exsists
/// in the table it will return the id to that string instead of appending it.
///
/// Very useful for joint names since there will be many models with simular bone names.
///
/// Typically used in tools, at runtime duplicated should of already been removed.
///
/// InsertUnique() is O(log n)
template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
class StringTableUnique : public StringTable<NumBlocks, BlockSize, Alignment, IdType>
{
public:
    X_INLINE StringTableUnique();
    X_INLINE ~StringTableUnique();

    X_INLINE IdType addStringUnqiue(const char* Str);
    X_INLINE IdType addStringUnqiue(const char* Str, size_t Len);

protected:
    static const int CHAR_TRIE_SIZE = 128; // support Ascii set

    typedef struct Node
    {
        Node()
        {
            core::zero_this(this);
        }
        IdType id;
        union
        {
            void* sentinel;
            union
            {
                Node* chars[CHAR_TRIE_SIZE];
            };
        };
    } Node;

    X_INLINE void AddStringToTrie(const char* Str, IdType id);
    X_INLINE bool FindString(const char* Str, size_t Len, IdType& id);

    size_t LongestStr_;
    size_t NumNodes_;
    Node searchTree_;
};

#include "StringTable.inl"

X_NAMESPACE_END

#endif // X_STRING_TABLE_H_