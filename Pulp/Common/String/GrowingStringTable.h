#pragma once

#ifndef X_GROWING_STRING_TABLE_H_
#define X_GROWING_STRING_TABLE_H_

#include "Containers\Array.h"

#include <ISerialize.h>

X_NAMESPACE_BEGIN(core)

struct XFile;

// valid IdTypes: uint16_t, uint32_t

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
class GrowingStringTable : public ISerialize
{
    X_PRAGMA(pack(push, 4))
    struct Header_t
    {
        static const uint8_t HEADER_MAGIC = 0xAF;

        uint8_t magic;
        uint8_t pad[2]; // i use these later
        uint8_t Length; // 255 max
    };
    X_PRAGMA(pack(pop))

public:
    typedef IdType IdType;
    static const IdType InvalidId = IdType(-1);
    // copy's of the tmeplate values.
    static const size_t ALIGNMENT = Alignment;
    static const size_t BLOCK_SIZE = BlockSize;
    static const size_t BLOCK_GRANULARITY = blockGranularity;
    //	static const size_t MAX_BLOCKS = (std::numeric_limits<IdType>::max() / BLOCK_SIZE);
    //	static const size_t MAX_BYTES = MAX_BLOCKS * BLOCK_SIZE;

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
    X_INLINE size_t bytesUsed(void) const;

    // ISerialize
    virtual bool SSave(XFile* pFile) const X_FINAL;
    virtual bool SLoad(XFile* pFile) X_FINAL;
    // ~ISerialize

private:
    // request X number of free blocks.
    bool requestFreeBlocks(size_t numBlocks);
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

protected:
    core::MemoryArenaBase* arena_;
};

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
class GrowingStringTableUnique : public GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>
{
    typedef GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType> BaseT;

public:
    GrowingStringTableUnique(core::MemoryArenaBase* arena);
    ~GrowingStringTableUnique();

    void free(void);

    X_INLINE IdType addStringUnqiue(const char* Str);
    X_INLINE IdType addStringUnqiue(const char* Str, size_t Len);

private:
    static const size_t CHAR_TRIE_SIZE = 128; // support Ascii set

    typedef struct Node
    {
        Node()
        {
            core::zero_this(this);
        }
        ~Node()
        {
        }

        void free(core::MemoryArenaBase* arena)
        {
            for (size_t i = /*skip sentinel*/ 1; i < CHAR_TRIE_SIZE; i++) {
                if (chars[i] != nullptr) {
                    chars[i]->free(arena);
                    X_DELETE_AND_NULL(chars[i], arena);
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
    } Node;

    X_INLINE void AddStringToTrie(const char* Str, IdType id);
    X_INLINE bool FindString(const char* Str, size_t Len, IdType& id);

private:
    size_t LongestStr_;
    size_t NumNodes_;
    Node searchTree_;
};

#include "GrowingStringTable.inl"

X_NAMESPACE_END

#endif // !X_GROWING_STRING_TABLE_H_