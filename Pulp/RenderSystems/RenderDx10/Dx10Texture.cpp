#include "stdafx.h"

#include "IShader.h"
#include "ITexture.h"
#include "../Common/Textures/XTexture.h"
#include "../Common/Textures/XTextureFile.h"

#include "Dx10Render.h"


X_USING_NAMESPACE;

using namespace texture;
using namespace render;
using namespace shader;


XTexture* XTexture::s_pCurrentTexture[TEX_MAX_SLOTS] = { nullptr };

bool XTexture::createDeviceTexture(core::ReferenceCountedOwner<XTextureFile>& image_data)
{
#if X_DEBUG
	image_data->pName_ = this->FileName.c_str();
#endif // !X_DEBUG
	return g_Dx11D3D.rThread()->RC_CreateDeviceTexture(this, image_data.instance());
}


bool XTexture::RT_CreateDeviceTexture(XTextureFile* image_data)
{
	/// must be called from the render thread :)
	ReleaseDeviceTexture();

	// cube map is just multiple 2D textures.
	if (this->type == TextureType::T2D || this->type == TextureType::TCube)
	{
		if (!g_Dx11D3D.Create2DTexture(image_data, this->DeviceTexture))
		{
			X_FATAL("Texture", "failed to create 2dTexture");
			return false;
		}
	}
	else
	{
		X_ASSERT_NOT_IMPLEMENTED();
		return false;
	}

	// Create a ShaderResiyrceView.
	ID3D11ShaderResourceView* pSRView = nullptr;
	ID3D11Texture2D* pTexture;
	ID3D11Device* dv = g_Dx11D3D.DxDevice();

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	HRESULT hr;

	core::zero_object(srvDesc);

	pTexture = this->DeviceTexture.get2DTexture();

	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DCGIFormatFromTexFmt(image_data->getFormat());
	srvDesc.Texture2D.MipLevels = image_data->getNumMips();
	srvDesc.Texture2D.MostDetailedMip = 0;

	hr = dv->CreateShaderResourceView(pTexture, &srvDesc, &pSRView);
	
	if (FAILED(hr))
	{
		X_FATAL("Texture", "failed to create shader resource");
		return false;
	}
	else
	{
		X_LOG0("Texture", "\"%s\" uploaded to gpu", this->FileName.c_str());
	}

	// we only delete on true.
	// caller decides what todo on fail.
//	if (!image_data->dontDelete())
//		X_DELETE(image_data, g_rendererArena);


	D3DDebug::SetDebugObjectName(pSRView, this->FileName);

	setTexStates();

	// set the SRV member.
	pDeviceShaderResource_ = pSRView;
	return true;
}

bool XTexture::ReleaseDeviceTexture(void)
{
	ID3D11ShaderResourceView* pSRView = (ID3D11ShaderResourceView*)pDeviceShaderResource_;

	if (pSRView) {
		pSRView->Release();
		pSRView = nullptr;
	}

	DeviceTexture.Release();

	return true;
}

void XTexture::setTexStates()
{
	s_DefaultTexState = s_GlobalDefaultTexState;

	defaultTexStateId_ = XTexture::getTexStateId(s_DefaultTexState);
}

void XTexture::setSamplerState(int tex_sampler_id, int state_id, ShaderType::Enum shader_type)
{
	if (state_id < 0) // if no state use this textures default texture state.
		state_id = defaultTexStateId_;



	XTexState& state = s_TexStates[state_id];
	ID3D11DeviceContext* pd3dContex = render::g_Dx11D3D.DxDeviceContext();
	ID3D11SamplerState* pSamp;

	pSamp = (ID3D11SamplerState*)state.getDeviceState();

	if (shader_type == ShaderType::Pixel)
		pd3dContex->PSSetSamplers(tex_sampler_id, 1, &pSamp);
	else if (shader_type == ShaderType::Vertex)
		pd3dContex->VSSetSamplers(tex_sampler_id, 1, &pSamp);
	else
	{
		X_ASSERT_NOT_IMPLEMENTED();
	}
	
}

void XTexture::unbind(void)
{
	uint32_t i;
	for (i = 0; i < TEX_MAX_SLOTS; i++) {
		if (s_pCurrentTexture[i] == this) {
			s_pCurrentTexture[i] = nullptr;
			ID3D11ShaderResourceView *RV = NULL;
			render::g_Dx11D3D.DxDeviceContext()->PSSetShaderResources(i, 1, &RV);
		}
	}
}

X_DISABLE_WARNING(4458) //  warning C4458: declaration of 'type' hides class member
void XTexture::apply(int slot, int state_id, shader::ShaderType::Enum type)
{
	ID3D11DeviceContext* dv = g_Dx11D3D.DxDeviceContext();
	ID3D11ShaderResourceView* pResView = nullptr;

	if (slot >= TEX_MAX_SLOTS) {
		X_ERROR("Texture", "texture slot exceededs max. slot: %i max: %i");
		return;
	}

	// valid?
	if (this->flags.IsSet(TexFlag::LOAD_FAILED))
	{
		XTexture::s_pTexDefault->apply(slot,state_id, type);
		return;
	}

	// changed?
	if (s_pCurrentTexture[slot] == this)
		return;
	s_pCurrentTexture[slot] = this;


	// can't bind a texture that dose not have a resource.
	X_ASSERT_NOT_NULL(this->pDeviceShaderResource_);

	setSamplerState(slot, state_id, type);

	pResView = (ID3D11ShaderResourceView*)this->pDeviceShaderResource_;

	dv->PSSetShaderResources(slot, 1, &pResView);
}
X_ENABLE_WARNING(4458)

void XTexture::updateTextureRegion(byte* data, int nX, int nY, int USize, int VSize, 
	Texturefmt::Enum srcFmt)
{
	g_Dx11D3D.rThread()->RC_UpdateTextureRegion(this, data, nX, nY, USize, VSize, srcFmt);
}

void XTexture::RT_UpdateTextureRegion(byte* data, int nX, int nY, int USize, int VSize, 
	Texturefmt::Enum srcFmt)
{
	if (this->type != TextureType::T2D) {
		X_ERROR("Texture", "can't update a none 2d texture");
		return;
	}
	
//	DXGI_FORMAT frmtSrc = (DXGI_FORMAT)DCGIFormatFromTexFmt(srcFmt);
	X_DISABLE_WARNING(4838) // conversion from 'int' to 'UINT' requires a narrowing conversion
	D3D11_BOX rc = { nX, nY, 0, nX + USize, nY + VSize, 1 };
	X_ENABLE_WARNING(4838);

	if (!is_dxt(srcFmt))
	{
		int rowPitch = get_data_size(USize, 1, 1, 1, srcFmt);

		render::g_Dx11D3D.DxDeviceContext()->UpdateSubresource(
			DeviceTexture.get2DTexture(),
			0, 
			&rc, 
			data, 
			rowPitch, 
			0
		);
	}

}


ID3D11RenderTargetView* XTexture::getRenderTargetView(void)
{
	ID3D11RenderTargetView* pRenTarView = (ID3D11RenderTargetView*)pDeviceRenderTargetView_;
	HRESULT hr = S_OK;

	if (pRenTarView == nullptr)
	{
		D3D11_RENDER_TARGET_VIEW_DESC DescRT;
		ID3D11Texture2D* p2dTex = nullptr;

		if (type == TextureType::T2D)
		{
			p2dTex = DeviceTexture.get2DTexture();

			DescRT.Format = DCGIFormatFromTexFmt(format);
			DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			DescRT.Texture2D.MipSlice = 0;

			hr = render::g_Dx11D3D.DxDevice()->CreateRenderTargetView(
				p2dTex, &DescRT, &pRenTarView);
		}
		else if (type == TextureType::TCube)
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}
		else
		{
			X_ASSERT_UNREACHABLE();
		}

		// if HR has failed shit is goaty!
		if (FAILED(hr)) {
			X_ASSERT(false,"failed to create render target view.")(hr);
			pRenTarView = nullptr;
		}
	}

	return pRenTarView;
}


// ================================== Static ====================================


bool XTexture::setDefaultFilterMode(FilterMode::Enum filter)
{
	s_GlobalDefaultTexState.setClampMode(
		TextureAddressMode::WRAP,
		TextureAddressMode::WRAP,
		TextureAddressMode::WRAP
		);

	s_GlobalDefaultTexState.setFilterMode(filter);

	// get it.
	s_GlobalDefaultTexStateId = getTexStateId(s_GlobalDefaultTexState);


	return true;
}

// ~================================== Static ====================================


// ======================================================================

namespace {

	static char TextureAddressModeToD3D(TextureAddressMode::Enum type)
	{
		char mode;

		switch (type)
		{
			case TextureAddressMode::WRAP:
				mode = D3D11_TEXTURE_ADDRESS_WRAP;
			break;
			case TextureAddressMode::CLAMP:
				mode = D3D11_TEXTURE_ADDRESS_CLAMP;
			break;
			case TextureAddressMode::BORDER:
				mode = D3D11_TEXTURE_ADDRESS_BORDER;
			break;
			case TextureAddressMode::MIRROR:
				mode = D3D11_TEXTURE_ADDRESS_MIRROR;
			break;
			default:		
#if X_DEBUG || X_ENABLE_ASSERTIONS
			X_ASSERT_UNREACHABLE();
			return D3D11_TEXTURE_ADDRESS_WRAP;
#else
			X_NO_SWITCH_DEFAULT;
#endif
		}
		return mode;
	}

}

void XTexState::setComparisonFilter(bool bEnable)
{
	clearDevice();

	m_bComparison = bEnable;
}

bool XTexState::setClampMode(
	TextureAddressMode::Enum addressU, 
	TextureAddressMode::Enum addressV, 
	TextureAddressMode::Enum addressW)
{
	clearDevice();

	m_nAddressU = TextureAddressModeToD3D(addressU);
	m_nAddressV = TextureAddressModeToD3D(addressV);
	m_nAddressW = TextureAddressModeToD3D(addressW);
	return true;
}

bool XTexState::setFilterMode(FilterMode::Enum filter)
{
/*	if (nFilter < 0)
	{
		XTexState *pTS = &CTexture::s_TexStates[CTexture::s_nGlobalDefState];
		m_nMinFilter = pTS->m_nMinFilter;
		m_nMagFilter = pTS->m_nMagFilter;
		m_nMipFilter = pTS->m_nMipFilter;
		return true;
	}
	*/

	clearDevice();

	switch (filter)
	{
		case FilterMode::POINT:
		case FilterMode::NONE:
			m_nMinFilter = FilterMode::POINT;
			m_nMagFilter = FilterMode::POINT;
			m_nMipFilter = FilterMode::NONE;
			break;
		case FilterMode::LINEAR:
			m_nMinFilter = FilterMode::LINEAR;
			m_nMagFilter = FilterMode::LINEAR;
			m_nMipFilter = FilterMode::NONE;
			break;
		case FilterMode::BILINEAR:
			m_nMinFilter = FilterMode::LINEAR;
			m_nMagFilter = FilterMode::LINEAR;
			m_nMipFilter = FilterMode::POINT;
			break;
		case FilterMode::TRILINEAR:
			m_nMinFilter = FilterMode::LINEAR;
			m_nMagFilter = FilterMode::LINEAR;
			m_nMipFilter = FilterMode::LINEAR;
			break;

#if X_RENDER_ALLOW_ANISOTROPIC
		case FilterMode::ANISO2X:
		case FilterMode::ANISO4X:
		case FilterMode::ANISO8X:
		case FilterMode::ANISO16X:

		X_ASSERT_NOT_IMPLEMENTED();
				
			break;
#endif // !X_RENDER_ALLOW_ANISOTROPIC


		default:
#if X_DEBUG || X_ENABLE_ASSERTIONS
		X_ASSERT_UNREACHABLE();
		return false;
#else
		X_NO_SWITCH_DEFAULT;
#endif
	}

	return true;
}

void XTexState::setBorderColor(Color8u color)
{
	clearDevice();
	m_dwBorderColor = color;
}

void XTexState::postCreate()
{
	// already got a device state?
	if (m_pDeviceState)
		return;

	D3D11_SAMPLER_DESC Desc;
	ID3D11SamplerState *pSamp = NULL;
	core::zero_object(Desc);

	Desc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)m_nAddressU;
	Desc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)m_nAddressV;
	Desc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)m_nAddressW;
	Colorf col = Colorf(m_dwBorderColor);
	Desc.BorderColor[0] = col.r;
	Desc.BorderColor[1] = col.g;
	Desc.BorderColor[2] = col.b;
	Desc.BorderColor[3] = col.a;

	if (m_bComparison)
		Desc.ComparisonFunc = D3D11_COMPARISON_LESS;
	else
		Desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	Desc.MaxAnisotropy = 1;
	Desc.MinLOD = 0;

	if (m_nMipFilter == FilterMode::NONE)
	{
		Desc.MaxLOD = 0.0f;
	}
	else
	{
		Desc.MaxLOD = 100.0f;
	}

	Desc.MipLODBias = 0;

	if (m_bComparison)
	{
		if (m_nMinFilter == FilterMode::LINEAR && 
			m_nMagFilter == FilterMode::LINEAR && 
			m_nMipFilter == FilterMode::LINEAR ||
			m_nMinFilter == FilterMode::TRILINEAR ||
			m_nMagFilter == FilterMode::TRILINEAR)
		{
			Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		}
		else if (
			m_nMinFilter == FilterMode::LINEAR && 
			m_nMagFilter == FilterMode::LINEAR &&
			(m_nMipFilter == FilterMode::NONE || m_nMipFilter == FilterMode::POINT) ||
			m_nMinFilter == FilterMode::BILINEAR ||
			m_nMagFilter == FilterMode::BILINEAR)
		{
			Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		}
		else if (
			m_nMinFilter == FilterMode::POINT && 
			m_nMagFilter == FilterMode::POINT &&
			(m_nMipFilter == FilterMode::NONE || m_nMipFilter == FilterMode::POINT))
		{
			Desc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		}

	}
	else
	{
		if (m_nMinFilter == FilterMode::LINEAR && 
			m_nMagFilter == FilterMode::LINEAR && 
			m_nMipFilter == FilterMode::LINEAR ||
			m_nMinFilter == FilterMode::TRILINEAR || 
			m_nMagFilter == FilterMode::TRILINEAR)
		{
			Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
		else if (
			m_nMinFilter == FilterMode::LINEAR && 
			m_nMagFilter == FilterMode::LINEAR && 
			(m_nMipFilter == FilterMode::NONE || m_nMipFilter == FilterMode::POINT) || 
			m_nMinFilter == FilterMode::BILINEAR || 
			m_nMagFilter == FilterMode::BILINEAR)
		{
			Desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		}
		else if (
			m_nMinFilter == FilterMode::POINT && 
			m_nMagFilter == FilterMode::POINT && 
			(m_nMipFilter == FilterMode::NONE || m_nMipFilter == FilterMode::POINT))
		{
			Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		}
#if X_RENDER_ALLOW_ANISOTROPIC
		else if (
			m_nMinFilter >= FilterMode::ANISO2X &&
			m_nMagFilter >= FilterMode::ANISO2X &&
			m_nMipFilter >= FilterMode::ANISO2X)
		{
			Desc.Filter = D3D11_FILTER_ANISOTROPIC;
			Desc.MaxAnisotropy = m_nAnisotropy;
		}
#endif // !X_RENDER_ALLOW_ANISOTROPIC
		else
		{
			X_ASSERT_UNREACHABLE();
		}
	}

	HRESULT hr = render::g_Dx11D3D.DxDevice()->CreateSamplerState(&Desc, &pSamp);
	if (SUCCEEDED(hr))
	{
		m_pDeviceState = pSamp;

		D3DDebug::SetDebugObjectName(pSamp, "XTexState");
	}
	else
	{
		X_ASSERT(false, "Failed to create texture sampler")(hr);
	}
}


XTexState::~XTexState()
{
	clearDevice();
}

XTexState::XTexState(const XTexState& src)
{
	memcpy(this, &src, sizeof(src));
	if (m_pDeviceState)
	{
		ID3D11SamplerState *pSamp = (ID3D11SamplerState *)m_pDeviceState;
		pSamp->AddRef();
	}
}

void XTexState::clearDevice(void)
{
	if (m_pDeviceState) {
		ID3D11SamplerState* pSamp = (ID3D11SamplerState*)m_pDeviceState;
		pSamp->Release();
		m_pDeviceState = nullptr;
	}
}