


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
	if(numBlocks > currentBlockSpace_)
	{
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
	size_t NumBlocks = (Len + sizeof(Header_t)+BlockSize) / BlockSize;

	// grow if needed. returns false if IdType can't represent.
	if(!requestFreeBlocks(NumBlocks)) {
		X_ERROR("GrowingStringTable", "Reached the limit of id. sizeof(IdType) = ", sizeof(IdType));
		return InvalidId;
	}

	// get header that is aligned after the header.
	Header_t* pHeader = getCurrentAlignedHeader();

	// set length
	pHeader->Length = safe_static_cast<uint8_t, size_t>(Len);

	// copy the string.
	memcpy(pHeader + 1, str, Len + 1);

	// update current block.
	CurrentBlock_ += safe_static_cast<IdType, size_t>(NumBlocks);

	// stats
	NumStrings_++;
	WasteSize_ += (NumBlocks * BlockSize) - (Len + sizeof(Header_t)+1);

	// return id.
	return Block;
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
const char* GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::getString(IdType ID) const
{
	X_ASSERT(ID < CurrentBlock_, "String out of range")(ID, CurrentBlock_);

	// get alligned buffer.
	const uint8_t* pStart = core::pointerUtil::AlignTop(buffer_.ptr() + sizeof(Header_t),
		Alignment) - sizeof(Header_t);

	// return string.
	return reinterpret_cast<const char*>(&pStart[(BlockSize * ID) + sizeof(Header_t)]);
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
bool GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::requestFreeBlocks(size_t numBlocks)
{
	static const size_t MAX_BLOCKS = (std::numeric_limits<IdType>::max() / BLOCK_SIZE);
	static const size_t MAX_BYTES = MAX_BLOCKS * BLOCK_SIZE;

	size_t freeBlocks = currentBlockSpace_ - CurrentBlock_;

	if (freeBlocks > numBlocks)
	{
		return true;
	}

	size_t potentialBlocks = MAX_BLOCKS - currentBlockSpace_;
	// can we evern represent the requested blocks with this type?
	if(potentialBlocks < numBlocks)
	{
		return false;
	}

	if ((CurrentBlock_ + numBlocks) > currentBlockSpace_)
	{
		size_t requiredBytes = buffer_.capacity() + (blockGranularity * BlockSize);

		// if frist alloc make room for alingment
		if (buffer_.capacity() == 0)
		{
			requiredBytes += Alignment + sizeof(Header_t);
			// grow to exact size first time.
			buffer_.setGranularity(1);
		}

		// add more space.
		buffer_.reserve(requiredBytes);

		// any grows after don't include alignment.
		buffer_.setGranularity(blockGranularity * BlockSize);

		// update max blocks.
		currentBlockSpace_ = safe_static_cast<IdType, size_t>(
			core::Min(MAX_BYTES,buffer_.capacity()) / BlockSize
		);
		return true;
	}

	return true;
}

template<size_t blockGranularity, size_t BlockSize, size_t Alignment, typename IdType>
typename GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::Header_t*
GrowingStringTable<blockGranularity, BlockSize, Alignment, IdType>::getCurrentAlignedHeader(void)
{

	uint8_t* pHeaderBegin = core::pointerUtil::AlignTop(buffer_.ptr() + sizeof(Header_t),
		Alignment) - sizeof(Header_t);

	X_ASSERT(core::pointerUtil::IsAligned(pHeaderBegin,
		static_cast<uint32_t>(Alignment), sizeof(Header_t)), "Headers is not aligned")(pHeaderBegin, Alignment);

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
