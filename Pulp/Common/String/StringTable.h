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

    X_INLINE IdType addString(const char* pStr);
    X_INLINE IdType addString(const char* pStr, size_t len);

    X_INLINE const char* getString(IdType ID) const;

    X_INLINE size_t numStrings(void) const
    {
        return numStrings_;
    }

    X_INLINE size_t wastedBytes(void) const
    {
        return wasteSize_;
    }

protected:
    const IdType maxBlocks_;
    IdType currentBlock_;
    size_t numStrings_;
    size_t wasteSize_;
    core::MemCursor cursor_;
    uint8_t buffer_[NumBlocks * BlockSize];
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
    using MyType = StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>;

    static const int CHAR_TRIE_SIZE = 128; // support Ascii set

    struct Node
    {
        Node()
        {
            core::zero_this(this);
        }

        void freeChildren(core::MemoryArenaBase* arena) {
            // skip sentinel
            for (int i = 1; i < CHAR_TRIE_SIZE; i++) {
                if (chars[i] != nullptr) {
                    chars[i]->freeChildren(arena);
                    X_DELETE(chars[i], arena);
                }
            }
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
    };

public:
    X_INLINE StringTableUnique(core::MemoryArenaBase* arena);
    X_INLINE ~StringTableUnique();

    X_INLINE IdType addStringUnqiue(const char* pStr);
    X_INLINE IdType addStringUnqiue(const char* pStr, size_t len);

private:
    X_INLINE void AddStringToTrie(const char* pStr, IdType id);
    X_INLINE bool FindString(const char* pStr, size_t len, IdType& id);

private:
    core::MemoryArenaBase* arena_;
    size_t longestStr_;
    Node searchTree_;
};

#include "StringTable.inl"

X_NAMESPACE_END

#endif // X_STRING_TABLE_H_