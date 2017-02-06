#include "EngineCommon.h"
#include "CmdBucket.h"

#include <Sorting\RadixSort.h>

X_NAMESPACE_BEGIN(render)



// -------------------------------------------------------
CmdPacketAllocator::CmdPacketAllocator(core::MemoryArenaBase* arena, size_t threadAllocatorSize) :
	arena_(arena),
	threadAllocatorSize_(threadAllocatorSize)
{
	core::zero_object(allocators_);
}

CmdPacketAllocator::~CmdPacketAllocator()
{
	// free all the allocators
	for (size_t i = 0; i < MAX_THREAD_COUNT; i++) {
		if (allocators_[i]) {
			X_DELETE(allocators_[i], arena_);
		}
	}

	if (pBuf_) {
		X_DELETE_ARRAY(pBuf_, arena_);
	}
}

void CmdPacketAllocator::createAllocaotrsForThreads(core::V2::JobSystem& jobSys)
{
	core::V2::JobSystem::ThreadIdArray threadIds = jobSys.getThreadIds();

	for (auto threadId : threadIds) {
		threadIdToIndex_.push_back(threadId);
	}

	// calling thread
	threadIdToIndex_.push_back(core::Thread::GetCurrentID());

	// create the allocators.
	const size_t numAllocators = threadIdToIndex_.size();
	const size_t totalBufSize = numAllocators * threadAllocatorSize_;

	pBuf_ = X_NEW_ARRAY_ALIGNED(uint8_t, totalBufSize, arena_, "CmdPcketAllocatorBuf", 16);

	for (size_t i = 0; i < numAllocators; i++)
	{
		uint8_t* pAllocatorBuf = pBuf_ + (threadAllocatorSize_ * i);
		allocators_[i] = X_NEW(ThreadAllocator, arena_, "CmdPacketThreadAllocator")(pAllocatorBuf, pAllocatorBuf + threadAllocatorSize_);
	}
}

CmdPacketAllocator::ThreadAllocator::ThreadAllocator(void* pStart, void* pEnd) :
	allocator_(pStart, pEnd),
	arena_(&allocator_, "CmdBucketArena")
{

}


// -------------------------------------------------------

CommandBucketBase::CommandBucketBase(core::MemoryArenaBase* arena, size_t size, const XCamera& cam, const XViewPort& viewport) :
	current_(0),
	packets_(arena, size),
	sortedIdx_(arena, size),
	viewport_(viewport)
{
	X_UNUSED(cam);
}


template <typename KeyT>
CommandBucket<KeyT>::CommandBucket(core::MemoryArenaBase* arena, CmdPacketAllocator& packetAlloc, 
	size_t size, const XCamera& cam, const XViewPort& viewport) :
	CommandBucketBase(arena, size, cam, viewport),
	packetAlloc_(packetAlloc),
	arena_(arena),
	keys_(arena, size)
{

	X_ASSERT_ALIGNMENT(&threadSlotsInfo_, 64, 0);

	// null all the packet pointers.
	// needed since a thread may take some slots but never fill them.
	// so we can't rly on all packets below current_ to be valid

	for (auto& p : packets_) {
		p = nullptr;
	}
}

template <typename KeyT>
CommandBucket<KeyT>::~CommandBucket()
{

}

template <typename KeyT>
void CommandBucket<KeyT>::sort(void)
{
	// lets trim the bucket to it's actual size?
	// first we need to find the slot with lowest offset
	AlignedIntArr sortedSlots(threadSlotsInfo_);

	std::sort(sortedSlots.begin(), sortedSlots.end(), [](const ThreadSlotInfo& lhs, const ThreadSlotInfo& rhs) {
		return lhs.offset < rhs.offset;
	});

	int32_t total = current_;

	static_assert(core::compileTime::IsPOD<KeyT>::Value, "This logic requires pod keys");
	static_assert(core::compileTime::IsPOD<PacketArr::Type>::Value, "This logic requires pod packet values");

	for (size_t i=0; i<sortedSlots.size(); i++)
	{
		const auto& slot = sortedSlots[i];
		if (slot.remaining)
		{
			total -= slot.remaining;

			// we need to move the next slots data down into this offset.
			int32_t curEnd = slot.offset + (FETCH_SIZE - slot.remaining);

			// so we search util we find a block with some items?
			for (i++; i<sortedSlots.size(); i++)
			{
				const auto& nextSlot = sortedSlots[i];

				if (nextSlot.offset <= slot.offset) {
					continue;
				}

				total -= nextSlot.remaining;

				// we have some to shit.
				// note buffers may overlap.
				const int32_t numToShift = (FETCH_SIZE - nextSlot.remaining);
				std::memmove(&keys_[curEnd], &keys_[nextSlot.offset], numToShift * sizeof(KeyT));
				std::memmove(&packets_[curEnd], &packets_[nextSlot.offset], numToShift * sizeof(PacketArr::Type));

#if X_DEBUG
				std::memset(&keys_[nextSlot.offset], 0xDB, numToShift * sizeof(KeyT));
				std::memset(&packets_[nextSlot.offset], 0xDB, numToShift * sizeof(PacketArr::Type));
#endif // X_DEBUG

				curEnd += numToShift;
			}
		}
	}

	keys_.resize(total);

	core::Sorting::radix_sort_buf<uint32_t>(keys_.begin(), keys_.end(), sortedIdx_, arena_);
}

template <typename KeyT>
void CommandBucket<KeyT>::clear(void)
{
	X_ASSERT_NOT_IMPLEMENTED();
}


template class CommandBucket<uint8_t>;
template class CommandBucket<uint16_t>;
template class CommandBucket<uint32_t>;
template class CommandBucket<uint64_t>;

X_NAMESPACE_END