#include "stdafx.h"
#include "PipelineState.h"
#include "RootSignature.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(render)

namespace
{
    inline void hashShader(core::Hash::xxHash64& hasher, D3D12_SHADER_BYTECODE& s)
    {
#if X_ENABLE_RENDER_SHADER_RELOAD

        if (s.pShaderBytecode) {
            hasher.updateBytes(s.pShaderBytecode, s.BytecodeLength);
        }

#else
        // if we don't reload shaders hashing the pointer to byte code is enougth.
        X_UNUSED(hasher, s);
#endif // !X_ENABLE_RENDER_SHADER_RELOAD
    };

} // namespace

PSODeviceCache::PSODeviceCache(core::MemoryArenaBase* arena, ID3D12Device* pDevice) :
    pDevice_(pDevice),
    cache_(arena, 32),
    helpWithWorkonPSOStall_(0)
{
}

PSODeviceCache::~PSODeviceCache()
{
    destoryAll();

    if (gEnv && gEnv->pConsole) {
        gEnv->pConsole->UnregisterVariable("r_pso_stall_help_with_work");
    }
}

void PSODeviceCache::registerVars(void)
{
    ADD_CVAR_REF("r_pso_stall_help_with_work", helpWithWorkonPSOStall_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "When waiting for a PSO to finish compiling on another thread, help process worker jobs");
}

void PSODeviceCache::destoryAll(void)
{
    core::CriticalSection::ScopedLock lock(cacheLock_);

    for (auto& pso : cache_) {
        ID3D12PipelineState* pPSO = pso.second;
        if (reinterpret_cast<uintptr_t>(pPSO) != INVALID_PSO) {
            pPSO->Release();
        }
    }

    cache_.clear();
}

bool PSODeviceCache::compile(D3D12_GRAPHICS_PIPELINE_STATE_DESC& gpsoDesc, ID3D12PipelineState** pPSO)
{
    HashVal hash = getHash(gpsoDesc);

    ID3D12PipelineState** pPSORef = nullptr;
    bool notCompiled = false;
    {
        core::CriticalSection::ScopedLock lock(cacheLock_);

        auto it = cache_.find(hash);
        if (it != cache_.end()) {
            // get ref of hash location.
            // which might have a value of null.
            pPSORef = &it->second;
        }
        else {
            notCompiled = true;
            // insert the entry for this hash, before we start compile.
            // that way we don't have to lock for whole compile time.
            auto insertIt = cache_.insert(std::make_pair(hash, nullptr));

            pPSORef = &insertIt.first->second;
        }
    }

    if (notCompiled) {
        HRESULT hr = pDevice_->CreateGraphicsPipelineState(&gpsoDesc, IID_PPV_ARGS(pPSO));
        if (FAILED(hr)) {
            Error::Description Dsc;
            X_ERROR("Dx12", "Failed to create graphics PSO: %s", Error::ToString(hr, Dsc));

            // so other threads waiting can find out that this failed.
            *pPSORef = reinterpret_cast<ID3D12PipelineState*>(INVALID_PSO);
            return false;
        }

        D3DDebug::SetDebugObjectName(*pPSO, "PSO");

        *pPSORef = *pPSO;
    }
    else // if(*pPSORef == nullptr)
    {
        // wait for it to finish compiling on another thread.
        int32_t backOff = 0;

        while (*pPSORef == nullptr) {
            core::Thread::backOff(backOff++);
        }
    }

    // did it fail to compile?
    if (reinterpret_cast<uintptr_t>(*pPSORef) == INVALID_PSO) {
        X_ERROR("Dx12", "Failed to create graphics PSO(cache)");
        return false;
    }

    *pPSO = *pPSORef;
    return true;
}

bool PSODeviceCache::compile(D3D12_COMPUTE_PIPELINE_STATE_DESC& cpsoDesc, ID3D12PipelineState** pPSO)
{
    HashVal hash = getHash(cpsoDesc);

    ID3D12PipelineState** pPSORef = nullptr;
    bool notCompiled = false;
    {
        core::CriticalSection::ScopedLock lock(cacheLock_);

        auto it = cache_.find(hash);
        if (it != cache_.end()) {
            pPSORef = &it->second;
        }
        else {
            notCompiled = true;
            auto insertIt = cache_.insert(std::make_pair(hash, nullptr));
            pPSORef = &insertIt.first->second;
        }
    }

    if (notCompiled) {
        HRESULT hr = pDevice_->CreateComputePipelineState(&cpsoDesc, IID_PPV_ARGS(pPSO));
        if (FAILED(hr)) {
            Error::Description Dsc;
            X_FATAL("Dx12", "Failed to create compute PSO: %s", Error::ToString(hr, Dsc));
            *pPSORef = reinterpret_cast<ID3D12PipelineState*>(-1);
            return false;
        }

        *pPSORef = *pPSO;
    }
    else {
        if (*pPSORef == nullptr) {

            int32_t backOff = 0;

            while (*pPSORef == nullptr) {
                core::Thread::backOff(backOff++);
            }

            // did the compile that we waited for fail?
            if (*pPSORef == reinterpret_cast<ID3D12PipelineState*>(-1)) {
                return false;
            }
        }

        *pPSO = *pPSORef;
    }

    return true;
}

PSODeviceCache::HashVal PSODeviceCache::getHash(D3D12_GRAPHICS_PIPELINE_STATE_DESC& gpsoDesc)
{
    core::Hash::xxHash64 hasher;

    // we don't want to include the pointer in the hash.
    // we have root sig pointer but that's ok.
    const auto ILCopy = gpsoDesc.InputLayout;
    gpsoDesc.InputLayout.pInputElementDescs = nullptr;

    hasher.reset(0x52652);
    hasher.update(gpsoDesc);
    hasher.update(core::make_span(ILCopy.pInputElementDescs, ILCopy.NumElements));

    hashShader(hasher, gpsoDesc.VS);
    hashShader(hasher, gpsoDesc.PS);
    hashShader(hasher, gpsoDesc.DS);
    hashShader(hasher, gpsoDesc.HS);
    hashShader(hasher, gpsoDesc.GS);

    gpsoDesc.InputLayout = ILCopy;
    return hasher.finalize();
}

PSODeviceCache::HashVal PSODeviceCache::getHash(D3D12_COMPUTE_PIPELINE_STATE_DESC& cpsoDesc)
{
    core::Hash::xxHash64 hasher;

    hasher.reset(0x79371); // diffrent seed to graphics desc.
    hasher.update(cpsoDesc);

    hashShader(hasher, cpsoDesc.CS);

    return hasher.finalize();
}

// --------------------------------------------------

void PSO::setRootSignature(const RootSignature& BindMappings)
{
    pRootSignature_ = &BindMappings;
}

// --------------------------------------------------

bool GraphicsPSO::finalize(PSODeviceCache& cache)
{
    // Make sure the root signature is finalized first
    PSODesc_.pRootSignature = pRootSignature_->getSignature();

    X_ASSERT(PSODesc_.pRootSignature != nullptr, "root signature must be finalized before finalize PSO")();

    return cache.compile(PSODesc_, &pPSO_);
}

void GraphicsPSO::setRenderTargetFormats(uint32_t numRTVs, const DXGI_FORMAT* pRTVFormats,
    DXGI_FORMAT DSVFormat, uint32_t MsaaCount, uint32_t MsaaQuality)
{
    X_ASSERT_NOT_NULL(pRTVFormats);

    for (uint32_t i = 0; i < numRTVs; ++i) {
        PSODesc_.RTVFormats[i] = pRTVFormats[i];
    }
    for (uint32_t i = numRTVs; i < PSODesc_.NumRenderTargets; ++i) {
        PSODesc_.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    }

    PSODesc_.NumRenderTargets = numRTVs;
    PSODesc_.DSVFormat = DSVFormat;
    PSODesc_.SampleDesc.Count = MsaaCount;
    PSODesc_.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPSO::setInputLayout(size_t numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs)
{
    PSODesc_.InputLayout.NumElements = safe_static_cast<uint32_t, size_t>(numElements);
    PSODesc_.InputLayout.pInputElementDescs = pInputElementDescs;
}

// ----------------------------------------

bool ComputePSO::finalize(PSODeviceCache& cache)
{
    // Make sure the root signature is finalized first
    PSODesc_.pRootSignature = pRootSignature_->getSignature();

    X_ASSERT(PSODesc_.pRootSignature != nullptr, "root signature must be finalized before finalize PSO")();

    return cache.compile(PSODesc_, &pPSO_);
}

void ComputePSO::setComputeShader(const void* pBinary, size_t size)
{
    PSODesc_.CS = CD3D12_SHADER_BYTECODE(pBinary, size);
}

X_NAMESPACE_END