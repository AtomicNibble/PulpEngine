

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::GrowingStringTable(core::MemoryArenaBase* arena) :
    CurrentBlock_(0),
    currentBlockSpace_(0),
    NumStrings_(0),
    WasteSize_(0),
    buffer_(arena),
    arena_(arena)
{
    X_ASSERT(blockGranularity >= 1, "blockGranularity must be atleast 1")(blockGranularity);
    // 4 is silly but i'll allow it, 8 is the sensible min.
    X_ASSERT(BlockSize >= 4, "Block size must be atleast 4 bytes")(BlockSize);
    // the block size must be atleast the alignment
    X_ASSERT(BlockSize >= Alignment, "Block size must be equal or greater than Alignment")(BlockSize, Alignment);
    // lets also make it a multiple of the alignment.
    X_ASSERT((BlockSize % Alignment) == 0, "Block size must be multiple of Alignment")(BlockSize, Alignment);
    // Not stupid alignment.
    X_ASSERT(Alignment <= 64, "Alignment must be 64 or lower")(Alignment);
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::~GrowingStringTable()
{
    free();
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
void GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::reserve(size_t numBlocks)
{
    if (numBlocks > currentBlockSpace_) {
        ensureFreeBlocks(numblocks - currentBlockSpace_);
    }
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
void GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::free(void)
{
    CurrentBlock_ = 0;
    currentBlockSpace_ = 0;
    // stat reset.
    NumStrings_ = 0;
    WasteSize_ = 0;

    buffer_.free();
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
IdType GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::addString(const char* str)
{
    size_t len = strlen(str);
    return addString(str, len);
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
IdType GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::addString(const char* str, size_t Len)
{
    X_ASSERT(Len < 255, "string is longer than the 255 max")(Len);

    IdType Block = CurrentBlock_;

    // ensure space for null terminator as we return pointers to this memory.
    size_t NumBlocks = (Len + sizeof(Header_t) + BlockSize) / BlockSize;

    // grow if needed. returns false if IdType can't represent.
    if (!requestFreeBlocks(NumBlocks)) {
        X_ERROR("GrowingStringTable", "Reached the limit of id. sizeof(IdType) = ", sizeof(IdType));
        return InvalidId;
    }

    // get header that is aligned after the header.
    Header_t* pHeader = getCurrentAlignedHeader();

    // set magic.
    pHeader->magic = Header_t::HEADER_MAGIC;

    // set length
    pHeader->Length = safe_static_cast<uint8_t, size_t>(Len);

    // copy the string.
    memcpy(pHeader + 1, str, Len + 1);

    // update current block.
    CurrentBlock_ += safe_static_cast<IdType, size_t>(NumBlocks);

    // stats
    NumStrings_++;
    WasteSize_ += (NumBlocks * BlockSize) - (Len + sizeof(Header_t) + 1);

    // return id.
    return Block;
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
const char* GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::getString(IdType ID) const
{
    X_ASSERT(ID < CurrentBlock_, "String out of range")(ID, CurrentBlock_);

    // get alligned buffer.
    const uint8_t* pStart = core::pointerUtil::AlignTop(buffer_.ptr() + sizeof(Header_t),
                                Alignment)
                            - sizeof(Header_t);

    // return string.
    return reinterpret_cast<const char*>(&pStart[(BlockSize * ID) + sizeof(Header_t)]);
}

X_DISABLE_WARNING(4723);

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
bool GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::requestFreeBlocks(size_t numBlocks)
{
    static const size_t MAX_BLOCKS = (std::numeric_limits<IdType>::max() / BLOCK_SIZE);
    static const size_t MAX_BYTES = MAX_BLOCKS * BLOCK_SIZE;

    size_t freeBlocks = currentBlockSpace_ - CurrentBlock_;

    if (freeBlocks > numBlocks) {
        return true;
    }

    size_t potentialBlocks = MAX_BLOCKS - currentBlockSpace_;
    // can we evern represent the requested blocks with this type?
    if (potentialBlocks < numBlocks) {
        return false;
    }

    if ((CurrentBlock_ + numBlocks) > currentBlockSpace_) {
        size_t requiredBytes = buffer_.capacity() + (blockGranularity * BlockSize);

        // if frist alloc make room for alingment
        if (buffer_.capacity() == 0) {
            requiredBytes += Alignment + sizeof(Header_t);
            // grow to exact size first time.
            buffer_.setGranularity(1);
        }

        // add more space.
        buffer_.resize(requiredBytes);

        // any grows after don't include alignment.
        buffer_.setGranularity(blockGranularity * BlockSize);

        // update max blocks.
        currentBlockSpace_ = safe_static_cast<IdType, size_t>(
            core::Min(MAX_BYTES, buffer_.capacity()) / BlockSize);
        return true;
    }
    return true;
}
X_ENABLE_WARNING(4723);

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
typename GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::Header_t*
    GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::getCurrentAlignedHeader(void)
{
    uint8_t* pHeaderBegin = core::pointerUtil::AlignTop(buffer_.ptr() + sizeof(Header_t),
                                Alignment)
                            - sizeof(Header_t);

    X_ASSERT(core::pointerUtil::IsAligned(pHeaderBegin,
                 static_cast<uint32_t>(Alignment), sizeof(Header_t)),
        "Headers is not aligned")
    (pHeaderBegin, Alignment);

    // int blocks
    pHeaderBegin += (CurrentBlock_ * BlockSize);

    // cast and return
    return reinterpret_cast<Header_t*>(pHeaderBegin);
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
size_t GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::numStrings(void) const
{
    return NumStrings_;
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
size_t GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::wastedBytes(void) const
{
    return WasteSize_;
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
size_t GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::allocatedBytes(void) const
{
    return buffer_.capacity();
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
size_t GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::bytesUsed(void) const
{
    return BlockSize * CurrentBlock_;
}

// ISerialize
template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
bool GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::SSave(XFile* pFile) const
{
    X_ASSERT_NOT_NULL(pFile);

    uint32_t typeSize = safe_static_cast<uint32_t, size_t>(sizeof(IdType));
    uint32_t BlockSize32 = safe_static_cast<uint32_t, size_t>(BlockSize);
    uint32_t NumStrings32 = safe_static_cast<uint32_t, size_t>(NumStrings_);
    uint32_t WasteSize32 = safe_static_cast<uint32_t, size_t>(WasteSize_);

    pFile->writeObj(typeSize);
    pFile->writeObj(BlockSize32);

    // these are size of the type.
    pFile->writeObj(CurrentBlock_);
    pFile->writeObj(currentBlockSpace_);

    pFile->writeObj(NumStrings32);
    pFile->writeObj(WasteSize32);

    uint32_t numBytes = BlockSize * CurrentBlock_;
    return pFile->write(buffer_.ptr(), numBytes) == numBytes;
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
bool GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::SLoad(XFile* pFile)
{
    X_ASSERT_NOT_NULL(pFile);
    free();

    IdType CurrentBlock, currentBlockSpace;
    uint32_t TypeSize, blockSizeCheck, NumStrings, WasteSize;
    size_t read = 0;

    read += pFile->readObj(TypeSize);
    read += pFile->readObj(blockSizeCheck);

    if (read != (sizeof(uint32_t) * 2)) {
        X_ERROR("GrowingStringTable", "failed to read info from file");
        return false;
    }

    // make sure it was saved with same type of GST
    if (TypeSize != sizeof(IdType)) {
        X_ERROR("GrowingStringTable", "Tried to read a GrowingStringTable from disk,"
                                      " but the type size is invalid. required: %i provided: %i",
            sizeof(IdType), TypeSize);
        return false;
    }

    if (blockSizeCheck != BlockSize) {
        X_ERROR("GrowingStringTable", "string table on disk has a unmatching BlockSize. "
                                      "required: %i provided: %i",
            BlockSize, blockSizeCheck);
        return false;
    }

    read = pFile->readObj(CurrentBlock);
    read += pFile->readObj(currentBlockSpace);
    read += pFile->readObj(NumStrings);
    read += pFile->readObj(WasteSize); // ok to read into member.

    if (read != ((sizeof(IdType) * 2) + (sizeof(uint32_t) * 2))) {
        X_ERROR("GrowingStringTable", "failed to read info from file");
        return false;
    }

    if (!requestFreeBlocks(CurrentBlock)) {
        X_ERROR("GrowingStringTable", "Failed to acquire required blocks. num: %i", CurrentBlock);
        return false;
    }

    CurrentBlock_ = CurrentBlock;
    NumStrings_ = NumStrings;
    WasteSize_ = WasteSize;

    uint32_t numBytes = BlockSize * CurrentBlock_;
    return pFile->read(buffer_.ptr(), numBytes) == numBytes;
}

// ~ISerialize

// --------------------------------------------------------------------

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
GrowingStringTableUnique<blockGranularity, BlockSize, Alignment, IdType>::
    GrowingStringTableUnique(core::MemoryArenaBase* arena) :
    GrowingStringTable(arena)
{
    LongestStr_ = 0;
    NumNodes_ = 0;
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
GrowingStringTableUnique<blockGranularity, BlockSize, Alignment, IdType>::~GrowingStringTableUnique()
{
    free();
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
void GrowingStringTableUnique<blockGranularity, BlockSize, Alignment, IdType>::free(void)
{
    GrowingStringTable::free();

    searchTree_.free(arena_);
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
IdType GrowingStringTableUnique<blockGranularity, BlockSize, Alignment, IdType>::addStringUnqiue(const char* Str)
{
    size_t len = strlen(Str);
    return addStringUnqiue(Str, len);
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
IdType GrowingStringTableUnique<blockGranularity, BlockSize, Alignment, IdType>::addStringUnqiue(const char* Str, size_t Len)
{
    IdType id;
    if (FindString(Str, Len, id))
        return id;

    // Update longest string
    LongestStr_ = core::Max(LongestStr_, Len);

    id = BaseT::addString(Str, Len);

    AddStringToTrie(Str, id); // add to search Trie

    return id;
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
void GrowingStringTableUnique<blockGranularity, BlockSize, Alignment, IdType>::AddStringToTrie(const char* Str, IdType id)
{
    Node* node = &searchTree_;
    const char* Txt = Str;
    int c;
    while ((c = *Txt++)) {
        if (node->chars[c] == nullptr) {
            node->chars[c] = X_NEW(Node, arena_, "GSTNode");
            NumNodes_++;
        }
        node = node->chars[c];
    }
    node->id = id;
    node->sentinel = (void*)!nullptr;
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
bool GrowingStringTableUnique<blockGranularity, BlockSize, Alignment, IdType>::FindString(const char* Str, size_t Len, IdType& id)
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
