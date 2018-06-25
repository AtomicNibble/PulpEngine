#include "stdafx.h"
#include "XRender.h"

#include "Texture\TextureVars.h"
#include "Texture\TextureManager.h"
#include "Texture\Texture.h"
#include "Texture\TextureUtil.h"

#include "Allocators\LinearAllocator.h"
#include "Buffers\BufferManager.h"
#include "Buffers\DepthBuffer.h"
#include "CommandContex.h"
#include "PipelineState.h"

#include "Util\StateHelpers.h"

#include "CmdBucket.h"

#include <IConsole.h>
#include <String\HumanSize.h>

X_NAMESPACE_BEGIN(render)

// ---------------------------------------------------------------------

XRender::XRender(core::MemoryArenaBase* arena) :
    arena_(arena),
    pDevice_(nullptr),
    pAdapter_(nullptr),
    pSwapChain_(nullptr),
    pShaderMan_(nullptr),
    pTextureMan_(nullptr),
    pContextMan_(nullptr),
    cmdListManager_(arena),
    pBuffMan_(nullptr),
    adapterIdx_(-1),
    adapters_(arena),
    pDescriptorAllocator_(nullptr),
    pDescriptorAllocatorPool_(nullptr),
    pRootSigCache_(nullptr),
    pPSOCache_(nullptr),
    //	presentRS_(arena),
    currentBufferIdx_(0),

    statePoolHeap_(
        core::bitUtil::RoundUpToMultiple<size_t>(
            StatePoolArena::getMemoryRequirement(MAX_STATE_ALOC_SIZE) * MAX_STATES,
            core::VirtualMem::GetPageSize())),
    statePoolAllocator_(statePoolHeap_.start(), statePoolHeap_.end(),
        StatePoolArena::getMemoryRequirement(MAX_STATE_ALOC_SIZE),
        StatePoolArena::getMemoryAlignmentRequirement(MAX_STATE_ALOC_ALIGN),
        StatePoolArena::getMemoryOffsetRequirement()),
    statePool_(&statePoolAllocator_, "StatePool")

#if PSO_HOT_RELOAD
    ,
    deviceStates_(arena)
#endif // !PSO_HOT_RELOAD
{
    core::zero_object(pDisplayPlanes_);

    X_ASSERT(arena_->isThreadSafe(), "Arena must be thread safe")();

    pTexVars_ = X_NEW(texture::TextureVars, arena_, "TextVars");
    pShaderMan_ = X_NEW(shader::XShaderManager, arena_, "ShaderMan")(arena_);

    adapters_.setGranularity(2);
}

XRender::~XRender()
{
    if (pTexVars_) {
        X_DELETE(pTexVars_, arena_);
    }
    if (pTextureMan_) {
        X_DELETE(pTextureMan_, arena_);
    }
    if (pShaderMan_) {
        X_DELETE(pShaderMan_, arena_);
    }
}

void XRender::registerVars(void)
{
    X_ASSERT_NOT_NULL(pTexVars_);
    X_ASSERT_NOT_NULL(pShaderMan_);

    vars_.registerVars();
    vars_.setNativeRes(currentNativeRes_);
    vars_.setRes(displayRes_);

    pTexVars_->registerVars();
    pShaderMan_->registerVars();
}

void XRender::registerCmds(void)
{
    ADD_COMMAND_MEMBER("r_listAdapters", this, XRender, &XRender::Cmd_ListAdapters,
        core::VarFlag::SYSTEM, "List the gpu adapters");

    ADD_COMMAND_MEMBER("r_list_device_features", this, XRender, &XRender::Cmd_ListDeviceFeatures,
        core::VarFlag::SYSTEM, "List the gpu devices features");

    if (pTextureMan_) {
        pTextureMan_->registerCmds();
    }

    if (pShaderMan_) {
        pShaderMan_->registerCmds();
    }
}

bool XRender::init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height, texture::Texturefmt::Enum depthFmt, bool reverseZ)
{
    X_ASSERT(vars_.varsRegisterd(), "Vars must be init before calling XRender::Init()")(vars_.varsRegisterd());
    X_PROFILE_NO_HISTORY_BEGIN("RenderInit", core::profiler::SubSys::RENDER);

    if (hWnd == static_cast<HWND>(0)) {
        X_ERROR("Dx12", "target window is not valid");
        return false;
    }

    currentNativeRes_ = Vec2<uint32_t>(width, height);
    displayRes_ = Vec2<uint32_t>(width, height);

    HRESULT hr;

    Error::Description Dsc;

#if X_DEBUG && 1
    // force enable debug layer.
#else
    if (vars_.enableDebugLayer())
#endif // |_DEBUG
    {
        X_LOG0("Dx12", "Enabling debug layer");

        Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
        if (FAILED(hr)) {
            X_ERROR("Dx12", "Failed to CreateDevice: %s", Error::ToString(hr, Dsc));
            return false;
        }

        debugInterface->EnableDebugLayer();
    }

    // Obtain the DXGI factory
    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));

    if (FAILED(hr)) {
        X_ERROR("Dx12", "Failed to create DXGI Factory %s", Error::ToString(hr, Dsc));
        return false;
    }

    // Create the D3D graphics device
    {
        X_PROFILE_NO_HISTORY_BEGIN("CreateDevice", core::profiler::SubSys::RENDER);

        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        featureLvl_ = D3D_FEATURE_LEVEL_11_0;

        for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(Idx, &adapter); ++Idx) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            Adapter& a = adapters_.AddOne();
            a.deviceName.set(desc.Description);
            a.dedicatedSystemMemory = desc.DedicatedSystemMemory;
            a.dedicatedvideoMemory = desc.DedicatedVideoMemory;
            a.sharedSystemMemory = desc.SharedSystemMemory;
            a.software = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        }

        // sort them?
#if 0
		std::sort(adapters_.begin(), adapters_.end(), [](const Adapter& lhs, const Adapter& rhs) {
			return lhs.dedicatedvideoMemory > rhs.dedicatedvideoMemory;
		});
#endif

        for (size_t i = 0; i < adapters_.size(); i++) {
            Adapter& a = adapters_[i];
            if (a.software) {
                continue;
            }

            if (DXGI_ERROR_NOT_FOUND == dxgiFactory->EnumAdapters1(static_cast<uint32_t>(i), &adapter)) {
                X_FATAL("Dx12", "Adapater lookup failure");
                continue;
            }

            hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice_));
            if (FAILED(hr)) {
                X_WARNING("Dx12", "Failed to create device for adpater: \"%s\" res: %s", a.deviceName.c_str(), Error::ToString(hr, Dsc));
                continue;
            }

            adapterIdx_ = safe_static_cast<int32_t>(i);

            X_LOG0("Dx12", "D3D12-capable hardware found: \"%ls\" (%u MB)", a.deviceName.c_str(), a.dedicatedvideoMemory >> 20);
            {
                Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter3;
                adapter.As(&adapter3);

                X_ASSERT(pAdapter_ == nullptr, "pAdapter already valid")(pAdapter_);
                pAdapter_ = adapter3.Detach();
            }

            break;
        }
    }

    if (!pDevice_) {
        X_ERROR("Dx12", "Failed to CreateDevice: %s", Error::ToString(hr, Dsc));
        return false;
    }

    populateFeatureInfo();

    if (!deviceIsSupported()) {
        X_ERROR("Dx12", "The device does not meet the minimum requirements.");
        return false;
    }

#if X_DEBUG && 1
    // force enable debug layer.
#else
    if (vars_.enableDebugLayer())
#endif // |_DEBUG
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        if (SUCCEEDED(pDevice_->QueryInterface(IID_PPV_ARGS(&pInfoQueue)))) {
            D3D12_MESSAGE_SEVERITY Severities[] = {
                //	D3D12_MESSAGE_SEVERITY_CORRUPTION,
                //	D3D12_MESSAGE_SEVERITY_ERROR,
                //	D3D12_MESSAGE_SEVERITY_WARNING,
                D3D12_MESSAGE_SEVERITY_INFO,
                //	D3D12_MESSAGE_SEVERITY_MESSAGE
            };

            D3D12_INFO_QUEUE_FILTER NewFilter;
            core::zero_object(NewFilter);

            NewFilter.DenyList.NumSeverities = X_ARRAY_SIZE(Severities);
            NewFilter.DenyList.pSeverityList = Severities;

            hr = pInfoQueue->PushStorageFilter(&NewFilter);
            if (FAILED(hr)) {
                X_ERROR("Dx12", "failed to push storage filter: %s", Error::ToString(hr, Dsc));
            }

            pInfoQueue->Release();
        }
        else {
            X_WARNING("Dx12", "Failed to get InfoQue interface");
        }
    }

    pDescriptorAllocator_ = X_NEW(DescriptorAllocator, arena_, "DescriptorAllocator")(arena_, pDevice_);
    pDescriptorAllocatorPool_ = X_NEW(DescriptorAllocatorPool, arena_, "DescriptorAllocatorPool")(arena_, pDevice_, cmdListManager_);

    DescriptorAllocator& descriptorAllocator = *pDescriptorAllocator_;
    DescriptorAllocatorPool& descriptorAllocatorPool = *pDescriptorAllocatorPool_;

    pLinearAllocatorMan_ = X_NEW(LinearAllocatorManager, arena_, "LinAlocMan")(arena_, pDevice_, cmdListManager_);
    pContextMan_ = X_NEW(ContextManager, arena_, "ContextMan")(arena_, pDevice_, cmdListManager_, descriptorAllocatorPool, *pLinearAllocatorMan_);
    pRootSigCache_ = X_NEW(RootSignatureDeviceCache, arena_, "RootSignatureDeviceCache")(arena_, pDevice_);
    pPSOCache_ = X_NEW(PSODeviceCache, arena_, "PSODeviceCache")(arena_, pDevice_);
    pSamplerCache_ = X_NEW(SamplerDescriptorCache, arena_, "SamplerDescriptorCache")(arena_, pDevice_);

    pBuffMan_ = X_NEW(BufferManager, arena_, "BufferManager")(arena_, pDevice_, *pContextMan_, *pDescriptorAllocator_);
    if (!pBuffMan_->init()) {
        X_ERROR("Render", "failed to init buffer manager");
        return false;
    }

    pPSOCache_->registerVars();

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    core::zero_object(swapChainDesc);

    currentNativeRes_.x = width;
    currentNativeRes_.y = height;
    targetNativeRes_ = currentNativeRes_;

    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = SWAP_CHAIN_FORMAT;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    // Turn multisampling off.
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    if (!cmdListManager_.create(pDevice_)) {
        X_ERROR("Dx12", "Failed to init cmd list");
        return false;
    }

    // must be after cmdListMan
    pTextureMan_ = X_NEW(texture::TextureManager, arena_, "TexMan")(arena_, pDevice_, *pTexVars_, *pContextMan_,
        descriptorAllocator, texture::Util::DXGIFormatFromTexFmt(depthFmt), reverseZ);
    if (!pTextureMan_->init()) {
        X_ERROR("Render", "failed to init texture system");
        return false;
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    hr = dxgiFactory->CreateSwapChainForHwnd(cmdListManager_.getCommandQueue(), hWnd,
        &swapChainDesc, nullptr, nullptr, &swapChain);
    if (FAILED(hr)) {
        X_ERROR("Dx12", "failed to create swap chain: %s", Error::ToString(hr, Dsc));
        return false;
    }

    {
        Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
        swapChain.As(&swapChain3);

        pSwapChain_ = swapChain3.Detach();
    }

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
        Microsoft::WRL::ComPtr<ID3D12Resource> displayPlane;
        hr = pSwapChain_->GetBuffer(i, IID_PPV_ARGS(&displayPlane));
        if (FAILED(hr)) {
            X_ERROR("Dx12", "failed to get swap chain buffer: %s", Error::ToString(hr, Dsc));
            return false;
        }

        core::StackString512 name;
        name.appendFmt("$backbuffer_%i", i);

        pDisplayPlanes_[i] = pTextureMan_->createPixelBuffer(name.c_str(), Vec2i::zero(), 1, PixelBufferType::COLOR);

        ColorBuffer& colBuf = pDisplayPlanes_[i]->getColorBuf();
        colBuf.createFromSwapChain(pDevice_, descriptorAllocator, displayPlane.Detach());
        colBuf.setClearColor(vars_.getClearCol());
    }

    X_ASSERT_NOT_NULL(pShaderMan_);
    if (!pShaderMan_->init()) {
        X_ERROR("Render", "failed to init shader system");
        return false;
    }

    initILDescriptions();

    // send a event for initial size.
    gEnv->pCore->GetCoreEventDispatcher()->OnCoreEvent(CoreEvent::RENDER_RES_CHANGED, displayRes_.x, displayRes_.y);

    return true;
}

void XRender::shutDown(void)
{
    //	presentRS_.free();

    if (pBuffMan_) {
        pBuffMan_->shutDown();
        X_DELETE_AND_NULL(pBuffMan_, arena_);
    }

    if (pContextMan_) {
        pContextMan_->destroyAllContexts();
        X_DELETE_AND_NULL(pContextMan_, arena_);
    }

    if (pLinearAllocatorMan_) {
        pLinearAllocatorMan_->destroy();
        X_DELETE_AND_NULL(pLinearAllocatorMan_, arena_);
    }

    if (pPSOCache_) {
        pPSOCache_->destoryAll();
        X_DELETE_AND_NULL(pPSOCache_, arena_);
    }
    if (pRootSigCache_) {
        pRootSigCache_->destoryAll();
        X_DELETE_AND_NULL(pRootSigCache_, arena_);
    }
    if (pSamplerCache_) {
        X_DELETE_AND_NULL(pSamplerCache_, arena_);
    }

    if (pShaderMan_) {
        pShaderMan_->shutDown();
        X_DELETE_AND_NULL(pShaderMan_, arena_);
    }

    freeSwapChainResources();

    if (pTextureMan_) {
        pTextureMan_->shutDown();
        X_DELETE_AND_NULL(pTextureMan_, arena_);
    }

    cmdListManager_.shutdown();

    if (pDescriptorAllocator_) {
        pDescriptorAllocator_->destoryAllHeaps();
        X_DELETE_AND_NULL(pDescriptorAllocator_, arena_);
    }
    if (pDescriptorAllocatorPool_) {
        pDescriptorAllocatorPool_->destoryAll();
        X_DELETE_AND_NULL(pDescriptorAllocatorPool_, arena_);
    }

    core::SafeReleaseDX(pSwapChain_);
    core::SafeReleaseDX(pAdapter_);

#if X_DEBUG && 1
    // force enable debug layer.
#else
    if (vars_.enableDebugLayer())
#endif // |_DEBUG
    {
        ID3D12DebugDevice* pDebugInterface;
        if (SUCCEEDED(pDevice_->QueryInterface(&pDebugInterface))) {
            pDebugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
            pDebugInterface->Release();
        }
        else {
            X_ERROR("dx12", "Failed to get debug device interface");
        }
    }

    core::SafeReleaseDX(pDevice_);
}

void XRender::freeResources(void)
{
}

void XRender::release(void)
{
    X_DELETE(this, g_rendererArena);
}

void XRender::renderBegin(void)
{
#if PSO_HOT_RELOAD

    {
        core::CriticalSection::ScopedLock lock(dsCS_);

        // forgive me.
        for (auto* pState : deviceStates_) {
            const shader::ShaderPermatation& perm = *static_cast<const shader::ShaderPermatation*>(pState->pPerm);

            if (!perm.isCompiled()) {
                X_LOG0("Dx12", "^6Updating state");

                // would be nice if we only did this loop
                // if a shader ws recently invalidated which i think we can do.
                if (!updateStateState(pState)) {
                    X_ERROR("Dx12", "Failed to update state");
                }
            }
        }
    }

#endif // !PSO_HOT_RELOAD

#if RENDER_STATS
    core::atomic::Exchange(&stats_.numBatches, 0);
    core::atomic::Exchange(&stats_.numDrawCall, 0);
    core::atomic::Exchange(&stats_.numPoly, 0);
    core::atomic::Exchange(&stats_.numStatesChanges, 0);
    core::atomic::Exchange(&stats_.numVariableStateChanges, 0);
    core::atomic::Exchange(&stats_.numVBChanges, 0);
    core::atomic::Exchange(&stats_.numTexUpload, 0);
    core::atomic::Exchange(&stats_.numTexUploadSize, 0);
#endif // !RENDER_STATS

    ColorBuffer& colBuf = pDisplayPlanes_[currentBufferIdx_]->getColorBuf();

    D3D12_CPU_DESCRIPTOR_HANDLE RTVs[] = {
        colBuf.getRTV()};

    colBuf.setClearColor(vars_.getClearCol());

    GraphicsContext* pContext = pContextMan_->allocateGraphicsContext();

    pContext->transitionResource(colBuf.getGpuResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    pContext->clearColor(colBuf);
    // pContext->setRenderTargets(_countof(RTVs), RTVs);

    //	pContext->setViewportAndScissor(0, 0, currentNativeRes_.x, currentNativeRes_.y);
    //	pContext->setRootSignature(presentRS_);
    //
    //	pContext->setRenderTargets(_countof(RTVs), RTVs);
    //	pContext->setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //	pContext->draw(3);

    // pContext->clearColor(displayPlane_[currentBufferIdx_]);
    //	pContext->transitionResource(displayPlane_[currentBufferIdx_], D3D12_RESOURCE_STATE_PRESENT);
    pContext->finishAndFree(false);
}

void XRender::renderEnd(void)
{
    GraphicsContext* pContext = pContextMan_->allocateGraphicsContext();

    ColorBuffer& colBuf = pDisplayPlanes_[currentBufferIdx_]->getColorBuf();

    pContext->transitionResource(colBuf.getGpuResource(), D3D12_RESOURCE_STATE_PRESENT);
    pContext->finishAndFree(true);

    HRESULT hr = pSwapChain_->Present(0, 0);
    if (FAILED(hr)) {
        Error::Description Dsc;
        X_ERROR("Dx12", "Present failed. err: %s", Error::ToString(hr, Dsc));
    }

    currentBufferIdx_ = (currentBufferIdx_ + 1) % SWAP_CHAIN_BUFFER_COUNT;

    handleResolutionChange();
}

void XRender::submitCommandPackets(CommandBucket<uint32_t>& cmdBucket)
{
#if RENDER_STATS
    ++stats_.numBatches;
#endif // !RENDER_STATS

    const auto& sortedIdx = cmdBucket.getSortedIdx();
    const auto& packets = cmdBucket.getPackets();
    //	const auto& keys = cmdBucket.getKeys();

    const auto& viewMat = cmdBucket.getViewMatrix();
    const auto& projMat = cmdBucket.getProjMatrix();
    const auto& viewport = cmdBucket.getViewport();
    const auto& rtvs = cmdBucket.getRTVS();

    const uint32_t numRtvs = safe_static_cast<uint32_t, size_t>(rtvs.size());

    // anything to do?
    if (sortedIdx.isEmpty()) {
        //	return;
    }

    // will we ever not need one?
    if (rtvs.isEmpty()) {
        X_FATAL("Dx12", "atleast one rt is required");
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE RTVs[MAX_RENDER_TARGETS];
    DXGI_FORMAT RTVFormats[MAX_RENDER_TARGETS];

    core::zero_object(RTVs);
    core::zero_object(RTVFormats);

    for (size_t i = 0; i < rtvs.size(); i++) {
        const texture::Texture& tex = *static_cast<const texture::Texture*>(rtvs[i]);
        const ColorBuffer& colBuf = tex.getColorBuf();
        RTVs[i] = colBuf.getRTV();
        RTVFormats[i] = tex.getFormatDX();
    }

    // we should validate that RTVFormats matches the pass state.

    GraphicsContext* pContext = pContextMan_->allocateGraphicsContext();
    GraphicsContext& context = *pContext;

    for (size_t i = 0; i < rtvs.size(); i++) {
        texture::Texture& tex = *static_cast<texture::Texture*>(rtvs[i]);
        context.transitionResource(tex.getGpuResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    if (cmdBucket.getDepthStencil()) {
        render::IPixelBuffer* pDethStencil = static_cast<render::IPixelBuffer*>(cmdBucket.getDepthStencil());
        render::DepthBindFlags bindFlags = cmdBucket.getDepthBindFlags();
        if (pDethStencil->getBufferType() != PixelBufferType::DEPTH) {
            X_ERROR("Render", "Pixel buffer of type: \"%s\" can't be set as depthStencil",
                PixelBufferType::ToString(pDethStencil->getBufferType()));
            return;
        }

        texture::Texture& tex = *static_cast<texture::Texture*>(pDethStencil);
        DepthBuffer& depthBuf = tex.getDepthBuf();

        if (bindFlags.IsSet(DepthBindFlag::WRITE)) {
            context.transitionResource(depthBuf.getGpuResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
        }
        else {
            context.transitionResource(depthBuf.getGpuResource(), D3D12_RESOURCE_STATE_DEPTH_READ, true);
        }

        if (bindFlags.IsSet(DepthBindFlag::CLEAR)) {
            context.clearDepth(depthBuf);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE dsv = bindFlags.IsSet(DepthBindFlag::WRITE) ? depthBuf.getDSV() : depthBuf.getDSV_ReadOnly();

        context.setRenderTargets(numRtvs, RTVs, dsv);
    }
    else {
        context.setRenderTargets(numRtvs, RTVs);
    }

    context.setViewportAndScissor(viewport);

    State curState;

    for (size_t i = 0; i < sortedIdx.size(); ++i) {
        CommandPacket::Packet pPacket = packets[sortedIdx[i]];

#if X_DEBUG
        // this meants the sort logic and merge logic of command bucket is not quite correct / corrupt.

#if X_64
        X_ASSERT(union_cast<uintptr_t>(pPacket) != static_cast<uintptr_t>(0xDBDBDBDBDBDBDBDB), "Invalid packet")(pPacket);
#else
        X_ASSERT(union_cast<uintptr_t>(pPacket) != static_cast<uintptr_t>(0xDBDBDBDB), "Invalid packet")(pPacket);
#endif // !X_64

#endif

        do {
            const CommandPacket::Command::Enum cmdType = CommandPacket::loadCommandType(pPacket);
            const void* pCmd = CommandPacket::loadCommand(pPacket);

            switch (cmdType) {
                case Commands::Command::NOP: {
#if X_DEBUG && 0
                    // lets check we actually using this for chaining.
                    if (CommandPacket::loadNextCommandPacket(pPacket) == nullptr) {
                        X_WARNING("Dx12", "NOP command without any child commands");
                    }
#endif // X_DEBUG
                    break;
                }

                case Commands::Command::DRAW: {
                    const Commands::Draw* pDraw = reinterpret_cast<const Commands::Draw*>(pCmd);

                    applyState(context, curState, pDraw->stateHandle, pDraw->vertexBuffers,
                        pDraw->resourceState, CommandPacket::getAuxiliaryMemory(pDraw));

                    context.draw(pDraw->vertexCount, pDraw->startVertex);

#if RENDER_STATS
                    ++curState.numDrawCall;
                    curState.numPoly += pDraw->vertexCount;
#endif // !RENDER_STATS
                    break;
                }
                case Commands::Command::DRAW_INDEXED: {
                    const Commands::DrawIndexed* pDraw = reinterpret_cast<const Commands::DrawIndexed*>(pCmd);

                    applyState(context, curState, pDraw->stateHandle, pDraw->vertexBuffers,
                        pDraw->resourceState, CommandPacket::getAuxiliaryMemory(pDraw));

                    X_ASSERT(pDraw->indexBuffer != INVALID_BUF_HANLDE, "Index buffer must be valid")();

                    applyIndexBuffer(context, curState, pDraw->indexBuffer);

                    context.drawIndexed(pDraw->indexCount, pDraw->startIndex, pDraw->baseVertex);

#if RENDER_STATS
                    ++curState.numDrawCall;
                    curState.numPoly += pDraw->indexCount;
#endif // !RENDER_STATS
                    break;
                }
                case Commands::Command::DRAW_INSTANCED: {
#if RENDER_STATS
                    ++curState.numDrawCall;
#endif // !RENDER_STATS

                    const Commands::DrawInstanced* pDraw = reinterpret_cast<const Commands::DrawInstanced*>(pCmd);

                    applyState(context, curState, pDraw->stateHandle, pDraw->vertexBuffers,
                        pDraw->resourceState, CommandPacket::getAuxiliaryMemory(pDraw));

                    context.drawInstanced(pDraw->vertexCountPerInstance, pDraw->instanceCount,
                        pDraw->startVertexLocation, pDraw->startInstanceLocation);

#if RENDER_STATS
                    ++curState.numDrawCall;
                    curState.numPoly += (pDraw->vertexCountPerInstance * pDraw->instanceCount);
#endif // !RENDER_STATS
                    break;
                }
                case Commands::Command::DRAW_INSTANCED_INDEXED: {
                    const Commands::DrawInstancedIndexed* pDraw = reinterpret_cast<const Commands::DrawInstancedIndexed*>(pCmd);

                    applyState(context, curState, pDraw->stateHandle, pDraw->vertexBuffers,
                        pDraw->resourceState, CommandPacket::getAuxiliaryMemory(pDraw));

                    X_ASSERT(pDraw->indexBuffer != INVALID_BUF_HANLDE, "Index buffer must be valid")();

                    applyIndexBuffer(context, curState, pDraw->indexBuffer);

                    context.drawIndexedInstanced(pDraw->indexCountPerInstance, pDraw->instanceCount,
                        pDraw->startIndexLocation, pDraw->baseVertexLocation, pDraw->startInstanceLocation);

#if RENDER_STATS
                    ++curState.numDrawCall;
                    curState.numPoly += (pDraw->indexCountPerInstance * pDraw->instanceCount);
#endif // !RENDER_STATS
                    break;
                }
                case Commands::Command::COPY_CONST_BUF_DATA: {
                    const Commands::CopyConstantBufferData& updateCB = *reinterpret_cast<const Commands::CopyConstantBufferData*>(pCmd);
                    auto pCBuf = pBuffMan_->CBFromHandle(updateCB.constantBuffer);

                    X_ASSERT(pCBuf->getUsage() != BufUsage::IMMUTABLE, "Can't update a IMMUTABLE buffer")(pCBuf->getUsage());

                    if (core::pointerUtil::IsAligned(updateCB.pData, 16, 0)) {
                        context.writeBuffer(pCBuf->getBuf(), 0, updateCB.pData, updateCB.size);
                    }
                    else {
                        context.writeBufferUnAligned(pCBuf->getBuf(), 0, updateCB.pData, updateCB.size);
                    }
                } break;
                case Commands::Command::COPY_INDEXES_BUF_DATA: {
                    const Commands::CopyIndexBufferData& updateIB = *reinterpret_cast<const Commands::CopyIndexBufferData*>(pCmd);
                    auto pIBuf = pBuffMan_->IBFromHandle(updateIB.indexBuffer);

                    X_ASSERT(pIBuf->getUsage() != BufUsage::IMMUTABLE, "Can't update a IMMUTABLE buffer")(pIBuf->getUsage());

                    context.writeBuffer(pIBuf->getBuf(), updateIB.dstOffset, updateIB.pData, updateIB.size);
                } break;
                case Commands::Command::COPY_VERTEX_BUF_DATA: {
                    const Commands::CopyVertexBufferData& updateVB = *reinterpret_cast<const Commands::CopyVertexBufferData*>(pCmd);
                    auto pVBuf = pBuffMan_->IBFromHandle(updateVB.vertexBuffer);

                    X_ASSERT(pVBuf->getUsage() != BufUsage::IMMUTABLE, "Can't update a IMMUTABLE buffer")(pVBuf->getUsage());

                    context.writeBuffer(pVBuf->getBuf(), updateVB.dstOffset, updateVB.pData, updateVB.size);
                } break;

                case Commands::Command::UPDATE_TEXTUTE_BUF_DATA: {
                    const Commands::CopyTextureBufferData& updateTex = *reinterpret_cast<const Commands::CopyTextureBufferData*>(pCmd);

                    pTextureMan_->updateTextureData(context, updateTex.textureId, static_cast<const uint8_t*>(updateTex.pData), updateTex.size);

#if RENDER_STATS
                    ++curState.numTexUpload;
                    curState.numTexUploadSize += updateTex.size;
#endif // !RENDER_STATS
                } break;

                case Commands::Command::UPDATE_TEXTUTE_SUB_BUF_DATA: {
                    const Commands::CopyTextureSubRegionBufferData& updateSubTex = *reinterpret_cast<const Commands::CopyTextureSubRegionBufferData*>(pCmd);

                    pTextureMan_->updateTextureData(context, updateSubTex.textureId, static_cast<const uint8_t*>(updateSubTex.pData), updateSubTex.size);

#if RENDER_STATS
                    ++curState.numTexUpload;
                    curState.numTexUploadSize += updateSubTex.size;
#endif // !RENDER_STATS
                } break;

                case Commands::Command::CLEAR_DEPTH_STENCIL: {
                    const Commands::ClearDepthStencil& clearDepth = *reinterpret_cast<const Commands::ClearDepthStencil*>(pCmd);
                    texture::Texture* pTexture = static_cast<texture::Texture*>(clearDepth.pDepthBuffer);

                    X_ASSERT(pTexture->getBufferType() == PixelBufferType::DEPTH, "Invalid buffer passed to clear depth")();

                    DepthBuffer& depthBuf = pTexture->getDepthBuf();

                    context.clearDepth(depthBuf);
                } break;

                case Commands::Command::CLEAR_COLOR: {
                    const Commands::ClearColor& clearColor = *reinterpret_cast<const Commands::ClearColor*>(pCmd);
                    texture::Texture* pTexture = static_cast<texture::Texture*>(clearColor.pColorBuffer);

                    X_ASSERT(pTexture->getBufferType() == PixelBufferType::COLOR, "Invalid buffer passed to clear color")();

                    ColorBuffer& colBuf = pTexture->getColorBuf();

                    context.clearColor(colBuf);
                } break;

                default:
#if X_DEBUG
                    X_ASSERT_NOT_IMPLEMENTED();
                    break;
#else
                    X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
            }

            pPacket = CommandPacket::loadNextCommandPacket(pPacket);
        } while (pPacket != nullptr);
    }

    // for now wait.
    pContext->finishAndFree(true);

#if RENDER_STATS
    core::atomic::Add(&stats_.numDrawCall, curState.numDrawCall);
    core::atomic::Add(&stats_.numPoly, curState.numPoly);
    core::atomic::Add(&stats_.numStatesChanges, curState.numStatesChanges);
    core::atomic::Add(&stats_.numVariableStateChanges, curState.numVariableStateChanges);
    core::atomic::Add(&stats_.numVBChanges, curState.numVBChanges);
    core::atomic::Add(&stats_.numTexUpload, curState.numTexUpload);
    core::atomic::Add(&stats_.numTexUploadSize, curState.numTexUploadSize);
#endif // !RENDER_STATS
}

X_INLINE void XRender::createVBView(GraphicsContext& context, const VertexHandleArr& vertexBuffers,
    D3D12_VERTEX_BUFFER_VIEW viewsOut[VertexStream::ENUM_COUNT], uint32_t& numVertexStreams)
{
    numVertexStreams = 0;

    for (uint32_t i = 0; i < VertexStream::ENUM_COUNT; i++) {
        if (vertexBuffers[i]) {
            const auto pVertBuf = pBuffMan_->VBFromHandle(vertexBuffers[i]);
            auto& buffer = pVertBuf->getBuf();

            // transition if needed.
            context.transitionResource(buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            viewsOut[i] = buffer.vertexBufferView();

            // it's total that need to be passed to device, which inclde
            // any null ones in between.
            numVertexStreams = i + 1;
        }
        else {
            viewsOut[i].BufferLocation = 0;
            viewsOut[i].SizeInBytes = 0;
            viewsOut[i].StrideInBytes = 0;
        }
    }
}

void XRender::applyState(GraphicsContext& context, State& curState, const StateHandle handle,
    const VertexHandleArr& vertexBuffers, const Commands::ResourceStateBase& resourceState, const char* pStateData)
{
    const DeviceState& newState = *reinterpret_cast<const DeviceState*>(handle);

    if (curState.handle != handle) // if the handle is the same, everything is the same.
    {
#if RENDER_STATS
        ++curState.numStatesChanges;
#endif // !RENDER_STATS

        // the render system should not have to check ever state is valid, the 3dengine should check at creation time.
        // so it's a one off cost not a cost we pay for every fucking state change.
        X_ASSERT(handle != INVALID_STATE_HANLDE, "Don't pass me invalid states you cunt")(handle, INVALID_STATE_HANLDE);

        curState.handle = handle;

        // contains a redundant check.
        context.setRootSignature(newState.rootSig);

        if (curState.pPso != newState.pPso) {
            X_ASSERT_NOT_NULL(newState.pPso);
            context.setPipelineState(newState.pPso);
            curState.pPso = newState.pPso;
        }
        if (curState.topo != newState.topo) {
            context.setPrimitiveTopology(newState.topo);
            curState.topo = newState.topo;
        }
    }

    // vertex buffers are staying fixed size.
    // later i will try make the handles smaller tho ideally 16bit.
    if (std::memcmp(curState.vertexBuffers.data(), vertexBuffers.data(), sizeof(vertexBuffers)) != 0) {
#if RENDER_STATS
        ++curState.numVBChanges;
#endif // !RENDER_STATS

        curState.vertexBuffers = vertexBuffers;

        uint32_t numVertexStreams = 0;
        D3D12_VERTEX_BUFFER_VIEW vertexViews[VertexStream::ENUM_COUNT] = {0};
        createVBView(context, vertexBuffers, vertexViews, numVertexStreams);

        context.setVertexBuffers(0, numVertexStreams, vertexViews);
    }

    // got a variable state?
    // i want some nice texture and vertex stream state checks.
    // since we allow variable size now it's not so simple as to juust memcmp
    // we would need either a hash or something else.
    // for vertex buffers it might just be better to not make it variable.
    // as most of time alot will be set.
    // and it solves the slot problem.
    // maybe if we have some sort of index flags.
    // so the state stores the indexes that are set.
    // so then we can check if bound indexes are same, if so we can just memcmp the variable sized array.

    // work out if any resource state bound.
    if (curState.variableStateSize != resourceState.getStateSize() || std::memcmp(curState.variableState, pStateData, resourceState.getStateSize()) != 0) {
        std::memcpy(curState.variableState, pStateData, resourceState.getStateSize());
        curState.variableStateSize = resourceState.getStateSize();

#if RENDER_STATS
        ++curState.numVariableStateChanges;
#endif // !RENDER_STATS

        if (resourceState.anySet()) {
            if (resourceState.getNumTextStates()) {
                X_ASSERT(newState.texRootIdxBase != std::numeric_limits<decltype(newState.texRootIdxBase)>::max(), "Texture rootIdx base is invalid")();

                D3D12_CPU_DESCRIPTOR_HANDLE textureSRVS[render::TextureSlot::ENUM_COUNT] = {};
                const TextureState* pTexStates = resourceState.getTexStates(pStateData);

                for (int32_t t = 0; t < resourceState.getNumTextStates(); t++) {
                    const auto& texState = pTexStates[t];
                    auto* pTex = pTextureMan_->getByID(texState.textureId);
                    X_ASSERT_NOT_NULL(pTex);

                    // texture may not have a device texture yet.
                    // maybe just use default if not loaded?
                    auto& gpuResource = pTex->getGpuResource();

                    context.transitionResource(gpuResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

                    // HACK: spin till valid.
                    auto srv = pTex->getSRV();
                    if(srv.ptr == render::D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
                    {
                        int backOff = 0;
                        while (srv.ptr == render::D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
                        {
                            core::Thread::backOff(backOff);
                        }
                    }

                    textureSRVS[t] = pTex->getSRV();

                    X_ASSERT(textureSRVS[t].ptr != render::D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN &&
                            textureSRVS[t].ptr != render::D3D12_GPU_VIRTUAL_ADDRESS_NULL,
                        "Invalid handle")(pTex->getName().c_str(), textureSRVS[t].ptr, pTex->getSRV().ptr);
                }

                // for now assume all slots are linera and no gaps.
                const auto count = resourceState.getNumTextStates();
                context.setDynamicDescriptors(newState.texRootIdxBase, 0, count, textureSRVS);

                int goat = 0;
                goat = 1;
            }

            if (resourceState.getNumBuffers()) {
                X_ASSERT(newState.bufferRootIdxBase != std::numeric_limits<decltype(newState.bufferRootIdxBase)>::max(), "Buffer rootIdx base is invalid")();

                const BufferState* pBuffers = resourceState.getBuffers(pStateData);
                const int32_t count = resourceState.getNumBuffers();

                //	D3D12_CPU_DESCRIPTOR_HANDLE bufferSRVS[render::TextureSlot::ENUM_COUNT] = {};

                for (int32_t t = 0; t < count; t++) {
                    auto& bufState = pBuffers[t];
                    X_ASSERT(bufState.buf != INVALID_BUF_HANLDE, "Buffer handle is invalid")(bufState.buf);

                    uint32_t rootIdx = newState.bufferRootIdxBase + t;

                    // support inline dynalic buffer descriptions.
                    if (pBuffMan_->ValidHandle(bufState.buf)) {
                        auto* pCbuf = pBuffMan_->VBFromHandle(bufState.buf);
                        auto& buf = pCbuf->getBuf();

                        context.transitionResource(buf, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                        context.setBufferSRV(rootIdx, buf);
                    }
                    else {
                        auto* pDynamicDesc = reinterpret_cast<const DynamicBufferDesc*>(bufState.buf);
                        X_ASSERT(pDynamicDesc->magic == DynamicBufferDesc::MAGIC, "Expected dynamic buffer desc")();
                        X_ASSERT(pDynamicDesc->size > 0, "Empty buffer")();
                        X_ASSERT_NOT_NULL(pDynamicDesc->pData);

                        context.setDynamicSRV(rootIdx, pDynamicDesc->size, pDynamicDesc->pData);
                    }
                }

                //	context.setDynamicDescriptors(newState.bufferRootIdxBase, 0, count, bufferSRVS);
            }

            // this may be zero even if we have samplers, if they are all static.
            if (resourceState.getNumSamplers()) {
                X_ASSERT(newState.samplerRootIdxBase != std::numeric_limits<decltype(newState.samplerRootIdxBase)>::max(), "Sampler rootIdx base is invalid")();

                D3D12_CPU_DESCRIPTOR_HANDLE samplerSRVS[render::TextureSlot::ENUM_COUNT] = {};
                const SamplerState* pSamplers = resourceState.getSamplers(pStateData);

                // potentially I want todo redundancy here.
                // I may also want to require the 3dengine to make sampler states in advance so not done here.
                // and we just pass sampler id's that map to samplerDescriptors.
                for (int32_t t = 0; t < resourceState.getNumTextStates(); t++) {
                    const auto& sampler = pSamplers[t];
                    auto samplerDescriptor = pSamplerCache_->createDescriptor(*pDescriptorAllocator_, sampler);

                    samplerSRVS[t] = samplerDescriptor.getCpuDescriptorHandle();
                }

                const auto count = resourceState.getNumSamplers();
                context.setDynamicSamplerDescriptors(newState.samplerRootIdxBase, 0, count, samplerSRVS);
            }

            if (resourceState.getNumCBs()) {
                const ConstantBufferHandle* pCBVs = resourceState.getCBs(pStateData);

                X_ASSERT(newState.cbRootIdxBase != std::numeric_limits<decltype(newState.cbRootIdxBase)>::max(), "CB rootIdx base is invalid")();

                for (int32_t t = 0; t < resourceState.getNumCBs(); t++) {
                    ConstantBufferHandle cbh = pCBVs[t];

                    if (cbh == curState.constBuffers[t]) {
                        continue;
                    }

                    // this needs clearing when rootsig changes.
                    curState.constBuffers[t] = cbh;

                    // we need to know the index this cb should be bound to.
                    auto* pCbuf = pBuffMan_->CBFromHandle(cbh);
                    auto& buf = pCbuf->getBuf();
                    uint32_t rootIdx = newState.cbRootIdxBase + t;

                    context.transitionResource(buf, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                    context.setConstantBuffer(rootIdx, buf.getGpuVirtualAddress());
                }
            }
        }
        else {
            // clear?
            // or just leave them bound.
        }
    }
}

void XRender::applyIndexBuffer(GraphicsContext& context, State& curState, IndexBufferHandle ib)
{
    if (curState.indexBuffer != ib) {
        curState.indexBuffer = ib;
        const auto pIBuf = pBuffMan_->IBFromHandle(ib);

        context.transitionResource(pIBuf->getBuf(), D3D12_RESOURCE_STATE_INDEX_BUFFER);
        context.setIndexBuffer(pIBuf->getBuf().indexBufferView());
    }
}

Vec2<uint32_t> XRender::getDisplayRes(void) const
{
    return displayRes_;
}

IPixelBuffer* XRender::createDepthBuffer(const char* pNickName, Vec2i dim)
{
    texture::Texture* pPixelBuf = pTextureMan_->createPixelBuffer(pNickName, dim, 1, PixelBufferType::DEPTH);

    return pPixelBuf;
}

IPixelBuffer* XRender::createColorBuffer(const char* pNickName, Vec2i dim, uint32_t numMips,
    texture::Texturefmt::Enum fmt)
{
    texture::Texture* pColBuf = pTextureMan_->createPixelBuffer(pNickName, dim, numMips, PixelBufferType::COLOR);
    ColorBuffer& colBuf = pColBuf->getColorBuf();

    DXGI_FORMAT dxFmt = texture::Util::DXGIFormatFromTexFmt(fmt);
    colBuf.create(pDevice_, *pDescriptorAllocator_, dim.x, dim.y, numMips, dxFmt);

    return pColBuf;
}

IRenderTarget* XRender::getCurBackBuffer(uint32_t* pIdx)
{
    if (pIdx) {
        *pIdx = currentBufferIdx_;
    }

    return pDisplayPlanes_[currentBufferIdx_];
}

VertexBufferHandle XRender::createVertexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
    return pBuffMan_->createVertexBuf(numElements, elementSize, nullptr, usage, accessFlag);
}

VertexBufferHandle XRender::createVertexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
    return pBuffMan_->createVertexBuf(numElements, elementSize, pInitialData, usage, accessFlag);
}

IndexBufferHandle XRender::createIndexBuffer(uint32_t elementSize, uint32_t numElements, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
    return pBuffMan_->createIndexBuf(numElements, elementSize, nullptr, usage, accessFlag);
}

IndexBufferHandle XRender::createIndexBuffer(uint32_t elementSize, uint32_t numElements, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
    return pBuffMan_->createIndexBuf(numElements, elementSize, pInitialData, usage, accessFlag);
}

void XRender::destoryVertexBuffer(VertexBufferHandle handle)
{
    X_ASSERT(handle != INVALID_BUF_HANLDE, "Can't pass invalid handles")();

    pBuffMan_->freeVB(handle);
}

void XRender::destoryIndexBuffer(IndexBufferHandle handle)
{
    X_ASSERT(handle != INVALID_BUF_HANLDE, "Can't pass invalid handles")();

    pBuffMan_->freeIB(handle);
}

void XRender::getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize)
{
    X_ASSERT(handle != INVALID_BUF_HANLDE, "Can't pass invalid handles")();

    pBuffMan_->getBufSize(handle, pOriginal, pDeviceSize);
}

void XRender::getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize)
{
    X_ASSERT(handle != INVALID_BUF_HANLDE, "Can't pass invalid handles")();

    pBuffMan_->getBufSize(handle, pOriginal, pDeviceSize);
}

Stats XRender::getStats(void) const
{
    Stats stats;
#if RENDER_STATS
    stats = stats_;
#endif // !RENDER_STATS
    return stats;
}

// cb's
ConstantBufferHandle XRender::createConstBuffer(const shader::XCBuffer& cb, BufUsage::Enum usage)
{
    const auto& data = cb.getCpuData();

    ConstantBufferHandle handle = pBuffMan_->createConstBuf(cb.getBindSize(), data.data(), usage, render::CpuAccess::WRITE);

    return handle;
}

void XRender::destoryConstBuffer(ConstantBufferHandle handle)
{
    pBuffMan_->freeCB(handle);
}

IDeviceTexture* XRender::getDeviceTexture(int32_t id, const char* pNickName)
{
    texture::Texture* pText = pTextureMan_->getDeviceTexture(id, pNickName);

    return pText;
}

IDeviceTexture* XRender::createTexture(const char* pNickName, Vec2i dim,
    texture::Texturefmt::Enum fmt, BufUsage::Enum usage, const uint8_t* pInitialData)
{
    texture::Texture* pText = pTextureMan_->createTexture(pNickName, dim, fmt, usage, pInitialData);

    return pText;
}

bool XRender::initDeviceTexture(IDeviceTexture* pTex)
{
    return pTextureMan_->initDeviceTexture(static_cast<texture::Texture*>(pTex));
}

bool XRender::initDeviceTexture(IDeviceTexture* pTex, const texture::XTextureFile& imgFile)
{
    return pTextureMan_->initDeviceTexture(static_cast<texture::Texture*>(pTex), imgFile);
}

shader::IShaderSource* XRender::getShaderSource(const core::string& sourceName)
{
    return pShaderMan_->sourceforName(sourceName);
}

shader::IHWShader* XRender::createHWShader(shader::ShaderType::Enum type, const core::string& entry, const core::string& customDefines,
    shader::IShaderSource* pSourceFile, shader::PermatationFlags permFlags, render::shader::VertexFormat::Enum vertFmt)
{
    return pShaderMan_->createHWShader(type, entry, customDefines, pSourceFile, permFlags, vertFmt);
}

shader::IShaderPermatation* XRender::createPermatation(const shader::ShaderStagesArr& stages)
{
    return pShaderMan_->createPermatation(stages);
}

void XRender::releaseShaderPermatation(shader::IShaderPermatation* pPerm)
{
    pShaderMan_->releaseShaderPermatation(pPerm);
}

void XRender::releaseTexture(IDeviceTexture* pTex)
{
    pTextureMan_->releaseTexture(pTex);
}

void XRender::releasePixelBuffer(render::IPixelBuffer* pPixelBuf)
{
    pTextureMan_->releasePixelBuffer(pPixelBuf);
}

PassStateHandle XRender::createPassState(const RenderTargetFmtsArr& rtfs)
{
    PassState* pPass = X_NEW(PassState, &statePool_, "PassState");
    pPass->rtfs = rtfs;

#if RENDER_STATS
    ++stats_.numPassStates;
    stats_.maxPassStates = core::Max(stats_.maxPassStates, stats_.numPassStates);
#endif // !RENDER_STATS

    return reinterpret_cast<PassStateHandle>(pPass);
}

void XRender::destoryPassState(PassStateHandle passHandle)
{
    PassState* pPassState = reinterpret_cast<PassState*>(passHandle);

#if RENDER_STATS
    --stats_.numPassStates;
#endif // !RENDER_STATS

    X_DELETE(pPassState, &statePool_);
}

D3D12_SHADER_VISIBILITY stageFlagsToStageVisibility(shader::ShaderStageFlags stageFlags)
{
    const auto flagsInt = stageFlags.ToInt();

    switch (flagsInt) {
        case shader::ShaderStage::Vertex:
            return D3D12_SHADER_VISIBILITY_VERTEX;
        case shader::ShaderStage::Pixel:
            return D3D12_SHADER_VISIBILITY_PIXEL;
        case shader::ShaderStage::Domain:
            return D3D12_SHADER_VISIBILITY_DOMAIN;
        case shader::ShaderStage::Geometry:
            return D3D12_SHADER_VISIBILITY_GEOMETRY;
        case shader::ShaderStage::Hull:
            return D3D12_SHADER_VISIBILITY_HULL;

        // any combination results in all visibility.
        default:
            return D3D12_SHADER_VISIBILITY_ALL;
    }
}

#if PSO_HOT_RELOAD
bool XRender::updateStateState(DeviceState* pState)
{
    const shader::ShaderPermatation& perm = *static_cast<const shader::ShaderPermatation*>(pState->pPerm);
    const PassState* pPassState = pState->pPassState;
    const StateDesc& desc = pState->cpuDesc;
    const auto& staticSamplers = pState->staticSamplers;

    // shaders only for now?
    if (perm.isCompiled()) {
        return false;
    }

    // need to compile the shaders.
    if (!pShaderMan_->compilePermatation(const_cast<shader::IShaderPermatation*>(pState->pPerm))) {
        return false;
    }

    // the RootSig may need updating.
    if (!buildRootSig(pState, perm, staticSamplers.ptr(), staticSamplers.size())) {
        return false;
    }

    // new PSO BOI !
    GraphicsPSO pso;

    if (!buildPSO(pso, pPassState, desc, pState->rootSig, perm)) {
        return false;
    }

    pState->pPso = pso.getPipelineStateObject();
    return true;
}
#endif // !PSO_HOT_RELOAD

StateHandle XRender::createState(PassStateHandle passHandle, const shader::IShaderPermatation* pPerm,
    const StateDesc& desc,
    const SamplerState* pStaticSamplers, size_t numStaticSamplers)
{
    X_ASSERT_NOT_NULL(pPerm);

    const PassState* pPassState = reinterpret_cast<const PassState*>(passHandle);

    // permatations are currently compiled on creation.
    const shader::ShaderPermatation& perm = *static_cast<const shader::ShaderPermatation*>(pPerm);

    if (!perm.isCompiled()) {
#if X_ENABLE_RENDER_SHADER_RELOAD
        // this could of happend if we hot reloaded the shader.
        X_WARNING("Dx12", "Compining perm in state creation, shader was likley reloaded");

        if (!pShaderMan_->compilePermatation(const_cast<shader::IShaderPermatation*>(pPerm))) {
            return INVALID_STATE_HANLDE;
        }
#else
        return INVALID_STATE_HANLDE;
#endif // !X_ENABLE_RENDER_SHADER_RELOAD
    }

    if (perm.getILFmt() != shader::Util::ILfromVertexFormat(desc.vertexFmt)) {
        X_ERROR("Dx12", "Hardware tech's input layout does not match state description \"%s\" -> %s",
            shader::InputLayoutFormat::ToString(perm.getILFmt()),
            shader::InputLayoutFormat::ToString(shader::Util::ILfromVertexFormat(desc.vertexFmt)));

        // this is user error, trying to use a permatation with a diffrent vertex fmt than it was compiled for.
        // or maybe we have some permatation selection logic issues..
        return INVALID_STATE_HANLDE;
    }

#if 0
	if (!hwTech.canDraw()) {
		return INVALID_STATE_HANLDE;
	}
	if (hwTech.IlFmt != shader::Util::ILfromVertexFormat(desc.vertexFmt)) {
		X_ERROR("Dx12", "Hardware tech's input layout does not match state description \"%s\" -> %s",
			shader::InputLayoutFormat::ToString(hwTech.IlFmt),
			shader::InputLayoutFormat::ToString(shader::Util::ILfromVertexFormat(desc.vertexFmt)));

		// this is user error, trying to use a permatation with a diffrent vertex fmt than it was compiled for.
		// or maybe we have some permatation selection logic issues..
		return INVALID_STATE_HANLDE;
	}
#endif

    DeviceState* pState = X_NEW(DeviceState, &statePool_, "DeviceState")(arena_);
    pState->pPso = nullptr;
    pState->topo = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

#if DEVICE_STATE_STORE_CPU_DESC
    pState->cpuDesc = desc;
#endif // !DEVICE_STATE_STORE_CPU_DESC

#if PSO_HOT_RELOAD
    pState->pPassState = pPassState;
    pState->pPerm = pPerm;

    for (size_t i = 0; i < numStaticSamplers; i++) {
        pState->staticSamplers.append(pStaticSamplers[i]);
    }
#endif // !PSO_HOT_RELOAD

    // we need a root sig to compile this PSO with.
    // but it don't have to be the rootSig we render with.

    if (!buildRootSig(pState, perm, pStaticSamplers, numStaticSamplers)) {
        X_DELETE(pState, &statePool_);
        return INVALID_STATE_HANLDE;
    }

    // we need to create a PSO.
    GraphicsPSO pso;

    if (!buildPSO(pso, pPassState, desc, pState->rootSig, perm)) {
        X_DELETE(pState, &statePool_);
        return INVALID_STATE_HANLDE;
    }

    pState->pPso = pso.getPipelineStateObject();
    pState->topo = topoFromDesc(desc);

#if RENDER_STATS
    ++stats_.numStates;
    stats_.maxStates = core::Max(stats_.maxStates, stats_.numStates);
#endif // !RENDER_STATS

#if PSO_HOT_RELOAD
    {
        core::CriticalSection::ScopedLock lock(dsCS_);
        deviceStates_.push_back(pState);
    }
#endif // !PSO_HOT_RELOAD

    return reinterpret_cast<StateHandle>(pState);
}

void XRender::destoryState(StateHandle handle)
{
    // this implies you are not checking they are valid when creating, which you should!
    X_ASSERT(handle != INVALID_STATE_HANLDE, "Destoring invalid states is not allowed")(handle, INVALID_STATE_HANLDE);

    DeviceState* pState = reinterpret_cast<DeviceState*>(handle);

#if PSO_HOT_RELOAD
    {
        core::CriticalSection::ScopedLock lock(dsCS_);
        deviceStates_.remove(pState);
    }
#endif // !PSO_HOT_RELOAD

#if RENDER_STATS
    --stats_.numStates;
#endif // !RENDER_STATS

    X_DELETE(pState, &statePool_);
}

bool XRender::buildRootSig(DeviceState* pState, const shader::ShaderPermatation& perm,
    const SamplerState* pStaticSamplers, size_t numStaticSamplers)
{
    RootSignature& rootSig = pState->rootSig;

#if 1

    // we are going to create a rootsig based on the shader.
    // we should try be smart about it like using static samplers if possible.
    // all cbv's are gonna go in root sig.
    // we need to know all the srv's we need also
    // we may want to support textures in none pixel stage also.

    // this is a list of cbuffers used by all stages and also defining what stages they need to be visible.
    const auto& cbufLinks = perm.getCbufferLinks();
    const auto& buffers = perm.getBuffers();

    size_t numParams = 0;

    numParams += cbufLinks.size(); // one for each cbuffer.
    numParams += buffers.size();   // one for each buffer.

    if (perm.isStageSet(shader::ShaderType::Pixel)) {
        auto* pPixelShader = perm.getStage(shader::ShaderType::Pixel);

        if (pPixelShader->getNumTextures() > 0) {
            numParams++; // a descriptor range
        }

        if (!numStaticSamplers) {
            if (pPixelShader->getNumSamplers() > 0) {
                numParams++; // a descriptor range
            }
        }
        else {
            // currently only allow static samplers, if you provide all the samplers.
            X_ASSERT(pPixelShader->getNumSamplers() == numStaticSamplers, "Static samplers must match sampler count")(pPixelShader->getNumSamplers(), numStaticSamplers);
        }
    }

    rootSig.reset(numParams, numStaticSamplers);

    uint32_t currentParamIdx = 0;

    if (cbufLinks.isNotEmpty()) {
        pState->cbRootIdxBase = currentParamIdx;

        for (size_t i = 0; i < cbufLinks.size(); i++) {
            const auto& cbLink = cbufLinks[i];
            auto vis = stageFlagsToStageVisibility(cbLink.stages);
            auto& cb = *cbLink.pCBufer;
            auto bindPoint = cb.getBindPoint();

            rootSig.getParamRef(currentParamIdx++).initAsCBV(bindPoint, vis);
        }
    }
    if (buffers.isNotEmpty()) {
        pState->bufferRootIdxBase = currentParamIdx;

        for (size_t i = 0; i < buffers.size(); i++) {
            const auto& buffer = buffers[i];
            auto bindPoint = buffer.getBindPoint();

            rootSig.getParamRef(currentParamIdx++).initAsSRV(bindPoint, D3D12_SHADER_VISIBILITY_VERTEX);
            //	rootSig.getParamRef(currentParamIdx++).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        }
    }

    if (perm.isStageSet(shader::ShaderType::Pixel)) {
        auto* pPixelShader = perm.getStage(shader::ShaderType::Pixel);

        auto numTextures = pPixelShader->getNumTextures();
        auto numSamplers = pPixelShader->getNumSamplers();

        if (numTextures > 0) {
            pState->texRootIdxBase = currentParamIdx;

            rootSig.getParamRef(currentParamIdx++).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, numTextures, D3D12_SHADER_VISIBILITY_PIXEL);
        }

        if (!numStaticSamplers) {
            if (numSamplers > 0) {
                pState->samplerRootIdxBase = currentParamIdx;

                rootSig.getParamRef(currentParamIdx++).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, numSamplers, D3D12_SHADER_VISIBILITY_PIXEL);
            }
        }
        else {
            auto& samplers = pPixelShader->getSamplers();

            for (size_t i = 0; i < numStaticSamplers; i++) {
                auto& ss = pStaticSamplers[i];

                auto bindPoint = samplers[i].getBindPoint();

                SamplerDesc desc;
                samplerDescFromState(ss, desc);
                // can we know the index?
                rootSig.initStaticSampler(bindPoint, desc, D3D12_SHADER_VISIBILITY_PIXEL);
            }
        }
    }

#else
    // we need a root sig that maps correct with this shader.
    // maybe just having the root sig in the shader def makes sense then?
    // id rather not i think the render system should be able to decide;
    // but should it just be worked out based on the shader?
    // so the description is part of the hwshader.
    rootSig.getParamRef(0).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, TextureSlot::ENUM_COUNT, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSig.getParamRef(1).initAsCBV(0, D3D12_SHADER_VISIBILITY_VERTEX);
    //	rootSig.getParamRef(2).initAsSRV(1, D3D12_SHADER_VISIBILITY_PIXEL);
    //	rootSig.getParamRef(2).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
    //	rootSig.getParamRef(3).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
    rootSig.initStaticSampler(0, samplerLinearClampDesc_, D3D12_SHADER_VISIBILITY_PIXEL);
#endif

    if (!rootSig.finalize(*pRootSigCache_,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            // should i just disable these for now?
            // seams not:
            //		"Don't simultaneously set visible and deny flags for the same shader stages on root table entries
            //		For current drivers the deny flags only work when D3D12_SHADER_VISIBILITY_ALL is set"
            //	| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
            //	| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
            //	| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
            )) {
        return false;
    }

#if X_DEBUG == 0 || 1
    rootSig.freeParams();
#endif

    return true;
}

bool XRender::buildPSO(GraphicsPSO& pso, const PassState* pPassState,
    const StateDesc& desc, const RootSignature& rootSig, const shader::ShaderPermatation& perm)
{
    DXGI_FORMAT RTVFormats[MAX_RENDER_TARGETS];
    DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN;
    core::zero_object(RTVFormats);

    for (size_t i = 0; i < pPassState->rtfs.size(); i++) {
        RTVFormats[i] = texture::Util::DXGIFormatFromTexFmt(pPassState->rtfs[i]);
    }

    // we can leave depth bound if if not testing no?
    //	if (desc.stateFlags.IsSet(StateFlag::DEPTHWRITE))
    {
        DSVFormat = pTextureMan_->getDepthFmt();
    }

    D3D12_BLEND_DESC blendDesc;
    D3D12_RASTERIZER_DESC rasterizerDesc;
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
    createDescFromState(desc, blendDesc);
    createDescFromState(desc, rasterizerDesc);
    createDescFromState(desc, depthStencilDesc);

    pso.setRootSignature(rootSig);
    pso.setBlendState(blendDesc);
    pso.setRasterizerState(rasterizerDesc);
    pso.setDepthStencilState(depthStencilDesc);
    pso.setSampleMask(0xFFFFFFFF);
    pso.setRenderTargetFormats(static_cast<uint32_t>(pPassState->rtfs.size()), RTVFormats, DSVFormat, 1, 0);
    pso.setPrimitiveTopologyType(topoTypeFromDesc(desc));

    VertexLayoutDescArr* pInputLayout = &ilDescriptions_[desc.vertexFmt];
    if (desc.stateFlags.IsSet(StateFlag::VERTEX_STREAMS)) {
        pInputLayout = &ilStreamedDescriptions_[desc.vertexFmt];
    }

    X_ASSERT_NOT_NULL(pInputLayout);

    // this must stay valid during call to finalize.
    VertexLayoutDescArr inputDesc;

    if (desc.stateFlags.IsSet(StateFlag::INSTANCED_POS_COLOR)) {
        if (desc.stateFlags.IsSet(StateFlag::HWSKIN)) {
            // TODO..
            X_ASSERT_NOT_IMPLEMENTED();
        }

        // make a copy and append instanced descriptions.
        inputDesc = *pInputLayout;
        for (const auto& il : ilInstanced_) {
            inputDesc.append(il);
        }

        pso.setInputLayout(inputDesc.size(), inputDesc.ptr());
    }
    else if (desc.stateFlags.IsSet(StateFlag::HWSKIN)) {
        // make a copy and append instanced descriptions.
        inputDesc = *pInputLayout;
        for (const auto& il : ilHwSkin_) {
            inputDesc.append(il);
        }

        pso.setInputLayout(inputDesc.size(), inputDesc.ptr());
    }
    else {
        pso.setInputLayout(pInputLayout->size(), pInputLayout->ptr());
    }

    if (perm.isStageSet(shader::ShaderType::Vertex)) {
        const auto* pVertexShader = perm.getStage(shader::ShaderType::Vertex);
        const auto& byteCode = pVertexShader->getShaderByteCode();
        pso.setVertexShader(byteCode.data(), byteCode.size());
    }
    if (perm.isStageSet(shader::ShaderType::Pixel)) {
        const auto* pPixelShader = perm.getStage(shader::ShaderType::Pixel);
        const auto& byteCode = pPixelShader->getShaderByteCode();
        pso.setPixelShader(byteCode.data(), byteCode.size());
    }
    if (perm.isStageSet(shader::ShaderType::Hull) || perm.isStageSet(shader::ShaderType::Domain) || perm.isStageSet(shader::ShaderType::Geometry)) {
        // in order to allow these check if the root sig flags need changing then just duplicate
        // the bytecode setting logic.
        X_ERROR("Dx12", "Domain, Hull, Geo are not enabled currently");
    }

    if (!pso.finalize(*pPSOCache_)) {
        return false;
    }

    return true;
}

bool XRender::freeSwapChainResources(void)
{
    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
        if (pDisplayPlanes_[i]) {
            pDisplayPlanes_[i]->destroy();

            pTextureMan_->releasePixelBuffer(pDisplayPlanes_[i]);
        }
    }

    core::zero_object(pDisplayPlanes_);
    return true;
}

void XRender::initILDescriptions(void)
{
    const uint32_t num = shader::VertexFormat::ENUM_COUNT;

    D3D12_INPUT_ELEMENT_DESC elem_pos = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    D3D12_INPUT_ELEMENT_DESC elem_nor101010 = {"NORMAL", 0, DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    //6	D3D11_INPUT_ELEMENT_DESC elem_nor8888 = { "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    D3D12_INPUT_ELEMENT_DESC elem_nor323232 = {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    D3D12_INPUT_ELEMENT_DESC elem_col8888 = {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    D3D12_INPUT_ELEMENT_DESC elem_uv3232 = {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    D3D12_INPUT_ELEMENT_DESC elem_uv1616 = {"TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    //	D3D11_INPUT_ELEMENT_DESC elem_uv32323232 = { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

    D3D12_INPUT_ELEMENT_DESC elem_t3f = {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

    D3D12_INPUT_ELEMENT_DESC elem_tagent101010 = {"TANGENT", 0, DXGI_FORMAT_R10G10B10A2_TYPELESS, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    D3D12_INPUT_ELEMENT_DESC elem_tagent323232 = {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    D3D12_INPUT_ELEMENT_DESC elem_biNormal101010 = {"BINORMAL", 0, DXGI_FORMAT_R10G10B10A2_TYPELESS, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    D3D12_INPUT_ELEMENT_DESC elem_biNormal323232 = {"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

    for (uint32_t i = 0; i < num; i++) {
        auto& layout = ilDescriptions_[i];

        // for now all positions are just 32bit floats baby!
        elem_pos.AlignedByteOffset = 0;
        elem_pos.SemanticIndex = 0;
        elem_uv3232.SemanticIndex = 0;
        layout.emplace_back(elem_pos);

        if (i == shader::VertexFormat::P3F_T2S || i == shader::VertexFormat::P3F_T2S_C4B || i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F || i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10) {
            elem_uv1616.AlignedByteOffset = 12;
            layout.emplace_back(elem_uv1616);
        }
        if (i == shader::VertexFormat::P3F_T2S_C4B || i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F || i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10) {
            elem_col8888.AlignedByteOffset = 12 + 4;
            layout.emplace_back(elem_col8888);
        }

        if (i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F) {
            elem_nor323232.AlignedByteOffset = 12 + 4 + 4;
            layout.emplace_back(elem_nor323232); // 12 bytes
        }
        if (i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F) {
            elem_tagent323232.AlignedByteOffset = 12 + 4 + 4 + 12;
            layout.emplace_back(elem_tagent323232); // 12 bytes

            elem_biNormal323232.AlignedByteOffset = 12 + 4 + 4 + 12 + 12;
            layout.emplace_back(elem_biNormal323232); // 12 bytes
        }

        if (i == shader::VertexFormat::P3F_T2F_C4B) {
            elem_uv3232.AlignedByteOffset = 12;
            layout.emplace_back(elem_uv3232);

            elem_col8888.AlignedByteOffset = 20;
            layout.emplace_back(elem_col8888);
        }
        else if (i == shader::VertexFormat::P3F_T3F) {
            elem_t3f.AlignedByteOffset = 12;
            layout.emplace_back(elem_t3f);
        }

        if (i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10) {
            // 12 + 4 + 4
            elem_nor101010.AlignedByteOffset = 20;
            layout.emplace_back(elem_nor101010);
        }
        if (i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10) {
            elem_tagent101010.AlignedByteOffset = 24;
            layout.emplace_back(elem_tagent101010);
            elem_biNormal101010.AlignedByteOffset = 28;
            layout.emplace_back(elem_biNormal101010);
        }

        if (i == shader::VertexFormat::P3F_T4F_C4B_N3F) {
            // big man texcoords
            elem_uv3232.AlignedByteOffset = 12;
            layout.emplace_back(elem_uv3232);

            // two of them
            elem_uv3232.AlignedByteOffset = 20;
            elem_uv3232.SemanticIndex = 1;
            layout.emplace_back(elem_uv3232);
            elem_uv3232.SemanticIndex = 0;

            // byte offset is zero since diffrent stream.
            elem_col8888.AlignedByteOffset = 28;
            layout.emplace_back(elem_col8888);

            elem_nor323232.AlignedByteOffset = 32;
            layout.emplace_back(elem_nor323232);
        }
    }

    // Streams

    // color stream
    static D3D12_INPUT_ELEMENT_DESC elem_stream_color[] = {
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, VertexStream::COLOR, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    // normals stream
    static D3D12_INPUT_ELEMENT_DESC elem_stream_normals[] = {
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, VertexStream::NORMALS, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    // Tangent / binormal stream
    static D3D12_INPUT_ELEMENT_DESC elem_stream_tangents[] = {
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, VertexStream::TANGENT_BI, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, VertexStream::TANGENT_BI, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    for (uint32_t i = 0; i < num; i++) {
        auto& layout = ilStreamedDescriptions_[i];

        // Streams
        // Vert + uv
        // Color
        // Normal
        // Tan + Bi

        elem_pos.AlignedByteOffset = 0;
        elem_pos.SemanticIndex = 0;
        elem_uv3232.SemanticIndex = 0;
        layout.emplace_back(elem_pos);

        // uv
        if (i == shader::VertexFormat::P3F_T2S || i == shader::VertexFormat::P3F_T2S_C4B || i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F || i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10) {
            elem_uv1616.AlignedByteOffset = 12;
            layout.emplace_back(elem_uv1616);
        }

        // col
        if (i == shader::VertexFormat::P3F_T2S_C4B || i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F || i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10) {
            // seperate stream
            elem_col8888.AlignedByteOffset = 0;
            elem_col8888.InputSlot = 1;
            layout.emplace_back(elem_col8888);
        }

        // nor
        if (i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F) {
            elem_nor323232.AlignedByteOffset = 0;
            elem_nor323232.InputSlot = 2;
            layout.emplace_back(elem_nor323232); // 12 bytes
        }
        //  tan + bi
        if (i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F) {
            elem_tagent323232.InputSlot = 3;
            elem_tagent323232.AlignedByteOffset = 0;
            layout.emplace_back(elem_tagent323232); // 12 bytes

            elem_biNormal323232.InputSlot = 3;
            elem_biNormal323232.AlignedByteOffset = 12;
            layout.emplace_back(elem_biNormal323232); // 12 bytes
        }

        // 32 bit floats
        if (i == shader::VertexFormat::P3F_T2F_C4B) {
            elem_uv3232.AlignedByteOffset = 12;
            layout.emplace_back(elem_uv3232);

            elem_col8888.InputSlot = 1;
            elem_col8888.AlignedByteOffset = 0;
            layout.emplace_back(elem_col8888);
        }
        else if (i == shader::VertexFormat::P3F_T3F) {
            elem_t3f.AlignedByteOffset = 12;
            layout.emplace_back(elem_t3f);
        }

        if (i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10) {
            // 12 + 4 + 4
            elem_nor101010.InputSlot = 2;
            elem_nor101010.AlignedByteOffset = 0;
            layout.emplace_back(elem_nor101010);
        }
        if (i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10) {
            elem_tagent101010.InputSlot = 3;
            elem_tagent101010.AlignedByteOffset = 0;
            layout.emplace_back(elem_tagent101010);

            elem_biNormal101010.InputSlot = 3;
            elem_biNormal101010.AlignedByteOffset = 4;
            layout.emplace_back(elem_biNormal101010);
        }

        if (i == shader::VertexFormat::P3F_T4F_C4B_N3F) {
            // big man texcoords
            elem_uv3232.AlignedByteOffset = 12;
            layout.emplace_back(elem_uv3232);

            // two of them
            elem_uv3232.AlignedByteOffset = 20;
            elem_uv3232.SemanticIndex = 1;
            layout.emplace_back(elem_uv3232);
            elem_uv3232.SemanticIndex = 0;

            // byte offset is zero since diffrent stream.
            elem_col8888.AlignedByteOffset = 0;
            elem_col8888.InputSlot = 1;
            layout.emplace_back(elem_col8888);

            elem_nor323232.AlignedByteOffset = 0;
            elem_nor323232.InputSlot = 2;
            layout.emplace_back(elem_nor323232);
        }
    }

    // instanced streams
    D3D12_INPUT_ELEMENT_DESC elem_inst_vec4 = {
        "POSITION",
        1,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        VertexStream::INSTANCE,
        0,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
        1 // one pos per instance
    };

    D3D12_INPUT_ELEMENT_DESC elem_inst_col8888 = {
        "COLOR",
        1,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        VertexStream::INSTANCE,
        0,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
        1 // one color per instance
    };

    // 4x4
    for (uint32_t i = 0; i < 4; i++) {
        elem_inst_vec4.SemanticIndex = i + 1;
        elem_inst_vec4.AlignedByteOffset = i * sizeof(Vec4f);
        ilInstanced_.append(elem_inst_vec4);
    }

    // col.
    elem_inst_col8888.AlignedByteOffset = 4 * sizeof(Vec4f);
    ilInstanced_.append(elem_inst_col8888);

    // ----------------------------------------

    // HwSkin.
    static D3D12_INPUT_ELEMENT_DESC elem_stream_skin[] = {
        {"BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, VertexStream::HWSKIN, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"BLENDWEIGHT", 0, DXGI_FORMAT_R16G16B16A16_UNORM, VertexStream::HWSKIN, 4, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    // we need to support skinning on top of any camel.
    ilHwSkin_.append(elem_stream_skin[0]);
    ilHwSkin_.append(elem_stream_skin[1]);
}

bool XRender::initRenderBuffers(Vec2<uint32_t> res)
{
    return true;
}

bool XRender::resize(uint32_t width, uint32_t height)
{
    X_LOG1("Dx12", "Resizing display res to: x:%" PRIu32 " y:%", width, height);
    X_ASSERT_NOT_NULL(pSwapChain_);
    X_ASSERT_NOT_NULL(pDescriptorAllocator_);

    // wait till gpu idle.
    cmdListManager_.idleGPU();

    displayRes_.x = width;
    displayRes_.y = height;
    vars_.setRes(displayRes_);

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
        if (pDisplayPlanes_[i]) {
            pDisplayPlanes_[i]->destroy();
        }
    }

    pSwapChain_->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, displayRes_.x, displayRes_.y, SWAP_CHAIN_FORMAT, 0);

    for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
        ID3D12Resource* pDisplayPlane;
        HRESULT hr = pSwapChain_->GetBuffer(i, IID_PPV_ARGS(&pDisplayPlane));
        if (FAILED(hr)) {
            Error::Description Dsc;
            X_ERROR("Dx12", "Failed to get swap chain buffer: %s", Error::ToString(hr, Dsc));
            return false;
        }

        ColorBuffer& colBuf = pDisplayPlanes_[i]->getColorBuf();
        colBuf.createFromSwapChain(pDevice_, *pDescriptorAllocator_, pDisplayPlane);
    }

    // post a event.
    gEnv->pCore->GetCoreEventDispatcher()->OnCoreEvent(CoreEvent::RENDER_RES_CHANGED, displayRes_.x, displayRes_.y);
    return true;
}

void XRender::handleResolutionChange(void)
{
    if (currentNativeRes_ == targetNativeRes_) {
        return;
    }

    X_LOG1("Dx12", "Changing native res from: x:%" PRIu32 " y:%" PRIu32 " to: x:%" PRIu32 " y:%" PRIu32,
        currentNativeRes_.x, currentNativeRes_.y,
        targetNativeRes_.x, targetNativeRes_.y);

    currentNativeRes_ = targetNativeRes_;
    vars_.setNativeRes(currentNativeRes_);

    // wait till gpu idle.
    cmdListManager_.idleGPU();

    // re int buffers
    initRenderBuffers(targetNativeRes_);
}

void XRender::populateFeatureInfo(void)
{
    switch (featureLvl_) {
        case D3D_FEATURE_LEVEL_12_1:
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            features_.maxShaderModel = (featureLvl_ >= D3D_FEATURE_LEVEL_12_0) ? ShaderModel(5, 1) : ShaderModel(5, 0);
            features_.maxTextureWidth = features_.maxTextureHeight = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            features_.maxTextureDepth = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            features_.maxTextureCubeSize = D3D12_REQ_TEXTURECUBE_DIMENSION;
            features_.maxTextureArrayLength = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
            features_.maxVertexTextureUnits = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
            features_.maxPixelTextureUnits = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
            features_.maxGeometryTextureUnits = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
            features_.maxSimultaneousRts = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
            features_.maxSimultaneousUavs = D3D12_PS_CS_UAV_REGISTER_COUNT;
            features_.csSupport = true;
            break;

        default:
            X_ASSERT_NOT_IMPLEMENTED();
            break;
    }

    switch (featureLvl_) {
        case D3D_FEATURE_LEVEL_12_1:
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            features_.maxVertexStreams = D3D12_STANDARD_VERTEX_ELEMENT_COUNT;
            break;

        default:
            X_ASSERT_NOT_IMPLEMENTED();
            break;
    }
    switch (featureLvl_) {
        case D3D_FEATURE_LEVEL_12_1:
        case D3D_FEATURE_LEVEL_12_0:
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0:
            features_.maxTextureAnisotropy = D3D12_MAX_MAXANISOTROPY;
            break;

        default:
            X_ASSERT_NOT_IMPLEMENTED();
            break;
    }

    {
        D3D12_FEATURE_DATA_ARCHITECTURE archFeature;
        archFeature.NodeIndex = 0;
        auto hr = pDevice_->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &archFeature, sizeof(archFeature));
        if (FAILED(hr)) {
            Error::Description Dsc;
            X_ERROR("Dx12", "CheckFeatureSupport failed. err: %s", Error::ToString(hr, Dsc));
            return;
        }

        features_.isTbdr = archFeature.TileBasedRenderer ? true : false;
        features_.isUMA = archFeature.UMA ? true : false;
        features_.isUMACacheCoherent = archFeature.CacheCoherentUMA ? true : false;
    }

    {
        D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevels;
        core::zero_object(featureLevels);
        D3D_FEATURE_LEVEL FeatureLevelsList[] = {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_12_1,
        };
        featureLevels.NumFeatureLevels = X_ARRAY_SIZE(FeatureLevelsList);
        featureLevels.pFeatureLevelsRequested = FeatureLevelsList;

        auto hr = pDevice_->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevels, sizeof(featureLevels));
        if (FAILED(hr)) {
            Error::Description Dsc;
            X_ERROR("Dx12", "CheckFeatureSupport failed. err: %s", Error::ToString(hr, Dsc));
            return;
        }

        featureLvl_ = featureLevels.MaxSupportedFeatureLevel;
    }

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS options;
        auto hr = pDevice_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
        if (FAILED(hr)) {
            Error::Description Dsc;
            X_ERROR("Dx12", "CheckFeatureSupport failed. err: %s", Error::ToString(hr, Dsc));
            return;
        }
    }

    features_.hwInstancingSupport = true;
    features_.instanceIdSupport = true;
    features_.streamOutputSupport = true;
    features_.alphaToCoverageSupport = true;
    features_.primitiveRestartSupport = true;
    features_.multithreadRenderingSupport = true;
    features_.multithreadResCreatingSupport = true;
    features_.mrtIndependentBitDepthsSupport = true;
    features_.standardDerivativesSupport = true;
    features_.shaderTextureLodSupport = true;
    features_.logicOpSupport = true;
    features_.independentBlendSupport = true;
    features_.drawIndirectSupport = true;
    features_.noOverwriteSupport = true;
    features_.fullNpotTextureSupport = true;
    features_.renderToTextureArraySupport = true;
    features_.gsSupport = true;
    features_.hsSupport = true;
    features_.dsSupport = true;

    // R32f / R16f
    features_.packToRgbaRequired = false;
    // D24S8 / D16
    features_.depthTextureSupport = true;
    // o.o
    features_.fpColorSupport = true;

    features_.init = true;
}

bool XRender::deviceIsSupported(void) const
{
    X_ASSERT(features_.init, "Feature info must be init before checking if device meets requirements")();

    // check the device supports like max dimensions we support.
    // if this is ever a problem the engine just needs to support dropping higer dimensions at runtime.
    // that might get built in anyway as part of quality options.
    {
        if (features_.maxTextureWidth < texture::TEX_MAX_DIMENSIONS) {
            X_ERROR("Dx12", "Device does not support required texture width: %i supported: %i",
                texture::TEX_MAX_DIMENSIONS, features_.maxTextureWidth);
            return false;
        }
        if (features_.maxTextureHeight < texture::TEX_MAX_DIMENSIONS) {
            X_ERROR("Dx12", "Device does not support required texture height: %i supported: %i",
                texture::TEX_MAX_DIMENSIONS, features_.maxTextureHeight);
            return false;
        }
        if (features_.maxTextureDepth < texture::TEX_MAX_DEPTH) {
            X_ERROR("Dx12", "Device does not support required depth size: %i supported: %i",
                texture::TEX_MAX_DEPTH, features_.maxTextureDepth);
            return false;
        }
        if (features_.maxTextureCubeSize < texture::TEX_MAX_FACES) {
            X_ERROR("Dx12", "Device does not support required cube size: %i supported: %i",
                texture::TEX_MAX_FACES, features_.maxTextureCubeSize);
            return false;
        }
    }

    return true;
}

void XRender::Cmd_ListAdapters(core::IConsoleCmdArgs* pCmd)
{
    core::HumanSize::Str sizeStr;
    for (size_t i = 0; i < adapters_.size(); i++) {
        const auto& a = adapters_[i];

        X_LOG0("Dx12", "Name: \"%ls\" active: %s", a.deviceName.c_str(), i == adapterIdx_ ? "^81" : "^10");
        X_LOG0("Dx12", "DedicatedVideoMemory: ^6%s (%" PRIuS ")", core::HumanSize::toString(sizeStr, a.dedicatedvideoMemory), a.dedicatedvideoMemory);
        X_LOG0("Dx12", "DedicatedSystemMemory: ^6%s (%" PRIuS ")", core::HumanSize::toString(sizeStr, a.dedicatedSystemMemory), a.dedicatedSystemMemory);
        X_LOG0("Dx12", "SharedSystemMemory: ^6%s (%" PRIuS ")", core::HumanSize::toString(sizeStr, a.sharedSystemMemory), a.sharedSystemMemory);
        X_LOG0("Dx12", "Software: ^6%s", a.software ? "true" : "false");
    }
}

void XRender::Cmd_ListDeviceFeatures(core::IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    if (adapterIdx_ < 0) {
        return;
    }

    const auto& a = adapters_[adapterIdx_];

    X_LOG0("Dx12", "------ Device Info ------");
    X_LOG0("Dx12", "Name: \"%ls\"", a.deviceName.c_str());
    X_LOG0("Dx12", "MaxTextureWidth: ^6%" PRIu32, features_.maxTextureWidth);
    X_LOG0("Dx12", "MaxTextureHeight: ^6%" PRIu32, features_.maxTextureHeight);
    X_LOG0("Dx12", "MaxTextureDepth: ^6%" PRIu32, features_.maxTextureDepth);
    X_LOG0("Dx12", "MaxTextureCubeSize: ^6%" PRIu32, features_.maxTextureCubeSize);
    X_LOG0("Dx12", "MaxTextureCubeSize: ^6%" PRIu32, features_.maxTextureArrayLength);

    X_LOG0("Dx12", "MaxVertexTextureUnits: ^6%" PRIu8, features_.maxVertexTextureUnits);
    X_LOG0("Dx12", "MaxPixelTextureUnits: ^6%" PRIu8, features_.maxPixelTextureUnits);
    X_LOG0("Dx12", "MaxGeometryTextureUnits: ^6%" PRIu8, features_.maxGeometryTextureUnits);
    X_LOG0("Dx12", "MaxSimultaneousRts: ^6%" PRIu8, features_.maxSimultaneousRts);
    X_LOG0("Dx12", "MaxSimultaneousUavs: ^6%" PRIu8, features_.maxSimultaneousUavs);
    X_LOG0("Dx12", "MaxVertexStreams: ^6%" PRIu8, features_.maxVertexStreams);
    X_LOG0("Dx12", "MaxTextureAnisotropy: ^6%" PRIu8, features_.maxTextureAnisotropy);

    X_LOG0("Dx12", "TileBasedRenderer: ^6%s", features_.isTbdr ? "true" : "false");
    X_LOG0("Dx12", "UnifiedMemoryArchitecture: ^6%s", features_.isUMA ? "true" : "false");
    X_LOG0("Dx12", "UMACacheCoherent: ^6%s", features_.isUMACacheCoherent ? "true" : "false");

    X_LOG0("Dx12", "hwInstancingSupport: ^6%s", features_.hwInstancingSupport ? "true" : "false");
    X_LOG0("Dx12", "instanceIdSupport: ^6%s", features_.instanceIdSupport ? "true" : "false");
    X_LOG0("Dx12", "streamOutputSupport: ^6%s", features_.streamOutputSupport ? "true" : "false");
    X_LOG0("Dx12", "alphaToCoverageSupport: ^6%s", features_.alphaToCoverageSupport ? "true" : "false");
    X_LOG0("Dx12", "primitiveRestartSupport: ^6%s", features_.primitiveRestartSupport ? "true" : "false");
    X_LOG0("Dx12", "multithreadRenderingSupport: ^6%s", features_.multithreadRenderingSupport ? "true" : "false");
    X_LOG0("Dx12", "multithreadResCreatingSupport: ^6%s", features_.multithreadResCreatingSupport ? "true" : "false");
    X_LOG0("Dx12", "mrtIndependentBitDepthsSupport: ^6%s", features_.mrtIndependentBitDepthsSupport ? "true" : "false");
    X_LOG0("Dx12", "standardDerivativesSupport: ^6%s", features_.standardDerivativesSupport ? "true" : "false");
    X_LOG0("Dx12", "shaderTextureLodSupport: ^6%s", features_.shaderTextureLodSupport ? "true" : "false");
    X_LOG0("Dx12", "logicOpSupport: ^6%s", features_.logicOpSupport ? "true" : "false");
    X_LOG0("Dx12", "independentBlendSupport: ^6%s", features_.independentBlendSupport ? "true" : "false");
    X_LOG0("Dx12", "depthTextureSupport: ^6%s", features_.depthTextureSupport ? "true" : "false");
    X_LOG0("Dx12", "fpColorSupport: ^6%s", features_.fpColorSupport ? "true" : "false");
    X_LOG0("Dx12", "packToRgbaRequired: ^6%s", features_.packToRgbaRequired ? "true" : "false");
    X_LOG0("Dx12", "drawIndirectSupport: ^6%s", features_.drawIndirectSupport ? "true" : "false");
    X_LOG0("Dx12", "noOverwriteSupport: ^6%s", features_.noOverwriteSupport ? "true" : "false");
    X_LOG0("Dx12", "fullNpotTextureSupport: ^6%s", features_.fullNpotTextureSupport ? "true" : "false");
    X_LOG0("Dx12", "renderToTextureArraySupport: ^6%s", features_.renderToTextureArraySupport ? "true" : "false");
    X_LOG0("Dx12", "gsSupport: ^6%s", features_.gsSupport ? "true" : "false");
    X_LOG0("Dx12", "csSupport: ^6%s", features_.csSupport ? "true" : "false");
    X_LOG0("Dx12", "hsSupport: ^6%s", features_.hsSupport ? "true" : "false");
    X_LOG0("Dx12", "dsSupport: ^6%s", features_.dsSupport ? "true" : "false");

    X_LOG0("Dx12", "-------------------------");
}

X_NAMESPACE_END
