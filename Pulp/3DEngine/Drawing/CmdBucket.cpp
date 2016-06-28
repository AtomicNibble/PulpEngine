#include "stdafx.h"
#include "CmdBucket.h"

X_NAMESPACE_BEGIN(engine)


namespace backendDispatch
{
	void Draw(const void* pData)
	{
		const Commands::Draw* pCmdData = union_cast<const Commands::Draw*>(pData);
	
		X_UNUSED(pCmdData);
	}

	void DrawIndexed(const void* pData)
	{
		const Commands::DrawIndexed* pCmdData = union_cast<const Commands::DrawIndexed*>(pData);
	
		X_UNUSED(pCmdData);
	}

	void CopyConstantBufferData(const void* pData)
	{
		const Commands::CopyConstantBufferData* pCmdData = union_cast<const Commands::CopyConstantBufferData*>(pData);

		X_UNUSED(pCmdData);
	}

} // namespace backendDispatch


namespace Commands
{
	const BackendDispatchFunction::Pointer Draw::DISPATCH_FUNCTION = backendDispatch::Draw;
	const BackendDispatchFunction::Pointer DrawIndexed::DISPATCH_FUNCTION = backendDispatch::DrawIndexed;
	const BackendDispatchFunction::Pointer CopyConstantBufferData::DISPATCH_FUNCTION = backendDispatch::CopyConstantBufferData;


} // namespace Commands



CommandPacket::Packet* CommandPacket::getNextCommandPacket(Packet pPacket)
{
	return union_cast<Packet*>(reinterpret_cast<char*>(pPacket) + OFFSET_NEXT_COMMAND_PACKET);
}

CommandPacket::BackendDispatchFunction::Pointer* CommandPacket::getBackendDispatchFunction(Packet pPacket)
{
	return union_cast<BackendDispatchFunction::Pointer*>(reinterpret_cast<char*>(pPacket) + OFFSET_BACKEND_DISPATCH_FUNCTION);
}

void CommandPacket::storeNextCommandPacket(Packet pPacket, Packet nextPacket)
{
	*getNextCommandPacket(pPacket) = nextPacket;
}

void CommandPacket::storeBackendDispatchFunction(Packet pPacket, BackendDispatchFunction::Pointer dispatchFunction)
{
	*getBackendDispatchFunction(pPacket) = dispatchFunction;
}

const CommandPacket::Packet CommandPacket::loadNextCommandPacket(const Packet pPacket)
{
	return *getNextCommandPacket(pPacket);
}

const CommandPacket::BackendDispatchFunction::Pointer CommandPacket::loadBackendDispatchFunction(const Packet pPacket)
{
	return *getBackendDispatchFunction(pPacket);
}

const void* CommandPacket::loadCommand(const Packet pPacket)
{
	return reinterpret_cast<char*>(pPacket) + OFFSET_COMMAND;
}


// -------------------------------------------------------
CmdPacketAllocator::CmdPacketAllocator(core::MemoryArenaBase* arena, size_t threadAllocatorSize) :
	arena_(arena),
	threadAllocatorSize_(threadAllocatorSize)
{
	core::zero_object(allocators_);
}

CmdPacketAllocator::~CmdPacketAllocator()
{

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

	pBuf_ = X_NEW_ARRAY(uint8_t, totalBufSize, arena_, "");

	for (size_t i = 0; i < numAllocators; i++)
	{
		uint8_t* pAllocatorBuf = pBuf_ + (threadAllocatorSize_ * i);
		allocators_[i] = X_NEW(ThreadAllocator, arena_, "")(pAllocatorBuf, pAllocatorBuf + threadAllocatorSize_);
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
	packets_(arena, size)
{
	X_UNUSED(cam);

	X_ASSERT_ALIGNMENT(&offsets_, 64, 0);
	X_ASSERT_ALIGNMENT(&slotsLeft_, 64, 0);

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

}

template <typename KeyT>
void CommandBucket<KeyT>::submit(void)
{
	setRenderTargets();
	setMatrices();

	const int32_t current = current_;

	for (int32_t i = 0; i < current_; ++i)
	{
		Packet pPacket = packets_[i];
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
void CommandBucket<KeyT>::submitPacket(const Packet pPacket)
{
	const BackendDispatchFunction::Pointer pFunc = CommandPacket::loadBackendDispatchFunction(pPacket);
	const void* pCmd = CommandPacket::loadCommand(pPacket);
	pFunc(pCmd);
}


template class CommandBucket<uint8_t>;
template class CommandBucket<uint16_t>;
template class CommandBucket<uint32_t>;
template class CommandBucket<uint64_t>;

X_NAMESPACE_END