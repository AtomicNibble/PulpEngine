#pragma once

#include <Traits\FunctionTraits.h>
#include <Traits\MemberFunctionTraits.h>

#include <Threading\AtomicInt.h>
#include <Threading\ThreadLocalStorage.h>
#include <Threading\JobSystem2.h>

#include <Memory\AllocationPolicies\LinearAllocator.h>

#include <IRender.h>

X_NAMESPACE_BEGIN(engine)

namespace CommandPacket
{
	typedef void* Packet;
	typedef render::Commands::Command Command;
	const size_t OFFSET_NEXT_COMMAND_PACKET = 0u;
	const size_t OFFSET_COMMAND_TYPE = OFFSET_NEXT_COMMAND_PACKET + sizeof(Packet);
	const size_t OFFSET_COMMAND = OFFSET_COMMAND_TYPE + sizeof(Command::Enum);

	template <typename CommandT>
	X_INLINE size_t getPacketSize(size_t auxMemorySize);

	Packet* getNextCommandPacket(Packet pPacket);

	template <typename CommandT>
	X_INLINE Packet* getNextCommandPacket(CommandT* command);

	Command::Enum* getCommandType(Packet pPacket);

	template <typename CommandT>
	X_INLINE CommandT* getCommand(Packet packet);

	template <typename CommandT>
	X_INLINE char* getAuxiliaryMemory(CommandT* command);
	void storeNextCommandPacket(Packet pPacket, Packet nextPacket);

	template <typename CommandT>
	X_INLINE void storeNextCommandPacket(CommandT* command, Packet nextPacket);

	void storeCommandType(Packet pPacket, Command::Enum dispatchFunction);
	const Packet loadNextCommandPacket(const Packet pPacket);
	const Command::Enum loadCommandType(const Packet pPacket);
	const void* loadCommand(const Packet pPacket);

} // namespace CommandPacket



class CmdPacketAllocator 
{
public:
	static const size_t MAX_THREAD_COUNT = core::V2::JobSystem::HW_THREAD_MAX + 1; // job system + main

private:
	typedef core::MemoryArena<
		core::LinearAllocator,
		core::SingleThreadPolicy,
		core::SimpleBoundsChecking,
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging
	> LinearArena;

	typedef core::FixedArray<uint32_t, MAX_THREAD_COUNT> ThreadIdToIndex;

public:
	CmdPacketAllocator(core::MemoryArenaBase* arena, size_t size);
	~CmdPacketAllocator();

	// creates allocators for calling thread and all jobSys threads.
	void createAllocaotrsForThreads(core::V2::JobSystem& jobSys);

	template <typename CommandT>
	X_INLINE CommandPacket::Packet create(size_t threadIdx, size_t auxMemorySize);

	X_INLINE size_t getThreadIdx(void);

private:
	// shove each arena in it's own cache lane
	X_DISABLE_WARNING(4324)
	struct X_ALIGNED_SYMBOL(ThreadAllocator, 64)
	{
		ThreadAllocator(void* pStart, void* pEnd);

	public:
		core::LinearAllocator allocator_;
		LinearArena arena_;
	};
	X_ENABLE_WARNING(4324)

	core::MemoryArenaBase* arena_;
	uint8_t* pBuf_;
	size_t threadAllocatorSize_;
	ThreadIdToIndex threadIdToIndex_;
	ThreadAllocator* allocators_[MAX_THREAD_COUNT];
};

template <typename KeyT>
class CommandBucket
{
	// number of slots to fetch for each thread, should be atleast 32 to prevent fales sharing.
	// could make this number based on the size of KeyT.
	static const size_t FETCH_SIZE = 64 / sizeof(KeyT); 

private:
	X_DISABLE_WARNING(4324)
	struct X_ALIGNED_SYMBOL(ThreadSlotInfo, 64)
	{
		X_INLINE ThreadSlotInfo() : offset(0), remaining(0) {};

		uint32_t offset;
		uint32_t remaining;
	};
	X_ENABLE_WARNING(4324)

	typedef std::array<ThreadSlotInfo, CmdPacketAllocator::MAX_THREAD_COUNT> AlignedIntArr;

public:
	typedef KeyT Key;

public:
	CommandBucket(core::MemoryArenaBase* arena, // used to allocate key and packet arrays
		CmdPacketAllocator& packetAlloc,		// used to allocate the packets, needs to be thread safe
		size_t size,							// number of keys / packets.
		const XCamera& cam);
	~CommandBucket();

	void sort(void);
	void submit(void);

	template <typename CommandT>
	X_INLINE CommandT* addCommand(Key key, size_t auxMemorySize);

	template <typename CommandT, typename ParentCmdT>
	X_INLINE CommandT* appendCommand(ParentCmdT* pCommand, size_t auxMemorySize);

private:
	void setRenderTargets(void);
	void setMatrices(void);
	void submitPacket(const CommandPacket::Packet packet);


private:
	Matrix44f view_;
	Matrix44f proj_;

	core::AtomicInt current_;	

	// offset and coutns for each thread adding commands
	X_ALIGNED_SYMBOL(AlignedIntArr, 64) threadSlotsInfo_;

	core::Array<Key> keys_;
	core::Array<CommandPacket::Packet> packets_;
	core::Array<uint32_t> sortedIdx_;

	CmdPacketAllocator& packetAlloc_;
	core::MemoryArenaBase* arena_;
};

X_NAMESPACE_END

#include "CmdBucket.inl"