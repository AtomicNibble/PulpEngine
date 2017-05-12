#include "stdafx.h"
#include "Dx10Render.h"

#include "Util\ToggleChecker.h"

// #include "../Common/Textures/XTextureFile.h"
#include "../Common/Textures/XTexture.h"
#include "../Common/Shader/XShader.h"
#include "Dx10Shader.h"

#include "Dx10RenderAux.h"

X_NAMESPACE_BEGIN(render)


DX11XRender g_Dx11D3D;

DX11XRender::DX11XRender()  :
#if X_DEBUG
	d3dDebug_(nullptr),
#endif

	device_(nullptr),
	deviceContext_(nullptr),

	swapChain_(nullptr),
	renderTargetView_(nullptr),
	depthStencilBuffer_(nullptr),
	depthStencilViewReadOnly_(nullptr),
	depthStencilView_(nullptr),

	CurTopology_(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED),

	ViewMat_(nullptr),
	ProMat_(nullptr),

	BlendStates_(nullptr),
	RasterStates_(nullptr),
	DepthStates_(nullptr),
	AuxGeo_(nullptr),

	CurBlendState_((uint32_t)-1),
	CurRasterState_((uint32_t)-1),
	CurDepthState_((uint32_t)-1)
{

	gRenDev = this;
}

DX11XRender::~DX11XRender()
{

}


void DX11XRender::SetArenas(core::MemoryArenaBase* arena)
{
	X_ASSERT_NOT_NULL(arena);
//	int i;

	XRender::SetArenas(arena);

	BlendStates_.setArena(arena);
	RasterStates_.setArena(arena);
	DepthStates_.setArena(arena);

	ViewMat_.SetArena(arena);
	ProMat_.SetArena(arena);

//	for (i = 0; i < shader::VertexFormat::Num; i++)
//		State_.vertexLayoutDescriptions[i].setArena(arena);

}

bool DX11XRender::Init(HWND hWnd, 
	uint32_t width, uint32_t hieght)
{
	float screenWidth = static_cast<float>(width);
	float screenHeight = static_cast<float>(hieght);

//	float screenDepth = 1000.f;
//	float screenNear = 1.f;

	if (!XRender::Init(hWnd, width, hieght)) {
		return false;
	}

	if (hWnd == static_cast<HWND>(0))
	{
		X_ERROR("dx10", "target window is not valid");
		return false;
	}

	SetArenas(g_rendererArena);


	ViewMat_.SetDepth(16);
	ProMat_.SetDepth(16);


	float fieldOfView, screenAspect;
	ID3D11Texture2D* backBufferPtr = nullptr;
	HRESULT result;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	DepthState depthStencil;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_VIEWPORT viewport;
	RasterState raster;
	BlendState  blend;
//	D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc;

	// Zero my nipples
	core::zero_object(swapChainDesc);
	core::zero_object(depthBufferDesc);
	core::zero_object(depthStencilViewDesc);
	core::zero_object(viewport);
//	core::zero_object(depthDisabledStencilDesc);


	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = static_cast<UINT>(screenWidth);
	swapChainDesc.BufferDesc.Height = static_cast<UINT>(screenHeight);

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hWnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	swapChainDesc.Windowed = true;

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;


	D3D_FEATURE_LEVEL  FeatureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};
	D3D_FEATURE_LEVEL featureout;
	const UINT numLevelsRequested = sizeof(FeatureLevels) / sizeof(D3D_FEATURE_LEVEL);

#if X_DEBUG 
	bool debugNotAvaliable = false;
#endif // !X_DEBUG

	// Create the swap chain and the Direct3D device.
	result = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
#if X_DEBUG
		D3D11_CREATE_DEVICE_DEBUG,
#else
		0,
#endif // !X_DEBUG
		FeatureLevels,
		numLevelsRequested,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&swapChain_, 
		&device_,
		&featureout,
		&deviceContext_
	);

#if X_DEBUG 
	if (FAILED(result) && result == 0x887a002d)
	{
		result = D3D11CreateDeviceAndSwapChain(
			NULL,
			D3D_DRIVER_TYPE_HARDWARE,
			NULL,
			0,
			FeatureLevels,
			numLevelsRequested,
			D3D11_SDK_VERSION,
			&swapChainDesc,
			&swapChain_,
			&device_,
			&featureout,
			&deviceContext_
		);

		if (SUCCEEDED(result)) {
			debugNotAvaliable = true;
			X_WARNING("Dx10", "Unable to create debug device!");
		}
	}
#endif // !X_DEBUG

	if (FAILED(result))
	{
		X_ERROR("Dx10", "Failed to CreateDevice: 0x%x", result);
		return false;
	}

	// Get the pointer to the back buffer.
	result = swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(result))
	{
		X_ERROR("Dx10", "Failed to get back buffer: 0x%x", result);
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = device_->CreateRenderTargetView(backBufferPtr, NULL, &renderTargetView_);
	if (FAILED(result))
	{
		X_ERROR("Dx10", "Failed to create render target view: 0x%x", result);
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = 0;

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = static_cast<UINT>(screenWidth);
	depthBufferDesc.Height = static_cast<UINT>(screenHeight);
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	result = device_->CreateTexture2D(&depthBufferDesc, NULL, &depthStencilBuffer_);
	if (FAILED(result))
	{
		X_ERROR("Dx10", "Failed to create depth buffer: 0x%x", result);
		return false;
	}



	// Set up the description of the stencil state.
	depthStencil.Desc.DepthEnable = true;
	depthStencil.Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencil.Desc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencil.Desc.StencilEnable = false;
	depthStencil.Desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depthStencil.Desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	// Stencil operations if pixel is front-facing.
	depthStencil.Desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencil.Desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencil.Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	depthStencil.Desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencil.Desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencil.Desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencil.Desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	depthStencil.Desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;


	// Create the depth stencil state.
	if (!this->SetDepthState(depthStencil))
	{
		X_ERROR("Dx10", "Failed to set depth states");
		return false;
	}


	blend.Desc.AlphaToCoverageEnable =	false;
	blend.Desc.IndependentBlendEnable =	false;
	blend.Desc.RenderTarget[0].BlendEnable =	false;
	blend.Desc.RenderTarget[0].SrcBlend =	D3D11_BLEND_ONE;
	blend.Desc.RenderTarget[0].DestBlend =	D3D11_BLEND_ZERO;
	blend.Desc.RenderTarget[0].BlendOp	 =	D3D11_BLEND_OP_ADD;
	blend.Desc.RenderTarget[0].SrcBlendAlpha =	D3D11_BLEND_ONE;
	blend.Desc.RenderTarget[0].DestBlendAlpha =	D3D11_BLEND_ZERO;
	blend.Desc.RenderTarget[0].BlendOpAlpha =	D3D11_BLEND_OP_ADD;
	blend.Desc.RenderTarget[0].RenderTargetWriteMask =	D3D11_COLOR_WRITE_ENABLE_ALL;

	// Create the depth stencil state.
	if (!this->SetBlendState(blend))
	{
		X_ERROR("Dx10", "Failed to blend states");
		return false;
	}



	// Gbuffer Stencil state.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	depthStencilViewDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH;

	// Create the depth stencil view.
	result = device_->CreateDepthStencilView(depthStencilBuffer_, &depthStencilViewDesc, &depthStencilViewReadOnly_);
	if (FAILED(result))
	{
		X_ERROR("Dx10", "Failed to set deptch stencil view: 0x%x", result);
		return false;
	}


	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	depthStencilViewDesc.Flags = 0;

	// Create the depth stencil view.
	result = device_->CreateDepthStencilView(depthStencilBuffer_, &depthStencilViewDesc, &depthStencilView_);
	if (FAILED(result))
	{
		X_ERROR("Dx10", "Failed to create depth stencil view: 0x%x", result);
		return false;
	}

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	deviceContext_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);

	// Setup the raster description which will determine how and what polygons will be drawn.
	raster.Desc.AntialiasedLineEnable = false;
	raster.Desc.FillMode = D3D11_FILL_SOLID;
	raster.Desc.CullMode =  D3D11_CULL_BACK;
	raster.Desc.DepthBias = 0;
	raster.Desc.SlopeScaledDepthBias = 0.0f;
	raster.Desc.DepthBiasClamp = 0.0f;
	raster.Desc.DepthClipEnable = true;
	raster.Desc.FrontCounterClockwise = false;
	raster.Desc.MultisampleEnable = false;
	raster.Desc.ScissorEnable = false;
	

	// Create the rasterizer state from the description we just filled out.
	// result = device_->CreateRasterizerState(&rasterDesc, &m_rasterState);
	if (!SetRasterState(raster))
	{
		X_ERROR("Dx10", "Failed to set raster state");
		return false;
	}


	SetStencilState(
		StencilState::create(
		StencilState::FUNC_ALWAYS,
		StencilState::OP_KEEP,
		StencilState::OP_KEEP,
		StencilState::OP_KEEP
		)
	);

	SetCullMode(CullMode::BACK);


	// Setup the viewport for rendering.
	viewport.Width = screenWidth;
	viewport.Height = screenHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;


	// Create the viewport.
	deviceContext_->RSSetViewports(1, &viewport);

	// Setup the projection matrix.
	fieldOfView = PIf / 4.0f;
	screenAspect = screenWidth / screenHeight;

	if (!OnPostCreateDevice()) {
		X_ERROR("Dx10", "Post device creation operations failed");
		return false;
	}

#if X_DEBUG
	if (debugNotAvaliable) {
		return true;
	}

	result = device_->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3dDebug_));
	if (SUCCEEDED(result))
	{
		return true;
	}
	// where is my debug interface slut!
	X_ERROR("Dx10", "Failed to create debug interface");
	return false;
#else
	return true;
#endif
}


void DX11XRender::ShutDown(void)
{
	size_t i;

	// needs clearingh before zrender::Shutdown.
	// as we have VB / IB to free.
	if (AuxGeo_) {
		AuxGeo_->ReleaseDeviceObjects();
		// might move
		X_DELETE(AuxGeo_, g_rendererArena);
	}

	FreeDynamicBuffers();

	XRender::ShutDown();
	shader::XHWShader_Dx10::shutDown();


	// clear the stacks
	ViewMat_.Clear();
	ProMat_.Clear();

	if (DxDeviceContext()) {
		DxDeviceContext()->OMSetBlendState(nullptr, 0, 0xFFFFFFFF);
		DxDeviceContext()->OMSetDepthStencilState(nullptr, 0);
		DxDeviceContext()->OMSetDepthStencilState(nullptr, 0);
	}

	for (i = 0; i < BlendStates_.size(); ++i)
		BlendStates_[i].pState->Release();
	for (i = 0; i < RasterStates_.size(); ++i)
		RasterStates_[i].pState->Release();
	for (i = 0; i < DepthStates_.size(); ++i)
		DepthStates_[i].pState->Release();

	BlendStates_.free();
	RasterStates_.free();
	DepthStates_.free();

	RenderResources_.free();

	State_.~RenderState();
	// --------------------------


	// Before shutting down set to windowed mode or when releasing the swap chain it will throw an exception.
	if (swapChain_)
	{
		swapChain_->SetFullscreenState(false, NULL);
	}

	if (depthStencilView_)
	{
		depthStencilView_->Release();
		depthStencilView_ = 0;
	}

	if (depthStencilBuffer_)
	{
		depthStencilBuffer_->Release();
		depthStencilBuffer_ = 0;
	}

	if (renderTargetView_)
	{
		renderTargetView_->Release();
		renderTargetView_ = 0;
	}

	if (swapChain_)
	{
		swapChain_->Release();
		swapChain_ = 0;
	}

	if (deviceContext_)
	{
		deviceContext_->ClearState();
		deviceContext_->Flush();
		deviceContext_->Release();
		deviceContext_ = nullptr;
	}

	if (device_)
	{
		device_->Release();
		device_ = 0;
	}

#if X_DEBUG
	if (d3dDebug_)
	{
		d3dDebug_->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_SUMMARY);
		d3dDebug_->Release();
	}
	else
	{
		X_WARNING("D3D", "Debug interface is invalid");
	}
#endif // !X_DEBUG

}

bool DX11XRender::OnPostCreateDevice(void)
{
	shader::XHWShader_Dx10::Init();

	if (!ShaderMan_.Init()) {
		return false;
	}

	FX_Init();

	InitDynamicBuffers();

	InitResources();


	AuxGeo_ = X_NEW(XRenderAuxImp, g_rendererArena, "AuxGeo")(*this);
	AuxGeo_->RestoreDeviceObjects();
	return true;
}

void DX11XRender::InitResources(void)
{
	if (LoadResourceDeffintion())
	{
		// create the render targets.

		for (auto target : RenderResources_)
		{
			

		}

	}
}



void DX11XRender::InitDynamicBuffers(void)
{
	int i;
	for (i = 0; i<VertexPool::PoolMax; i++)
	{
		int vertSize, numVerts;
		vertSize = 0;
		numVerts = 0;
		switch (i)
		{
			case VertexPool::P3F_T2F_C4B:
				numVerts = 32768 / 4;
				vertSize = sizeof(Vertex_P3F_T2F_C4B);
				break;
			case VertexPool::P3F_T2S_C4B:
				numVerts = 32768 / 4;
				vertSize = sizeof(Vertex_P3F_T2S_C4B);
				break;
			default:
#if X_DEBUG
				X_ASSERT_UNREACHABLE();
				break;

#else
				X_NO_SWITCH_DEFAULT;
				break;
#endif // !X_DEBUG

		}

		DynVB_[i].CreateVB(&vidMemMng_, numVerts, vertSize);
	}
}


void DX11XRender::FreeDynamicBuffers(void)
{
	int i;
	for (i = 0; i<VertexPool::PoolMax; i++)
	{
		DynVB_[i].Release();
	}
}



void DX11XRender::RenderBegin(void)
{
	X_PROFILE_BEGIN("DXRenderBegin", core::profiler::SubSys::RENDER);

	XRender::RenderBegin();

	X_ASSERT(ViewMat_.getDepth() == 0, "stack depth is not zero at start of frame")(ViewMat_.getDepth());
	X_ASSERT(ProMat_.getDepth() == 0, "stack depth is not zero at start of frame")(ProMat_.getDepth());

	texture::XTexture::setDefaultFilterMode(shader::FilterMode::TRILINEAR);

	// Clear the back buffer.
	deviceContext_->ClearRenderTargetView(renderTargetView_, r_clear_color);
	// Clear the depth buffer.
	deviceContext_->ClearDepthStencilView(depthStencilView_, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


}

void DX11XRender::RenderEnd(void)
{
	X_PROFILE_BEGIN("DXRenderEnd", core::profiler::SubSys::RENDER);
	XRender::RenderEnd();


	if (AuxGeo_) {
	//	AuxGeo_->GetRenderAuxGeom()->flush();
		AuxGeo_->GetRenderAuxGeom()->Reset();
	}


	swapChain_->Present(0, 0);
}


bool DX11XRender::DefferedBegin(void)
{
	using namespace texture;

	ID3D11RenderTargetView* pViews[3] = {
		XTexture::s_GBuf_Albedo->getRenderTargetView(),
		XTexture::s_GBuf_Normal->getRenderTargetView(),
		XTexture::s_GBuf_Depth->getRenderTargetView(),
	};

	Colorf clear_col(0.0f, 0.f, 0.0f);

	deviceContext_->ClearRenderTargetView(pViews[0], clear_col);
	deviceContext_->ClearRenderTargetView(pViews[1], clear_col);
	deviceContext_->ClearRenderTargetView(pViews[2], clear_col);


	SetZPass();

	deviceContext_->OMSetRenderTargets(3, pViews, depthStencilView_);

	return true;
}

bool DX11XRender::DefferedEnd(void)
{
	using namespace shader;
	using namespace texture;

	// the shader
	XShader* pSh = XShaderManager::s_pDefferedShaderVis_;
	uint32_t pass;


	// switch the render target back
	ID3D11RenderTargetView* pViewsReset[3] = {
		renderTargetView_,
		NULL,
		NULL,
	};

	deviceContext_->OMSetRenderTargets(3, pViewsReset, depthStencilView_);

	SetCullMode(CullMode::NONE);

	core::StrHash albedo("VisualizeAlbedo");
	core::StrHash normal("VisualizeNormals");
	core::StrHash depth("VisualizeDepth");
	
	// depth, normal, albedo

	XTexture::s_GBuf_Depth->apply(0);
	XTexture::s_GBuf_Normal->apply(1);
	XTexture::s_GBuf_Albedo->apply(2);

	pSh->FXSetTechnique(albedo);
	pSh->FXBegin(&pass, 0);
	pSh->FXBeginPass(pass);

	DrawQuadImage(
		-1.0f, -0.4f, 0.6f, -0.6f, 
		0, Col_White);

	pSh->FXSetTechnique(normal);
	pSh->FXBegin(&pass, 0);
	pSh->FXBeginPass(pass);

	DrawQuadImage(
		-0.4f, -0.4f, 0.6f, -0.6f,
		0, Col_White);

	pSh->FXSetTechnique(depth);
	pSh->FXBegin(&pass, 0);
	pSh->FXBeginPass(pass);


	DrawQuadImage(
		0.2f, -0.4f, 0.6f, -0.6f,
		0, Col_White);


	XTexture::s_GBuf_Albedo->unbind();
	XTexture::s_GBuf_Normal->unbind();
	XTexture::s_GBuf_Depth->unbind();


	return true;
}





void DX11XRender::Set2D(bool enable, float znear, float zfar)
{
	Matrix44f* m;

	static core::ToggleChecker in2dAlready(false);

	// assets if already in 2d mode.
	// might make it part of the ASSERT macro's
	in2dAlready = enable;

	if (enable) 
	{
		// make a matrix that maps shit on to the screen.
		// we need the screen dimensions so that we can multiple 
		// the values by the correct amount.
		
		ProMat_.Push();
		m = ProMat_.GetTop();

		float width = getWidthf();
		float height = getHeightf();

		MatrixOrthoOffCenterRH(m, 0, width, height, 0, znear, zfar);

		// want a identiy view.
		PushViewMatrix();
		ViewMat_.LoadIdentity();


	} else {
		ProMat_.Pop();
		PopViewMatrix();
	}

	SetCameraInfo();
}

// Camera


void DX11XRender::SetCameraInfo(void)
{
	// calculate the current camera shit.

	GetModelViewMatrix(&ViewMatrix_);
	GetProjectionMatrix(&ProjMatrix_);


//	ViewProjMatrix_ = ViewMatrix_ * ProjMatrix_;
	ViewProjMatrix_ = ProjMatrix_ * ViewMatrix_;
	ViewProjInvMatrix_ = ViewProjMatrix_.inverted();

	// if the camera has changed then we need to tell the shader system 
	// that the cameras are dirty!
	// instead of updating the cbuf's
	// since if this function is called multiple times with no drawining.
	// be wasted cbuffer updates.
	// this also means camera cbuffer should not
	// be updated when teh camera has not changed.

	shader::XHWShader_Dx10::SetCameraDirty();
}



void DX11XRender::SetCamera(const XCamera& cam)
{
	float ProjectionRatio = cam.getProjectionRatio();
	float fov = cam.getFov();

//	float wT = math<float>::tan(fov*0.5f)*cam.GetNearPlane();
//	float wB = -wT;
//	float wR = wT * ProjectionRatio;
//	float wL = -wR;

	Matrix34f m = cam.getMatrix();
	Vec3f vEye = cam.getPosition();
	Vec3f vAt = vEye + Vec3f(m.m01, m.m11, m.m21);
	Vec3f vUp = Vec3f(m.m02, m.m12, m.m22);
//	Vec3f vUp = Vec3f(0,0,1);

	// View
	Matrix44f* pView = ViewMat_.GetTop();
	MatrixLookAtRH(pView, vEye, vAt, vUp);

	// Proj
	Matrix44f* pProj = ProMat_.GetTop();
	MatrixPerspectiveFovRH(pProj, fov, ProjectionRatio, 1.0f, 6000.0f);


	cam_ = cam;

	SetCameraInfo();
}



// ~Camera



// Textures 

bool DX11XRender::Create2DTexture(texture::XTextureFile* img_data, texture::XDeviceTexture& dev_tex)
{
	X_ASSERT_NOT_NULL(img_data);

	D3D11_TEXTURE2D_DESC Desc;
	D3D11_SUBRESOURCE_DATA SubResource[20];
	D3D11_SUBRESOURCE_DATA* pSubResource;
	ID3D11Texture2D* pTexture2D;

	core::zero_object(Desc);
	core::zero_object(SubResource);

	uint32 nBindFlags = D3D11_BIND_SHADER_RESOURCE;
	uint32 nMiscFlags = 0;
	int i;
	HRESULT hr;

	if (img_data->getFlags().IsSet(texture::TexFlag::RENDER_TARGET))
		nBindFlags |= D3D11_BIND_RENDER_TARGET;



	if (img_data->getType() == texture::TextureType::T2D)
	{
		X_ASSERT(img_data->getNumFaces() == 1, "invalid number of faces for a 2d image")(img_data->getNumFaces());
	}
	else if (img_data->getType() == texture::TextureType::TCube)
	{
		X_ASSERT(img_data->getNumFaces() == 6, "invalid number of faces for a 2d image")(img_data->getNumFaces());
	}
	else
	{
		// this texture type should not be in here.
		X_ASSERT_UNREACHABLE();
		return false;
	}


	Desc.Height = img_data->getHeight();
	Desc.Width = img_data->getWidth();
	Desc.MipLevels = img_data->getNumMips();
	Desc.ArraySize = img_data->getNumFaces();
	Desc.Format = texture::XTexture::DCGIFormatFromTexFmt(img_data->getFormat());
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = nBindFlags;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = nMiscFlags;

	// copy info into required directx format.
	if (img_data->getFlags().IsSet(texture::TexFlag::RENDER_TARGET))
	{
		// must be a null pointer.
		pSubResource = nullptr;
	}
	else if (img_data->getType() == texture::TextureType::TCube)
	{
		pSubResource = &SubResource[0];
		for (i = 0; i < img_data->getNumFaces(); i++)
		{
#if 1
			X_ASSERT_NOT_IMPLEMENTED();
#else
			X_ASSERT(img_data->SubInfo[i].pSysMem != nullptr, "system mem must be set for all faces")();

			SubResource[i].pSysMem = img_data->SubInfo[i].pSysMem;
			SubResource[i].SysMemPitch = img_data->SubInfo[i].SysMemPitch;
			SubResource[i].SysMemSlicePitch = 0; // img_data->SubInfo[0].SysMemSlicePitch;
#endif
		}
	}
	else
	{
		pSubResource = &SubResource[0];
		for (i = 0; i < img_data->getNumMips(); i++)
		{
#if 1
			X_ASSERT_NOT_IMPLEMENTED();
#else
			SubResource[i].pSysMem = img_data->SubInfo[i].pSysMem;
			SubResource[i].SysMemPitch = img_data->SubInfo[i].SysMemPitch;
			SubResource[i].SysMemSlicePitch = img_data->SubInfo[i].SysMemSlicePitch;
#endif

		}
	}

	hr = device_->CreateTexture2D(&Desc, pSubResource, &pTexture2D);


	// worky?
	if (SUCCEEDED(hr))
	{
		dev_tex.setTexture(pTexture2D);
#if X_DEBUG
		X_ASSERT_NOT_IMPLEMENTED();

//		D3DDebug::SetDebugObjectName(pTexture2D, img_data->getName());
#endif // !X_DEBUG

		return true;
	}

	return false;
}




void DX11XRender::ReleaseTexture(texture::TexID id)
{
	if (id < 0)
		return;

	texture::ITexture* pTex = texture::XTexture::getByID(id);

	core::SafeRelease(pTex);
}


bool DX11XRender::SetTexture(int texId)
{
	texture::XTexture* pTex = texture::XTexture::getByID(texId);

	// should not be null.
	X_ASSERT_NOT_NULL(pTex);

	pTex->apply(0);
	return true;
}

// ~Textures 

// Shaders 

void DX11XRender::FX_PipelineShutdown(void)
{

	shader::XHWShader_Dx10::shutDown();

}

// ~Shaders

void DX11XRender::FX_Init(void)
{
	InitILDescriptions();

}


void DX11XRender::InitILDescriptions(void)
{
	using namespace shader;

	// build all the diffrent layouts.
	int i;
	const int max = VertexFormat::Num;

	D3D11_INPUT_ELEMENT_DESC elem_pos = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_nor101010 = { "NORMAL", 0, DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
//6	D3D11_INPUT_ELEMENT_DESC elem_nor8888 = { "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_nor323232 = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_col8888 = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_uv3232 = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_uv1616 = { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
//	D3D11_INPUT_ELEMENT_DESC elem_uv32323232 = { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	D3D11_INPUT_ELEMENT_DESC elem_t3f = { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	D3D11_INPUT_ELEMENT_DESC elem_tagent101010 = { "TANGENT", 0, DXGI_FORMAT_R10G10B10A2_TYPELESS, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_tagent323232 = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_biNormal101010 = { "BINORMAL", 0, DXGI_FORMAT_R10G10B10A2_TYPELESS, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_biNormal323232 = { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };


	for (i = 0; i < max; i++)
	{
		RenderState::XVertexLayout& layout = State_.ILDescriptions[i];

		// for now all positions are just 32bit floats baby!
		elem_pos.AlignedByteOffset = 0;
		elem_pos.SemanticIndex = 0;
		elem_uv3232.SemanticIndex = 0;
		layout.append(elem_pos);

		if (i == VertexFormat::P3F_T2S || i == VertexFormat::P3F_T2S_C4B ||
			i == VertexFormat::P3F_T2S_C4B_N3F || i == VertexFormat::P3F_T2S_C4B_N3F_TB3F || 
			i == VertexFormat::P3F_T2S_C4B_N10 || i == VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_uv1616.AlignedByteOffset = 12;
			layout.append(elem_uv1616);
		}
		if (i == VertexFormat::P3F_T2S_C4B ||
			i == VertexFormat::P3F_T2S_C4B_N3F || i == VertexFormat::P3F_T2S_C4B_N3F_TB3F ||
			i == VertexFormat::P3F_T2S_C4B_N10 || i == VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_col8888.AlignedByteOffset = 12 + 4;
			layout.append(elem_col8888);
		}

		if (i == VertexFormat::P3F_T2S_C4B_N3F || i == VertexFormat::P3F_T2S_C4B_N3F_TB3F)
		{
			elem_nor323232.AlignedByteOffset = 12 + 4 + 4;
			layout.append(elem_nor323232); // 12 bytes
		}
		if (i == VertexFormat::P3F_T2S_C4B_N3F_TB3F)
		{
			elem_tagent323232.AlignedByteOffset = 12 + 4 + 4 + 12;
			layout.append(elem_tagent323232); // 12 bytes

			elem_biNormal323232.AlignedByteOffset = 12 + 4 + 4 + 12 + 12;
			layout.append(elem_biNormal323232); // 12 bytes
		}

		if (i == VertexFormat::P3F_T2F_C4B)
		{
			elem_uv3232.AlignedByteOffset = 12;
			layout.append(elem_uv3232);

			elem_col8888.AlignedByteOffset = 20;
			layout.append(elem_col8888);

		}
		else if(i == VertexFormat::P3F_T3F)
		{
			elem_t3f.AlignedByteOffset = 12;
			layout.append(elem_t3f);
		}


		if (i == VertexFormat::P3F_T2S_C4B_N10 || i == VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			// 12 + 4 + 4
			elem_nor101010.AlignedByteOffset = 20;
			layout.append(elem_nor101010);
		}
		if (i == VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_tagent101010.AlignedByteOffset = 24;
			layout.append(elem_tagent101010);
			elem_biNormal101010.AlignedByteOffset = 28;
			layout.append(elem_biNormal101010);
		}

		if (i == VertexFormat::P3F_T4F_C4B_N3F)
		{
			// big man texcoords
			elem_uv3232.AlignedByteOffset = 12;
			layout.append(elem_uv3232);

			// two of them
			elem_uv3232.AlignedByteOffset = 20;
			elem_uv3232.SemanticIndex = 1;
			layout.append(elem_uv3232);
			elem_uv3232.SemanticIndex = 0;

			// byte offset is zero since diffrent stream.
			elem_col8888.AlignedByteOffset = 28;
			layout.append(elem_col8888);

			elem_nor323232.AlignedByteOffset = 32;
			layout.append(elem_nor323232); 
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


	for (i = 0; i < max; i++)
	{
		RenderState::XVertexLayout& layout = State_.streamedILDescriptions[i];

		// Streams
		// Vert + uv
		// Color
		// Normal
		// Tan + Bi

		elem_pos.AlignedByteOffset = 0;
		elem_pos.SemanticIndex = 0;
		elem_uv3232.SemanticIndex = 0;
		layout.append(elem_pos);

		// uv
		if (i == VertexFormat::P3F_T2S || i == VertexFormat::P3F_T2S_C4B ||
			i == VertexFormat::P3F_T2S_C4B_N3F || i == VertexFormat::P3F_T2S_C4B_N3F_TB3F ||
			i == VertexFormat::P3F_T2S_C4B_N10 || i == VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_uv1616.AlignedByteOffset = 12;
			layout.append(elem_uv1616);
		}

		// col
		if (i == VertexFormat::P3F_T2S_C4B ||
			i == VertexFormat::P3F_T2S_C4B_N3F || i == VertexFormat::P3F_T2S_C4B_N3F_TB3F ||
			i == VertexFormat::P3F_T2S_C4B_N10 || i == VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			// seperate stream
			elem_col8888.AlignedByteOffset = 0;
			elem_col8888.InputSlot = 1;
			layout.append(elem_col8888);
		}

		// nor
		if (i == VertexFormat::P3F_T2S_C4B_N3F || i == VertexFormat::P3F_T2S_C4B_N3F_TB3F)
		{
			elem_nor323232.AlignedByteOffset = 0;
			elem_nor323232.InputSlot = 2;
			layout.append(elem_nor323232); // 12 bytes
		}
		//  tan + bi
		if (i == VertexFormat::P3F_T2S_C4B_N3F_TB3F)
		{
			elem_tagent323232.InputSlot = 3;
			elem_tagent323232.AlignedByteOffset = 0;
			layout.append(elem_tagent323232); // 12 bytes

			elem_biNormal323232.InputSlot = 3;
			elem_biNormal323232.AlignedByteOffset = 12;
			layout.append(elem_biNormal323232); // 12 bytes
		}

		// 32 bit floats
		if (i == VertexFormat::P3F_T2F_C4B)
		{
			elem_uv3232.AlignedByteOffset = 12;
			layout.append(elem_uv3232);

			elem_col8888.InputSlot = 1;
			elem_col8888.AlignedByteOffset = 0;
			layout.append(elem_col8888);
		}
		else if (i == VertexFormat::P3F_T3F)
		{
			elem_t3f.AlignedByteOffset = 12;
			layout.append(elem_t3f);
		}


		if (i == VertexFormat::P3F_T2S_C4B_N10 || i == VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			// 12 + 4 + 4
			elem_nor101010.InputSlot = 2;
			elem_nor101010.AlignedByteOffset = 0;
			layout.append(elem_nor101010);
		}
		if (i == VertexFormat::P3F_T2S_C4B_N10_TB10)
		{
			elem_tagent101010.InputSlot = 3;
			elem_tagent101010.AlignedByteOffset = 0;
			layout.append(elem_tagent101010);

			elem_biNormal101010.InputSlot = 3;
			elem_biNormal101010.AlignedByteOffset = 4;
			layout.append(elem_biNormal101010);
		}

		if (i == VertexFormat::P3F_T4F_C4B_N3F)
		{
			// big man texcoords
			elem_uv3232.AlignedByteOffset = 12;
			layout.append(elem_uv3232);

			// two of them
			elem_uv3232.AlignedByteOffset = 20;
			elem_uv3232.SemanticIndex = 1;
			layout.append(elem_uv3232);
			elem_uv3232.SemanticIndex = 0;

			// byte offset is zero since diffrent stream.
			elem_col8888.AlignedByteOffset = 0;
			elem_col8888.InputSlot = 1;
			layout.append(elem_col8888);

			elem_nor323232.AlignedByteOffset = 0;
			elem_nor323232.InputSlot = 2;
			layout.append(elem_nor323232);
		}
	}


	// Skinning!
	// I support 4 bones per vert.
	// so 4 weights and 4 indexs.
	// i support 255 bones so 8bits can fit the bone index.
#if 0
	static_assert(model::MODEL_MAX_BONES <= 4, "code here only supports 4 bones");
	static D3D11_INPUT_ELEMENT_DESC elem_skinning[] =
	{
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, VertexStream::HWSKIN, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDEXS", 0, DXGI_FORMAT_R8G8B8A8_UNORM, VertexStream::HWSKIN, 4, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
#endif


}


ID3D11InputLayout* DX11XRender::CreateILFromDesc(const shader::VertexFormat::Enum vertexFmt,
	const RenderState::XVertexLayout& layout)
{
	ID3D11InputLayout* pInputLayout = nullptr;
	ID3DBlob* pBlob = nullptr;
	HRESULT hr;

	if (layout.isEmpty())
	{
		X_ERROR("Render", "Failed to set input 'layout description' is empty. fmt: %i", vertexFmt);
		return nullptr;
	}

	// need the current shaders byte code / length.
	// lets not require a Hardware shader bet set.
	// only a current tech.
#if 1
	if (!State_.pCurShader || !State_.pCurShaderTech) {
		X_ERROR("Render", "Failed to set input layout no shader currently set.");
		return nullptr;
	}

	// get the required ILfmt for the vertex fmt.
	shader::InputLayoutFormat::Enum requiredIlFmt = shader::ILfromVertexFormat(vertexFmt);
	// get the tech
	shader::XShaderTechnique* pTech = State_.pCurShaderTech;
	// find one that fits.
	for (auto& it : pTech->hwTechs)
	{
		if (it.IlFmt == requiredIlFmt)
		{
			shader::XHWShader_Dx10* pVs;
			if (!it.pVertexShader) {
				X_ERROR("Render", "Failed to set input layout, tech VS is null");
				return nullptr;
			}

			pVs = reinterpret_cast<shader::XHWShader_Dx10*>(it.pVertexShader);

			// make sure it's compiled.
			if (!pVs->activate()) {
				X_ERROR("Render", "Failed to set input layout, VS failed to compile");
				return nullptr;
			}

			pBlob = pVs->getshaderBlob();
			break;
		}
	}

	if (!pBlob)
	{
		X_ERROR("Render", "Failed to set input layout shader does not support input layout: %s",
			shader::InputLayoutFormat::ToString(requiredIlFmt));
		return nullptr;
	}

#else
	if (!shader::XHWShader_Dx10::pCurVS_)
	{
		X_ERROR("Render", "Failed to set input layout no shader currently set.");
		return (HRESULT)-1;
	}

	ID3DBlob* pBlob = shader::XHWShader_Dx10::pCurVS_->getshaderBlob();
#endif

	if (FAILED(hr = device_->CreateInputLayout(
		layout.ptr(),
		static_cast<uint>(layout.size()),
		pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(),
		&pInputLayout))
		)
	{
		const char* pShaderName = shader::XHWShader_Dx10::pCurVS_->getName();
		X_ERROR("Render", "Failed to CreateInputLayout: %i", hr);
		X_ERROR("Render", "CurrentShader: %s", pShaderName);
		X_ERROR("Render", "Layout:");
		X_LOG_BULLET;
		RenderState::XVertexLayout::const_iterator it;
		for (it = layout.begin(); it != layout.end(); ++it)
		{
			X_LOG0("Layout", "\"%s(%i)\" ByteOffset: %i Slot: %i",
				it->SemanticName, it->SemanticIndex,
				it->AlignedByteOffset, it->InputSlot);
		}

		// sleep in debug mode
#if X_DEBUG
		core::Thread::Sleep(500);
#endif // !X_DEBUG
		return nullptr;
	}

	// debug name
	D3DDebug::SetDebugObjectName(pInputLayout,
		shader::VertexFormat::toString(vertexFmt));

	return pInputLayout;
}

bool DX11XRender::FX_SetVertexDeclaration(shader::VertexFormat::Enum vertexFmt, bool streamed)
{
	ID3D11InputLayout* pLayout = nullptr;

	if (streamed)
	{
		if (State_.streamedILCache[vertexFmt] == nullptr)
		{
			RenderState::XVertexLayout& layout = State_.streamedILDescriptions[vertexFmt];
			pLayout = CreateILFromDesc(vertexFmt, layout);
			State_.streamedILCache[vertexFmt] = pLayout;

			if (!pLayout) {
				return false;
			}
		}
		else
		{
			pLayout = State_.streamedILCache[vertexFmt];
		}
	}
	else
	{
		if (State_.ILCache[vertexFmt] == nullptr)
		{
			RenderState::XVertexLayout& layout = State_.ILDescriptions[vertexFmt];
			pLayout = CreateILFromDesc(vertexFmt, layout);
			State_.ILCache[vertexFmt] = pLayout;

			if (!pLayout) {
				return false;
			}
		}
		else
		{
			pLayout = State_.ILCache[vertexFmt];
		}
	}			
	
	if (State_.pCurrentVertexFmt != pLayout)
	{
		State_.pCurrentVertexFmt = pLayout;
		State_.CurrentVertexFmt = vertexFmt;
		State_.streamedIL = streamed;
		deviceContext_->IASetInputLayout(pLayout);
	}

	return true;
}

void DX11XRender::FX_SetVStream(ID3D11Buffer* pVertexBuffer, VertexStream::Enum streamSlot,
	uint32 stride, uint32 offset)
{
	// check for redundant calls
	RenderState::XStreamInfo& info = State_.VertexStreams[streamSlot];
	if (info.pBuf != pVertexBuffer || info.offset != offset)
	{
		info.pBuf = pVertexBuffer;
		info.offset = offset;

		deviceContext_->IASetVertexBuffers(streamSlot, 1, &pVertexBuffer, &stride, &offset);
	}
}

void DX11XRender::FX_SetIStream(ID3D11Buffer* pIndexBuffer)
{
	if (State_.pIndexStream != pIndexBuffer)
	{
		State_.pIndexStream = pIndexBuffer;

		deviceContext_->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	}
}

void DX11XRender::FX_SetVStream(uint32_t VertexBuffer, VertexStream::Enum streamSlot,
	uint32 stride, uint32 offset)
{
	FX_SetVStream(
		vidMemMng_.getD3DVB(VertexBuffer),
		streamSlot,
		stride,
		offset
	);
}

void DX11XRender::FX_SetIStream(uint32_t IndexBuffer)
{
	FX_SetIStream(vidMemMng_.getD3DIB(IndexBuffer));
}


void DX11XRender::FX_UnBindVStream(ID3D11Buffer* pVertexBuffer)
{
	// check if the vertex stream is currently bound to the pipeline
	// if so unset.
	uint32_t i;
	for (i = 0; i < VertexStream::ENUM_COUNT; i++)
	{
		RenderState::XStreamInfo& info = State_.VertexStreams[i];
		if (info.pBuf == pVertexBuffer)
		{
			deviceContext_->IASetVertexBuffers(i, 0, nullptr,  nullptr, nullptr);
			info.pBuf = nullptr;
		}
	}
}

void DX11XRender::FX_UnBindIStream(ID3D11Buffer* pIndexBuffer)
{
	// check if this index's are currently bound to the pipeline
	// if so unset.

	if (State_.pIndexStream == pIndexBuffer) {
		deviceContext_->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
		State_.pIndexStream = nullptr;
	}
}

void DX11XRender::FX_DrawPrimitive(const PrimitiveType::Enum type,
	const int StartVertex, const int VerticesCount)
{
	int numPrimatives;

	switch (type)
	{
		case PrimitiveType::TriangleList:
			numPrimatives = VerticesCount / 3;
			X_ASSERT(VerticesCount % 3 == 0, "invalid vert count for triangle list.")(VerticesCount);
		break;
		case PrimitiveType::TriangleStrip:
			numPrimatives = VerticesCount - 2;
			X_ASSERT(VerticesCount > 2, "invalid vert count for triangle strip.")(VerticesCount);
		break;
		case PrimitiveType::LineList:
			numPrimatives = VerticesCount / 2;
			X_ASSERT(VerticesCount % 2 == 0, "invalid vert count for line list.")(VerticesCount);
		break;
		case PrimitiveType::LineStrip:
			numPrimatives = VerticesCount - 1;
			X_ASSERT(VerticesCount > 1, "invalid vert count for line strip.")(VerticesCount);
		break;
		case PrimitiveType::PointList:
			numPrimatives = VerticesCount;
			X_ASSERT(VerticesCount > 0, "invalid vert count for point list.")(VerticesCount);
		break;

#if X_DEBUG || X_ENABLE_ASSERTIONS
		default:
			X_ASSERT_NOT_IMPLEMENTED();
			break;
#else
		X_NO_SWITCH_DEFAULT;
#endif
	}

	const D3D11_PRIMITIVE_TOPOLOGY nativeType = FX_ConvertPrimitiveType(type);

//	FX_SetIStream(nullptr);

	SetPrimitiveTopology(nativeType);
	deviceContext_->Draw(VerticesCount, StartVertex);
}

void DX11XRender::FX_DrawIndexPrimitive(const PrimitiveType::Enum type, const int IndexCount,
	const int StartIndex,
	const int BaseVertexLocation) // A value added to each index before reading a vertex from the vertex buffer.
{
	int numPrimatives;

	switch (type)
	{
		case PrimitiveType::TriangleList:
			numPrimatives = IndexCount / 3;
			X_ASSERT(IndexCount % 3 == 0, "invalid vert count for triangle list.")(IndexCount);
			break;
		case PrimitiveType::TriangleStrip:
			numPrimatives = IndexCount - 2;
			X_ASSERT(IndexCount > 2, "invalid vert count for triangle strip.")(IndexCount);
			break;
		case PrimitiveType::LineList:
			numPrimatives = IndexCount / 2;
			X_ASSERT(IndexCount % 2 == 0, "invalid vert count for line list.")(IndexCount);
			break;

#if X_DEBUG || X_ENABLE_ASSERTIONS
		default:
			X_ASSERT_NOT_IMPLEMENTED();
			break;
#else
			X_NO_SWITCH_DEFAULT;
#endif
	}


	const D3D11_PRIMITIVE_TOPOLOGY nativeType = FX_ConvertPrimitiveType(type);

	SetPrimitiveTopology(nativeType);
	deviceContext_->DrawIndexed(IndexCount, StartIndex, BaseVertexLocation);
}

IRenderAux* DX11XRender::GetIRenderAuxGeo(void)
{
	return AuxGeo_->GetRenderAuxGeom();
}


X_NAMESPACE_END
