

X_NAMESPACE_BEGIN(render)


void DX11XRender::GetModelViewMatrix(Matrix44f* pMat) 
{
	*pMat = *ViewMat_.GetTop();
	*pMat *= modelMat_;
}

void DX11XRender::GetProjectionMatrix(Matrix44f* pMat)
{
	*pMat = *ProMat_.GetTop();
}


const D3D11_PRIMITIVE_TOPOLOGY DX11XRender::FX_ConvertPrimitiveType(
	const PrimitiveType::Enum type) const
{
	return static_cast<D3D11_PRIMITIVE_TOPOLOGY>(type);
}


PrimitiveType::Enum DX11XRender::PrimitiveTypeToInternal(PrimitiveTypePublic::Enum type) const
{
	if (type == PrimitiveTypePublic::LineList)
		return PrimitiveType::LineList;
	if (type == PrimitiveTypePublic::LineStrip)
		return PrimitiveType::LineStrip;
	if (type == PrimitiveTypePublic::TriangleList)
		return PrimitiveType::TriangleList;
	if (type == PrimitiveTypePublic::TriangleStrip)
		return PrimitiveType::TriangleStrip;
	X_ASSERT_UNREACHABLE();
	return PrimitiveType::TriangleStrip;
}

void DX11XRender::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topType)
{
	if (CurTopology_ != topType)
	{
		CurTopology_ = topType;
		deviceContext_->IASetPrimitiveTopology(CurTopology_);
	}
}


ID3D11Device* DX11XRender::DxDevice(void) const
{
	return device_;
}

ID3D11DeviceContext* DX11XRender::DxDeviceContext(void) const
{
	return deviceContext_;
}

void DX11XRender::PushViewMatrix(void)
{
	ViewMat_.Push();
}

void DX11XRender::PopViewMatrix(void)
{
	ViewMat_.Pop();
	DirtyMatrix();
}

void DX11XRender::DirtyMatrix(void)
{

}

Matrix44f* DX11XRender::pCurViewMat(void)
{
	return ViewMat_.GetTop();
}

Matrix44f* DX11XRender::pCurProjMat(void)
{
	return ProMat_.GetTop();
}

bool DX11XRender::IsDeviceLost(void) const
{
	return false;
}

void DX11XRender::SetModelMatrix(const Matrix44f& mat)
{
	modelMat_ = mat;
}


BlendState& DX11XRender::curBlendState(void)
{ 
	return BlendStates_[CurBlendState_]; 
}

RasterState& DX11XRender::curRasterState(void)
{ 
	return RasterStates_[CurRasterState_]; 
}

DepthState& DX11XRender::curDepthState(void)
{ 
	return DepthStates_[CurDepthState_]; 
}



X_NAMESPACE_END