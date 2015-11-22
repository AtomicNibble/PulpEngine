#pragma once



X_NAMESPACE_BEGIN(shader)


bool XHWShader_Dx10::bind()
{
	if (this->type_ == ShaderType::Vertex)
		return bindVS();
	if (this->type_ == ShaderType::Pixel)
		return bindPS();
	if (this->type_ == ShaderType::Geometry)
		return bindGS();
	X_ASSERT_UNREACHABLE();
	return false;
}


ShaderStatus::Enum XHWShader_Dx10::getStatus(void) const
{
	return status_;
}

bool XHWShader_Dx10::isValid(void) const
{
	return status_ == ShaderStatus::ReadyToRock;
}

bool XHWShader_Dx10::FailedtoCompile(void) const
{
	return status_ == ShaderStatus::FailedToCompile;
}

void XHWShader_Dx10::setShader()
{
	ID3D11DeviceContext* pDevice = render::g_Dx11D3D.DxDeviceContext();
	if (isValid())
	{
		if (this->type_ == ShaderType::Vertex)
			pDevice->VSSetShader(reinterpret_cast<ID3D11VertexShader*>(pHWHandle_), NULL, 0);
		else if (this->type_ == ShaderType::Pixel)
			pDevice->PSSetShader(reinterpret_cast<ID3D11PixelShader*>(pHWHandle_), NULL, 0);
		else if (this->type_ == ShaderType::Geometry)
			pDevice->GSSetShader(reinterpret_cast<ID3D11GeometryShader*>(pHWHandle_), NULL, 0);
		else
		{
			// O'Deer
			X_ASSERT_UNREACHABLE();
		}
	}
}

void XHWShader_Dx10::setConstBuffer(ShaderType::Enum type, uint slot, ID3D11Buffer* pBuf)
{
	ID3D11DeviceContext* pDevice = render::g_Dx11D3D.DxDeviceContext();

	switch (type)
	{
	case ShaderType::Vertex:
		pDevice->VSSetConstantBuffers(slot, 1, &pBuf);
		break;

	case ShaderType::Pixel:
		pDevice->PSSetConstantBuffers(slot, 1, &pBuf);
		break;

	case ShaderType::Geometry:
		pDevice->GSSetConstantBuffers(slot, 1, &pBuf);
		break;

#if X_DEBUG
	default:
		X_ASSERT_UNREACHABLE();
		break;
#else
		X_NO_SWITCH_DEFAULT;
#endif
	}
}



void XHWShader_Dx10::unMapConstbuffer(ShaderType::Enum shaderType,
	ConstbufType::Enum bufType)
{
	ID3D11DeviceContext* pDeviceContext;

	pDeviceContext = render::g_Dx11D3D.DxDeviceContext();

	// mapped?
	// if no values in the const buffer where set.
	// this will be null.
	// preventing a pointles update.
	if (!pConstBuffData_[shaderType][bufType]) {
		return;
	}

	// unmapp it.
	pDeviceContext->Unmap(pCurRequestCB_[shaderType][bufType], 0);

	// clear data pointer, to flag remapping.
	pConstBuffData_[shaderType][bufType] = nullptr;

	// Update device!
	setConstBuffer(shaderType, 0, pCurRequestCB_[shaderType][bufType]);
}


bool XHWShader_Dx10::mapConstBuffer(ShaderType::Enum shaderType,
	ConstbufType::Enum bufType, int maxVectors)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ID3D11DeviceContext* pDeviceContext;
	pDeviceContext = render::g_Dx11D3D.DxDeviceContext();

	if (pConstBuffData_[shaderType][bufType])
	{
		// mapping a buffer that's still mapped O.o?
		// send it to the gpu.
		unMapConstbuffer(shaderType, bufType);
	}

	if (!pConstBuffers_[shaderType][bufType])
	{
		X_ERROR("Shader", "Buffer pointers have not yet been init");
		return false;
	}

	// if buffer not created, make it :D !
	if (!createConstBuffer(shaderType, bufType, maxVectors))
	{
		X_ERROR("Shader", "failed to create device buffer");
		return false;
	}


	pCurRequestCB_[shaderType][bufType] = pConstBuffers_[shaderType][bufType][maxVectors];

	pDeviceContext->Map(
		pCurRequestCB_[shaderType][bufType],
		0,
		D3D11_MAP_WRITE_DISCARD,
		0,
		&mappedResource
		);

	pConstBuffData_[shaderType][bufType] = static_cast<Vec4f*>(mappedResource.pData);

	return true;
}

bool XHWShader_Dx10::createConstBuffer(ShaderType::Enum shaderType,
	ConstbufType::Enum bufType, int maxVectors)
{
	ID3D11Device* pDevice;
	D3D11_BUFFER_DESC bd;
	HRESULT hr = S_OK;

	if (maxVectors == 0) {
		return false;
	}

	// buffer already created?
	if (pConstBuffData_[shaderType][bufType]) {
		return true;
	}

	core::zero_object(bd);
	pDevice = render::g_Dx11D3D.DxDevice();

	// set size.
	curMaxVecs_[shaderType][bufType] = maxVectors;

	if (!pConstBuffers_[shaderType][bufType][maxVectors] && maxVectors)
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.ByteWidth = maxVectors * sizeof(Vec4f);
		hr = pDevice->CreateBuffer(&bd, NULL,
			&pConstBuffers_[shaderType][bufType][maxVectors]);
	}

	return SUCCEEDED(hr);
}


void XHWShader_Dx10::setConstBuffer(int nReg, int nCBufSlot, ShaderType::Enum shaderType,
	const Vec4f* pData, const int nVecs, int nMaxVecs)
{
	if (!pConstBuffData_[shaderType][nCBufSlot]) {
		mapConstBuffer(shaderType, (ConstbufType::Enum)nCBufSlot, nMaxVecs);
	}

	memcpy(&pConstBuffData_[shaderType][nCBufSlot][nReg], pData, nVecs << 4);
}


void XHWShader_Dx10::setParameterRegA(int nReg, int nCBufSlot, ShaderType::Enum shaderType,
	const Vec4f* pData, int nComps, int nMaxVecs)
{
	setConstBuffer(nReg, nCBufSlot, shaderType, pData, nComps, nMaxVecs);
}

void XHWShader_Dx10::setParameteri(XShaderParam* pParam, const Vec4f* pData,
	ShaderType::Enum shaderType, uint32_t maxVecs)
{
	if (!pParam || pParam->bind < 0)
		return;

	int nReg = pParam->bind;
	setParameterRegA(nReg, pParam->constBufferSlot, shaderType, pData, pParam->numParameters, maxVecs);
}

void XHWShader_Dx10::setParameterf(XShaderParam* pParam, const Vec4f* pData,
	ShaderType::Enum shaderType, uint32_t maxVecs)
{
	if (!pParam || pParam->bind < 0)
		return;

	int nReg = pParam->bind;
	setParameterRegA(nReg, pParam->constBufferSlot, shaderType, pData, pParam->numParameters, maxVecs);
}


int XHWShader_Dx10::getMaxVecs(XShaderParam* pParam) const
{
	return maxVecs_[pParam->constBufferSlot];
}

ID3DBlob* XHWShader_Dx10::getshaderBlob(void) const
{
	X_ASSERT_NOT_NULL(pBlob_);
	return pBlob_;
}


void XHWShader_Dx10::setMaxVecs(int maxVecs[3])
{
	memcpy(maxVecs_, maxVecs, sizeof(maxVecs_));
}

const core::Array<XShaderParam>& XHWShader_Dx10::getBindVars(void) const
{
	return bindVars_;
}

void XHWShader_Dx10::setBindVars(core::Array<XShaderParam>& vars)
{
	bindVars_ = vars;
}

uint32_t XHWShader_Dx10::getD3DCompileFlags(void) const
{
	return D3DCompileflags_;
}

void XHWShader_Dx10::setBlob(ID3DBlob* pBlob)
{
	pBlob_ = pBlob;
}

void XHWShader_Dx10::setStatus(ShaderStatus::Enum status)
{
	status_ = status;
}

X_NAMESPACE_END
