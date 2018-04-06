#include "stdafx.h"
#include "CommandSignature.h"
#include "RootSignature.h"

X_NAMESPACE_BEGIN(render)

CommandSignature::CommandSignature(core::MemoryArenaBase* arena, size_t numParams) :
    params_(arena, numParams)
{
}

CommandSignature::~CommandSignature()
{
    free();
}

void CommandSignature::clear(void)
{
    params_.clear();

    core::SafeReleaseDX(pSignature_);
}

void CommandSignature::free(void)
{
    params_.free();

    core::SafeReleaseDX(pSignature_);
}

void CommandSignature::reset(size_t numParams)
{
    params_.resize(numParams);
}

void CommandSignature::finalize(ID3D12Device* pDevice, const RootSignature* pRootSignature)
{
    uint32_t byteStride = 0;
    bool requiresRootSignature = false;

    if (pSignature_) {
        X_WARNING("Dx12", "command sig already have a valid driver sig");
        return;
    }

    X_ASSERT_NOT_NULL(pDevice);

    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc;
    commandSignatureDesc.ByteStride = byteStride;
    commandSignatureDesc.NumArgumentDescs = safe_static_cast<uint32_t, size_t>(params_.size());
    commandSignatureDesc.pArgumentDescs = reinterpret_cast<const D3D12_INDIRECT_ARGUMENT_DESC*>(params_.data());
    commandSignatureDesc.NodeMask = 1;

    ID3D12RootSignature* pRootSig = nullptr;
    if (requiresRootSignature && pRootSignature) {
        pRootSig = pRootSignature->getSignature();
    }

    HRESULT hr = pDevice->CreateCommandSignature(&commandSignatureDesc, pRootSig, IID_PPV_ARGS(&pSignature_));
    if (FAILED(hr)) {
        X_FATAL("Dx12", "Failed to create command signature: %" PRIu32, hr);
        return;
    }

    D3DDebug::SetDebugObjectName(pSignature_, L"CommandSignature");
}

X_NAMESPACE_END