#include "stdafx.h"
#include "CommandAllocatorPool.h"

X_NAMESPACE_BEGIN(render)

CommandAllocatorPool::CommandAllocatorPool(core::MemoryArenaBase* arena, D3D12_COMMAND_LIST_TYPE type) :
    pDevice_(nullptr),
    commandListType_(type),
    allocatorPool_(arena),
    readyAllocator_(arena)
{
}

CommandAllocatorPool::~CommandAllocatorPool()
{
    shutdown();
}

void CommandAllocatorPool::create(ID3D12Device* pDevice)
{
    X_ASSERT_NOT_NULL(pDevice);
    pDevice_ = pDevice;

    allocatorPool_.reserve(32);
    readyAllocator_.reserve(32);
}

void CommandAllocatorPool::shutdown(void)
{
    for (size_t i = 0; i < allocatorPool_.size(); ++i) {
        allocatorPool_[i]->Release();
    }

    allocatorPool_.clear();
}

ID3D12CommandAllocator* CommandAllocatorPool::requestAllocator(uint64_t CompletedFenceValue)
{
    X_ASSERT_NOT_NULL(pDevice_); // check init was called.

    core::CriticalSection::ScopedLock lock(cs_);

    ID3D12CommandAllocator* pAllocator = nullptr;

    if (readyAllocator_.isNotEmpty()) {
        const std::pair<uint64_t, ID3D12CommandAllocator*>& allocatorPair = readyAllocator_.peek();

        if (allocatorPair.first <= CompletedFenceValue) // check it's safe to use / gpu done with it.
        {
            pAllocator = allocatorPair.second;
            HRESULT hr = pAllocator->Reset();
            if (FAILED(hr)) {
                X_ERROR("Dx12", "Failed to reset command allocator: %" PRId32, hr);
                pAllocator = nullptr;
            }
            else {
                readyAllocator_.pop();
            }
        }
    }

    // If no allocator's were ready to be reused, create a new one
    if (pAllocator == nullptr) {
        HRESULT hr = pDevice_->CreateCommandAllocator(commandListType_, IID_PPV_ARGS(&pAllocator));
        if (FAILED(hr)) {
            X_ERROR("Dx12", "Failed to create command allocator: %" PRId32, hr);
        }

        core::StackString<32, wchar_t> name;
        name.appendFmt(L"CommandAllocator %" PRIuS, allocatorPool_.size());

        D3DDebug::SetDebugObjectName(pAllocator, name.c_str());
        allocatorPool_.push_back(pAllocator);
    }

    X_ASSERT_NOT_NULL(pAllocator);
    return pAllocator;
}

void CommandAllocatorPool::discardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* pAllocator)
{
    core::CriticalSection::ScopedLock lock(cs_);

    readyAllocator_.push(std::make_pair(FenceValue, pAllocator));
}

X_NAMESPACE_END
