#pragma once

#include <IShader.h>


X_NAMESPACE_DECLARE(render,
	namespace Commands
	{
		struct ResourceStateBase;
	} // namespace
);

X_NAMESPACE_BEGIN(engine)

class VariableStateManager
{
	typedef core::MemoryArena<
		core::PoolAllocator,
		core::MultiThreadPolicy<core::Spinlock>, // allow multi thread creation.
		core::SimpleBoundsChecking,
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging> PoolArena;

public:
	static const uint32_t MAX_CONST_BUFFERS = render::MAX_CONST_BUFFERS_BOUND;
	static const uint32_t MAX_TEX_STATES = render::MAX_TEXTURES_BOUND;

	static const uint32_t MAX_STATES = 1024;

public:
	VariableStateManager();
	~VariableStateManager();


	render::Commands::ResourceStateBase* createVariableState(int8_t numTexStates, int8_t numCBs);

private:
	static constexpr size_t allocSize(int8_t numTexStates, int8_t numCBs);

private:
	// do i want linera allocator which i think would work well or a pool.
	// down side of lineraalocator is i can't free them so for now we pool.
	// could create multiple pools to help reduce wastage.
	core::HeapArea      statePoolHeap_;
	core::PoolAllocator statePoolAllocator_;
	PoolArena			statePool_;
};



X_NAMESPACE_END