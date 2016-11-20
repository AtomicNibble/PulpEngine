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
		void setSizes(int8_t numTex, int8_t numCBs) {
			numTextStates = numTex;
			numCbs = numCBs;
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
	statePool_(&statePoolAllocator_, "StatePool")
{


}

VariableStateManager::~VariableStateManager()
{

}

render::Commands::ResourceStateBase* VariableStateManager::createVariableState(int8_t numTexStates, int8_t numCBs)
{
	static_assert(core::compileTime::IsPOD<render::Commands::ResourceStateBase>::Value, "ResourceStateBase must be pod");

	const size_t requiredBytes = allocSize(numTexStates, numCBs);
	void* pData = statePool_.allocate(requiredBytes, MAX_ALIGN, 0, "State", "ResourceStateBase", X_SOURCE_INFO);

	auto* pState = reinterpret_cast<ResourceStateInit*>(pData);

	pState->setSizes(numTexStates, numCBs);

	return pState;
}

X_INLINE constexpr size_t VariableStateManager::allocSize(int8_t numTexStates, int8_t numCBs)
{
	return sizeof(render::Commands::ResourceStateBase) +
		(sizeof(render::TextureState) * numTexStates) +
		(sizeof(render::ConstantBufferHandle) * numCBs);
}

X_NAMESPACE_END