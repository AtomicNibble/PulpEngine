#include "stdafx.h"
#include "XRender.h"


X_NAMESPACE_BEGIN(render)


XRender::XRender(core::MemoryArenaBase* arena) :
	pDevice_(nullptr),
	pDebug_(nullptr),
	pSwapChain_(nullptr),
	cmdListManager_(arena),
	dedicatedvideoMemory_(0)
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
				pAdapter->GetDesc1(&desc);
				X_LOG0("Dx12", "D3D12-capable hardware found: %ls (%u MB)\n", desc.Description, desc.DedicatedVideoMemory >> 20);

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


	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	core::zero_object(swapChainDesc);

	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
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

	}


	return true;
}

void XRender::ShutDown(void)
{
	cmdListManager_.shutdown();


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

	if (pDevice_) {
		pDevice_->Release();
	}

	if (pDebug_) {
		pDebug_->Release();
	}

}

void XRender::freeResources(void)
{

}

void XRender::RenderBegin(void)
{

}

void XRender::RenderEnd(void)
{

}


bool XRender::InitRenderBuffers(uint32_t width, uint32_t hegith)
{


	return true;
}


bool XRender::Resize(uint32_t width, uint32_t hegith)
{


	return true;
}

X_NAMESPACE_END