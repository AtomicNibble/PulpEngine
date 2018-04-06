#include "stdafx.h"
#include "CommandQue.h"

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

        D3DDebug::SetDebugObjectName(pCommandQueue_, L"CommandListManager::pCommandQueue_");
    }

    HRESULT hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence_));
    if (FAILED(hr)) {
        X_ERROR("Dx12", "failed to create fence: %i", hr);
        return false;
    }

    D3DDebug::SetDebugObjectName(pFence_, L"CommandListManager::pFence_");

    // we store the queue type in the top 8 msb's of the fence value.
    // that way we can get the queue type from fence value.
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
    core::CriticalSection::ScopedLock lock(fenceCs_);
    pCommandQueue_->Signal(pFence_, nextFenceValue_);

    return nextFenceValue_++;
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
        core::CriticalSection::ScopedLock lock(eventCs_);

        pFence_->SetEventOnCompletion(fenceValue, fenceEventHandle_);
        WaitForSingleObject(fenceEventHandle_, 0xFFFFFFFF /* INFINITE */);
        lastCompletedFenceValue_ = fenceValue;
    }
}

uint64_t CommandQue::executeCommandList(ID3D12CommandList* pList)
{
    core::CriticalSection::ScopedLock lock(fenceCs_);
    X_ASSERT_NOT_NULL(pList);

    HRESULT hr = static_cast<ID3D12GraphicsCommandList*>(pList)->Close();
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Command list failed to close!");
        return 0;
    }

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

X_NAMESPACE_END