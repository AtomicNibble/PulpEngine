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

namespace
{
	uint8 g_StencilFuncLookup[9] =
	{
		D3D11_COMPARISON_NEVER,
		D3D11_COMPARISON_NEVER,				
		D3D11_COMPARISON_LESS,	
		D3D11_COMPARISON_LESS_EQUAL,		
		D3D11_COMPARISON_GREATER,			
		D3D11_COMPARISON_GREATER_EQUAL,		
		D3D11_COMPARISON_EQUAL,				
		D3D11_COMPARISON_NOT_EQUAL,
		D3D11_COMPARISON_ALWAYS,
	};

	uint8 g_StencilOpLookup[9] =
	{
		D3D11_STENCIL_OP_KEEP,
		D3D11_STENCIL_OP_KEEP,		
		D3D11_STENCIL_OP_ZERO,
		D3D11_STENCIL_OP_REPLACE,	
		D3D11_STENCIL_OP_INCR_SAT,			
		D3D11_STENCIL_OP_DECR_SAT,	
		D3D11_STENCIL_OP_INVERT,
		D3D11_STENCIL_OP_INCR,				
		D3D11_STENCIL_OP_DECR,				
	};

}

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
	int i;

	XRender::SetArenas(arena);

	m_BlendStates.setArena(arena);
	m_RasterStates.setArena(arena);
	m_DepthStates.setArena(arena);

	for (i = 0; i < shader::VertexFormat::Num; i++)
		m_State.vertexLayoutDescriptions[i].setArena(arena);

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

	ViewPort_.view = Vec4<int>(0, 0, width, hieght);
	ViewPort_.z = Vec2f(0.f,1.f);

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
	int32_t i;

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
			case VertexPool::P3F_C4B_T2F:
				numVerts = 32768 / 4;
				vertSize = sizeof(Vertex_P3F_C4B_T2F);
				break;
			case VertexPool::P3F_C4B_T2S:
				numVerts = 32768 / 4;
				vertSize = sizeof(Vertex_P3F_C4B_T2S);
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


void test(Matrix44f* pMat)
{

	// Calculate the infinite matrix used for Z - Fail
	float aspect_ratio = 800.f / 600.f;
	float yScale = 1 / ((float)math<float>::tan(90.f) / 2.0f);
	float xScale = (1 / aspect_ratio) * yScale;

	float e = 0.000001f;

	pMat->setToIdentity();

	pMat->m00 = xScale;
	pMat->m11 = yScale;
	pMat->m22 = -1 + e;
	pMat->m32 = -1;
	pMat->m23 = -0.001f;
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


//	m_ViewMat.LoadIdentity();
//	m_ProMat.LoadIdentity();

	
//	D3DXMatrixPerspectiveRH(m_ProMat.GetTop(), 800,600,0.001f, 1.0f);
	
//	test(m_ProMat.GetTop());

	static float rotate = 0.f;
	/*
	XCamera cam;
	cam.SetFrustum(800,600);
	cam.SetPosition(Vec3f(0, 0, -5.f));
	cam.SetPosition(Vec3f(-2, -3, -1.f));


	SetCamera(cam);
	FX_ComitParams();
	*/


	// Deffered test.
#if 0
	DefferedTest();
#else

	/*
	shader::XShader* pSh = shader::XShaderManager::m_FixedFunction;
	uint32_t pass;

	pSh->FXSetTechnique("SolidTestWorld");
	pSh->FXBegin(&pass, 0);
	pSh->FXBeginPass(pass);

	texture::XTexture* pTex;
	pTex = texture::XTexture::FromName("core_assets/Textures/color_grid.dds", texture::TextureFlags::DONT_STREAM);
	pTex->apply(0);


	FX_SetVStream(g_pVertexBuffer, 0, sizeof(model::Vertex), 0);
	FX_SetIStream(g_pIndexBuffer);
	if (SUCCEEDED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2S)))
		FX_DrawIndexPrimitive(PrimitiveType::TriangleList, g_numFaces * 3, 0, 0);

	FX_SetIStream(nullptr);
	*/

#endif

#if 0
	Set2D(true);

#if 0
	pTex = texture::XTexture::FromName("core_assets/Textures/potato_256.dds", texture::TextureFlags::DONT_STREAM);


	DrawImage(
		300, 300, 0,
		128, 128,
		pTex->getID(), // tex id
		0, 1, 1, 0,
		Col_White);

	DrawImage(
		450, 300, 0,
		128, 128,
		pTex->getID(), // tex id
		0, 1, 1, 0,
		Col_Olive);

	// --------------------

/*
	Vec3f points[] = {
		Vec3f(400, 400, 0),
		Vec3f(600, 400, 0),
		Vec3f(400, 500, 0),
		Vec3f(600, 600, 0),
	};

	DrawRect(300,440,50,50, Col_White);
	DrawRect(320, 450, 55, 50, Col_Aquamarine);
*/

//	DrawLines(points, 4, Col_Lime);
#endif

	Set2D(false);
#endif

#if 0

	IRenderAux *pAux = GetIRenderAuxGeo();
	XAuxGeomRenderFlags flags = pAux->getRenderFlags();
	flags.SetMode2D3DFlag(AuxGeom_Mode2D3D::Mode2D);
	pAux->setRenderFlags(flags);

	float x1, x2, y1, y2;
	x1 = 0.02f;
	x2 = 0.6f;
	y1 = 0.1f;
	y2 = 0.5f;


	Matrix44f test(
		2, 0, 0, 0,
		0, -2, 0, 0,
		0, 0, 0, 0,
		-1, 1, 0, 1);

	test.transpose();

	Vec4f vec(x1, y2, 0, 0);
	Vec4f out = test * vec;

	Color8u col = Col_Salmon;

	Vec3f points[] = {
		Vec3f(x1, y1, 0), Vec3f(x2, y1, 0),
		Vec3f(x1, y2, 0), Vec3f(x2, y2, 0),
		Vec3f(x1, y1, 0), Vec3f(x1, y2, 0),
		Vec3f(x2, y1, 0), Vec3f(x2, y2, 0),
	};
	Color8u colors[] = {
		Col_Salmon, Col_Red,
		Col_Beige, Col_Darkgoldenrod,
		Col_Firebrick, Col_Darkkhaki,
		Col_Fuchsia, Col_Forestgreen,
	};


	pAux->drawLines(points, 8, colors);

	Vec3f top(0.70f, 0.5f, 0);
	Vec3f leftb(0.5f,0.85f,0.f);
	Vec3f rightb(0.9f,0.85f,0.f);

	pAux->drawTriangle(top, Col_Firebrick, leftb, Col_Darkgoldenrod, rightb, Col_Fuchsia);

	Vec3f pos(0.10f,0.75f,0);
	Vec3f dir(0.1f,0.4f,0.5f);
	pAux->drawCylinder(pos,dir, 0.05f, 0.3f, Col_Forestgreen);

	pos = Vec3f(0.35f, 0.65f, 0);
	dir = Vec3f(0.1f, 0.8f, 0.5f);
	pAux->drawCone(pos, dir, 0.1f,0.3f, Col_Tomato);

	Sphere sphere;
	sphere.setRadius(0.08f);
	sphere.setCenter(Vec3f(0.2f, 0.35f, 0.5f));
	pAux->drawSphere(sphere, Col_Gray);

	AABB aabb;
	aabb.min = Vec3f(0.1f,0.1f,0.1f);
	aabb.max = Vec3f(0.2f,0.3f,0.2f);


	Matrix34f trans = Matrix34f::createTranslation(Vec3f(0.9f,0.1f,0.0f));
	
	Vec3f direction(dir.normalized());
	Vec3f orthogonal(direction.getOrthogonal().normalized());
	Matrix33f matRot;
	matRot.setToIdentity();
	matRot.setColumn(0, orthogonal);
	matRot.setColumn(1, direction);
	matRot.setColumn(2, orthogonal.cross(direction));

	Matrix34f mat = trans * matRot;

	pAux->drawAABB(aabb, mat, false, Col_Red);

	QuatTransf bone;
	QuatTransf child;
	QuatTransf child2;
	QuatTransf child3;

	bone.setTranslation(Vec3f(0.2f, 0.2f, 0.f));
	child.setTranslation(Vec3f(0.4f, 0.2f, 0.f));
	child2.setTranslation(Vec3f(0.6f, 0.55f, 0.f));
	child3.setTranslation(Vec3f(0.2f, 0.55f, 0.f));

	pAux->drawBone(bone, child, Col_Red);
	pAux->drawBone(child, child2, Col_Red);
	pAux->drawBone(child2, child3, Col_Red);

#endif
}

void DX11XRender::RenderEnd()
{
	X_PROFILE_BEGIN("DXRenderEnd", core::ProfileSubSys::RENDER);
	XRender::RenderEnd();



	if (m_AuxGeo_) {
		m_AuxGeo_->GetRenderAuxGeom()->flush();
		m_AuxGeo_->GetRenderAuxGeom()->Reset();
	}


// slow as goat cus if large sleep.
//	this->rThread()->syncMainWithRender();


//	X_LOG0("render", "Swap");

	m_swapChain->Present(0, 0);
}


void DX11XRender::DefferedTest()
{
	using namespace shader;

	Color color = Col_White;
	m_deviceContext->ClearRenderTargetView(texture::XTexture::s_GBuf_Albedo->getRenderTargetView(), color);
	color = Col_Orange;
	m_deviceContext->ClearRenderTargetView(texture::XTexture::s_GBuf_Depth->getRenderTargetView(), color);


	ID3D11RenderTargetView* pViews[3] = {
		texture::XTexture::s_GBuf_Albedo->getRenderTargetView(),
		texture::XTexture::s_GBuf_Normal->getRenderTargetView(),
		texture::XTexture::s_GBuf_Depth->getRenderTargetView(),
	};

	SetZPass();

	m_deviceContext->OMSetRenderTargets(3, pViews, m_depthStencilView);


	float fZ = 0.2f;
	float val = 1.0f;
	Vec3f v0, v1, v2, v3;


	// Draw somthing i can see.
//	FX_SetVStream(g_pVertexBuffer, 0, sizeof(model::Vertex), 0);
//	FX_SetIStream(g_pIndexBuffer);
//	if (SUCCEEDED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2S)))
//		FX_DrawIndexPrimitive(PrimitiveType::TriangleList, 12 * 3, 0, 0);
//	FX_SetIStream(nullptr);
	

	XShader* pSh = XShaderManager::m_DefferedShaderVis;
	uint32_t pass;


	pSh->FXSetTechnique("VisualizeAlbedo");
	pSh->FXBegin(&pass, 0);
	pSh->FXBeginPass(pass);

	// switch the render target back
	ID3D11RenderTargetView* pViewsReset[3] = {
		m_renderTargetView,
		NULL,
		NULL,
	};

	m_deviceContext->OMSetRenderTargets(3, pViewsReset, m_depthStencilView);


#if 1
	Set2D(true);


	DrawImage(
		20, 50, 0,
		256, 256,
		texture::XTexture::s_GBuf_Albedo->getID(), // tex id
		0, 1, 1, 0,
		Col_White);

	// need to unbind it so it can be used
	// as part of gbuffer again.
	texture::XTexture::s_GBuf_Albedo->unbind();

	Set2D(false);

#else
	texture::XTexture::s_GBuf_Albedo->apply(2);


	v0 = Vec3f(0.5f,-0.50f, fZ);
	v1 = Vec3f(0.5, -val, fZ);
	v2 = Vec3f(val, -0.5f, fZ);
	v3 = Vec3f(val, -val, fZ);

	DrawQuad3d(v0, v1, v2, v3, Col_White);

	pSh->FXSetTechnique("VisualizeDepth");
	pSh->FXBegin(&pass, 0);
	pSh->FXBeginPass(pass);

	texture::XTexture::s_GBuf_Albedo->unbind();
	texture::XTexture::s_GBuf_Depth->apply(0);

	v0 = Vec3f(0, -0.50f, fZ);
	v1 = Vec3f(0, -val, fZ);
	v2 = Vec3f(0.5, -0.5f, fZ);
	v3 = Vec3f(0.5, -1.f, fZ);

	DrawQuad3d(v0, v1, v2, v3, Col_White);

	texture::XTexture::s_GBuf_Depth->unbind();
#endif

//	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
}

void DX11XRender::DrawQuad3d(const Vec3f& pos0, const Vec3f& pos1, const Vec3f& pos2,
	const Vec3f& pos3, const Color& col)
{

	uint32 nOffs;
	Vertex_P3F_C4B_T2F* Quad = (Vertex_P3F_C4B_T2F*)m_DynVB[VertexPool::P3F_C4B_T2F].LockVB(4, nOffs);

	Quad[0].pos = pos0;
	Quad[1].pos = pos1;
	Quad[2].pos = pos2;
	Quad[3].pos = pos3;


	for (uint32 i = 0; i < 4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f::zero();
	}

	m_DynVB[VertexPool::P3F_C4B_T2F].UnlockVB();
	m_DynVB[VertexPool::P3F_C4B_T2F].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2F)))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}



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


	Desc.Height = img_data->getHeight();
	Desc.Width = img_data->getWidth();
	Desc.MipLevels = img_data->getNumMips();
	Desc.ArraySize = 1;
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


void DX11XRender::SetState(StateFlag state)
{
	m_pRt->RC_SetState(state);
}

void DX11XRender::SetStencilState(StencilState::Value ss)
{
	DepthState depth = curDepthState();

	StencilState::Value::Face& front = ss.faces[0];

	int test = front.getStencilFuncIdx();
	int test1 = front.getStencilFailOpIdx();
	int test2 = front.getStencilZFailOpIdx();
	int test3 = front.getStencilPassOpIdx();

	depth.Desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	depth.Desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	depth.Desc.FrontFace.StencilFunc = (D3D11_COMPARISON_FUNC)g_StencilFuncLookup[front.getStencilFuncIdx()];
	depth.Desc.FrontFace.StencilFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[front.getStencilFailOpIdx()];
	depth.Desc.FrontFace.StencilDepthFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[front.getStencilZFailOpIdx()];
	depth.Desc.FrontFace.StencilPassOp = (D3D11_STENCIL_OP)g_StencilOpLookup[front.getStencilPassOpIdx()];

	if (ss.backFaceInfo())
	{
		StencilState::Value::Face& back = ss.faces[1];

		depth.Desc.BackFace.StencilFunc = (D3D11_COMPARISON_FUNC)g_StencilFuncLookup[back.getStencilFuncIdx()];
		depth.Desc.BackFace.StencilFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[back.getStencilFailOpIdx()];
		depth.Desc.BackFace.StencilDepthFailOp = (D3D11_STENCIL_OP)g_StencilOpLookup[back.getStencilZFailOpIdx()];
		depth.Desc.BackFace.StencilPassOp = (D3D11_STENCIL_OP)g_StencilOpLookup[back.getStencilPassOpIdx()];
	}
	else
	{
		depth.Desc.BackFace = depth.Desc.FrontFace;
	}

	SetDepthState(depth);
}

void DX11XRender::SetCullMode(CullMode::Enum mode)
{
	m_pRt->RC_SetCullMode(mode);
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

		float width = 800;
		float height = 600;

		MatrixOrthoOffCenterLH(m, 0, width, height, 0, znear, zfar);

		PushMatrix();
		m_ViewMat.LoadIdentity();

		// set the 2d shader.
		// do i want to slap a goat?
		// I have a GUI shader that want's positions in screenspace -1.0 - 1.0
		// meaning we don't need a matrix.
		// the maxtrix above can be used to transform to screenspace.
		// so that none gui stuff can be draw in 2d.
		// need to switch to Fixedfunction tho.
		// SetFFE();

	} else {
		m_ProMat.Pop();
		PopMatrix();
	}

	SetCameraInfo();
}

void DX11XRender::SetCameraInfo(void)
{
	m_pRt->RT_SetCameraInfo();
}

void DX11XRender::RT_SetCameraInfo(void)
{
	// calculate the current camera shit.

	GetModelViewMatrix(&ViewMatrix_);
	GetProjectionMatrix(&ProjMatrix_);


	ViewProjMatrix_ = ViewMatrix_ * ProjMatrix_;
	ViewProjInvMatrix_ = ViewProjMatrix_.inverted();

	// if the camera has changed then we need to tell the shader system 
	// that the cameras are dirty!
	// instead of updating the cbuf's
	// since if this function is called multiple times with no drawining.
	// be wasted cbuffer updates.

	shader::XHWShader_Dx10::SetCameraDirty();
}

void DX11XRender::RT_SetState(StateFlag state)
{
	BlendState blend = curBlendState();
	RasterState raster = curRasterState();
	DepthState depth = curDepthState();
	
	bool bDirtyBS = false;
	bool bDirtyRS = false;
	bool bDirtyDS = false;

	int Changed;
	Changed = state.ToInt() ^ m_State.currentState.ToInt();

	
	if (Changed & States::WIREFRAME)
	{
		bDirtyRS = true;

		if (state.IsSet(StateFlag::WIREFRAME))
			raster.Desc.FillMode = D3D11_FILL_WIREFRAME;
		else
			raster.Desc.FillMode = D3D11_FILL_SOLID;
	}

	if (Changed & States::DEPTHWRITE)
	{
		bDirtyDS = true;
		if (state.IsSet(StateFlag::DEPTHWRITE))
			depth.Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		else
			depth.Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	}

	if (Changed & States::NO_DEPTH_TEST)
	{
		bDirtyDS = true;
		if (state.IsSet(StateFlag::NO_DEPTH_TEST))
			depth.Desc.DepthEnable = FALSE;
		else
			depth.Desc.DepthEnable = TRUE;
	}

	if (Changed & States::STENCIL)
	{
		bDirtyDS = true;
		if (state.IsSet(StateFlag::STENCIL))
			depth.Desc.StencilEnable = TRUE;
		else
			depth.Desc.StencilEnable = FALSE;
	}


	if (Changed & States::DEPTHFUNC_MASK)
	{
		bDirtyDS = true;

		switch (state.ToInt() & States::DEPTHFUNC_MASK)
		{
			case States::DEPTHFUNC_LEQUAL:
				depth.Desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
				break;
			case States::DEPTHFUNC_EQUAL:
				depth.Desc.DepthFunc = D3D11_COMPARISON_EQUAL;
				break;
			case States::DEPTHFUNC_GREAT:
				depth.Desc.DepthFunc = D3D11_COMPARISON_GREATER;
				break;
			case States::DEPTHFUNC_LESS:
				depth.Desc.DepthFunc = D3D11_COMPARISON_LESS;
				break;
			case States::DEPTHFUNC_GEQUAL:
				depth.Desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
				break;
			case States::DEPTHFUNC_NOTEQUAL:
				depth.Desc.DepthFunc = D3D11_COMPARISON_NOT_EQUAL;
				break;
		}
	}



	if (Changed & States::BLEND_MASK)
	{
		bDirtyBS = true;

		// Blend.
		if (state.IsSet(States::BLEND_MASK))
		{
			for (size_t i = 0; i<4; ++i)
				blend.Desc.RenderTarget[i].BlendEnable = TRUE;


			// Blend Src.
			if (state.IsSet(States::BLEND_SRC_MASK))
			{
				switch (state.ToInt() & States::BLEND_SRC_MASK)
				{
					case States::BLEND_SRC_ZERO:
						blend.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
						blend.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
						break;
					case States::BLEND_SRC_ONE:
						blend.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
						blend.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
						break;
					case States::BLEND_SRC_DEST_COLOR:
						blend.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
						blend.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
						break;
					case States::BLEND_SRC_INV_DEST_COLOR:
						blend.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
						blend.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
						break;
					case States::BLEND_SRC_SRC_ALPHA:
						blend.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
						blend.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
						break;
					case States::BLEND_SRC_INV_SRC_ALPHA:
						blend.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_SRC_ALPHA;
						blend.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
						break;
					case States::BLEND_SRC_DEST_ALPHA:
						blend.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_ALPHA;
						blend.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_ALPHA;
						break;
					case States::BLEND_SRC_INV_DEST_ALPHA:
						blend.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_ALPHA;
						blend.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
						break;
					case States::BLEND_SRC_ALPHA_SAT:
						blend.Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA_SAT;
						blend.Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA_SAT;
						break;
				}
			}

			// Blend Dst.
			if (state.IsSet(States::BLEND_DEST_MASK))
			{
				switch (state.ToInt() & States::BLEND_DEST_MASK)
				{
					case States::BLEND_DEST_ZERO:
						blend.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
						blend.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
						break;
					case States::BLEND_DEST_ONE:
						blend.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
						blend.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
						break;
					case States::BLEND_DEST_SRC_COLOR:
						blend.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
						blend.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
						break;
					case States::BLEND_DEST_INV_SRC_COLOR:
						blend.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
						blend.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
						break;
					case States::BLEND_DEST_SRC_ALPHA:
						blend.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
						blend.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
						break;
					case States::BLEND_DEST_INV_SRC_ALPHA:
						blend.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
						blend.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
						break;
					case States::BLEND_DEST_DEST_ALPHA:
						blend.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_ALPHA;
						blend.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
						break;
					case States::BLEND_DEST_INV_DEST_ALPHA:
						blend.Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;
						blend.Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
						break;
				}

			}

			//Blending operation
			D3D11_BLEND_OP blendOperation = D3D11_BLEND_OP_ADD;

			switch (state.ToInt() & States::BLEND_OP_MASK)
			{
				case States::BLEND_OP_ADD:
					blendOperation = D3D11_BLEND_OP_ADD;
					break;
				case States::BLEND_OP_SUB:
					blendOperation = D3D11_BLEND_OP_SUBTRACT;
					break;
				case States::BLEND_OP_REB_SUB:
					blendOperation = D3D11_BLEND_OP_REV_SUBTRACT;
					break;
				case States::BLEND_OP_MIN:
					blendOperation = D3D11_BLEND_OP_MIN;
					break;
				case States::BLEND_OP_MAX:
					blendOperation = D3D11_BLEND_OP_MAX;
					break;
			}

			// todo: add separate alpha blend support for mrt
			for (size_t i = 0; i < 4; ++i)
			{
				blend.Desc.RenderTarget[i].BlendOp = blendOperation;
				blend.Desc.RenderTarget[i].BlendOpAlpha = blendOperation;
			}
		}
		else
		{
			// disabel blending.
			for (size_t i = 0; i < 4; ++i)
				blend.Desc.RenderTarget[i].BlendEnable = FALSE;
		}
	}
	
	bool bCurATOC = blend.Desc.AlphaToCoverageEnable != 0;
	bool bNewATOC = state.IsSet(States::ALPHATEST_MASK);
	bDirtyBS |= bNewATOC ^ bCurATOC;
	blend.Desc.AlphaToCoverageEnable = bNewATOC;

	m_State.currentState = state;

	if (bDirtyBS)
		SetBlendState(blend);
	if (bDirtyRS)
		SetRasterState(raster);
	if (bDirtyDS)
		SetDepthState(depth);
}



void DX11XRender::RT_SetCullMode(CullMode::Enum mode)
{
	if (this->m_State.cullMode == mode)
		return;

	RasterState state = curRasterState();

	switch (mode)
	{
		case CullMode::NONE:
			state.Desc.CullMode = D3D11_CULL_NONE;
		break;
		case CullMode::BACK:
			state.Desc.CullMode = D3D11_CULL_BACK;
		break;
		case CullMode::FRONT:
			state.Desc.CullMode = D3D11_CULL_FRONT;
		break;
	}

	m_State.cullMode = mode;

	SetRasterState(state);
}


bool DX11XRender::SetBlendState(BlendState& state)
{
	// try find a matching state.
	uint32_t i;
	HRESULT hr = S_OK;

	state.createHash();

	for (i = 0; i< (uint32_t)m_BlendStates.size(); i++)
	{
		if (m_BlendStates[i] == state) {
			break;
		}
	}

	if (i == m_BlendStates.size())
	{
		// we dont have this state of this type on the gpu yet.
		hr = DxDevice()->CreateBlendState(&state.Desc, &state.pState);
		// save it, and since we add 1 i becomes a valid index.
		m_BlendStates.push_back(state);
	}

	// needs changing?
	if (i != m_CurBlendState)
	{
		m_CurBlendState = i;
		DxDeviceContext()->OMSetBlendState(m_BlendStates[i].pState, 0, 0xFFFFFFFF);
	}

	return SUCCEEDED(hr);
}


bool DX11XRender::SetRasterState(RasterState& state)
{
	// try find a matching state.
	uint32_t i;
	HRESULT hr = S_OK;

	state.createHash();

	for (i = 0; i< (uint32_t)m_RasterStates.size(); i++)
	{
		if (m_RasterStates[i] == state) {
			break;
		}
	}

	if (i == m_RasterStates.size())
	{
		// we dont have this state of this type on the gpu yet.
		hr = DxDevice()->CreateRasterizerState(&state.Desc, &state.pState);
		// save it, and since we add 1 i becomes a valid index.
		m_RasterStates.push_back(state);
	}

	// needs changing?
	if (i != m_CurRasterState)
	{
		m_CurRasterState = i;
		DxDeviceContext()->RSSetState(m_RasterStates[i].pState);
	}

	return SUCCEEDED(hr);
}


bool DX11XRender::SetDepthState(DepthState& state)
{
	// try find a matching state.
	uint32_t i;
	HRESULT hr = S_OK;

	state.createHash();

	for (i = 0; i< (uint32_t)m_DepthStates.size(); i++)
	{
		if (m_DepthStates[i] == state) {
			break;
		}
	}

	if (i == m_DepthStates.size())
	{
		// we dont have this state of this type on the gpu yet.
		hr = DxDevice()->CreateDepthStencilState(&state.Desc, &state.pState);
		// save it, and since we add 1 i becomes a valid index.
		m_DepthStates.push_back(state);

		D3DDebug::SetDebugObjectName(state.pState, __FUNCTION__);
	}

	// needs changing?
	if (i != m_CurDepthState)
	{
		m_CurDepthState = i;
		DxDeviceContext()->OMSetDepthStencilState(m_DepthStates[i].pState, 0);
	}

	return SUCCEEDED(hr);
}


// ViewPort

void DX11XRender::GetViewport(int* x, int* y, int* width, int* height)
{
	X_ASSERT_NOT_NULL(x);
	X_ASSERT_NOT_NULL(y);
	X_ASSERT_NOT_NULL(width);
	X_ASSERT_NOT_NULL(height);

	*x = ViewPort_.view.x;
	*y = ViewPort_.view.y;
	*width = ViewPort_.view.z;
	*height = ViewPort_.view.w;
}

void DX11XRender::SetViewport(int x, int y, int width, int height)
{
	ViewPort_.view.x = x;
	ViewPort_.view.y = y;
	ViewPort_.view.z = width;
	ViewPort_.view.w = height;
}

void DX11XRender::GetViewport(Vec4<int>& viewport)
{
	viewport = ViewPort_.view;
}

void DX11XRender::SetViewport(const Vec4<int>& viewport)
{
	ViewPort_.view = viewport;
}

// ~ViewPort

// Camera







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

void DX11XRender::Draw2dImage(float xpos, float ypos,
	float w, float h, texture::TexID texture_id, ColorT<float>& col)
{
	DrawImage(xpos, ypos, 0.f, w, h, texture_id, 
		0, 1, 1, 0, col
		);
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

struct VertexStreams
{
	enum Enum
	{
		GENERAL,
		TANGENTS,
		HWSKIN   // skinned
	};
};


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
	D3D11_INPUT_ELEMENT_DESC elem_uv32323232 = { "TEXCOORD", 0, DXGI_FORMAT_R16G16_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	D3D11_INPUT_ELEMENT_DESC elem_t3f = { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };


	D3D11_INPUT_ELEMENT_DESC elem_tag101010 = { "TANGENT", 0, DXGI_FORMAT_R10G10B10A2_TYPELESS, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	D3D11_INPUT_ELEMENT_DESC elem_bi101010 = { "BINORMAL", 0, DXGI_FORMAT_R10G10B10A2_TYPELESS, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };


	// Tangent / binormal stream
	static D3D11_INPUT_ELEMENT_DESC elem_tangents[] =
	{
		{ "TANGENT", 0, DXGI_FORMAT_R16G16B16A16_SNORM, VertexStreams::TANGENTS, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R16G16B16A16_SNORM, VertexStreams::TANGENTS, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Skinning!
	// I support 4 bones per vert.
	// so 4 weights and 4 indexs.
	// i support 255 bones so 8bits can fit the bone index.
	static D3D11_INPUT_ELEMENT_DESC elem_skinning[] =
	{
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM, VertexStreams::HWSKIN, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDEXS", 0, DXGI_FORMAT_R8G8B8A8_UNORM, VertexStreams::HWSKIN, 4, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
	};


	for (i = 0; i < max; i++)
	{
		RenderState::XVertexLayout& layout = m_State.vertexLayoutDescriptions[i];

		// for now all positions are just 32bit floats baby!
		layout.append(elem_pos);

		if (i == VertexFormat::P3F_N10_C4B_T2S)
		{
			elem_nor101010.AlignedByteOffset = 12;
			layout.append(elem_nor101010);

			elem_col8888.AlignedByteOffset = 16;
			layout.append(elem_col8888);

			elem_uv1616.AlignedByteOffset = 20;
			layout.append(elem_uv1616);

		}
		else if (i == VertexFormat::P3F_T4F_N3F_C4B)
		{
			elem_uv32323232.AlignedByteOffset = 12;
			layout.append(elem_uv32323232);

			elem_nor101010.AlignedByteOffset = 28;
			layout.append(elem_nor323232);

			elem_col8888.AlignedByteOffset = 40;
			layout.append(elem_col8888);

		}
	/*	else if (i == VertexFormat::P3F_C4B_T2S_N10_T10_B10)
		{
			elem_col8888.AlignedByteOffset = 12;
			layout.append(elem_col8888);

			elem_uv1616.AlignedByteOffset = 16;
			layout.append(elem_uv1616);

			elem_nor101010.AlignedByteOffset = 20;
			layout.append(elem_nor101010);


			elem_tag101010.AlignedByteOffset = 24;
			layout.append(elem_tag101010);

			elem_bi101010.AlignedByteOffset = 28;
			layout.append(elem_bi101010);
		}*/
		else if (i == VertexFormat::P3F_T3F)
		{
			elem_t3f.AlignedByteOffset = 12;
			layout.append(elem_t3f);
		}
		else
		{
			elem_col8888.AlignedByteOffset = 12;
			layout.append(elem_col8888);


			elem_uv3232.AlignedByteOffset = 16;
			elem_uv1616.AlignedByteOffset = 16;

			if (i == VertexFormat::P3F_C4B_T2F) {
				layout.append(elem_uv3232);
			}
			else if (i == VertexFormat::P3F_C4B_T2S) {
				layout.append(elem_uv1616);
			}
		}
	}


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
			X_ERROR("Render", "Failed to CreateInputLayout: %i", hr);
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
		m_deviceContext->IASetInputLayout(pLayout);
	}

	return S_OK;
}

void DX11XRender::FX_SetVStream(ID3D11Buffer* pVertexBuffer, uint32 startslot, 
	uint32 stride, uint32 offset)
{
	X_ASSERT(startslot < RenderState::MAX_VERTEX_STREAMS, "invalid vertex input slot")(startslot, RenderState::MAX_VERTEX_STREAMS);

	RenderState::XStreamInfo& info = m_State.VertexStreams[startslot];

	if (info.pBuf != pVertexBuffer || info.offset != offset)
	{
		info.pBuf = pVertexBuffer;
		info.offset = offset;

		m_deviceContext->IASetVertexBuffers(startslot, 1, &pVertexBuffer, &stride, &offset);
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

void DX11XRender::FX_SetVStream(uint32_t VertexBuffer, uint32 startslot,
	uint32 stride, uint32 offset)
{
	FX_SetVStream(
		vidMemMng_.getD3DVB(VertexBuffer),
		startslot,
		stride,
		offset
	);
}

void DX11XRender::FX_SetIStream(uint32_t IndexBuffer)
{
	FX_SetIStream(vidMemMng_.getD3DIB(IndexBuffer));
}

void DX11XRender::DrawImage(float xpos, float ypos, float z, float w, float h,
	int texture_id, float s0, float t0, float s1, float t1, const Colorf& col, bool filtered)
{
	float s[4], t[4];

	s[0] = s0;	t[0] = 1.0f - t0;
	s[1] = s1;	t[1] = 1.0f - t0;
	s[2] = s0;	t[2] = 1.0f - t1;
	s[3] = s1;	t[3] = 1.0f - t1;

	DrawImageWithUV(xpos, ypos, 0, w, h, texture_id, s, t, col, filtered);
}


void DX11XRender::DrawImageWithUV(float xpos, float ypos, float z, float w, float h, 
	int texture_id, float *s, float *t, const Colorf& col, bool filtered)
{
	X_ASSERT_NOT_NULL(s);
	X_ASSERT_NOT_NULL(t);

	rThread()->RC_DrawImageWithUV(xpos, ypos, z, w, h, texture_id, s, t, col, filtered);
}


void DX11XRender::RT_DrawImageWithUV(float xpos, float ypos, float z, float w, float h, 
	int texture_id, float* s, float* t, const Colorf& col, bool filtered)
{
	using namespace shader;

	float fx = xpos;
	float fy = ypos;
	float fw = w;
	float fh = h;

	SetCullMode(CullMode::NONE);
	SetFFE(true);

	// Lock the entire buffer and obtain a pointer to the location where we have to
	uint32 nOffs;
	Vertex_P3F_C4B_T2F* Quad = (Vertex_P3F_C4B_T2F*)m_DynVB[VertexPool::P3F_C4B_T2F].LockVB(4, nOffs);

	// TL
	Quad[0].pos.x = xpos;
	Quad[0].pos.y = ypos;
	Quad[0].pos.z = z;
	// TR
	Quad[1].pos.x = xpos + w;
	Quad[1].pos.y = ypos;
	Quad[1].pos.z = z;
	// BL
	Quad[2].pos.x = xpos;
	Quad[2].pos.y = ypos + h;
	Quad[2].pos.z = z;
	// BR
	Quad[3].pos.x = xpos + w;
	Quad[3].pos.y = ypos + h;
	Quad[3].pos.z = z;

	for (uint32 i = 0; i<4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f(s[i], t[i]);
	}

	// We are finished with accessing the vertex buffer
 	m_DynVB[VertexPool::P3F_C4B_T2F].UnlockVB();

	
	XTexState state;
	state.setFilterMode(FilterMode::POINT);
	state.setClampMode(TextureAddressMode::MIRROR, 
		TextureAddressMode::MIRROR, TextureAddressMode::MIRROR);

	// bind the texture.
	texture::XTexture::applyFromId(
		0,
		texture_id,
		texture::XTexture::getTexStateId(state)
	);
	

	// Bind our vertex as the first data stream of our device
	m_DynVB[VertexPool::P3F_C4B_T2F].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2F)))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}


void DX11XRender::DrawVB(Vertex_P3F_C4B_T2F* pVertBuffer, uint32_t size,
	PrimitiveTypePublic::Enum type)
{
	X_PROFILE_BEGIN("drawVB", core::ProfileSubSys::RENDER);

	X_ASSERT_NOT_NULL(pVertBuffer);

	if (size == 0)
		return;

	uint32 nOffs;
	Vertex_P3F_C4B_T2F* pVertBuf;
	
	pVertBuf = (Vertex_P3F_C4B_T2F*)m_DynVB[VertexPool::P3F_C4B_T2F].LockVB(size, nOffs);

	// copy data into gpu buffer.
	memcpy(pVertBuf, pVertBuffer, size * sizeof(Vertex_P3F_C4B_T2F));

	m_DynVB[VertexPool::P3F_C4B_T2F].UnlockVB();
	m_DynVB[VertexPool::P3F_C4B_T2F].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2F)))
		return;


	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveTypeToInternal(type), nOffs, size);
}

void DX11XRender::DrawQuad(float x, float y, float width, float height, const Color& col)
{
	DrawQuad(x,y,0.f,width,height,col);
}

void DX11XRender::DrawQuad(float x, float y, float width, float height, 
	const Color& col, const Color& borderCol)
{
	DrawQuad(x, y, 0.f, width, height, col);
	DrawRect(x, y, width, height, borderCol);
}

void DX11XRender::DrawQuad(float x, float y, float z, float width, float height,
	const Color& col, const Color& borderCol)
{
	DrawQuad(x, y, z, width, height, col);
	DrawRect(x, y, width, height, borderCol);
}

void DX11XRender::DrawQuad(float x, float y, float z, float width, float height, const Color& col)
{
	SetCullMode(CullMode::NONE);
	SetFFE(false);

	float fx = x;
	float fy = y;
	float fz = z;
	float fw = width;
	float fh = height;

	uint32 nOffs;
	Vertex_P3F_C4B_T2F* Quad = (Vertex_P3F_C4B_T2F*)m_DynVB[VertexPool::P3F_C4B_T2F].LockVB(4, nOffs);

	// TL
	Quad[0].pos.x = fx;
	Quad[0].pos.y = fy;
	Quad[0].pos.z = fz;
	// TR
	Quad[1].pos.x = fx + fw;
	Quad[1].pos.y = fy;
	Quad[1].pos.z = fz;
	// BL
	Quad[2].pos.x = fx;
	Quad[2].pos.y = fy + fh;
	Quad[2].pos.z = fz;
	// BR
	Quad[3].pos.x = fx + fw;
	Quad[3].pos.y = fy + fh;
	Quad[3].pos.z = fz;

	for (uint32 i = 0; i<4; ++i)
	{
		Quad[i].color = col;
		Quad[i].st = Vec2f::zero();
	}

	m_DynVB[VertexPool::P3F_C4B_T2F].UnlockVB();
	m_DynVB[VertexPool::P3F_C4B_T2F].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2F)))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleStrip, nOffs, 4);
}

void DX11XRender::DrawQuad(Vec2<float> pos, float width, float height, const Color& col)
{
	DrawQuad(pos.x, pos.y, width, height, col);
}


void DX11XRender::DrawLines(Vec3f* points, uint32_t num, const Color& col)
{
	X_ASSERT_NOT_NULL(points);

	if (num < 2) // 2 points needed to make a line.
		return;

	rThread()->RC_DrawLines(points, num, col);
}


void DX11XRender::DrawLine(const Vec3f& pos1, const Vec3f& pos2)
{
	SetCullMode(CullMode::NONE);
	SetFFE(false);

	uint32 nOffs;
	Vertex_P3F_C4B_T2F* Quad = (Vertex_P3F_C4B_T2F*)m_DynVB[VertexPool::P3F_C4B_T2F].LockVB(2, nOffs);

	Quad[0].pos = pos1;
	Quad[0].color = Color::white();
	Quad[0].st = Vec2f::zero();

	Quad[1].pos = pos2;
	Quad[1].color = Color::white();
	Quad[1].st = Vec2f::zero();

	m_DynVB[VertexPool::P3F_C4B_T2F].UnlockVB();
	m_DynVB[VertexPool::P3F_C4B_T2F].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2F)))
		return;

	// Render the line
	FX_DrawPrimitive(PrimitiveType::LineList, nOffs, 2);
}


void DX11XRender::DrawLineColor(const Vec3f& pos1, const Color& color1,
	const Vec3f& pos2, const Color& vColor2)
{
	SetFFE(false);

	// Lock the entire buffer and obtain a pointer to the location where we have to
	uint32 nOffs;
	Vertex_P3F_C4B_T2F* Quad = (Vertex_P3F_C4B_T2F*)m_DynVB[VertexPool::P3F_C4B_T2F].LockVB(2, nOffs);

	Quad[0].pos = pos1;
	Quad[0].color = color1;
	Quad[1].pos = pos2;
	Quad[1].color = color1;

	m_DynVB[VertexPool::P3F_C4B_T2F].UnlockVB();
	m_DynVB[VertexPool::P3F_C4B_T2F].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2F)))
		return;

	// Render the line
	FX_DrawPrimitive(PrimitiveType::LineList, nOffs, 2);
}

void DX11XRender::DrawRect(float x, float y, float width, float height, Color col)
{
	float x1 = x;
	float y1 = y;
	float x2 = x + width;
	float y2 = y + height;

	// Top
	DrawLineColor(Vec3f(x1, y1, 0), col, Vec3f(x2, y1, 0), col);
	// bottom
	DrawLineColor(Vec3f(x1, y2, 0), col, Vec3f(x2, y2, 0), col);
	// left down
	DrawLineColor(Vec3f(x1, y1, 0), col, Vec3f(x1, y2, 0), col);
	// right down
	DrawLineColor(Vec3f(x2, y1, 0), col, Vec3f(x2, y2, 0), col);
}

void DX11XRender::DrawBarChart(const Rectf& rect, uint32_t num, float* heights,
	float padding, uint32_t max)
{
	X_ASSERT_NOT_NULL(heights);
	X_ASSERT(num <= max, "Darw Chart has more items than max")(num, max);

	if (num < 1)
		return;

	// calculate the bar width.
	const float bar_width = ((rect.getWidth() / max) - padding) + padding / max;
	

	uint32 i, nOffs;
	Vertex_P3F_C4B_T2F* Quads = (Vertex_P3F_C4B_T2F*)m_DynVB[VertexPool::P3F_C4B_T2F].LockVB(num * 6, nOffs);


	float right = rect.getX2();
	float bottom = rect.getY2();
	float height = rect.getHeight();
	float width = rect.getWidth();

	Color8u col8(Col_Coral);


	// TL - TR - BR
	// BR - BL - TL
	for (i = 0; i < num; i++)
	{
		Vertex_P3F_C4B_T2F* Quad = &Quads[i*6];
		float cur_bar = heights[i];

		// TL
		Quad[0].pos.x = right - bar_width;
		Quad[0].pos.y = bottom - (height * cur_bar);
		Quad[0].pos.z = 0.f;
		Quad[0].color = col8;

		// TR
		Quad[1].pos.x = right;
		Quad[1].pos.y = bottom - (height * cur_bar);
		Quad[1].pos.z = 0.f;
		Quad[1].color = col8;

		// BR
		Quad[2].pos.x = right;
		Quad[2].pos.y = bottom;
		Quad[2].pos.z = 0.f;
		Quad[2].color = col8;

		// BR
		Quad[3].pos.x = right;
		Quad[3].pos.y = bottom;
		Quad[3].pos.z = 0.f;
		Quad[3].color = col8;

		// BL
		Quad[4].pos.x = right - bar_width;
		Quad[4].pos.y = bottom;
		Quad[4].pos.z = 0.f;
		Quad[4].color = col8;

		// TL
		Quad[5].pos.x = right - bar_width;
		Quad[5].pos.y = bottom - (height * cur_bar);
		Quad[5].pos.z = 0.f;
		Quad[5].color = col8;

		right -= (bar_width + padding);
	}

	m_DynVB[VertexPool::P3F_C4B_T2F].UnlockVB();
	m_DynVB[VertexPool::P3F_C4B_T2F].Bind();

	if (FAILED(FX_SetVertexDeclaration(shader::VertexFormat::P3F_C4B_T2F)))
		return;

	// Render the two triangles from the data stream
	FX_DrawPrimitive(PrimitiveType::TriangleList, nOffs, 6 * num);	
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
