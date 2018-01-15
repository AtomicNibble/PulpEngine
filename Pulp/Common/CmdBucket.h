#pragma once

#include <Traits\FunctionTraits.h>
#include <Traits\MemberFunctionTraits.h>

#include <Threading\AtomicInt.h>
#include <Threading\ThreadLocalStorage.h>
#include <Threading\JobSystem2.h>

#include <Memory\AllocationPolicies\LinearAllocator.h>

#include <Containers\Array.h>

#include <Math\XViewPort.h>

#include <Util\PointerFlags.h>

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



struct DynamicBufferDesc
{
	static const uint32_t MAGIC = X_TAG('D', 'B', 'U', 'F');

	uint32_t magic;
	uint32_t size;
	const void* pData; // you own this, safe to clear after submitCommandPackets

	X_INLINE VertexBufferHandle asBufferHandle(void) const {
		return reinterpret_cast<VertexBufferHandle>(this);
	}
};


class CmdPacketAllocator 
{
public:
	static const size_t MAX_THREAD_COUNT = core::V2::JobSystem::HW_THREAD_MAX + 1; // job system + main

private:
	typedef core::MemoryArena<
		core::LinearAllocator,
		core::SingleThreadPolicy,

#if X_ENABLE_MEMORY_DEBUG_POLICIES
		core::SimpleBoundsChecking,
		core::NoMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> LinearArena;

	typedef core::FixedArray<uint32_t, MAX_THREAD_COUNT> ThreadIdToIndex;

public:
	CmdPacketAllocator(core::MemoryArenaBase* arena, size_t size);
	~CmdPacketAllocator();

	// creates allocators for calling thread and all jobSys threads.
	void createAllocaotrsForThreads(core::V2::JobSystem& jobSys);

	template <typename CommandT>
	X_INLINE CommandPacket::Packet create(size_t threadIdx, size_t auxMemorySize);

	X_INLINE uint8_t* auxAlloc(size_t size);

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


X_DECLARE_FLAGS8(DepthBindFlag)(
	CLEAR,	// clears on bind.
	WRITE	// can perform depth test and write.
);

typedef Flags8<DepthBindFlag> DepthBindFlags;

X_DECLARE_FLAG_OPERATORS(DepthBindFlags);

class CommandBucketBase
{
	X_NO_COPY(CommandBucketBase);
	X_NO_ASSIGN(CommandBucketBase);

public:
	typedef Commands::CmdBase CmdBase;
	typedef core::Array<CommandPacket::Packet> PacketArr;
	typedef core::Array<uint32_t> SortedIdxArr;
	typedef core::FixedArray<IRenderTarget*, MAX_RENDER_TARGETS> RenderTargetsArr;
	typedef core::PointerFlags<render::IPixelBuffer, 2> PixelBufferWithFlags;

#if X_COMPILER_CLANG == 0
	static_assert(PixelBufferWithFlags::BIT_COUNT >= DepthBindFlag::FLAGS_COUNT, "Not enougth space for flags");
#endif // !X_COMPILE_CLANG

protected:
	CommandBucketBase(core::MemoryArenaBase* arena, size_t size, const XCamera& cam, const XViewPort& viewport);
	~CommandBucketBase() = default;

public:

	// Maybe allowing diffrent index's to be set is better idea.
	// and what ever is not null is set.
	X_INLINE void appendRenderTarget(IRenderTarget* pRTV);
	X_INLINE void setDepthStencil(render::IPixelBuffer* pPB, DepthBindFlags bindFlags);

	X_INLINE const Matrix44f& getViewMatrix(void) const;
	X_INLINE const Matrix44f& getProjMatrix(void) const;
	X_INLINE const XViewPort& getViewport(void) const;
	

	X_INLINE const RenderTargetsArr& getRTVS(void) const;
	X_INLINE render::IPixelBuffer* getDepthStencil(void) const;
	X_INLINE DepthBindFlags getDepthBindFlags(void) const;
	X_INLINE const SortedIdxArr& getSortedIdx(void) const;
	X_INLINE const PacketArr& getPackets(void) const;

protected:
	Matrix44f view_;
	Matrix44f proj_;
	XViewPort viewport_;
	RenderTargetsArr rtvs_;
	PixelBufferWithFlags pDepthStencil_;

	core::AtomicInt current_;
	PacketArr packets_;
	SortedIdxArr sortedIdx_;
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

	template <typename CommandT>
	X_INLINE CommandT* appendCommand(CmdBase* pCommand, size_t auxMemorySize);

	template <typename CommandT>
	X_INLINE std::tuple<CommandT*, char*> addCommandGetAux(Key key, size_t auxMemorySize);

	template <typename CommandT>
	X_INLINE std::tuple<CommandT*, char*> appendCommandGetAux(CmdBase* pCommand, size_t auxMemorySize);

	X_INLINE DynamicBufferDesc* createDynamicBufferDesc(void);

public:
	X_INLINE const KeyArr& getKeys(void);

private:
	// offset and coutns for each thread adding commands
	X_ALIGNED_SYMBOL(AlignedIntArr, 64) threadSlotsInfo_;

	core::Array<Key> keys_;

	CmdPacketAllocator& packetAlloc_;
	core::MemoryArenaBase* arena_;
};

X_NAMESPACE_END

#include "CmdBucket.inl"