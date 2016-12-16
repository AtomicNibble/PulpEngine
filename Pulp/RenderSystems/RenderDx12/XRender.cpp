#include "stdafx.h"
#include "XRender.h"

#include "Texture\TextureManager.h"
#include "Texture\Texture.h"
#include "Texture\TextureUtil.h"
#include "Shader\ShaderManager.h"
#include "Shader\ShaderPermatation.h"
#include "Auxiliary\AuxRenderImp.h"

#include "Allocators\LinearAllocator.h"
#include "Buffers\BufferManager.h"
#include "CommandContex.h"
#include "PipelineState.h"

#include "Util\StateHelpers.h"

#include "CmdBucket.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(render)

namespace
{


	


} // namespace


XRender::XRender(core::MemoryArenaBase* arena) :
	arena_(arena),
	pDevice_(nullptr),
	pAdapter_(nullptr),
	pSwapChain_(nullptr),
	pShaderMan_(nullptr),
	pTextureMan_(nullptr),
	pAuxRender_(nullptr),
	pContextMan_(nullptr),
	cmdListManager_(arena),
	pBuffMan_(nullptr),
	dedicatedvideoMemory_(0),
	pDescriptorAllocator_(nullptr),
	pDescriptorAllocatorPool_(nullptr),
	pRootSigCache_(nullptr),
	pPSOCache_(nullptr),
//	presentRS_(arena),
	currentBufferIdx_(0),
	auxQues_ {arena, arena} 
{

}

XRender::~XRender()
{
	if (pTextureMan_) {
		X_DELETE(pTextureMan_, arena_);
	}
}

bool XRender::init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height)
{
	X_ASSERT(vars_.varsRegisterd(), "Vars must be init before calling XRender::Init()")(vars_.varsRegisterd());

	if (hWnd == static_cast<HWND>(0)) {
		X_ERROR("dx10", "target window is not valid");
		return false;
	}

	currentNativeRes_ = Vec2<uint32_t>(width, height);
	displayRes_ = Vec2<uint32_t>(width, height);

	HRESULT hr;

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
			X_ERROR("Dx12", "Failed to CreateDevice: 0x%x", hr);
			return false;
		}

		debugInterface->EnableDebugLayer();
	}


	// Obtain the DXGI factory
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));

	if (FAILED(hr)) {
		X_ERROR("Dx12", "Failed to create DXGI Factory");
		return false;
	}

	// Create the D3D graphics device
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

		for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(Idx, &adapter); ++Idx)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}

			featureLvl_ = D3D_FEATURE_LEVEL_11_0;

			hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice_));
			if (SUCCEEDED(hr))
			{
				X_LOG0("Dx12", "D3D12-capable hardware found: \"%ls\" (%u MB)", desc.Description, desc.DedicatedVideoMemory >> 20);

				deviceName_.set(desc.Description);
				dedicatedvideoMemory_ = desc.DedicatedVideoMemory;

				{
					Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter3;
					adapter.As(&adapter3);

					pAdapter_ = adapter3.Detach();
				}
				break;
			}
			else
			{
				X_WARNING("Dx12", "Failed to create device for adpater index %" PRIu32 " res: %" PRIu32, Idx, hr);
			}
		}
	}

	if (!pDevice_) {
		X_ERROR("Dx12", "Failed to CreateDevice: %" PRIu32, hr);
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
		if (SUCCEEDED(pDevice_->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
		{
			D3D12_MESSAGE_SEVERITY Severities[] =
			{
			//	D3D12_MESSAGE_SEVERITY_CORRUPTION,
			//	D3D12_MESSAGE_SEVERITY_ERROR,
			//	D3D12_MESSAGE_SEVERITY_WARNING,
				D3D12_MESSAGE_SEVERITY_INFO,
			//	D3D12_MESSAGE_SEVERITY_MESSAGE
			};

			D3D12_INFO_QUEUE_FILTER NewFilter;
			core::zero_object(NewFilter);

			NewFilter.DenyList.NumSeverities = _countof(Severities);
			NewFilter.DenyList.pSeverityList = Severities;

			hr = pInfoQueue->PushStorageFilter(&NewFilter);
			if (FAILED(hr)) {
				X_ERROR("Dx12", "failed to push storage filter: %" PRId32, hr);
			}

			pInfoQueue->Release();
		}
		else
		{
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

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
	hr = dxgiFactory->CreateSwapChainForHwnd(cmdListManager_.getCommandQueue(), hWnd,
		&swapChainDesc, nullptr, nullptr, &swapChain);
	if (FAILED(hr)) {
		X_ERROR("Dx12", "failed to create swap chain: %" PRId32, hr);
		return false;
	}

	{
		Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
		swapChain.As(&swapChain3);

		pSwapChain_ = swapChain3.Detach();
	}


	for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> displayPlane;
		hr = pSwapChain_->GetBuffer(i, IID_PPV_ARGS(&displayPlane));
		if (FAILED(hr)) {
			X_ERROR("Dx12", "failed to get swap chain buffer: %" PRId32, hr);
			return false;
		}

		displayPlane_[i].createFromSwapChain(pDevice_, descriptorAllocator, displayPlane.Detach());
		displayPlane_[i].setClearColor(vars_.getClearCol());
	}


	// Samplers.
#if 0
	samplerLinearWrapDesc_.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerLinearWrap_.create(pDevice_, descriptorAllocator, samplerLinearWrapDesc_);

	samplerAnisoWrapDesc_.MaxAnisotropy = 8;
	samplerAnisoWrap_.create(pDevice_, descriptorAllocator, samplerAnisoWrapDesc_);

	samplerShadowDesc_.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerShadowDesc_.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	samplerShadowDesc_.setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerShadow_.create(pDevice_, descriptorAllocator, samplerShadowDesc_);

	samplerLinearClampDesc_.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerLinearClampDesc_.setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerLinearClamp_.create(pDevice_, descriptorAllocator, samplerLinearClampDesc_);

	samplerVolumeWrapDesc_.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerVolumeWrap_.create(pDevice_, descriptorAllocator, samplerVolumeWrapDesc_);

	samplerPointClampDesc_.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerPointClampDesc_.setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerPointClamp_.create(pDevice_, descriptorAllocator, samplerPointClampDesc_);

	samplerPointBorderDesc_.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerPointBorderDesc_.setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
	samplerPointBorderDesc_.setBorderColor(Colorf(0.0f, 0.0f, 0.0f, 0.0f));
	samplerPointBorder_.create(pDevice_, descriptorAllocator, samplerPointBorderDesc_);

	samplerLinearBorderDesc_.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerLinearBorderDesc_.setTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
	samplerLinearBorderDesc_.setBorderColor(Colorf(0.0f, 0.0f, 0.0f, 0.0f));
	samplerLinearBorder_.create(pDevice_, descriptorAllocator, samplerLinearBorderDesc_);
#endif

	// RootSig
#if 0
	presentRS_.reset(4, 2);
	presentRS_.getParamRef(0).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
	presentRS_.getParamRef(1).initAsConstants(0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.getParamRef(2).initAsSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.getParamRef(3).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
	presentRS_.initStaticSampler(0, samplerLinearClampDesc_, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.initStaticSampler(1, samplerPointClampDesc_, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.finalize(*pRootSigCache_);
#endif

	pShaderMan_ = X_NEW(shader::XShaderManager, arena_, "ShaderMan")(arena_);
	if (!pShaderMan_->init()) {
		X_ERROR("Render", "failed to init shader system");
		return false;
	}

	pTextureMan_ = X_NEW(texture::TextureManager, arena_, "TexMan")(arena_, pDevice_, *pContextMan_, descriptorAllocator);
	if (!pTextureMan_->init()) {
		X_ERROR("Render", "failed to init texture system");
		return false;
	}

	pAuxRender_ = X_NEW(RenderAuxImp, arena_, "AuxRenderer")(arena_);
	if (!pAuxRender_->init(pDevice_, *pContextMan_, descriptorAllocator)) {
		X_ERROR("Render", "failed to init aux render system");
		return false;
	}

	initILDescriptions();

	return true;
}

void XRender::shutDown(void)
{
//	presentRS_.free();

	if (pBuffMan_) {
		pBuffMan_->shutDown();
		X_DELETE_AND_NULL(pBuffMan_, arena_);
	}

	if (pTextureMan_) {
		pTextureMan_->shutDown();
		X_DELETE_AND_NULL(pTextureMan_, arena_);
	}

	if (pAuxRender_) {
		pAuxRender_->shutDown();
		X_DELETE_AND_NULL(pAuxRender_, arena_);
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

	cmdListManager_.shutdown();

	freeSwapChainResources();

	if (pDescriptorAllocator_) {
		pDescriptorAllocator_->destoryAllHeaps();
		X_DELETE_AND_NULL(pDescriptorAllocator_, arena_);
	}
	if (pDescriptorAllocatorPool_) {
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
		if (SUCCEEDED(pDevice_->QueryInterface(&pDebugInterface)))
		{
			pDebugInterface->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
			pDebugInterface->Release();
		}
		else
		{
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

void XRender::registerVars(void)
{
	vars_.registerVars();


	vars_.setNativeRes(currentNativeRes_);
	vars_.setRes(displayRes_);

	if (pTextureMan_) {
		pTextureMan_->registerVars();
	}
}

void XRender::registerCmds(void)
{

	ADD_COMMAND_MEMBER("r_list_device_features", this, XRender, &XRender::Cmd_ListDeviceFeatures,
		core::VarFlag::SYSTEM, "List the gpu devices features");

	if (pTextureMan_) {
		pTextureMan_->registerCmds();
	}
}

void XRender::renderBegin(void)
{
	D3D12_CPU_DESCRIPTOR_HANDLE RTVs[] = {
		displayPlane_[currentBufferIdx_].getRTV()
	};

	displayPlane_[currentBufferIdx_].setClearColor(vars_.getClearCol());

	GraphicsContext* pContext = pContextMan_->allocateGraphicsContext();

	pContext->transitionResource(displayPlane_[currentBufferIdx_], D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pContext->clearColor(displayPlane_[currentBufferIdx_]);
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


	pContext->transitionResource(displayPlane_[currentBufferIdx_], D3D12_RESOURCE_STATE_PRESENT);
	pContext->finishAndFree(true);


	HRESULT hr = pSwapChain_->Present(0, 0);
	if (FAILED(hr)) {
		X_ERROR("Dx12", "Present failed. err: %" PRIu32, hr);
	}

	currentBufferIdx_ = (currentBufferIdx_ + 1) % SWAP_CHAIN_BUFFER_COUNT;

	handleResolutionChange();
}


void XRender::submitCommandPackets(CommandBucket<uint32_t>& cmdBucket)
{
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
		return;
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
		const ColorBuffer& rtv = *static_cast<ColorBuffer*>(rtvs[i]);
		RTVs[i] = rtv.getRTV();
		RTVFormats[i] = rtv.getFormat();
	}

	// we should validate that RTVFormats matches the pass state.

	GraphicsContext* pContext = pContextMan_->allocateGraphicsContext();
	GraphicsContext& context = *pContext;

	for (size_t i = 0; i < rtvs.size(); i++) {
		ColorBuffer& rtv = *static_cast<ColorBuffer*>(rtvs[i]);
		context.transitionResource(rtv, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	context.setRenderTargets(numRtvs, RTVs);
	context.setViewportAndScissor(viewport);

	State curState;

	for (size_t i = 0; i < sortedIdx.size(); ++i)
	{
		CommandPacket::Packet pPacket = packets[sortedIdx[i]];

		do
		{
			const CommandPacket::Command::Enum cmdType = CommandPacket::loadCommandType(pPacket);
			const void* pCmd = CommandPacket::loadCommand(pPacket);

			switch (cmdType)
			{
				case Commands::Command::DRAW:
				{
					const Commands::Draw* pDraw = reinterpret_cast<const Commands::Draw*>(pCmd);
					
					ApplyState(context, curState, pDraw->stateHandle, pDraw->vertexBuffers,
						pDraw->resourceState, CommandPacket::getAuxiliaryMemory(pDraw));

					context.draw(pDraw->vertexCount, pDraw->startVertex);
					break;
				}
				case Commands::Command::DRAW_INDEXED:
				{
#if 0
					const Commands::DrawIndexed& draw = *reinterpret_cast<const Commands::DrawIndexed*>(pCmd);

					ApplyState(context, curState, draw.stateHandle, draw.vertexBuffers);

					// don't bother looking up ib if same handle.
					if (curState.indexBuffer != draw.indexBuffer) {
						curState.indexBuffer = draw.indexBuffer;
						const auto pIBuf = pBuffMan_->IBFromHandle(draw.indexBuffer);
						context.setIndexBuffer(pIBuf->getBuf().indexBufferView());
					}

					context.drawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
#endif
					break;
				}
				case Commands::Command::COPY_CONST_BUF_DATA:
					break;
				case Commands::Command::COPY_INDEXES_BUF_DATA:
				{
					const Commands::CopyIndexBufferData& updateIB = *reinterpret_cast<const Commands::CopyIndexBufferData*>(pCmd);
					auto pIBuf = pBuffMan_->IBFromHandle(updateIB.indexBuffer);

					X_ASSERT(pIBuf->getUsage() != BufUsage::IMMUTABLE, "Can't update a IMMUTABLE buffer")(pIBuf->getUsage());

					context.writeBuffer(pIBuf->getBuf(), 0, updateIB.pData, updateIB.size);
				}
				break;
				case Commands::Command::COPY_VERTEX_BUF_DATA:
				{
					const Commands::CopyVertexBufferData& updateVB = *reinterpret_cast<const Commands::CopyVertexBufferData*>(pCmd);
					auto pVBuf = pBuffMan_->IBFromHandle(updateVB.vertexBuffer);

					X_ASSERT(pVBuf->getUsage() != BufUsage::IMMUTABLE, "Can't update a IMMUTABLE buffer")(pVBuf->getUsage());

					context.writeBuffer(pVBuf->getBuf(), 0, updateVB.pData, updateVB.size);
				}
				break;

				case Commands::Command::UPDATE_TEXTUTE_BUF_DATA:
				{
					const Commands::CopyTextureBufferData& updateTex = *reinterpret_cast<const Commands::CopyTextureBufferData*>(pCmd);
					
					pTextureMan_->updateTexture(context, updateTex.textureId, static_cast<const uint8_t*>(updateTex.pData), updateTex.size);
				}
				break;

				case Commands::Command::UPDATE_TEXTUTE_SUB_BUF_DATA:
				{
					const Commands::CopyTextureSubRegionBufferData& updateSubTex = *reinterpret_cast<const Commands::CopyTextureSubRegionBufferData*>(pCmd);
				
					pTextureMan_->updateTexture(context, updateSubTex.textureId, static_cast<const uint8_t*>(updateSubTex.pData), updateSubTex.size);
				}
				break;



				default:
#if X_DEBUG
					X_ASSERT_NOT_IMPLEMENTED();
					break;
#else
				X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
			}


			pPacket = CommandPacket::loadNextCommandPacket(pPacket);
		}
		while (pPacket != nullptr);
	}

	// for now wait.
	pContext->finishAndFree(true);

}

X_INLINE void XRender::CreateVBView(GraphicsContext& context, const VertexHandleArr& vertexBuffers,
	D3D12_VERTEX_BUFFER_VIEW viewsOut[VertexStream::ENUM_COUNT], uint32_t& numVertexStreams)
{
	numVertexStreams = 0;

	for (uint32_t i = 0; i < VertexStream::ENUM_COUNT; i++)
	{
		if (vertexBuffers[i]) 
		{
			const auto pVertBuf = pBuffMan_->VBFromHandle(vertexBuffers[i]);
			auto& buffer = pVertBuf->getBuf();

			// transition if needed.
			context.transitionResource(buffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

			viewsOut[i] = buffer.vertexBufferView();

			// it's total that need to be passed to device, which inclde 
			// any null ones in between.
			numVertexStreams = i + 1;
		}
		else
		{
			viewsOut[i].BufferLocation = 0;
			viewsOut[i].SizeInBytes = 0;
			viewsOut[i].StrideInBytes = 0;
		}
	}
}


void XRender::ApplyState(GraphicsContext& context, State& curState, const StateHandle handle,
	const VertexHandleArr& vertexBuffers, const Commands::ResourceStateBase& resourceState, const char* pStateData)
{
	if (curState.handle != handle) // if the handle is the same, everything is the same.
	{
		// the render system should not have to check ever state is valid, the 3dengine should check at creation time.
		// so it's a one off cost not a cost we pay for every fucking state change.
		X_ASSERT(handle != INVALID_STATE_HANLDE, "Don't pass me invalid states you cunt")(handle, INVALID_STATE_HANLDE);

		curState.handle = handle;

		// now we check what parts of the state are diffrent.
		const DeviceState& newState = *reinterpret_cast<const DeviceState*>(handle);

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
	if (std::memcmp(curState.vertexBuffers.data(), vertexBuffers.data(), sizeof(vertexBuffers)) != 0)
	{
		curState.vertexBuffers = vertexBuffers;

		uint32_t numVertexStreams = 0;
		D3D12_VERTEX_BUFFER_VIEW vertexViews[VertexStream::ENUM_COUNT] = { 0 };
		CreateVBView(context, vertexBuffers, vertexViews, numVertexStreams);

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
	if (curState.variableStateSize != resourceState.getStateSize() &&
		std::memcmp(curState.variableState, pStateData, resourceState.getStateSize()) != 0)
	{
		std::memcpy(curState.variableState, pStateData, resourceState.getStateSize());
		curState.variableStateSize = resourceState.getStateSize();

		if (resourceState.anySet())
		{
			if (resourceState.getNumTextStates())
			{
				D3D12_CPU_DESCRIPTOR_HANDLE textureSRVS[render::TextureSlot::ENUM_COUNT] = {};
				const TextureState* pTexStates = resourceState.getTexStates(pStateData);

				for (int32_t t = 0; t < resourceState.getNumTextStates(); t++)
				{
					const auto& texState = pTexStates[t];
					auto* pTex = pTextureMan_->getByID(texState.textureId);
					
					auto& gpuResource = pTex->getGpuResource();

					context.transitionResource(gpuResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

					textureSRVS[texState.slot] = pTex->getSRV();

					// we need to handle setting samplers...
					// we should first get the sampler from cache.
					// i currnetly have sampler states bound to textures.
					// which means each texture would need a seperate sampler deifned in the shader.
					// how do we know which index the sampler for this texture is?
					// should we just assume they share same register index?
					// for now i'll force them in same slot.
					// 

				}

				// for now assume all slots are linera and no gaps.
				const auto count = resourceState.getNumTextStates();
				context.setDynamicDescriptors(1, 0, count, textureSRVS);
			}

			// this may be zero even if we have samplers, if they are all static.
			if (resourceState.getNumSamplers())
			{
				D3D12_CPU_DESCRIPTOR_HANDLE samplerSRVS[render::TextureSlot::ENUM_COUNT] = {};
				const SamplerState* pSamplers = resourceState.getSamplers(pStateData);

				// potentially I want todo redundancy here.
				// I may also want to require the 3dengine to make sampler states in advance so not done here.
				// and we just pass sampler id's that map to samplerDescriptors.
				for (int32_t t = 0; t < resourceState.getNumTextStates(); t++)
				{
					const auto& sampler = pSamplers[t];
					auto samplerDescriptor = pSamplerCache_->createDescriptor(*pDescriptorAllocator_, sampler);

					samplerSRVS[t] = samplerDescriptor.getCpuDescriptorHandle();
				}

				const auto count = resourceState.getNumSamplers();
				context.setDynamicSamplerDescriptors(2, 0, count, samplerSRVS);
			}

			if (resourceState.getNumCBs())
			{
				const ConstantBufferHandle* pCBVs = resourceState.getCBs(pStateData);

				for (int32_t t = 0; t < resourceState.getNumCBs(); t++)
				{
					ConstantBufferHandle cbh = pCBVs[t];

					if (cbh == curState.constBuffers[t]) {
						continue;
					}

					// this needs clearing when rootsig changes.
					curState.constBuffers[t] = cbh;

					// we need to know the index this cb should be bound to.
					auto* pCbuf = pBuffMan_->CBFromHandle(cbh);
					auto& buf = pCbuf->getBuf();

					context.transitionResource(buf, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
					context.setConstantBuffer(pCbuf->getRootIdx(), buf.getGpuVirtualAddress());
				}
			}
		}
		else
		{
			// clear?
			// or just leave them bound.
		}

	}

}


IRenderAux* XRender::getAuxRender(AuxRenderer::Enum user)
{
	return &auxQues_[user];
}


Vec2<uint32_t> XRender::getDisplayRes(void) const
{
	return displayRes_;
}

IRenderTarget* XRender::createRenderTarget()
{
	return nullptr;
}


void XRender::destoryRenderTarget(IRenderTarget* pRT)
{
	X_UNUSED(pRT);
}

IRenderTarget* XRender::getCurBackBuffer(uint32_t* pIdx)
{
	if (pIdx) {
		*pIdx = currentBufferIdx_;
	}

	return &displayPlane_[currentBufferIdx_];
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
	pBuffMan_->freeVB(handle);
}

void XRender::destoryIndexBuffer(IndexBufferHandle handle)
{
	pBuffMan_->freeIB(handle);
}

void XRender::getVertexBufferSize(VertexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize)
{
	pBuffMan_->getBufSize(handle, pOriginal, pDeviceSize);
}

void XRender::getIndexBufferSize(IndexBufferHandle handle, int32_t* pOriginal, int32_t* pDeviceSize)
{
	pBuffMan_->getBufSize(handle, pOriginal, pDeviceSize);
}

// cb's
ConstantBufferHandle XRender::createConstBuffer(shader::XCBuffer& cb, BufUsage::Enum usage)
{
	// so we need to create a allocation and maybe also store some info about the cbuf?
	// potentially we just store a copy of the cbuffer :/
	// but then we start to build a web of dependancies.
	// since the cbuffer is from the shader.
	// so for now we copy PASTA!
	const auto& data = cb.getCpuData();

	X_ASSERT(cb.getRootIdx() >= 0, "CB has invalid root idx")(cb.getName().c_str(), static_cast<int32_t>(cb.getRootIdx()));

	ConstantBufferHandle handle = pBuffMan_->createConstBuf(cb.getBindSize(), cb.getRootIdx(), data.data(), usage, render::CpuAccess::WRITE);

	return handle; 
}

void XRender::destoryConstBuffer(ConstantBufferHandle handle)
{
	pBuffMan_->freeCB(handle);
}


::texture::ITexture* XRender::getTexture(const char* pName, texture::TextureFlags flags)
{
	texture::Texture* pText = pTextureMan_->forName(pName, flags);

	return pText;
}

::texture::ITexture* XRender::createTexture(const char* pNickName, Vec2i dim, texture::Texturefmt::Enum fmt, const uint8_t* pInitialData)
{
	texture::Texture* pText = pTextureMan_->createTexture(pNickName, dim, fmt, pInitialData);

	return pText;
}



shader::IShaderSource* XRender::getShaderSource(const char* pSourceName)
{
	return pShaderMan_->sourceforName(pSourceName);
}

shader::IHWShader* XRender::createHWShader(shader::ShaderType::Enum type, const core::string& entry, shader::IShaderSource* pSourceFile)
{
	return pShaderMan_->createHWShader(type, entry, pSourceFile);
}

shader::IShaderPermatation* XRender::createPermatation(shader::IHWShader* pVertex, shader::IHWShader* pPixel)
{
	return nullptr;
}

shader::IShaderPermatation* XRender::createPermatation(const shader::ShaderStagesArr& stages)
{
	return pShaderMan_->createPermatation(stages);
}


void XRender::releaseShaderPermatation(shader::IShaderPermatation* pPerm)
{
	pShaderMan_->releaseShaderPermatation(pPerm);
}

void XRender::releaseTexture(texture::ITexture* pTex)
{
	pTextureMan_->releaseTexture(pTex);
}


PassStateHandle XRender::createPassState(const RenderTargetFmtsArr& rtfs)
{
	PassState* pPass = X_NEW(PassState, arena_, "PassState");
	pPass->rtfs = rtfs;

	return reinterpret_cast<PassStateHandle>(pPass);
}

void XRender::destoryPassState(PassStateHandle passHandle)
{
	PassState* pPassState = reinterpret_cast<PassState*>(passHandle);

	X_DELETE(pPassState, arena_);
}

D3D12_SHADER_VISIBILITY stageFlagsToStageVisibility(shader::ShaderStageFlags stageFlags)
{
	const auto flagsInt = stageFlags.ToInt();

	switch (flagsInt)
	{
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

StateHandle XRender::createState(PassStateHandle passHandle, const shader::IShaderPermatation* pPerm,
	const StateDesc& desc, const TextureState* pTextStates, size_t numStates)
{
	const PassState* pPassState = reinterpret_cast<const PassState*>(passHandle);

	// permatations are currently compiled on creation.
	const shader::ShaderPermatation& perm = *static_cast<const shader::ShaderPermatation*>(pPerm);

	if (!perm.isCompiled()) {
		return INVALID_STATE_HANLDE;
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

	DeviceState* pState = X_NEW(DeviceState, arena_, "DeviceState")(arena_);
	pState->pPso = nullptr;
	pState->topo = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;


	// we need a root sig to compile this PSO with.
	// but it don't have to be the rootSig we render with.
	RootSignature& rootSig = pState->rootSig;

#if 1

	// we are going to create a rootsig based on the shader.
	// we should try be smart about it like using static samplers if possible.
	// all cbv's are gonna go in root sig.
	// we need to know all the srv's we need also
	// we may want to support textures in none pixel stage also.

	// this is a list of cbuffers used by all stages and also defining what stages they need to be visible.
	const auto& cbufLinks = perm.getCbufferLinks();

	size_t numParams = 0;

	numParams += cbufLinks.size(); // one for each cbuffer.

	if (perm.isStageSet(shader::ShaderType::Pixel))
	{
		auto* pPixelShader = perm.getStage(shader::ShaderType::Pixel);

		if (pPixelShader->getNumTextures() > 0) {
			numParams++; // a descriptor range
		}
		if (pPixelShader->getNumSamplers() > 0) {
			numParams++; // a descriptor range
		}
	}


	rootSig.reset(numParams, 0);

	uint32_t currentParamIdx = 0;
	for (size_t i=0; i<cbufLinks.size(); i++)
	{
		const auto& cbLink = cbufLinks[i];
		auto vis = stageFlagsToStageVisibility(cbLink.stages);
		auto& cb = *cbLink.pCBufer;
		auto bindPoint = cb.getBindPoint();

		// tell the render system what root index this needs to be bound at.
		cb.setRootIdx(currentParamIdx);
		rootSig.getParamRef(currentParamIdx++).initAsCBV(bindPoint, vis);
	}

	if (perm.isStageSet(shader::ShaderType::Pixel))
	{
		auto* pPixelShader = perm.getStage(shader::ShaderType::Pixel);

		auto numTextures = pPixelShader->getNumTextures();
		auto numSamplers = pPixelShader->getNumSamplers();

		if (numTextures > 0) {
			rootSig.getParamRef(currentParamIdx++).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				0, numTextures, D3D12_SHADER_VISIBILITY_PIXEL);
		}
		if (numSamplers > 0) {
			rootSig.getParamRef(currentParamIdx++).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
				0, numSamplers, D3D12_SHADER_VISIBILITY_PIXEL);
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
		X_DELETE(pState, arena_);
		return INVALID_STATE_HANLDE;
	}

	// we need to create a PSO.
	GraphicsPSO pso;

	DXGI_FORMAT RTVFormats[MAX_RENDER_TARGETS];
	core::zero_object(RTVFormats);

	for (size_t i = 0; i < pPassState->rtfs.size(); i++) {
		RTVFormats[i] = texture::Util::DXGIFormatFromTexFmt(pPassState->rtfs[i]);
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
	pso.setRenderTargetFormats(static_cast<uint32_t>(pPassState->rtfs.size()), RTVFormats, DXGI_FORMAT_UNKNOWN, 1, 0);
	pso.setPrimitiveTopologyType(topoTypeFromDesc(desc));

	const auto& inputDesc = ilDescriptions_[desc.vertexFmt];
	pso.setInputLayout(inputDesc.size(), inputDesc.ptr());

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
		X_DELETE(pState, arena_);
		return INVALID_STATE_HANLDE;
	}


	pState->pPso = pso.getPipelineStateObject();
	pState->topo = topoFromDesc(desc);
	pState->texStates.resize(numStates);
	std::memcpy(pState->texStates.data(), pTextStates, sizeof(TextureState) * numStates);
	return reinterpret_cast<StateHandle>(pState);
}

void XRender::destoryState(StateHandle handle)
{
	// this implies you are not checking they are valid when creating, which you should!
	X_ASSERT(handle != INVALID_STATE_HANLDE, "Destoring invalid states is not allowed")(handle, INVALID_STATE_HANLDE);

	DeviceState* pState = reinterpret_cast<DeviceState*>(handle);

	X_DELETE(pState, arena_);
}


bool XRender::freeSwapChainResources(void)
{
	for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
		displayPlane_[i].destroy();
	}

	return true;
}


void XRender::initILDescriptions(void)
{
	const uint32_t num = shader::VertexFormat::ENUM_COUNT;

	D3D12_INPUT_ELEMENT_DESC elem_pos = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	D3D12_INPUT_ELEMENT_DESC elem_nor101010 = { "NORMAL", 0, DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	//6	D3D11_INPUT_ELEMENT_DESC elem_nor8888 = { "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	D3D12_INPUT_ELEMENT_DESC elem_nor323232 = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	D3D12_INPUT_ELEMENT_DESC elem_col8888 = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	D3D12_INPUT_ELEMENT_DESC elem_uv3232 = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	D3D12_INPUT_ELEMENT_DESC elem_uv1616 = { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	//	D3D11_INPUT_ELEMENT_DESC elem_uv32323232 = { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_ELEMENT_DESC elem_t3f = { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_ELEMENT_DESC elem_tagent101010 = { "TANGENT", 0, DXGI_FORMAT_R10G10B10A2_TYPELESS, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	D3D12_INPUT_ELEMENT_DESC elem_tagent323232 = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	D3D12_INPUT_ELEMENT_DESC elem_biNormal101010 = { "BINORMAL", 0, DXGI_FORMAT_R10G10B10A2_TYPELESS, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	D3D12_INPUT_ELEMENT_DESC elem_biNormal323232 = { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };


	for (uint32_t i = 0; i < num; i++)
	{
		auto& layout = ilDescriptions_[i];

		// for now all positions are just 32bit floats baby!
		elem_pos.AlignedByteOffset = 0;
		elem_pos.SemanticIndex = 0;
		elem_uv3232.SemanticIndex = 0;
		layout.emplace_back(elem_pos);

		if (i == shader::VertexFormat::P3F_T2S || i == shader::VertexFormat::P3F_T2S_C4B ||
			i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F ||
			i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_uv1616.AlignedByteOffset = 12;
			layout.emplace_back(elem_uv1616);
		}
		if (i == shader::VertexFormat::P3F_T2S_C4B ||
			i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F ||
			i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_col8888.AlignedByteOffset = 12 + 4;
			layout.emplace_back(elem_col8888);
		}

		if (i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F)
		{
			elem_nor323232.AlignedByteOffset = 12 + 4 + 4;
			layout.emplace_back(elem_nor323232); // 12 bytes
		}
		if (i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F)
		{
			elem_tagent323232.AlignedByteOffset = 12 + 4 + 4 + 12;
			layout.emplace_back(elem_tagent323232); // 12 bytes

			elem_biNormal323232.AlignedByteOffset = 12 + 4 + 4 + 12 + 12;
			layout.emplace_back(elem_biNormal323232); // 12 bytes
		}

		if (i == shader::VertexFormat::P3F_T2F_C4B)
		{
			elem_uv3232.AlignedByteOffset = 12;
			layout.emplace_back(elem_uv3232);

			elem_col8888.AlignedByteOffset = 20;
			layout.emplace_back(elem_col8888);

		}
		else if (i == shader::VertexFormat::P3F_T3F)
		{
			elem_t3f.AlignedByteOffset = 12;
			layout.emplace_back(elem_t3f);
		}


		if (i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			// 12 + 4 + 4
			elem_nor101010.AlignedByteOffset = 20;
			layout.emplace_back(elem_nor101010);
		}
		if (i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_tagent101010.AlignedByteOffset = 24;
			layout.emplace_back(elem_tagent101010);
			elem_biNormal101010.AlignedByteOffset = 28;
			layout.emplace_back(elem_biNormal101010);
		}

		if (i == shader::VertexFormat::P3F_T4F_C4B_N3F)
		{
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
	static D3D11_INPUT_ELEMENT_DESC elem_stream_color[] =
	{
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, VertexStream::COLOR, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// normals stream
	static D3D11_INPUT_ELEMENT_DESC elem_stream_normals[] =
	{
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, VertexStream::NORMALS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Tangent / binormal stream
	static D3D11_INPUT_ELEMENT_DESC elem_stream_tangents[] =
	{
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, VertexStream::TANGENT_BI, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, VertexStream::TANGENT_BI, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};


	for (uint32_t i = 0; i < num; i++)
	{
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
		if (i == shader::VertexFormat::P3F_T2S || i == shader::VertexFormat::P3F_T2S_C4B ||
			i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F ||
			i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_uv1616.AlignedByteOffset = 12;
			layout.emplace_back(elem_uv1616);
		}

		// col
		if (i == shader::VertexFormat::P3F_T2S_C4B ||
			i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F ||
			i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			// seperate stream
			elem_col8888.AlignedByteOffset = 0;
			elem_col8888.InputSlot = 1;
			layout.emplace_back(elem_col8888);
		}

		// nor
		if (i == shader::VertexFormat::P3F_T2S_C4B_N3F || i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F)
		{
			elem_nor323232.AlignedByteOffset = 0;
			elem_nor323232.InputSlot = 2;
			layout.emplace_back(elem_nor323232); // 12 bytes
		}
		//  tan + bi
		if (i == shader::VertexFormat::P3F_T2S_C4B_N3F_TB3F)
		{
			elem_tagent323232.InputSlot = 3;
			elem_tagent323232.AlignedByteOffset = 0;
			layout.emplace_back(elem_tagent323232); // 12 bytes

			elem_biNormal323232.InputSlot = 3;
			elem_biNormal323232.AlignedByteOffset = 12;
			layout.emplace_back(elem_biNormal323232); // 12 bytes
		}

		// 32 bit floats
		if (i == shader::VertexFormat::P3F_T2F_C4B)
		{
			elem_uv3232.AlignedByteOffset = 12;
			layout.emplace_back(elem_uv3232);

			elem_col8888.InputSlot = 1;
			elem_col8888.AlignedByteOffset = 0;
			layout.emplace_back(elem_col8888);
		}
		else if (i == shader::VertexFormat::P3F_T3F)
		{
			elem_t3f.AlignedByteOffset = 12;
			layout.emplace_back(elem_t3f);
		}


		if (i == shader::VertexFormat::P3F_T2S_C4B_N10 || i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			// 12 + 4 + 4
			elem_nor101010.InputSlot = 2;
			elem_nor101010.AlignedByteOffset = 0;
			layout.emplace_back(elem_nor101010);
		}
		if (i == shader::VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_tagent101010.InputSlot = 3;
			elem_tagent101010.AlignedByteOffset = 0;
			layout.emplace_back(elem_tagent101010);

			elem_biNormal101010.InputSlot = 3;
			elem_biNormal101010.AlignedByteOffset = 4;
			layout.emplace_back(elem_biNormal101010);
		}

		if (i == shader::VertexFormat::P3F_T4F_C4B_N3F)
		{
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


}

bool XRender::initRenderBuffers(Vec2<uint32_t> res)
{

	return true;
}


bool XRender::resize(uint32_t width, uint32_t height)
{
	X_LOG1("Dx12", "Resizing display res to: x:%" PRIu32 " y:%" , width, height);
	X_ASSERT_NOT_NULL(pSwapChain_);
	X_ASSERT_NOT_NULL(pDescriptorAllocator_);

	// wait till gpu idle.
	cmdListManager_.idleGPU();

	displayRes_.x = width;
	displayRes_.y = height;
	vars_.setRes(displayRes_);

	freeSwapChainResources();

	pSwapChain_->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, displayRes_.x, displayRes_.y, SWAP_CHAIN_FORMAT, 0);

	for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		ID3D12Resource* pDisplayPlane;
		HRESULT hr = pSwapChain_->GetBuffer(i, IID_PPV_ARGS(&pDisplayPlane));
		if (FAILED(hr)) {
			X_ERROR("Dx12", "failed to get swap chain buffer: %" PRId32, hr);
			return false;
		}

		displayPlane_[i].createFromSwapChain(pDevice_, *pDescriptorAllocator_, pDisplayPlane);
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
	switch (featureLvl_)
	{
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

	switch (featureLvl_)
	{
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
	switch (featureLvl_)
	{
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
		pDevice_->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &archFeature, sizeof(archFeature));

		features_.isTbdr = archFeature.TileBasedRenderer ? true : false;
		features_.isUMA = archFeature.UMA ? true : false;
		features_.isUMACacheCoherent = archFeature.CacheCoherentUMA ? true : false;
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



void XRender::Cmd_ListDeviceFeatures(core::IConsoleCmdArgs* pCmd)
{
	X_UNUSED(pCmd);


	X_LOG0("Dx12", "------ Device Info ------");
	X_LOG0("Dx12", "Name: \"%ls\"", deviceName_.c_str());
	X_LOG0("Dx12", "MaxTextureWidth: %" PRIu32, features_.maxTextureWidth);
	X_LOG0("Dx12", "MaxTextureHeight: %" PRIu32, features_.maxTextureHeight);
	X_LOG0("Dx12", "MaxTextureDepth: %" PRIu32, features_.maxTextureDepth);
	X_LOG0("Dx12", "MaxTextureCubeSize: %" PRIu32, features_.maxTextureCubeSize);
	X_LOG0("Dx12", "MaxTextureCubeSize: %" PRIu32, features_.maxTextureArrayLength);

	X_LOG0("Dx12", "MaxVertexTextureUnits: %" PRIu8, features_.maxVertexTextureUnits);
	X_LOG0("Dx12", "MaxPixelTextureUnits: %" PRIu8, features_.maxPixelTextureUnits);
	X_LOG0("Dx12", "MaxGeometryTextureUnits: %" PRIu8, features_.maxGeometryTextureUnits);
	X_LOG0("Dx12", "MaxSimultaneousRts: %" PRIu8, features_.maxSimultaneousRts);
	X_LOG0("Dx12", "MaxSimultaneousUavs: %" PRIu8, features_.maxSimultaneousUavs);
	X_LOG0("Dx12", "MaxVertexStreams: %" PRIu8, features_.maxVertexStreams);
	X_LOG0("Dx12", "MaxTextureAnisotropy: %" PRIu8, features_.maxTextureAnisotropy);

	X_LOG0("Dx12", "TileBasedRenderer: %s", features_.isTbdr ? "true" : "false");
	X_LOG0("Dx12", "UnifiedMemoryArchitecture: %s", features_.isUMA ? "true" : "false");
	X_LOG0("Dx12", "UMACacheCoherent: %s", features_.isUMACacheCoherent ? "true" : "false");

	X_LOG0("Dx12", "hwInstancingSupport: %s", features_.hwInstancingSupport ? "true" : "false");
	X_LOG0("Dx12", "instanceIdSupport: %s", features_.instanceIdSupport ? "true" : "false");
	X_LOG0("Dx12", "streamOutputSupport: %s", features_.streamOutputSupport ? "true" : "false");
	X_LOG0("Dx12", "alphaToCoverageSupport: %s", features_.alphaToCoverageSupport ? "true" : "false");
	X_LOG0("Dx12", "primitiveRestartSupport: %s", features_.primitiveRestartSupport ? "true" : "false");
	X_LOG0("Dx12", "multithreadRenderingSupport: %s", features_.multithreadRenderingSupport ? "true" : "false");
	X_LOG0("Dx12", "multithreadResCreatingSupport: %s", features_.multithreadResCreatingSupport ? "true" : "false");
	X_LOG0("Dx12", "mrtIndependentBitDepthsSupport: %s", features_.mrtIndependentBitDepthsSupport ? "true" : "false");
	X_LOG0("Dx12", "standardDerivativesSupport: %s", features_.standardDerivativesSupport ? "true" : "false");
	X_LOG0("Dx12", "shaderTextureLodSupport: %s", features_.shaderTextureLodSupport ? "true" : "false");
	X_LOG0("Dx12", "logicOpSupport: %s", features_.logicOpSupport ? "true" : "false");
	X_LOG0("Dx12", "independentBlendSupport: %s", features_.independentBlendSupport ? "true" : "false");
	X_LOG0("Dx12", "depthTextureSupport: %s", features_.depthTextureSupport ? "true" : "false");
	X_LOG0("Dx12", "fpColorSupport: %s", features_.fpColorSupport ? "true" : "false");
	X_LOG0("Dx12", "packToRgbaRequired: %s", features_.packToRgbaRequired ? "true" : "false");
	X_LOG0("Dx12", "drawIndirectSupport: %s", features_.drawIndirectSupport ? "true" : "false");
	X_LOG0("Dx12", "noOverwriteSupport: %s", features_.noOverwriteSupport ? "true" : "false");
	X_LOG0("Dx12", "fullNpotTextureSupport: %s", features_.fullNpotTextureSupport ? "true" : "false");
	X_LOG0("Dx12", "renderToTextureArraySupport: %s", features_.renderToTextureArraySupport ? "true" : "false");
	X_LOG0("Dx12", "gsSupport: %s", features_.gsSupport ? "true" : "false");
	X_LOG0("Dx12", "csSupport: %s", features_.csSupport ? "true" : "false");
	X_LOG0("Dx12", "hsSupport: %s", features_.hsSupport ? "true" : "false");
	X_LOG0("Dx12", "dsSupport: %s", features_.dsSupport ? "true" : "false");


	X_LOG0("Dx12", "-------------------------");
}



X_NAMESPACE_END