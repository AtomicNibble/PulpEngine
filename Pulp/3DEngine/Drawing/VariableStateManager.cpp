#include "stdafx.h"
#include "VariableStateManager.h"

#include <IRenderCommands.h>

X_NAMESPACE_BEGIN(engine)

namespace
{
	// didnt make these static members as Vs won't show me the size when i hover over :(

	const uint32_t MAX_ALIGN = X_ALIGN_OF(render::Commands::ResourceStateBase);
	const uint32_t MAX_ALOC_SIZE = render::Commands::ResourceStateBase::getMaxStateSize();

	// used to access the protected members.
	// thought id try somthing diffrent than making the manager a friend :|
	struct ResourceStateInit : public render::Commands::ResourceStateBase
	{
		static const size_t lastMemberEnd = offsetof(ResourceStateBase, _pad) + sizeof(ResourceStateBase::_pad);
		// make sure that the compiler did not add in padding after last member.
		static_assert(sizeof(ResourceStateBase) == lastMemberEnd, "Compiler added paddin at end");

		void setSizes(int8_t numTex, int8_t numSamp, int8_t numCBs) {
			numTextStates = numTex;
			numSamplers = numSamp;
			numCbs = numCBs;
			_pad = 0;
		}
	};

} // namespace

VariableStateManager::VariableStateManager() :
	statePoolHeap_(
		core::bitUtil::RoundUpToMultiple<size_t>(
			PoolArena::getMemoryRequirement(MAX_ALOC_SIZE) * MAX_STATES,
			core::VirtualMem::GetPageSize()
		)
	),
	statePoolAllocator_(statePoolHeap_.start(), statePoolHeap_.end(),
		PoolArena::getMemoryRequirement(MAX_ALOC_SIZE),
		PoolArena::getMemoryAlignmentRequirement(MAX_ALIGN),
		PoolArena::getMemoryOffsetRequirement()
	),
	statePool_(&statePoolAllocator_, "StatePool"),
	pEmtpyState_(nullptr)
{

	pEmtpyState_ = createVariableState_Interal(0,0,0);

}

VariableStateManager::~VariableStateManager()
{

}

render::Commands::ResourceStateBase* VariableStateManager::createVariableState(size_t numTexStates, size_t numSamp, size_t numCBs)
{
	static_assert(core::compileTime::IsPOD<render::Commands::ResourceStateBase>::Value, "ResourceStateBase must be pod");

	// return the same state for empty ones.
	// reduces allocations.
	// and increases chance of cache hit for empty ones as same memory.
	if (numTexStates == 0 && numSamp == 0 && numCBs == 0) {
		return pEmtpyState_;
	}
	
	return createVariableState_Interal(safe_static_cast<int8_t>(numTexStates), safe_static_cast<int8_t>(numSamp), safe_static_cast<int8_t>(numCBs));
}

render::Commands::ResourceStateBase* VariableStateManager::createVariableState_Interal(int8_t numTexStates, int8_t numSamp, int8_t numCBs)
{
	static_assert(core::compileTime::IsPOD<render::Commands::ResourceStateBase>::Value, "ResourceStateBase must be pod");

	const size_t requiredBytes = allocSize(numTexStates, numSamp, numCBs);
	void* pData = statePool_.allocate(requiredBytes, MAX_ALIGN, 0, "State", "ResourceStateBase", X_SOURCE_INFO);

	auto* pState = reinterpret_cast<ResourceStateInit*>(pData);

	pState->setSizes(numTexStates, numSamp, numCBs);

	return pState;
}

X_INLINE constexpr size_t VariableStateManager::allocSize(int8_t numTexStates, int8_t numSamp, int8_t numCBs)
{
	return sizeof(render::Commands::ResourceStateBase) +
		(sizeof(render::TextureState) * numTexStates) +
		(sizeof(render::SamplerState) * numSamp) +
		(sizeof(render::ConstantBufferHandle) * numCBs);
}

X_NAMESPACE_END