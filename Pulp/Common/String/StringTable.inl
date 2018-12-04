

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
StringTable<NumBlocks, BlockSize, Alignment, IdType>::StringTable() :
    maxBlocks_(NumBlocks),
    currentBlock_(0),
    numStrings_(0),
    wasteSize_(0),
    cursor_(buffer_, NumBlocks * BlockSize)
{
    cursor_.seekBytes(4);
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTable<NumBlocks, BlockSize, Alignment, IdType>::addString(const char* pStr)
{
    return addString(pStr, strUtil::strlen(pStr));
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTable<NumBlocks, BlockSize, Alignment, IdType>::addString(const char* pStr, size_t len)
{
    X_ASSERT(len < 255, "string is longer than the 255 max")(len);
    X_ASSERT(currentBlock_ < maxBlocks_, "string table is full")(currentBlock_, maxBlocks_);

    IdType block = currentBlock_;

    // ensure space for null terminator as we return pointers to this memory.
    size_t NumBlocks = (len + sizeof(Header_t) + BlockSize) / BlockSize;

    // set the header
    Header_t* head = cursor_.getSeekPtr<Header_t>();
    head->Length = safe_static_cast<uint32_t, size_t>(len);

    // copy the string.
    memcpy(cursor_.getPtr<void>(), pStr, len);

    // ensusre null
    memset(cursor_.getPtr<BYTE>() + len, '\0', 1);

    cursor_.seekBytes(safe_static_cast<uint32_t, size_t>((NumBlocks * BlockSize) - sizeof(Header_t)));
    currentBlock_ += safe_static_cast<uint32_t, size_t>(NumBlocks);

    numStrings_++;
    wasteSize_ += (NumBlocks * BlockSize) - (len + sizeof(Header_t) + 1);
    return block;
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
const char* StringTable<NumBlocks, BlockSize, Alignment, IdType>::getString(IdType ID) const
{
    X_ASSERT(ID < maxBlocks_, "String out of range")(ID);
    return reinterpret_cast<const char*>(&buffer_[4 + (BlockSize * ID) + sizeof(Header_t)]);
}

// ---------------------------------

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::StringTableUnique(core::MemoryArenaBase* arena) :
    StringTable(),
    arena_(arena),
    longestStr_(0)
{
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::~StringTableUnique()
{
    searchTree_.freeChildren(arena_);
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::addStringUnqiue(const char* pStr)
{
    return addStringUnqiue(pStr, strUtil::strlen(pStr));
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::addStringUnqiue(const char* pStr, size_t len)
{
    IdType id = 0;
    if (MyType::FindString(pStr, len, id)) {
        return id;
    }

    // Update longest string
    longestStr_ = core::Max(longestStr_, len);

    id = addString(pStr, len);

    AddStringToTrie(pStr, id); // add to search Trie

    return id;
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
void StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::AddStringToTrie(const char* pStr, IdType id)
{
    Node* pNode = &searchTree_;
    const char* pTxt = pStr;

    int c;
    while ((c = *pTxt++)) {
        if (pNode->chars[c] == nullptr) {
            pNode->chars[c] = X_NEW(Node, arena_, "TireNode");
        }
        pNode = pNode->chars[c];
    }

    pNode->id = id;
    pNode->sentinel = (void*)!nullptr;
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
bool StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::FindString(const char* pStr, size_t len, IdType& id)
{
    if (len > longestStr_) { // we can skip checking for strings longer then any in the table.
        return false;
    }

    Node* pNode = &searchTree_;
    const char* pTxt = pStr;

    int  c;
    while ((c = *pTxt++)) {
        if (pNode->chars[c] == nullptr) {
            return false;
        }
        pNode = pNode->chars[c];
    }

    if (pNode->sentinel != nullptr) {
        id = pNode->id;
        return true;
    }

    return false;
}