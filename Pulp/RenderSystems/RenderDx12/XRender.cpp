#include "stdafx.h"
#include "XRender.h"

#include "Texture\TextureManager.h"
#include "Texture\Texture.h"
#include "Auxiliary\AuxRenderImp.h"

#include "Allocators\LinearAllocator.h"
#include "CommandContex.h"
#include "PipelineState.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(render)


XRender::XRender(core::MemoryArenaBase* arena) :
	arena_(arena),
	pDevice_(nullptr),
	pDebug_(nullptr),
	pSwapChain_(nullptr),
	pTextureMan_(nullptr),
	pAuxRender_(nullptr),
	shaderMan_(arena),
	pContextMan_(nullptr),
	cmdListManager_(arena),
	dedicatedvideoMemory_(0),
	pDescriptorAllocator_(nullptr),
	pDescriptorAllocatorPool_(nullptr),
	presentRS_(arena),
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
	if (hWnd == static_cast<HWND>(0)) {
		X_ERROR("dx10", "target window is not valid");
		return false;
	}

	currentNativeRes_ = Vec2<uint32_t>(width, height);
	displayRes_ = Vec2<uint32_t>(width, height);

	HRESULT hr;

	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug_));
	if (FAILED(hr)) {
		X_ERROR("Dx12", "Failed to CreateDevice: 0x%x", hr);
		return false;
	}

	pDebug_->EnableDebugLayer();


	// Obtain the DXGI factory
	IDXGIFactory4* pDxgiFactory;
	hr = CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), reinterpret_cast<void**>(&pDxgiFactory));

	if (!pDxgiFactory) {
		X_ERROR("Dx12", "Failed to create DXGI Factory");
		return false;
	}

	// Create the D3D graphics device
	{
		IDXGIAdapter1* pAdapter;
		for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != pDxgiFactory->EnumAdapters1(Idx, &pAdapter); ++Idx)
		{
			DXGI_ADAPTER_DESC1 desc;
			pAdapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}

			featureLvl_ = D3D_FEATURE_LEVEL_11_0;

			hr = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pDevice_));
			if (SUCCEEDED(hr))
			{
				X_LOG0("Dx12", "D3D12-capable hardware found: \"%ls\" (%u MB)\n", desc.Description, desc.DedicatedVideoMemory >> 20);

				deviceName_.set(desc.Description);
				dedicatedvideoMemory_ = desc.DedicatedVideoMemory;
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
	pContextMan_ = X_NEW(ContextManager, arena_, "ContextMan")(arena_, pDevice_, descriptorAllocatorPool, *pLinearAllocatorMan_);
	pRootSigCache_ = X_NEW(RootSignatureDeviceCache, arena_, "RootSignatureDeviceCache")(arena_, pDevice_);
	pPSOCache_ = X_NEW(PSODeviceCache, arena_, "PSODeviceCache")(arena_, pDevice_);

	

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


	hr = pDxgiFactory->CreateSwapChainForHwnd(cmdListManager_.getCommandQueue(), hWnd,
		&swapChainDesc, nullptr, nullptr, &pSwapChain_);
	if (FAILED(hr)) {
		X_ERROR("Dx12", "failed to create swap chain: %" PRId32, hr);
		return false;
	}

	for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		ID3D12Resource* pDisplayPlane;
		hr = pSwapChain_->GetBuffer(i, IID_PPV_ARGS(&pDisplayPlane));
		if (FAILED(hr)) {
			X_ERROR("Dx12", "failed to get swap chain buffer: %" PRId32, hr);
			return false;
		}

		displayPlane_[i].createFromSwapChain(pDevice_, descriptorAllocator, pDisplayPlane);
		displayPlane_[i].setClearColor(vars_.getClearCol());
	}


	// Samplers.
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

	// RootSig
	presentRS_.reset(4, 2);
	presentRS_.getParamRef(0).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
	presentRS_.getParamRef(1).initAsConstants(0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.getParamRef(2).initAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.getParamRef(3).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
	presentRS_.initStaticSampler(0, samplerLinearClampDesc_, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.initStaticSampler(1, samplerPointClampDesc_, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.finalize(*pRootSigCache_);


	if (!shaderMan_.init()) {
		X_ERROR("Render", "failed to init shader system");
		return false;
	}

	pTextureMan_ = X_NEW(texture::TextureManager, arena_, "TexMan")(arena_, pDevice_, *pContextMan_, cmdListManager_, descriptorAllocator);
	if (!pTextureMan_->init()) {
		X_ERROR("Render", "failed to init texture system");
		return false;
	}

	pAuxRender_ = X_NEW(RenderAuxImp, arena_, "AuxRenderer")(arena_);
	if (!pAuxRender_->init()) {
		X_ERROR("Render", "failed to init aux render system");
		return false;
	}

	return true;
}

void XRender::shutDown(void)
{
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


	shaderMan_.shutdown();

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

	core::SafeReleaseDX(pDevice_);
	core::SafeReleaseDX(pDebug_);
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

	GraphicsContext* pContext = pContextMan_->allocateGraphicsContext(cmdListManager_);

	pContext->transitionResource(displayPlane_[currentBufferIdx_], D3D12_RESOURCE_STATE_RENDER_TARGET);
	pContext->setViewportAndScissor(0, 0, currentNativeRes_.x, currentNativeRes_.y);
	pContext->setRootSignature(presentRS_);

	pContext->setRenderTargets(_countof(RTVs), RTVs);
	pContext->setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->draw(3);


	pContext->clearColor(displayPlane_[currentBufferIdx_]);
	pContext->transitionResource(displayPlane_[currentBufferIdx_], D3D12_RESOURCE_STATE_PRESENT);
	pContext->finish(cmdListManager_, false);
}

void XRender::renderEnd(void)
{
	currentBufferIdx_ = (currentBufferIdx_ + 1) % SWAP_CHAIN_BUFFER_COUNT;

	HRESULT hr = pSwapChain_->Present(0, 0);
	if (FAILED(hr)) {
		X_ERROR("Dx12", "Present failed. err: %" PRIu32, hr);
	}

	handleResolutionChange();
}


IRenderAux* XRender::getAuxRender(AuxRenderer::Enum user)
{
	return &auxQues_[user];
}


Vec2<uint32_t> XRender::getDisplayRes(void) const
{
	return displayRes_;
}

Vec2<float32_t> XRender::getDisplayResf(void) const
{
	return Vec2<float32_t>(static_cast<float32_t>(displayRes_.x), static_cast<float32_t>(displayRes_.y));
}


bool XRender::freeSwapChainResources(void)
{
	for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
		displayPlane_[i].destroy();
	}

	return true;
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

	X_LOG0("Dx12", "-------------------------");
}





// =============================================
// ============== OLD API ======================
// =============================================


void XRender::SetState(StateFlag state)
{
	X_UNUSED(state);
}

void XRender::SetStencilState(StencilState::Value ss)
{
	X_UNUSED(ss);
}

void XRender::SetCullMode(CullMode::Enum mode)
{
	X_UNUSED(mode);
}

void XRender::Set2D(bool value, float znear, float zfar)
{
	X_UNUSED(value);
	X_UNUSED(znear);
	X_UNUSED(zfar);

}



void XRender::GetViewport(int* x, int* y, int* width, int* height)
{
	X_ASSERT_NOT_NULL(x);
	X_ASSERT_NOT_NULL(y);
	X_ASSERT_NOT_NULL(width);
	X_ASSERT_NOT_NULL(height);
}

void XRender::SetViewport(int x, int y, int width, int height)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
}

void XRender::GetViewport(Recti& rect)
{
	X_UNUSED(rect);

}

void XRender::SetViewport(const Recti& rect)
{
	X_UNUSED(rect);

}

int XRender::getWidth(void) const
{
	return 0; // return 1 maybe as i might divide by this.
}

int XRender::getHeight(void) const
{
	return 0;
}

float XRender::getWidthf(void) const
{
	return 0.f; // return 1 maybe as i might divide by this.
}

float XRender::getHeightf(void) const
{
	return 0.f;
}


float XRender::ScaleCoordX(float value) const
{
	X_UNUSED(value);
	return 0.f;
}

float XRender::ScaleCoordY(float value) const
{
	X_UNUSED(value);
	return 0.f;
}

void XRender::ScaleCoord(float& x, float& y) const
{
	X_UNUSED(x);
	X_UNUSED(y);
}

void XRender::ScaleCoord(Vec2f& xy) const
{
	X_UNUSED(xy);
}


void XRender::SetCamera(const XCamera& cam)
{
	X_UNUSED(cam);
}

const XCamera& XRender::GetCamera()
{
	static XCamera cam;
	return cam;
}

// AuxGeo
IRenderAux* XRender::GetIRenderAuxGeo(void)
{
	return nullptr;
}
// ~AuxGeo


// Textures 
::texture::ITexture* XRender::LoadTexture(const char* pPath, texture::TextureFlags flags)
{
	X_ASSERT_NOT_NULL(pPath);

	texture::Texture* pText = pTextureMan_->forName(pPath, flags);

	return pText;
}


void XRender::ReleaseTexture(texture::TexID id)
{
	X_UNUSED(id);
}

bool XRender::SetTexture(texture::TexID id)
{
	X_UNUSED(id);
	return false;
}

// ~Textures


// Drawing


void XRender::DrawQuadSS(float x, float y, float width, float height,
	const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}


void XRender::DrawQuadSS(const Rectf& rect, const Color& col)
{
	X_UNUSED(rect);
	X_UNUSED(col);
}

void XRender::DrawQuadSS(float x, float y, float width, float height,
	const Color& col, const Color& borderCol)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
	X_UNUSED(borderCol);
}

void XRender::DrawQuadImageSS(float x, float y, float width, float height,
	texture::TexID texture_id, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(texture_id);
	X_UNUSED(col);
}

void XRender::DrawQuadImageSS(const Rectf& rect, texture::TexID texture_id,
	const Color& col)
{
	X_UNUSED(rect);
	X_UNUSED(texture_id);
	X_UNUSED(col);
}

void XRender::DrawRectSS(float x, float y, float width, float height,
	const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}

void XRender::DrawRectSS(const Rectf& rect, const Color& col)
{
	X_UNUSED(rect);
	X_UNUSED(col);
}

void XRender::DrawLineColorSS(const Vec2f& vPos1, const Color& color1,
	const Vec2f& vPos2, const Color& vColor2)
{
	X_UNUSED(vPos1);
	X_UNUSED(color1);
	X_UNUSED(vPos2);
	X_UNUSED(vColor2);
}

void XRender::DrawQuadImage(float x, float y, float width, float height,
	texture::TexID texture_id, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(texture_id);
	X_UNUSED(col);
}

void XRender::DrawQuadImage(float x, float y, float width, float height,
	texture::ITexture* pTexutre, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(pTexutre);
	X_UNUSED(col);
}

void XRender::DrawQuadImage(const Rectf& rect, texture::ITexture* pTexutre,
	const Color& col)
{
	X_UNUSED(rect);
	X_UNUSED(pTexutre);
	X_UNUSED(col);
}




void XRender::DrawQuad(float x, float y, float z, float width, float height, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(z);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}

void XRender::DrawQuad(float x, float y, float z, float width, float height,
	const Color& col, const Color& bordercol)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(z);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
	X_UNUSED(bordercol);
}

void XRender::DrawQuad(float x, float y, float width, float height,
	const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}

void XRender::DrawQuad(float x, float y, float width, float height,
	const Color& col, const Color& bordercol)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
	X_UNUSED(bordercol);
}

void XRender::DrawQuad(Vec2<float> pos, float width, float height, const Color& col)
{
	X_UNUSED(pos);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);

}

void XRender::DrawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2, const Vec3f& pos3, const Color& col)
{
	X_UNUSED(pos0);
	X_UNUSED(pos1);
	X_UNUSED(pos2);
	X_UNUSED(pos3);
	X_UNUSED(col);
}


void XRender::DrawLines(Vec3f* points, uint32_t num, const Color& col)
{
	X_ASSERT_NOT_NULL(points);
	X_UNUSED(points);
	X_UNUSED(num);
	X_UNUSED(col);
}

void XRender::DrawLine(const Vec3f& pos1, const Vec3f& pos2)
{
	X_UNUSED(pos1);
	X_UNUSED(pos2);

}

void XRender::DrawLineColor(const Vec3f& pos1, const Color& color1,
	const Vec3f& pos2, const Color& color2)
{
	X_UNUSED(pos1);
	X_UNUSED(pos2);
	X_UNUSED(color1);
	X_UNUSED(color2);

}

void XRender::DrawRect(float x, float y, float width, float height, const Color& col)
{
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(width);
	X_UNUSED(height);
	X_UNUSED(col);
}


void XRender::DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
	float padding, uint32_t max)
{
	X_UNUSED(rect);
	X_UNUSED(num);
	X_UNUSED(heights);
	X_UNUSED(padding);
	X_UNUSED(max);
}

void XRender::DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* format, va_list args)
{
	X_UNUSED(pos);
	X_UNUSED(ti);
	X_UNUSED(format);
	X_UNUSED(args);
}

void XRender::DrawTextQueued(Vec3f pos, const XDrawTextInfo& ti, const char* text)
{
	X_UNUSED(pos);
	X_UNUSED(ti);
	X_UNUSED(text);
}

void XRender::DrawAllocStats(Vec3f pos, const XDrawTextInfo& ti,
	const core::MemoryAllocatorStatistics& allocStats, const char* title)
{
	X_UNUSED(pos);
	X_UNUSED(ti);
	X_UNUSED(allocStats);
	X_UNUSED(title);

}

void XRender::FlushTextBuffer(void)
{

}

// ~Drawing



// Font

int XRender::FontCreateTexture(const Vec2i& size, BYTE* pData,
	texture::Texturefmt::Enum eTF, bool genMips)
{
	X_ASSERT_NOT_NULL(pData);
	X_UNUSED(size);
	X_UNUSED(pData);
	X_UNUSED(eTF);
	X_UNUSED(genMips);


	return 0;
}


bool XRender::FontUpdateTexture(int texId, int x, int y, int USize, int VSize,
	uint8_t* pData)
{
	X_ASSERT_NOT_NULL(pData);
	X_UNUSED(texId);
	X_UNUSED(x);
	X_UNUSED(y);
	X_UNUSED(USize);
	X_UNUSED(VSize);
	X_UNUSED(pData);

	return false;
}

bool XRender::FontSetTexture(int texId)
{
	X_UNUSED(texId);

	return false;
}

bool XRender::FontSetRenderingState()
{
	return false;
}

void XRender::FontRestoreRenderingState()
{

}

void XRender::FontSetBlending()
{

}


void XRender::DrawStringW(font::IXFont_RenderProxy* pFont, const Vec3f& pos,
	const wchar_t* pStr, const font::XTextDrawConect& ctx) const
{
	X_ASSERT_NOT_NULL(pFont);
	X_ASSERT_NOT_NULL(pStr);

	X_UNUSED(pFont);
	X_UNUSED(pos);
	X_UNUSED(pStr);
	X_UNUSED(ctx);


}


// ~Font


void XRender::DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
	PrimitiveTypePublic::Enum type)
{
	X_ASSERT_NOT_NULL(pVertBuffer);

	X_UNUSED(pVertBuffer);
	X_UNUSED(size);
	X_UNUSED(type);

}

// Shader Stuff

shader::XShaderItem XRender::LoadShaderItem(shader::XInputShaderResources& res)
{
	X_UNUSED(res);
	return shader::XShaderItem();
}


bool XRender::DefferedBegin(void)
{
	return false;
}

bool XRender::DefferedEnd(void)
{
	return false;
}

bool XRender::SetWorldShader(void)
{
	return false;
}

bool XRender::setGUIShader(bool textured)
{
	X_UNUSED(textured);

	return false;
}

// ~Shader Stuff

// Model
model::IRenderMesh* XRender::createRenderMesh(void)
{
	return nullptr;
}

model::IRenderMesh* XRender::createRenderMesh(const model::MeshHeader* pMesh,
	shader::VertexFormat::Enum fmt, const char* name)
{
	X_UNUSED(pMesh);
	X_UNUSED(fmt);
	X_UNUSED(name);

	return nullptr;
}

void XRender::freeRenderMesh(model::IRenderMesh* pMesh)
{
	X_UNUSED(pMesh);

}

void XRender::SetModelMatrix(const Matrix44f& mat)
{
	X_UNUSED(mat);
}

// ~Model



X_NAMESPACE_END