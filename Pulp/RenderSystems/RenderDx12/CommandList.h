#pragma once


#ifndef X_COMMAND_QUE_H_
#define X_COMMAND_QUE_H_

#include "CommandAllocatorPool.h"
#include <Threading\CriticalSection.h>

X_NAMESPACE_BEGIN(render)

class CommandQue
{
public:
	CommandQue(core::MemoryArenaBase* arena, D3D12_COMMAND_LIST_TYPE type);
	~CommandQue();

	bool create(ID3D12Device* pDevice);
	void shutdown(void);

	X_INLINE bool IsReady(void) const;
	uint64_t incrementFence(void);
	uint64_t getNextFenceValue(void);
	bool isFenceComplete(uint64_t fenceValue);
	void stallForProducer(CommandQue& producer);
	void waitForFence(uint64_t fenceValue);
	void waitForIdle(void);

	uint64_t executeCommandList(ID3D12CommandList* pList);

	X_INLINE ID3D12CommandQueue* getCommandQueue(void);
	ID3D12CommandAllocator* requestAllocator(void);
	void discardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

private:
	ID3D12CommandQueue* pCommandQueue_;
	ID3D12Fence* pFence_;
	const D3D12_COMMAND_LIST_TYPE type_;
	CommandAllocatorPool allocatorPool_;

	core::CriticalSection eventCs_;
	core::CriticalSection fenceCs_;

	uint64_t nextFenceValue_;
	uint64_t lastCompletedFenceValue_;
	HANDLE fenceEventHandle_;
};


class CommandListManger
{
public:
	CommandListManger(core::MemoryArenaBase* arena);
	~CommandListManger();

	bool create(ID3D12Device* pDevice);
	void shutdown(void);

	X_INLINE CommandQue& getGraphicsQueue(void);
	X_INLINE CommandQue& getComputeQueue(void);
	X_INLINE CommandQue& getCopyQueue(void);
	X_INLINE CommandQue& getQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT);

	X_INLINE ID3D12CommandQueue* getCommandQueue(void);


	bool isFenceComplete(uint64_t fenceValue);
	void waitForFence(uint64_t fenceValue);
	void idleGPU(void);

private:

	ID3D12Device* pDevice_;

	CommandQue graphicsQueue_;
	CommandQue computeQueue_;
	CommandQue copyQueue_;

};



X_NAMESPACE_END

#include "CommandList.inl"

#endif // !X_COMMAND_QUE_H_