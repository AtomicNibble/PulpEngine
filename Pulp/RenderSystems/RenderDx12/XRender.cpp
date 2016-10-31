#include "stdafx.h"
#include "XRender.h"

#include "Texture\TextureManager.h"
#include "Texture\Texture.h"
#include "Texture\TextureUtil.h"
#include "Shader\ShaderManager.h"
#include "Shader\Shader.h"
#include "Shader\HWShader.h"
#include "Auxiliary\AuxRenderImp.h"

#include "Allocators\LinearAllocator.h"
#include "Buffers\BufferManager.h"
#include "CommandContex.h"
#include "PipelineState.h"

#include "CmdBucket.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(render)

namespace
{
	static const uint8 g_StencilFuncLookup[9] =
	{
		D3D12_COMPARISON_FUNC_NEVER, // pad
		D3D12_COMPARISON_FUNC_NEVER,
		D3D12_COMPARISON_FUNC_LESS,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_COMPARISON_FUNC_GREATER,
		D3D12_COMPARISON_FUNC_GREATER_EQUAL,
		D3D12_COMPARISON_FUNC_EQUAL,
		D3D12_COMPARISON_FUNC_NOT_EQUAL,
		D3D12_COMPARISON_FUNC_ALWAYS,
	};

	static const uint8 g_StencilOpLookup[9] =
	{
		D3D12_STENCIL_OP_KEEP, // pad
		D3D12_STENCIL_OP_KEEP,
		D3D12_STENCIL_OP_ZERO,
		D3D12_STENCIL_OP_REPLACE,
		D3D12_STENCIL_OP_INCR_SAT,
		D3D12_STENCIL_OP_DECR_SAT,
		D3D12_STENCIL_OP_INVERT,
		D3D12_STENCIL_OP_INCR,
		D3D12_STENCIL_OP_DECR,
	};



	void createDescFromState(StateFlag state, D3D12_BLEND_DESC& blendDesc)
	{
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.AlphaToCoverageEnable = state.IsSet(States::ALPHATEST_MASK);

		if (state.IsSet(States::BLEND_MASK))
		{
			for (size_t i = 0; i < 8; ++i) {
				blendDesc.RenderTarget[i].BlendEnable = TRUE;
				// A combination of D3D12_COLOR_WRITE_ENABLE-typed values that are combined by using a bitwise OR operation.
				blendDesc.RenderTarget[i].RenderTargetWriteMask = 0;
			}

			// Blend Src.
			if (state.IsSet(States::BLEND_SRC_MASK))
			{
				switch (state.ToInt() & States::BLEND_SRC_MASK)
				{
					case States::BLEND_SRC_ZERO:
						blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
						blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
						break;
					case States::BLEND_SRC_ONE:
						blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
						blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
						break;
					case States::BLEND_SRC_DEST_COLOR:
						blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_COLOR;
						blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_DEST_ALPHA;
						break;
					case States::BLEND_SRC_INV_DEST_COLOR:
						blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
						blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;
						break;
					case States::BLEND_SRC_SRC_ALPHA:
						blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
						blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
						break;
					case States::BLEND_SRC_INV_SRC_ALPHA:
						blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_SRC_ALPHA;
						blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
						break;
					case States::BLEND_SRC_DEST_ALPHA:
						blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_ALPHA;
						blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_DEST_ALPHA;
						break;
					case States::BLEND_SRC_INV_DEST_ALPHA:
						blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_ALPHA;
						blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;
						break;
					case States::BLEND_SRC_ALPHA_SAT:
						blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA_SAT;
						blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA_SAT;
						break;
				}
			}

			// Blend Dst.
			if (state.IsSet(States::BLEND_DEST_MASK))
			{
				switch (state.ToInt() & States::BLEND_DEST_MASK)
				{
					case States::BLEND_DEST_ZERO:
						blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
						blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
						break;
					case States::BLEND_DEST_ONE:
						blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
						blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
						break;
					case States::BLEND_DEST_SRC_COLOR:
						blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
						blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
						break;
					case States::BLEND_DEST_INV_SRC_COLOR:
						blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
						blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
						break;
					case States::BLEND_DEST_SRC_ALPHA:
						blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_ALPHA;
						blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_SRC_ALPHA;
						break;
					case States::BLEND_DEST_INV_SRC_ALPHA:
						blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
						blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
						break;
					case States::BLEND_DEST_DEST_ALPHA:
						blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_DEST_ALPHA;
						blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_DEST_ALPHA;
						break;
					case States::BLEND_DEST_INV_DEST_ALPHA:
						blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_DEST_ALPHA;
						blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_DEST_ALPHA;
						break;
				}

			}

			//Blending operation
			D3D12_BLEND_OP blendOperation = D3D12_BLEND_OP_ADD;

			switch (state.ToInt() & States::BLEND_OP_MASK)
			{
				case States::BLEND_OP_ADD:
					blendOperation = D3D12_BLEND_OP_ADD;
					break;
				case States::BLEND_OP_SUB:
					blendOperation = D3D12_BLEND_OP_SUBTRACT;
					break;
				case States::BLEND_OP_REB_SUB:
					blendOperation = D3D12_BLEND_OP_REV_SUBTRACT;
					break;
				case States::BLEND_OP_MIN:
					blendOperation = D3D12_BLEND_OP_MIN;
					break;
				case States::BLEND_OP_MAX:
					blendOperation = D3D12_BLEND_OP_MAX;
					break;
			}

			// todo: add separate alpha blend support for mrt
			for (size_t i = 0; i < 8; ++i) {
				blendDesc.RenderTarget[i].BlendOp = blendOperation;
				blendDesc.RenderTarget[i].BlendOpAlpha = blendOperation;
			}
		}
		else
		{
			// disable blending.
			for (size_t i = 0; i < 8; ++i) {
				blendDesc.RenderTarget[i].BlendEnable = FALSE;
			}
		}

	}

	void createDescFromState(StateFlag state, D3D12_RASTERIZER_DESC& rasterizerDesc)
	{
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FrontCounterClockwise = TRUE;
		rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterizerDesc.DepthClipEnable = TRUE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		rasterizerDesc.ForcedSampleCount = 0;
		rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		if (state.IsSet(StateFlag::WIREFRAME)) {
			rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		}

		if (state.IsSet(States::CULL_MASK))
		{
			switch (state.ToInt() & States::CULL_MASK)
			{
				case States::CULL_NONE:
					rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
					break;
				case States::CULL_FRONT:
					rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
					break;
				case States::CULL_BACK:
					rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
					break;
			}
		}
	}

	void createDescFromState(StateFlag state, const StencilState& stencilState, D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
	{
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.StencilEnable = FALSE;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

		// Stencil operations if pixel is front-facing.
		{
			auto& frontFace = depthStencilDesc.FrontFace;
			const auto& state = stencilState.front;

			frontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.zFailOp]);
			frontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.failOp]);
			frontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.passOp]);
			frontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(g_StencilFuncLookup[state.stencilFunc]);
		}

		// Stencil operations if pixel is back-facing.
		{
			auto& backFace = depthStencilDesc.BackFace;
			const auto& state = stencilState.back;

			backFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.zFailOp]);
			backFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.failOp]);
			backFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(g_StencilOpLookup[state.passOp]);
			backFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(g_StencilFuncLookup[state.stencilFunc]);
		}



		if (state.IsSet(StateFlag::DEPTHWRITE)) {
			depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		}
		if (state.IsSet(StateFlag::NO_DEPTH_TEST)) {
			depthStencilDesc.DepthEnable = FALSE;
		}
		if (state.IsSet(StateFlag::STENCIL)) {
			depthStencilDesc.StencilEnable = TRUE;
		}

		switch (state.ToInt() & States::DEPTHFUNC_MASK)
		{
			case States::DEPTHFUNC_LEQUAL:
				depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
				break;
			case States::DEPTHFUNC_EQUAL:
				depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
				break;
			case States::DEPTHFUNC_GREAT:
				depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
				break;
			case States::DEPTHFUNC_LESS:
				depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
				break;
			case States::DEPTHFUNC_GEQUAL:
				depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
				break;
			case States::DEPTHFUNC_NOTEQUAL:
				depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
				break;

#if X_DEBUG
			default:
				X_ASSERT_UNREACHABLE();
				break;
#else
				X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
		}
	}

	StateFlag stateFlagFromDesc(const StateDesc& desc)
	{

		return StateFlag();
	}

	StencilState stencilStateFromDesc(const StateDesc& desc)
	{

		return StencilState();
	}

	D3D12_PRIMITIVE_TOPOLOGY_TYPE topoTypeFromDesc(const StateDesc& desc)
	{
		switch (desc.topo)
		{
			case TopoType::TRIANGLELIST:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			case TopoType::TRIANGLESTRIP:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			case TopoType::LINELIST:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			case TopoType::LINESTRIP:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			case TopoType::POINTLIST:
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

			default:
#if X_DEBUG
				X_ASSERT_UNREACHABLE();
				return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
#else
				X_NO_SWITCH_DEFAULT;
#endif // X_DEBUG
		}
	}

	D3D12_PRIMITIVE_TOPOLOGY topoFromDesc(const StateDesc& desc)
	{
		switch (desc.topo)
		{
			case TopoType::TRIANGLELIST:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case TopoType::TRIANGLESTRIP:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			case TopoType::LINELIST:
				return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case TopoType::LINESTRIP:
				return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case TopoType::POINTLIST:
				return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

			default:
#if X_DEBUG
				X_ASSERT_UNREACHABLE();
				return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
#else
				X_NO_SWITCH_DEFAULT;
#endif // X_DEBUG
		}
	}


	


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

	pBuffMan_ = X_NEW(BufferManager, arena_, "BufferManager")(arena_, pDevice_);
	if (!pBuffMan_->init()) {
		X_ERROR("Render", "failed to init buffer manager");
		return false;
	}
	
	pDescriptorAllocator_ = X_NEW(DescriptorAllocator, arena_, "DescriptorAllocator")(arena_, pDevice_);
	pDescriptorAllocatorPool_ = X_NEW(DescriptorAllocatorPool, arena_, "DescriptorAllocatorPool")(arena_, pDevice_, cmdListManager_);

	DescriptorAllocator& descriptorAllocator = *pDescriptorAllocator_;
	DescriptorAllocatorPool& descriptorAllocatorPool = *pDescriptorAllocatorPool_;

	pLinearAllocatorMan_ = X_NEW(LinearAllocatorManager, arena_, "LinAlocMan")(arena_, pDevice_, cmdListManager_);
	pContextMan_ = X_NEW(ContextManager, arena_, "ContextMan")(arena_, pDevice_, cmdListManager_, descriptorAllocatorPool, *pLinearAllocatorMan_);
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
	presentRS_.getParamRef(2).initAsSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.getParamRef(3).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);
	presentRS_.initStaticSampler(0, samplerLinearClampDesc_, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.initStaticSampler(1, samplerPointClampDesc_, D3D12_SHADER_VISIBILITY_PIXEL);
	presentRS_.finalize(*pRootSigCache_);


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
	presentRS_.free();

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

	pContext->transitionResource(displayPlane_[currentBufferIdx_], D3D12_RESOURCE_STATE_RENDER_TARGET);
	pContext->setViewportAndScissor(0, 0, currentNativeRes_.x, currentNativeRes_.y);
	pContext->setRootSignature(presentRS_);

	pContext->setRenderTargets(_countof(RTVs), RTVs);
	pContext->setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pContext->draw(3);

	pContext->clearColor(displayPlane_[currentBufferIdx_]);
	pContext->transitionResource(displayPlane_[currentBufferIdx_], D3D12_RESOURCE_STATE_PRESENT);
	pContext->finishAndFree(false);
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


void XRender::submitCommandPackets(CommandBucket<uint32_t>& cmdBucket, Commands::Key::Type::Enum keyType)
{
	const auto& sortedIdx = cmdBucket.getSortedIdx();
	const auto& packets = cmdBucket.getPackets();
	const auto& keys = cmdBucket.getKeys();

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
		RTVs[i] = rtv.getSRV();
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
		const uint32_t key = keys[i];
		CommandPacket::Packet pPacket = packets[sortedIdx[i]];

		while (pPacket != nullptr)
		{
			const CommandPacket::Command::Enum cmdType = CommandPacket::loadCommandType(pPacket);
			const void* pCmd = CommandPacket::loadCommand(pPacket);

			switch (cmdType)
			{
				case Commands::Command::DRAW:
				{
					const Commands::Draw& draw = *reinterpret_cast<const Commands::Draw*>(pCmd);

					ApplyState(context, curState, draw.stateHandle, draw.vertexBuffers);
					context.draw(draw.vertexCount, draw.startVertex);
					break;
				}
				case Commands::Command::DRAW_INDEXED:
				{
					const Commands::DrawIndexed& draw = *reinterpret_cast<const Commands::DrawIndexed*>(pCmd);

					ApplyState(context, curState, draw.stateHandle, draw.vertexBuffers);

					// don't bother looking up ib if same handle.
					if (curState.indexBuffer != draw.indexBuffer) {
						curState.indexBuffer = draw.indexBuffer;
						const auto pIBuf = pBuffMan_->IBFromHandle(draw.indexBuffer);
						context.setIndexBuffer(pIBuf->getBuf().indexBufferView());
					}

					context.drawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
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
	}

	// for now wait.
	pContext->finishAndFree(true);

}

X_INLINE void XRender::CreateVBView(const VertexHandleArr& vertexBuffers,
	D3D12_VERTEX_BUFFER_VIEW viewsOut[VertexStream::ENUM_COUNT], uint32_t& numVertexStreams)
{
	numVertexStreams = 0;

	core::zero_object(viewsOut);
	for (size_t i = 0; i < VertexStream::ENUM_COUNT; i++)
	{
		if (vertexBuffers[i]) 
		{
			const auto pVertBuf = pBuffMan_->VBFromHandle(vertexBuffers[i]);

			viewsOut[i] = pVertBuf->getBuf().vertexBufferView();
			numVertexStreams++;
		}
	}
}


void XRender::ApplyState(GraphicsContext& context, State& curState, const StateHandle handle,
	const VertexHandleArr& vertexBuffers)
{
#if 1
	if (curState.handle != handle) // if the handle is the same, everything is the same.
	{
		curState.handle = handle;

		// now we check what parts of the state are diffrent.
		const DeviceState& newState = *reinterpret_cast<const DeviceState*>(handle);

		// contains a redundant check.
		context.setRootSignature(newState.rootSig);

		if (curState.pPso != newState.pPso) {
			context.setPipelineState(newState.pPso);
			curState.pPso = newState.pPso;
		}
		if (curState.topo != newState.topo) {
			context.setPrimitiveTopology(newState.topo);
			curState.topo = newState.topo;
		}
	}

	// any handles diffrent?
	// wonder if this be quicker if i shove it on a boundary, have to check what compiler does.
	if (std::memcmp(curState.vertexBuffers.data(), vertexBuffers.data(), sizeof(vertexBuffers)) == 0)
	{
		curState.vertexBuffers = vertexBuffers;

		uint32_t numVertexStreams = 0;
		D3D12_VERTEX_BUFFER_VIEW vertexViews[VertexStream::ENUM_COUNT] = { 0 };
		CreateVBView(vertexBuffers, vertexViews, numVertexStreams);

		context.setVertexBuffers(0, numVertexStreams, vertexViews);
	}

#endif
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

VertexBufferHandle XRender::createVertexBuffer(uint32_t size, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	return pBuffMan_->createVertexBuf(size, nullptr, usage, accessFlag);
}

VertexBufferHandle XRender::createVertexBuffer(uint32_t size, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	return pBuffMan_->createVertexBuf(size, pInitialData, usage, accessFlag);
}

IndexBufferHandle XRender::createIndexBuffer(uint32_t size, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	return pBuffMan_->createIndexBuf(size, nullptr, usage, accessFlag);
}

IndexBufferHandle XRender::createIndexBuffer(uint32_t size, const void* pInitialData, BufUsage::Enum usage, CpuAccessFlags accessFlag)
{
	return pBuffMan_->createIndexBuf(size, pInitialData, usage, accessFlag);
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


::texture::ITexture* XRender::getTexture(const char* pName, texture::TextureFlags flags)
{
	texture::Texture* pText = pTextureMan_->forName(pName, flags);

	return pText;
}

shader::IShader* XRender::getShader(const char* pName)
{
	shader::XShader* pShader = pShaderMan_->forName(pName);

	return pShader;
}

void XRender::releaseTexture(texture::ITexture* pTex)
{
	pTextureMan_->releaseTexture(pTex);
}

void XRender::releaseShader(shader::IShader* pShader)
{
	pShaderMan_->releaseShader(static_cast<shader::XShader*>(pShader));
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

StateHandle XRender::createState(PassStateHandle passHandle, const StateDesc& desc, const TextureState* pTextStates, size_t numStates)
{
	const PassState* pPassState = reinterpret_cast<const PassState*>(passHandle);

	const StateFlag state = stateFlagFromDesc(desc);
	const StencilState stencilState = stencilStateFromDesc(desc);

	DeviceState* pState = X_NEW(DeviceState, arena_, "DeviceState")(arena_);
	
	// we need a root sig to compile this PSO with.
	// but it don't have to be the rootSig we render with.
	RootSignature& rootSig = pState->rootSig;
	rootSig.reset(1, 0);
	rootSig.getParamRef(0).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, TextureSlot::ENUM_COUNT, D3D12_SHADER_VISIBILITY_PIXEL);
//	rootSig.getParamRef(1).initAsConstants(0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
//	rootSig.getParamRef(2).initAsSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
//	rootSig.getParamRef(3).initAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1);

	rootSig.finalize(*pRootSigCache_, 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		// should i just disable these for now?
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
		| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

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
	createDescFromState(state, blendDesc);
	createDescFromState(state, rasterizerDesc);
	createDescFromState(state, stencilState, depthStencilDesc);

	pso.setRootSignature(rootSig);
	pso.setBlendState(blendDesc);
	pso.setRasterizerState(rasterizerDesc);
	pso.setDepthStencilState(depthStencilDesc);
	pso.setSampleMask(0xFFFFFFFF);
	pso.setRenderTargetFormats(static_cast<uint32_t>(pPassState->rtfs.size()), RTVFormats, DXGI_FORMAT_UNKNOWN, 1, 0);
	pso.setPrimitiveTopologyType(topoTypeFromDesc(desc));

	const auto& inputDesc = ilDescriptions_[desc.vertexFmt];
	pso.setInputLayout(inputDesc.size(), inputDesc.ptr());

	shader::XShaderTechniqueHW* pHWTech = nullptr;
	if (pHWTech->pVertexShader) {
		const auto& byteCode = pHWTech->pVertexShader->getShaderByteCode();
		pso.setVertexShader(byteCode.data(), byteCode.size());
	}
	if (pHWTech->pPixelShader) {
		const auto& byteCode = pHWTech->pPixelShader->getShaderByteCode();
		pso.setPixelShader(byteCode.data(), byteCode.size());
	}
	if (pHWTech->pDomainShader || pHWTech->pHullShader || pHWTech->pGeoShader) {
		// in order to allow these check if the root sig flags need changing then just duplicate 
		// the bytecode setting logic.
		X_ERROR("Dx12", "Domain, Hull, Geo are not enabled currently");
	}

	pso.finalize(*pPSOCache_);


	pState->pPso = pso.getPipelineStateObject();
	pState->topo = topoFromDesc(desc);
	pState->texStates.resize(numStates);
	std::memcpy(pState->texStates.data(), pTextStates, sizeof(TextureState) * numStates);

	return reinterpret_cast<StateHandle>(pState);
}

void XRender::destoryState(StateHandle handle)
{
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

	X_LOG0("Dx12", "MaxVertexTextureUnits: %" PRIu8, features_.maxVertexTextureUnits);
	X_LOG0("Dx12", "MaxPixelTextureUnits: %" PRIu8, features_.maxPixelTextureUnits);
	X_LOG0("Dx12", "MaxGeometryTextureUnits: %" PRIu8, features_.maxGeometryTextureUnits);
	X_LOG0("Dx12", "MaxSimultaneousRts: %" PRIu8, features_.maxSimultaneousRts);
	X_LOG0("Dx12", "MaxSimultaneousUavs: %" PRIu8, features_.maxSimultaneousUavs);
	X_LOG0("Dx12", "MaxVertexStreams: %" PRIu8, features_.maxVertexStreams);
	X_LOG0("Dx12", "MaxTextureAnisotropy: %" PRIu8, features_.maxTextureAnisotropy);

	X_LOG0("Dx12", "TileBasedRenderer: %s", features_.isTbdr ? "true" : "false");
	X_LOG0("Dx12", "UnifiedMemoryArchitecture: %s", features_.isUMA ? "true" : "false");

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





// =============================================
// ============== OLD API ======================
// =============================================


//void XRender::SetState(StateFlag state)
//{
//	X_UNUSED(state);
//}

//void XRender::SetStencilState(StencilState::Value ss)
//{
//	X_UNUSED(ss);
//}

//void XRender::SetCullMode(CullMode::Enum mode)
//{
//	X_UNUSED(mode);
//}

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


//void XRender::DrawVB(Vertex_P3F_T2F_C4B* pVertBuffer, uint32_t size,
//	PrimitiveTypePublic::Enum type)
//{
//	X_ASSERT_NOT_NULL(pVertBuffer);
//
//	X_UNUSED(pVertBuffer);
//	X_UNUSED(size);
//	X_UNUSED(type);
//
//}


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
#if 0
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
#endif

void XRender::SetModelMatrix(const Matrix44f& mat)
{
	X_UNUSED(mat);
}

// ~Model



X_NAMESPACE_END