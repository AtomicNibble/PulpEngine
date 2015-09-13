#include "stdafx.h"
#include "Dx10Render.h"

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

} // namespace


void DX11XRender::SetState(StateFlag state)
{
	pRt_->RC_SetState(state);
}

void DX11XRender::SetStencilState(StencilState::Value ss)
{
	DepthState depth = curDepthState();

	StencilState::Value::Face& front = ss.faces[0];

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
	pRt_->RC_SetCullMode(mode);
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

	for (i = 0; i< safe_static_cast<uint32_t, size_t>(BlendStates_.size()); i++)
	{
		if (BlendStates_[i] == state) {
			break;
		}
	}

	if (i == BlendStates_.size())
	{
		// we dont have this state of this type on the gpu yet.
		hr = DxDevice()->CreateBlendState(&state.Desc, &state.pState);
		// save it, and since we add 1 i becomes a valid index.
		BlendStates_.push_back(state);
	}

	// needs changing?
	if (i != CurBlendState_)
	{
		CurBlendState_ = i;
		DxDeviceContext()->OMSetBlendState(BlendStates_[i].pState, 0, 0xFFFFFFFF);
	}

	return SUCCEEDED(hr);
}


bool DX11XRender::SetRasterState(RasterState& state)
{
	// try find a matching state.
	uint32_t i;
	HRESULT hr = S_OK;

	state.createHash();

	for (i = 0; i< safe_static_cast<uint32_t, size_t>(RasterStates_.size()); i++)
	{
		if (RasterStates_[i] == state) {
			break;
		}
	}

	if (i == RasterStates_.size())
	{
		// we dont have this state of this type on the gpu yet.
		hr = DxDevice()->CreateRasterizerState(&state.Desc, &state.pState);
		// save it, and since we add 1 i becomes a valid index.
		RasterStates_.push_back(state);
	}

	// needs changing?
	if (i != CurRasterState_)
	{
		CurRasterState_ = i;
		DxDeviceContext()->RSSetState(RasterStates_[i].pState);
	}

	return SUCCEEDED(hr);
}


bool DX11XRender::SetDepthState(DepthState& state)
{
	// try find a matching state.
	uint32_t i;
	HRESULT hr = S_OK;

	state.createHash();

	for (i = 0; i< safe_static_cast<uint32_t,size_t>(DepthStates_.size()); i++)
	{
		if (DepthStates_[i] == state) {
			break;
		}
	}

	if (i == DepthStates_.size())
	{
		// we dont have this state of this type on the gpu yet.
		hr = DxDevice()->CreateDepthStencilState(&state.Desc, &state.pState);
		// save it, and since we add 1 i becomes a valid index.
		DepthStates_.push_back(state);

		D3DDebug::SetDebugObjectName(state.pState, __FUNCTION__);
	}

	// needs changing?
	if (i != CurDepthState_)
	{
		CurDepthState_ = i;
		DxDeviceContext()->OMSetDepthStencilState(DepthStates_[i].pState, 0);
	}

	return SUCCEEDED(hr);
}



X_NAMESPACE_END