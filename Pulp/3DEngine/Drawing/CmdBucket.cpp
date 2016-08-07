#include "stdafx.h"
#include "CmdBucket.h"

#include <Sorting\RadixSort.h>

X_NAMESPACE_BEGIN(engine)



namespace CommandPacket
{

	Packet* CommandPacket::getNextCommandPacket(Packet pPacket)
	{
		return union_cast<Packet*>(reinterpret_cast<char*>(pPacket) + OFFSET_NEXT_COMMAND_PACKET);
	}

	Command::Enum* CommandPacket::getCommandType(Packet pPacket)
	{
		return union_cast<Command::Enum*>(reinterpret_cast<char*>(pPacket) + OFFSET_COMMAND_TYPE);
	}

	void storeNextCommandPacket(Packet pPacket, Packet nextPacket)
	{
		*getNextCommandPacket(pPacket) = nextPacket;
	}

	void storeCommandType(CommandPacket::Packet pPacket, Command::Enum type)
	{
		*getCommandType(pPacket) = type;
	}

	const Packet loadNextCommandPacket(const Packet pPacket)
	{
		return *getNextCommandPacket(pPacket);
	}

	const Command::Enum loadBackendDispatchFunction(const Packet pPacket)
	{
		return *getCommandType(pPacket);
	}

	const void* loadCommand(const Packet pPacket)
	{
		return reinterpret_cast<char*>(pPacket) + OFFSET_COMMAND;
	}


} // namespace CommandPacket


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

	pBuf_ = X_NEW_ARRAY(uint8_t, totalBufSize, arena_, "CmdPcketAllocatorBuf");

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

template <typename KeyT>
CommandBucket<KeyT>::CommandBucket(core::MemoryArenaBase* arena, CmdPacketAllocator& packetAlloc, 
	size_t size, const XCamera& cam) :
	current_(0),
	packetAlloc_(packetAlloc),
	arena_(arena),
	keys_(arena, size),
	packets_(arena, size),
	sortedIdx_(arena, size)
{
	X_UNUSED(cam);

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
	const int32_t current = current_;

	core::Sorting::radix_sort_buf<uint32_t>(keys_.begin(), keys_.begin() + current, sortedIdx_, arena_);
}

template <typename KeyT>
void CommandBucket<KeyT>::submit(void)
{
	setRenderTargets();
	setMatrices();

	const int32_t current = current_;

	for (int32_t i = 0; i < current_; ++i)
	{
		CommandPacket::Packet pPacket = packets_[sortedIdx_[i]];
		while (pPacket != nullptr)
		{
			submitPacket(pPacket);
			pPacket = CommandPacket::loadNextCommandPacket(pPacket);
		}
	}
}

template <typename KeyT>
void CommandBucket<KeyT>::setRenderTargets(void)
{

}

template <typename KeyT>
void CommandBucket<KeyT>::setMatrices(void)
{

}

template <typename KeyT>
void CommandBucket<KeyT>::submitPacket(const CommandPacket::Packet pPacket)
{
	X_UNUSED(pPacket);
//	const CommandPacket::BackendDispatchFunction::Pointer pFunc = CommandPacket::loadBackendDispatchFunction(pPacket);
//	const void* pCmd = CommandPacket::loadCommand(pPacket);
//	pFunc(pCmd);
}


template class CommandBucket<uint8_t>;
template class CommandBucket<uint16_t>;
template class CommandBucket<uint32_t>;
template class CommandBucket<uint64_t>;

X_NAMESPACE_END