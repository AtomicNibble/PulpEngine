#include "stdafx.h"
#include "XRender.h"


X_NAMESPACE_BEGIN(render)


XRender::XRender(core::MemoryArenaBase* arena) :
	arena_(arena),
	pDevice_(nullptr),
	pDebug_(nullptr),
	pSwapChain_(nullptr),
	shaderMan_(arena),
	cmdListManager_(arena),
	dedicatedvideoMemory_(0),
	pDescriptorAllocator_(nullptr),
	pDescriptorAllocatorPool_(nullptr)
{

}

XRender::~XRender()
{

}

bool XRender::Init(PLATFORM_HWND hWnd, uint32_t width, uint32_t height)
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


	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	core::zero_object(swapChainDesc);

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





	if (!shaderMan_.Init()) {
		X_ERROR("Render", "failed to init shader system");
		return false;
	}

	return true;
}

void XRender::ShutDown(void)
{
	shaderMan_.Shutdown();

	cmdListManager_.shutdown();

	for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
		displayPlane_[i].destroy();
	}

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

void XRender::RenderBegin(void)
{

}

void XRender::RenderEnd(void)
{
	currentBufferIdx_ = (currentBufferIdx_ + 1) % SWAP_CHAIN_BUFFER_COUNT;


	HRESULT hr = pSwapChain_->Present(0, 0);
	if (FAILED(hr)) {
		X_ERROR("Dx12", "Present failed. err: %" PRIu32, hr);
	}

	HandleResolutionChange();
}


bool XRender::InitRenderBuffers(Vec2<uint32_t> res)
{


	return true;
}


bool XRender::Resize(uint32_t width, uint32_t height)
{
	X_LOG1("Dx12", "Resizing display res to: x:%x y:%x", width, height);
	X_ASSERT_NOT_NULL(pDescriptorAllocator_);

	displayRes_.x = width;
	displayRes_.y = height;

	for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i) {
		displayPlane_[i].destroy();
	}

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

void XRender::HandleResolutionChange(void)
{
	if (currentNativeRes_ == targetNativeRes_) {
		return;
	}

	X_LOG1("Dx12", "Changing native res from: x:%x y:%x to: x:%x y:%x", 
		currentNativeRes_.x, currentNativeRes_.y,
		targetNativeRes_.x, targetNativeRes_.y);

	currentNativeRes_ = targetNativeRes_;

	// wait till gpu idle.
	cmdListManager_.idleGPU();

	// re int buffers
	InitRenderBuffers(targetNativeRes_);
}

X_NAMESPACE_END