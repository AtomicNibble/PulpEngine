#pragma once

#include <Traits\FunctionTraits.h>
#include <Traits\MemberFunctionTraits.h>

#include <Threading\AtomicInt.h>
#include <Threading\ThreadLocalStorage.h>
#include <Threading\JobSystem2.h>

#include <Memory\AllocationPolicies\LinearAllocator.h>

#include <Containers\Array.h>

#include <Math\XViewPort.h>

#include <IRender.h>
#include <IRenderCommands.h>


// A single CommandBucket can and should be populated using multiple jobs.
// as it is designed to scale well.
// sorting is performed in a single thread, but performs caching so will early out if list is same as last time.
// sorting of multiple CommandBucket's at the same time is allowed.
// ideally a hierarchy or jobs should be created so that important CommandBucket's get populated and sorted first.
// but other CommandBucket get populated while sorting first list.


// I might make the command buckets take a RadixSort object, that way the buckets can be thrown instead of reset.
// making it more easy for the command buckets to use frame temp memory and change size each frame easy.


X_NAMESPACE_BEGIN(render)





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


class CommandBucketBase
{
	X_NO_COPY(CommandBucketBase);
	X_NO_ASSIGN(CommandBucketBase);

	typedef core::Array<CommandPacket::Packet> PacketArr;
	typedef core::Array<uint32_t> SortedIdxArr;
	typedef core::FixedArray<IRenderTarget*, MAX_RENDER_TARGETS> RenderTargetsArr;

protected:
	CommandBucketBase(core::MemoryArenaBase* arena, size_t size, const XCamera& cam, const XViewPort& viewport);
	~CommandBucketBase() = default;

public:

	X_INLINE void appendRenderTarget(IRenderTarget* pRTV);

	X_INLINE const Matrix44f& getViewMatrix(void);
	X_INLINE const Matrix44f& getProjMatrix(void);
	X_INLINE const XViewPort& getViewport(void);
	
	
	X_INLINE const RenderTargetsArr& getRTVS(void);
	X_INLINE const SortedIdxArr& getSortedIdx(void);
	X_INLINE const PacketArr& getPackets(void);


protected:
	Matrix44f view_;
	Matrix44f proj_;
	XViewPort viewport_;
	RenderTargetsArr rtvs_;

	core::AtomicInt current_;
	core::Array<CommandPacket::Packet> packets_;
	core::Array<uint32_t> sortedIdx_;
};

template <typename KeyT>
class CommandBucket : public CommandBucketBase
{
	// number of slots to fetch for each thread, should be atleast 32 to prevent fales sharing.
	// could make this number based on the size of KeyT.
	static const size_t FETCH_SIZE = 64 / sizeof(KeyT); 

private:
	X_DISABLE_WARNING(4324)
	struct X_ALIGNED_SYMBOL(ThreadSlotInfo, 64)
	{
		X_INLINE ThreadSlotInfo();

		uint32_t offset;
		uint32_t remaining;
	};
	X_ENABLE_WARNING(4324)

	typedef std::array<ThreadSlotInfo, CmdPacketAllocator::MAX_THREAD_COUNT> AlignedIntArr;

public:
	typedef KeyT Key;
	typedef core::Array<Key> KeyArr;

public:
	CommandBucket(core::MemoryArenaBase* arena, // used to allocate key and packet arrays
		CmdPacketAllocator& packetAlloc,		// used to allocate the packets, needs to be thread safe
		size_t size,							// number of keys / packets.
		const XCamera& cam, const XViewPort& viewport);
	~CommandBucket();

	void sort(void);
	void clear(void); // reset's with same size, keeping sorting info.

	template <typename CommandT>
	X_INLINE CommandT* addCommand(Key key, size_t auxMemorySize);

	template <typename CommandT, typename ParentCmdT>
	X_INLINE CommandT* appendCommand(ParentCmdT* pCommand, size_t auxMemorySize);

public:
	X_INLINE const KeyArr& getKeys(void);

private:
	core::AtomicInt current_;	

	// offset and coutns for each thread adding commands
	X_ALIGNED_SYMBOL(AlignedIntArr, 64) threadSlotsInfo_;

	core::Array<Key> keys_;

	CmdPacketAllocator& packetAlloc_;
	core::MemoryArenaBase* arena_;
};

X_NAMESPACE_END

#include "CmdBucket.inl"