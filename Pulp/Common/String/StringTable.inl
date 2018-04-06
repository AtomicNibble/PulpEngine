

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
StringTable<NumBlocks, BlockSize, Alignment, IdType>::StringTable() :
    MaxBlocks_(NumBlocks),
    CurrentBlock_(0),
    NumStrings_(0),
    WasteSize_(0),
    Cursor_(Buffer_, NumBlocks * BlockSize)
{
    Cursor_.seekBytes(4);
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTable<NumBlocks, BlockSize, Alignment, IdType>::addString(const char* Str)
{
    return addString(Str, strUtil::strlen(Str));
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
IdType StringTable<NumBlocks, BlockSize, Alignment, IdType>::addString(const char* Str, size_t Len)
{
    X_ASSERT(Len < 255, "string is longer than the 255 max")
    (Len);
    X_ASSERT(CurrentBlock_ < MaxBlocks_, "string table is full")
    (CurrentBlock_, MaxBlocks_);

    IdType Block = CurrentBlock_;

    // ensure space for null terminator as we return pointers to this memory.
    size_t NumBlocks = (Len + sizeof(Header_t) + BlockSize) / BlockSize;

    // set the header
    Header_t* head = Cursor_.getSeekPtr<Header_t>();
    head->Length = safe_static_cast<uint32_t, size_t>(Len);

    // copy the string.
    memcpy(Cursor_.getPtr<void>(), Str, Len);

    // ensusre null
    memset(Cursor_.getPtr<BYTE>() + Len, '\0', 1);

    Cursor_.seekBytes(safe_static_cast<uint32_t, size_t>((NumBlocks * BlockSize) - sizeof(Header_t)));
    CurrentBlock_ += safe_static_cast<uint32_t, size_t>(NumBlocks);

    NumStrings_++;
    WasteSize_ += (NumBlocks * BlockSize) - (Len + sizeof(Header_t) + 1);
    return Block;
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
const char* StringTable<NumBlocks, BlockSize, Alignment, IdType>::getString(IdType ID) const
{
    X_ASSERT(ID < MaxBlocks_, "String out of range")
    (ID);
    return reinterpret_cast<const char*>(&Buffer_[4 + (BlockSize * ID) + sizeof(Header_t)]);
}

// ---------------------------------

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
IdType StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::addStringUnqiue(const char* Str, size_t Len)
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
            node->chars[c] = new Node; // TODO: use a arena!!
            NumNodes_++;
        }
        node = node->chars[c];
    }
    node->id = id;
    node->sentinel = (void*)!nullptr;
}

template<int NumBlocks, int BlockSize, int Alignment, typename IdType>
bool StringTableUnique<NumBlocks, BlockSize, Alignment, IdType>::FindString(const char* Str, size_t Len, IdType& id)
{
    if (Len > LongestStr_) // we can skip checking for strings longer then any in the table.
        return false;

    Node* node = &searchTree_;
    const char* Txt = Str;
    size_t c;
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