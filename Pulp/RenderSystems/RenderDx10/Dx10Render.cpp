#include "stdafx.h"
#include "Dx10Render.h"

#include "Util\ToggleChecker.h"

#include "../Common/Textures/XTextureFile.h"
#include "../Common/Textures/XTexture.h"
#include "../Common/Shader/XShader.h"
#include "Dx10Shader.h"

#include "../3DEngine/ModelLoader.h"

#include "Dx10RenderAux.h"

X_NAMESPACE_BEGIN(render)


DX11XRender g_Dx11D3D;

DX11XRender::DX11XRender()  :
#if X_DEBUG
	m_d3dDebug(nullptr),
#endif

	m_ViewMat(),
	m_ProMat(),
	m_BlendStates(nullptr),
	m_RasterStates(nullptr),
	m_DepthStates(nullptr),
	m_AuxGeo_(nullptr)
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

	m_BlendStates.setArena(arena);
	m_RasterStates.setArena(arena);
	m_DepthStates.setArena(arena);

//	for (i = 0; i < shader::VertexFormat::Num; i++)
//		m_State.vertexLayoutDescriptions[i].setArena(arena);

}

bool DX11XRender::Init(HWND hWnd, 
	uint32_t width, uint32_t hieght)
{
	float screenWidth = (float)width;
	float screenHeight = (float)hieght;

//	float screenDepth = 1000.f;
//	float screenNear = 1.f;

	if (!XRender::Init(hWnd, width, hieght)) {
		return false;
	}

	if (hWnd == (HWND)0)
	{
		X_ERROR("dx10", "target window is not valid");
		return false;
	}

	SetArenas(g_rendererArena);


	m_ViewMat.SetDepth(16);
	m_ProMat.SetDepth(16);


	m_CurBlendState = (uint32_t)-1;
	m_CurRasterState = (uint32_t)-1;
	m_CurDepthState = (uint32_t)-1;


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
	swapChainDesc.BufferDesc.Width = (UINT)screenWidth;
	swapChainDesc.BufferDesc.Height = (UINT)screenHeight;

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


	// Create the swap chain and the Direct3D device.
	result = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
#if X_DEBUG
		D3D11_CREATE_DEVICE_DEBUG,
#else
		0,
#endif
		FeatureLevels,
		numLevelsRequested,
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&m_swapChain, 
		&m_device,
		&featureout,
		&m_deviceContext
		);

	if (FAILED(result))
	{
		return false;
	}

	// Get the pointer to the back buffer.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(result))
	{
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = m_device->CreateRenderTargetView(backBufferPtr, NULL, &m_renderTargetView);
	if (FAILED(result))
	{
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = 0;

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = (UINT)screenWidth;
	depthBufferDesc.Height = (UINT)screenHeight;
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
	result = m_device->CreateTexture2D(&depthBufferDesc, NULL, &m_depthStencilBuffer);
	if (FAILED(result))
	{
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
		return false;
	}



	// Gbuffer Stencil state.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	depthStencilViewDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH;

	// Create the depth stencil view.
	result = m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilViewReadOnly);
	if (FAILED(result))
	{
		return false;
	}


	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	depthStencilViewDesc.Flags = 0;

	// Create the depth stencil view.
	result = m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView);
	if (FAILED(result))
	{
		return false;
	}

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

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
	// result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterState);
	if (!SetRasterState(raster))
	{
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
	m_deviceContext->RSSetViewports(1, &viewport);

	// Setup the projection matrix.
	fieldOfView = (float)D3DX_PI / 4.0f;
	screenAspect = (float)screenWidth / (float)screenHeight;

	OnPostCreateDevice();

#if X_DEBUG
	if (SUCCEEDED(m_device->QueryInterface(__uuidof(ID3D11Debug), (void**)&m_d3dDebug)))
	{
		return true;
	}
	// where is my debug interface slut!
	return false;
#endif
	return true;
}


void DX11XRender::ShutDown()
{
	size_t i;

	// needs clearingh before zrender::Shutdown.
	// as we have VB / IB to free.
	if (m_AuxGeo_) {
		m_AuxGeo_->ReleaseDeviceObjects();
		// might move
		X_DELETE(m_AuxGeo_, g_rendererArena);
	}

	FreeDynamicBuffers();

	XRender::ShutDown();
	shader::XHWShader_Dx10::shutDown();


	// clear the stacks
	m_ViewMat.Clear();
	m_ProMat.Clear();

	DxDeviceContext()->OMSetBlendState(nullptr, 0, 0xFFFFFFFF);
	DxDeviceContext()->OMSetDepthStencilState(nullptr, 0);
	DxDeviceContext()->OMSetDepthStencilState(nullptr, 0);


	for (i = 0; i < m_BlendStates.size(); ++i)
		m_BlendStates[i].pState->Release();
	for (i = 0; i < m_RasterStates.size(); ++i)
		m_RasterStates[i].pState->Release();
	for (i = 0; i < m_DepthStates.size(); ++i)
		m_DepthStates[i].pState->Release();

	m_BlendStates.free();
	m_RasterStates.free();
	m_DepthStates.free();

	RenderResources_.free();

	m_State.~RenderState();
	// --------------------------


	// Before shutting down set to windowed mode or when releasing the swap chain it will throw an exception.
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	if (m_depthStencilView)
	{
		m_depthStencilView->Release();
		m_depthStencilView = 0;
	}

	if (m_depthStencilBuffer)
	{
		m_depthStencilBuffer->Release();
		m_depthStencilBuffer = 0;
	}

	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = 0;
	}

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = 0;
	}

	if (m_deviceContext)
	{
		m_deviceContext->ClearState();
		m_deviceContext->Flush();
		m_deviceContext->Release();
		m_deviceContext = nullptr;
	}

	if (m_device)
	{
		m_device->Release();
		m_device = 0;
	}

#if X_DEBUG
	if (m_d3dDebug)
	{
		m_d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_SUMMARY);
		m_d3dDebug->Release();
	}
	else
	{
		X_WARNING("D3D", "Debug interface is invalid");
	}
#endif // !X_DEBUG

}

void DX11XRender::OnPostCreateDevice(void)
{
	shader::XHWShader_Dx10::Init();

	m_ShaderMan.Init();

	FX_Init();

	InitDynamicBuffers();

	InitResources();


	m_AuxGeo_ = X_NEW(XRenderAuxImp, g_rendererArena, "AuxGeo")(*this);
	m_AuxGeo_->RestoreDeviceObjects();
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
		}

		m_DynVB[i].CreateVB(&vidMemMng_, numVerts, vertSize);
	}
}


void DX11XRender::FreeDynamicBuffers(void)
{
	int i;
	for (i = 0; i<VertexPool::PoolMax; i++)
	{
		m_DynVB[i].Release();
	}
}



void DX11XRender::RenderBegin()
{
	X_PROFILE_BEGIN("DXRenderBegin", core::ProfileSubSys::RENDER);

	XRender::RenderBegin();

	X_ASSERT(m_ViewMat.getDepth() == 0, "stack depth is not zero at start of frame")(m_ViewMat.getDepth());
	X_ASSERT(m_ProMat.getDepth() == 0, "stack depth is not zero at start of frame")(m_ProMat.getDepth());

	texture::XTexture::setDefaultFilterMode(shader::FilterMode::TRILINEAR);

	Colorf clear_col(0.057f, 0.221f, 0.400f);

	// Clear the back buffer.
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, clear_col);
	// Clear the depth buffer.
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


}

void DX11XRender::RenderEnd()
{
	X_PROFILE_BEGIN("DXRenderEnd", core::ProfileSubSys::RENDER);
	XRender::RenderEnd();


	if (m_AuxGeo_) {
	//	m_AuxGeo_->GetRenderAuxGeom()->flush();
		m_AuxGeo_->GetRenderAuxGeom()->Reset();
	}

// slow as goat cus if large sleep.
//	this->rThread()->syncMainWithRender();


	m_swapChain->Present(0, 0);
}


bool DX11XRender::DefferedBegin()
{
	using namespace texture;

	ID3D11RenderTargetView* pViews[3] = {
		XTexture::s_GBuf_Albedo->getRenderTargetView(),
		XTexture::s_GBuf_Normal->getRenderTargetView(),
		XTexture::s_GBuf_Depth->getRenderTargetView(),
	};

	Colorf clear_col(0.0f, 0.f, 0.0f);

	m_deviceContext->ClearRenderTargetView(pViews[0], clear_col);
	m_deviceContext->ClearRenderTargetView(pViews[1], clear_col);
	m_deviceContext->ClearRenderTargetView(pViews[2], clear_col);


	SetZPass();

	m_deviceContext->OMSetRenderTargets(3, pViews, m_depthStencilView);

	return true;
}

bool DX11XRender::DefferedEnd()
{
	using namespace shader;
	using namespace texture;

	// the shader
	XShader* pSh = XShaderManager::m_DefferedShaderVis;
	uint32_t pass;


	// switch the render target back
	ID3D11RenderTargetView* pViewsReset[3] = {
		m_renderTargetView,
		NULL,
		NULL,
	};

	m_deviceContext->OMSetRenderTargets(3, pViewsReset, m_depthStencilView);


	SetCullMode(CullMode::NONE);

	float height = this->getHeightf() - 10;

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
		
		m_ProMat.Push();
		m = m_ProMat.GetTop();

		float width = getWidthf();
		float height = getHeightf();

		MatrixOrthoOffCenterRH(m, 0, width, height, 0, znear, zfar);

		// want a identiy view.
		PushViewMatrix();
		m_ViewMat.LoadIdentity();


	} else {
		m_ProMat.Pop();
		PopViewMatrix();
	}

	SetCameraInfo();
}

// Camera


void DX11XRender::SetCameraInfo(void)
{
	m_pRt->RT_SetCameraInfo();
}

void DX11XRender::RT_SetCameraInfo(void)
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
	float ProjectionRatio = cam.GetProjectionRatio();
	float fov = cam.GetFov();

//	float wT = math<float>::tan(fov*0.5f)*cam.GetNearPlane();
//	float wB = -wT;
//	float wR = wT * ProjectionRatio;
//	float wL = -wR;

	Matrix34f m = cam.GetMatrix();
	Vec3f vEye = cam.GetPosition();
	Vec3f vAt = vEye + Vec3f(m.m01, m.m11, m.m21);
	Vec3f vUp = Vec3f(m.m02, m.m12, m.m22);
//	Vec3f vUp = Vec3f(0,0,1);

	// View
	Matrix44f* pView = m_ViewMat.GetTop();
	MatrixLookAtRH(pView, vEye, vAt, vUp);

	// Proj
	Matrix44f* pProj = m_ProMat.GetTop();
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
			X_ASSERT(img_data->SubInfo[i].pSysMem != nullptr, "system mem must be set for all faces")();
			SubResource[i].pSysMem = img_data->SubInfo[i].pSysMem;
			SubResource[i].SysMemPitch = img_data->SubInfo[i].SysMemPitch;
			SubResource[i].SysMemSlicePitch = 0; // img_data->SubInfo[0].SysMemSlicePitch;
		}
	}
	else
	{
		pSubResource = &SubResource[0];
		for (i = 0; i < img_data->getNumMips(); i++)
		{
			SubResource[i].pSysMem = img_data->SubInfo[i].pSysMem;
			SubResource[i].SysMemPitch = img_data->SubInfo[i].SysMemPitch;
			SubResource[i].SysMemSlicePitch = img_data->SubInfo[i].SysMemSlicePitch;
		}
	}

	hr = m_device->CreateTexture2D(&Desc, pSubResource, &pTexture2D);


	// worky?
	if (SUCCEEDED(hr))
	{
		dev_tex.setTexture(pTexture2D);
#if X_DEBUG
		D3DDebug::SetDebugObjectName(pTexture2D, img_data->getName());
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

void DX11XRender::FX_PipelineShutdown()
{

	shader::XHWShader_Dx10::shutDown();

}

// ~Shaders

void DX11XRender::FX_Init(void)
{
	InitVertexLayoutDescriptions();

}


void DX11XRender::InitVertexLayoutDescriptions(void)
{
	using namespace shader;

	// build all the diffrent layouts.
	int i;
	const int max = VertexFormat::Num;

	D3D11_INPUT_ELEMENT_DESC elem_pos = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_nor101010 = { "NORMAL", 0, DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_nor8888 = { "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
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
		RenderState::XVertexLayout& layout = m_State.vertexLayoutDescriptions[i];

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

			elem_col8888.AlignedByteOffset = 20;
			layout.append(elem_col8888);

			elem_nor323232.AlignedByteOffset = 24;
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


HRESULT DX11XRender::FX_SetVertexDeclaration(shader::VertexFormat::Enum vertexFmt)
{
	ID3D11InputLayout* pLayout;
	HRESULT hr;

	if (m_State.vertexLayoutCache[vertexFmt] == nullptr)
	{
		RenderState::XVertexLayout& layout = m_State.vertexLayoutDescriptions[vertexFmt];

		if (layout.isEmpty())
		{
			X_ERROR("Render", "Failed to set input 'layout description' is empty. fmt: %i", vertexFmt);
			return (HRESULT)-1;
		}

		// need the current shaders byte code / length.
		if (!shader::XHWShader_Dx10::pCurVS_)
		{
			X_ERROR("Render", "Failed to set input layout no shader currently set.");
			return (HRESULT)-1;
		}

		ID3DBlob* pBlob = shader::XHWShader_Dx10::pCurVS_->getshaderBlob();

		if (FAILED(hr = m_device->CreateInputLayout(
				layout.ptr(),
				(uint)layout.size(),
				pBlob->GetBufferPointer(),
				pBlob->GetBufferSize(),
				&m_State.vertexLayoutCache[vertexFmt]))
			)
		{
			const char* pShaderName = shader::XHWShader_Dx10::pCurVS_->getName();
			X_ERROR("Render", "Failed to CreateInputLayout: %i", hr);
			X_ERROR("Render", "CurrentShader: %s", pShaderName);
			X_ERROR("Render", "Layout:");
			X_LOG_BULLET;
			RenderState::XVertexLayout::const_iterator it;
			for ( it = layout.begin(); it != layout.end(); ++it)
			{
				X_LOG0("Layout", "\"%s(%i)\" ByteOffset: %i Slot: %i", 
					it->SemanticName, it->SemanticIndex,
					it->AlignedByteOffset, it->InputSlot );
			}

			// sleep in debug mode
#if X_DEBUG
			GoatSleep(500);
#endif // !X_DEBUG
			return hr;
		}

		// debug name
		D3DDebug::SetDebugObjectName(m_State.vertexLayoutCache[vertexFmt],
			shader::VertexFormat::toString(vertexFmt));
	}

	pLayout = m_State.vertexLayoutCache[vertexFmt];

	if (m_State.pCurrentVertexFmt != pLayout)
	{
		m_State.pCurrentVertexFmt = pLayout;
		m_State.CurrentVertexFmt = vertexFmt;
		m_deviceContext->IASetInputLayout(pLayout);
	}

	return S_OK;
}

void DX11XRender::FX_SetVStream(ID3D11Buffer* pVertexBuffer, VertexStream::Enum streamSlot,
	uint32 stride, uint32 offset)
{
	// check for redundant calls
	RenderState::XStreamInfo& info = m_State.VertexStreams[streamSlot];
	if (info.pBuf != pVertexBuffer || info.offset != offset)
	{
		info.pBuf = pVertexBuffer;
		info.offset = offset;

		m_deviceContext->IASetVertexBuffers(streamSlot, 1, &pVertexBuffer, &stride, &offset);
	}
}

void DX11XRender::FX_SetIStream(ID3D11Buffer* pIndexBuffer)
{
	if (m_State.pIndexStream != pIndexBuffer)
	{
		m_State.pIndexStream = pIndexBuffer;

		m_deviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
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
	m_deviceContext->Draw(VerticesCount, StartVertex);
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
	m_deviceContext->DrawIndexed(IndexCount, StartIndex, BaseVertexLocation);
}

IRenderAux* DX11XRender::GetIRenderAuxGeo(void)
{
	return m_AuxGeo_->GetRenderAuxGeom();
}


X_NAMESPACE_END
