#include "stdafx.h"
#include "CommandListManger.h"

#include "PipelineState.h"

X_NAMESPACE_BEGIN(render)

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

    if (!graphicsQueue_.create(pDevice) || !computeQueue_.create(pDevice) || !copyQueue_.create(pDevice)) {
        X_ERROR("Dx12", "Failed to create required que's");
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
    X_ASSERT(type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported")
    (type);

    switch (type) {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            *pAllocatorOut = graphicsQueue_.requestAllocator();
            break;
        case D3D12_COMMAND_LIST_TYPE_BUNDLE:
            break;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            *pAllocatorOut = computeQueue_.requestAllocator();
            break;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            *pAllocatorOut = copyQueue_.requestAllocator();
            break;
    }

    HRESULT hr = pDevice_->CreateCommandList(1, type, *pAllocatorOut, pInitialPso, IID_PPV_ARGS(pListOut));
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to create command list: %" PRIu32, hr);
    }

    D3DDebug::SetDebugObjectName((*pListOut), L"CommandList");
}

X_NAMESPACE_END