#include <EngineCommon.h>
#include "GrowingStringTable.h"


#include <Util\PointerUtil.h>

X_NAMESPACE_BEGIN(core)

template<typename IdType>
GrowingStringTable<IdType>::GrowingStringTable(core::MemoryArenaBase* arena) :
CurrentBlock_(0),
currentBlockSpace_(0),
blockGranularity(0),
BlockSize_(0),
Alignment_(0),
NumStrings_(0),
WasteSize_(0),
Buffer_(nullptr),
buffer_(arena),
arena_(arena)
{

}

template<typename IdType>
GrowingStringTable<IdType>::~GrowingStringTable() 
{
	free();
}


template<typename IdType>
void GrowingStringTable<IdType>::setSize(size_t blockGranularity,
	size_t BlockSize, size_t Alignment)
{
	X_ASSERT(blockGranularity >= 1, "blockGranularity must be atleast 1")(blockGranularity);
	// 4 is silly but i'll allow it, 8 is the sensible min.
	X_ASSERT(blockSize >= 4, "Block size must be atleast 4 bytes")(blockSize);
	// the block size must be atleast the alignment
	X_ASSERT(blockSize >= Alignment, "Block size must be equal or greater than Alignment")(blockSize, Alignment);
	// lets also make it a multiple of the alignment.
	X_ASSERT((blockSize % Alignment) == 0, "Block size must be multiple of Alignment")(blockSize, Alignment);

	blockGranularity_ = blockGranularity;
	BlockSize_ = BlockSize;
	Alignment_ = Alignment;
}

template<typename IdType>
void GrowingStringTable<IdType>::free(void)
{
	CurrentBlock_ = 0;
	currentBlockSpace_ = 0;
	// stat reset.
	NumStrings_ = 0;
	WasteSize_ = 0;

	buffer_.free();
}

template<typename IdType>
IdType GrowingStringTable<IdType>::addString(const char* str, size_t Len)
{
	X_ASSERT(Len < 255, "string is longer than the 255 max")(Len);

	IdType Block = CurrentBlock_;

	// ensure space for null terminator as we return pointers to this memory.
	size_t NumBlocks = (Len + sizeof(Header_t)+BlockSize) / BlockSize;

	// grow if needed.
	ensureFreeBlocks(NumBlocks);

	// get header that is aligned after the header.
	Header_t* pHeader = getCurrentAlignedHeader();

	// set length
	pHeader->Length = safe_static_cast<uint8_t,size_t>(Len);

	// copy the string.
	memcpy(pHeader + 1, Str, Len + 1);

	// update current block.
	CurrentBlock_ += NumBlocks;

	// stats
	NumStrings_++;
	WasteSize_ += (NumBlocks * BlockSize_) - (Len + sizeof(Header_t)+1);

	// return id.
	return Block;
}

template<typename IdType>
const char* GrowingStringTable<IdType>::getString(IdType ID) const
{
	X_ASSERT(ID < CurrentBlock_, "String out of range")(ID, CurrentBlock_);

	// get alligned buffer.
	uint8_t* pStart = core::pointerUtil::AlignTop(buffer_.ptr(), Alignment_);

	// return string.
	return reinterpret_cast<const char*>(&pStart[(BlockSize * ID) + sizeof(Header_t)]);
}

template<typename IdType>
void GrowingStringTable<IdType>::ensureFreeBlocks(size_t numBlocks)
{
	if ((CurrentBlock_ + numBlocks) > currentBlockSpace_)
	{
		size_t requiredBytes = buffer_.capacity() + (blockGranularity_ * BlockSize_);

		// if frist alloc make room for alingment
		if (buffer_.capacity() == 0) {
			requiredBytes += Alignment_;
		}

		// only grow this much.
		buffer_.setGranularity(blockGranularity_ * BlockSize_);

		// add more space.
		buffer_.ensureSize(requiredBytes);

		// update max blocks.
		currentBlockSpace_ = (buffer_.capacity() / BlockSize_);
	}
}


X_NAMESPACE_END