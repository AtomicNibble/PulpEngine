#include "stdafx.h"
#include "CommandList.h"

#include "PipelineState.h"

X_NAMESPACE_BEGIN(render)

CommandQue::CommandQue(core::MemoryArenaBase* arena, D3D12_COMMAND_LIST_TYPE type) :
	pCommandQueue_(nullptr),
	pFence_(nullptr),
	type_(type),
	allocatorPool_(arena, type),
	nextFenceValue_(static_cast<uint64_t>(type) << 56 | 1),
	lastCompletedFenceValue_(static_cast<uint64_t>(type) << 56),
	fenceEventHandle_(INVALID_HANDLE_VALUE)
{

}

CommandQue::~CommandQue()
{

}



bool CommandQue::create(ID3D12Device* pDevice)
{
	X_ASSERT_NOT_NULL(pDevice);

	{
		D3D12_COMMAND_QUEUE_DESC queueDesc;
		core::zero_object(queueDesc);
		queueDesc.Type = type_;
		queueDesc.NodeMask = 1;
		HRESULT hr = pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue_));
		if (FAILED(hr)) {
			X_ERROR("Dx12", "failed to create command que: %i", hr);
			return false;
		}

		D3DDebug::SetDebugObjectName(pCommandQueue_, L"CommandListManager::m_CommandQueue");
	}

	HRESULT hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence_));
	if (FAILED(hr)) {
		X_ERROR("Dx12", "failed to create fence: %i", hr);
		return false;
	}

	D3DDebug::SetDebugObjectName(pFence_, L"CommandListManager::m_pFence");
	pFence_->Signal((uint64_t)type_ << 56);


	// using win32 api directly here is ok, since directx is win32 only.
	fenceEventHandle_ = CreateEventW(nullptr, false, false, nullptr);
	if (fenceEventHandle_ == INVALID_HANDLE_VALUE) {
		core::lastError::Description Dsc;
		X_ERROR("Dx12", "failed to create fence event. Err: %s", core::lastError::ToString(Dsc));
		return false;
	}

	allocatorPool_.create(pDevice);

	// meow
	return true;
}

void CommandQue::shutdown(void)
{
	if (!pCommandQueue_) {
		return;
	}

	allocatorPool_.shutdown();

	if (!CloseHandle(fenceEventHandle_)) {
		core::lastError::Description Dsc;
		X_ERROR("Dx12", "failed to close fence event handle. Err: %s", core::lastError::ToString(Dsc));
	}

	core::SafeReleaseDX(pFence_);
	core::SafeReleaseDX(pCommandQueue_); // sets it to null.
}

uint64_t CommandQue::incrementFence(void)
{
	core::CriticalSection::ScopedLock  lock(fenceCs_);
	pCommandQueue_->Signal(pFence_, nextFenceValue_);

	return nextFenceValue_++;
}

uint64_t CommandQue::getNextFenceValue(void)
{
	return nextFenceValue_;
}

bool CommandQue::isFenceComplete(uint64_t fenceValue)
{
	// Avoid querying the fence value by testing against the last one seen.
	// The max() is to protect against an unlikely race condition that could cause the last
	// completed fence value to regress.
	if (fenceValue > lastCompletedFenceValue_) {
		lastCompletedFenceValue_ = core::Max(lastCompletedFenceValue_, pFence_->GetCompletedValue());
	}

	return fenceValue <= lastCompletedFenceValue_;
}


void CommandQue::stallForProducer(CommandQue& producer)
{
	X_ASSERT(producer.nextFenceValue_ > 0, "Invalid next fence value")(producer.nextFenceValue_);
	pCommandQueue_->Wait(producer.pFence_, producer.nextFenceValue_ - 1);
}

void CommandQue::waitForFence(uint64_t fenceValue)
{
	if (isFenceComplete(fenceValue)) {
		return;
	}


	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
	// wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
	// the fence can only have one event set on completion, then thread B has to wait for 
	// 100 before it knows 99 is ready.  Maybe insert sequential events?
	{
		core::CriticalSection::ScopedLock  lock(eventCs_);

		pFence_->SetEventOnCompletion(fenceValue, fenceEventHandle_);
		WaitForSingleObject(fenceEventHandle_, INFINITE);
		lastCompletedFenceValue_ = fenceValue;
	}
}

void CommandQue::waitForIdle(void)
{
	waitForFence(nextFenceValue_ - 1);
}

uint64_t CommandQue::executeCommandList(ID3D12CommandList* pList)
{
	core::CriticalSection::ScopedLock lock(fenceCs_);
	X_ASSERT_NOT_NULL(pList);

	static_cast<ID3D12GraphicsCommandList*>(pList)->Close();

	// Kickoff the command list
	pCommandQueue_->ExecuteCommandLists(1, &pList);

	// Signal the next fence value (with the GPU)
	pCommandQueue_->Signal(pFence_, nextFenceValue_);

	// And increment the fence value.  
	return nextFenceValue_++;
}

ID3D12CommandAllocator* CommandQue::requestAllocator(void)
{
	X_ASSERT_NOT_NULL(pFence_);

	uint64_t completedFence = pFence_->GetCompletedValue();

	return allocatorPool_.requestAllocator(completedFence);
}

void CommandQue::discardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* pAllocator)
{
	allocatorPool_.discardAllocator(fenceValue, pAllocator);
}

// ------------------------------------------------------------------


CommandListManger::CommandListManger(core::MemoryArenaBase* arena) :
	pDevice_(nullptr),
	graphicsQueue_(arena, D3D12_COMMAND_LIST_TYPE_DIRECT),
	computeQueue_(arena, D3D12_COMMAND_LIST_TYPE_COMPUTE),
	copyQueue_(arena, D3D12_COMMAND_LIST_TYPE_COPY)
{

}

CommandListManger::~CommandListManger()
{
	shutdown();
}

bool CommandListManger::create(ID3D12Device* pDevice)
{
	X_ASSERT_NOT_NULL(pDevice);
	pDevice_ = pDevice;

	if (!graphicsQueue_.create(pDevice) ||
		!computeQueue_.create(pDevice) ||
		!copyQueue_.create(pDevice)) 
	{

		return false;
	}

	return true;
}

void CommandListManger::shutdown(void)
{
	graphicsQueue_.shutdown();
	computeQueue_.shutdown();
	copyQueue_.shutdown();
}

void CommandListManger::createNewCommandList(D3D12_COMMAND_LIST_TYPE type, PSO& initialPso,
	ID3D12GraphicsCommandList** pListOut, ID3D12CommandAllocator** pAllocatorOut)
{
	// you stupid fuck, pso needs to be a finalized().
	X_ASSERT_NOT_NULL(initialPso.getPipelineStateObject());

	createNewCommandList(type, initialPso.getPipelineStateObject(), pListOut, pAllocatorOut);
}

void CommandListManger::createNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12PipelineState* pInitialPso,
	ID3D12GraphicsCommandList** pListOut, ID3D12CommandAllocator** pAllocatorOut)
{
	X_ASSERT(type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported")(type);

	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT: *pAllocatorOut = graphicsQueue_.requestAllocator(); break;
	case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE: *pAllocatorOut = computeQueue_.requestAllocator(); break;
	case D3D12_COMMAND_LIST_TYPE_COPY: *pAllocatorOut = copyQueue_.requestAllocator(); break;
	}

	HRESULT hr = pDevice_->CreateCommandList(1, type, *pAllocatorOut, pInitialPso, IID_PPV_ARGS(pListOut));
	if (FAILED(hr)) {
		X_FATAL("Dx12", "Failed to create command list: %" PRIu32, hr);
	}

	D3DDebug::SetDebugObjectName((*pListOut), L"CommandList");
}


X_NAMESPACE_END