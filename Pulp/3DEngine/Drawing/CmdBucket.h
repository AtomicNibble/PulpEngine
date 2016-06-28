#pragma once

#include <Traits\FunctionTraits.h>
#include <Traits\MemberFunctionTraits.h>

#include <threading\AtomicInt.h>
#include <threading\ThreadLocalStorage.h>
#include <Threading\JobSystem2.h>

#include <Memory\AllocationPolicies\LinearAllocator.h>


X_NAMESPACE_BEGIN(engine)


namespace Commands
{
	typedef core::traits::Function<void(const void*)> BackendDispatchFunction;
	typedef void* VertexLayoutHandle;
	typedef void* VertexBufferHandle;
	typedef void* IndexBufferHandle;
	typedef void* ConstantBufferHandle;


	struct Draw
	{
		static const BackendDispatchFunction::Pointer DISPATCH_FUNCTION;

		uint32_t vertexCount;
		uint32_t startVertex;

		VertexLayoutHandle vertexLayoutHandle;
		VertexBufferHandle vertexBuffer;
		IndexBufferHandle indexBuffer;
	};

	struct DrawIndexed
	{
		static const BackendDispatchFunction::Pointer DISPATCH_FUNCTION;

		uint32_t indexCount;
		uint32_t startIndex;
		uint32_t baseVertex;

		VertexLayoutHandle vertexLayoutHandle;
		VertexBufferHandle vertexBuffer;
		IndexBufferHandle indexBuffer;
	};

	struct CopyConstantBufferData
	{
		static const BackendDispatchFunction::Pointer DISPATCH_FUNCTION;

		ConstantBufferHandle constantBuffer;
		void* data;
		uint32_t size;
	};

	static_assert(core::compileTime::IsPOD<Draw>::Value, "Draw command must be POD");
	static_assert(core::compileTime::IsPOD<DrawIndexed>::Value, "DrawIndexed command must be POD");
	static_assert(core::compileTime::IsPOD<CopyConstantBufferData>::Value, "CopyConstantBufferData command must be POD");

} // namespace Commands



class CommandPacket
{
public:
	typedef void* Packet;

private:
	typedef Commands::BackendDispatchFunction BackendDispatchFunction;
	static const size_t OFFSET_NEXT_COMMAND_PACKET = 0u;
	static const size_t OFFSET_BACKEND_DISPATCH_FUNCTION = OFFSET_NEXT_COMMAND_PACKET + sizeof(Packet);
	static const size_t OFFSET_COMMAND = OFFSET_BACKEND_DISPATCH_FUNCTION + sizeof(BackendDispatchFunction::Pointer);

protected:
	template <typename CommandT>
	static X_INLINE size_t getPacketSize(size_t auxMemorySize);

	static Packet* getNextCommandPacket(Packet pPacket);

	template <typename CommandT>
	static X_INLINE Packet* getNextCommandPacket(CommandT* command);

	static BackendDispatchFunction::Pointer* getBackendDispatchFunction(Packet pPacket);

	template <typename CommandT>
	static X_INLINE CommandT* getCommand(Packet packet);

	template <typename CommandT>
	static X_INLINE char* getAuxiliaryMemory(CommandT* command);
	static void storeNextCommandPacket(Packet pPacket, Packet nextPacket);

	template <typename CommandT>
	static X_INLINE void storeNextCommandPacket(CommandT* command, Packet nextPacket);

	static void storeBackendDispatchFunction(Packet pPacket, BackendDispatchFunction::Pointer dispatchFunction);
	static const Packet loadNextCommandPacket(const Packet pPacket);
	static const BackendDispatchFunction::Pointer loadBackendDispatchFunction(const Packet pPacket);
	static const void* loadCommand(const Packet pPacket);
};



class CmdPacketAllocator : private CommandPacket
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
class CommandBucket : private CommandPacket
{
	// number of slots to fetch for each thread, should be atleast 32 to prevent fales sharing.
	// could make this number based on the size of KeyT.
	static const size_t FETCH_SIZE = 32; 

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

	CmdPacketAllocator& packetAlloc_;
	core::MemoryArenaBase* arena_;
};

X_NAMESPACE_END

#include "CmdBucket.inl"