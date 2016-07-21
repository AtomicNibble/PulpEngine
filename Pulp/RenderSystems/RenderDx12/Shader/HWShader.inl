

X_NAMESPACE_BEGIN(render)

namespace shader
{
	X_INLINE const char* XHWShader::getName(void) const {
		return name_.c_str();
	}
	X_INLINE const char* XHWShader::getSourceFileName(void) const {
		return sourceFileName_.c_str();
	}
	X_INLINE const char* XHWShader::getEntryPoint(void) const {
		return entryPoint_.c_str();
	}

	X_INLINE ShaderStatus::Enum XHWShader::getStatus(void) const {
		return status_;
	}
	X_INLINE TechFlags XHWShader::getTechFlags(void) const {
		return techFlags_;
	}
	X_INLINE ShaderType::Enum XHWShader::getType(void) const {
		return type_;
	}
	X_INLINE InputLayoutFormat::Enum XHWShader::getILFormat(void) const {
		return IlFmt_;
	}

	X_INLINE uint32_t XHWShader::getNumRenderTargets(void) const {
		return numRenderTargets_;
	}
	X_INLINE uint32_t XHWShader::getNumSamplers(void) const {
		return numSamplers_;
	}
	X_INLINE uint32_t XHWShader::getNumConstantBuffers(void) const {
		return numConstBuffers_;
	}
	X_INLINE uint32_t XHWShader::getNumInputParams(void) const {
		return numInputParams_;
	}

	X_INLINE bool XHWShader::isValid(void) const
	{
		return status_ == ShaderStatus::ReadyToRock;
	}

	X_INLINE bool XHWShader::FailedtoCompile(void) const
	{
		return status_ == ShaderStatus::FailedToCompile;
	}

	X_INLINE bool XHWShader::isCompiling(void) const
	{
		return status_ == ShaderStatus::Compiling || status_ == ShaderStatus::AsyncCompileDone;
	}

	X_INLINE ID3DBlob* XHWShader::getshaderBlob(void) const
	{
		return pBlob_;
	}

#if 0

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



	void XHWShader_Dx10::setShader()
	{
		X_ASSERT_NOT_IMPLEMENTED();
	}

	void XHWShader_Dx10::setConstBuffer(ShaderType::Enum type, uint slot, ID3D11Buffer* pBuf)
	{
		X_ASSERT_NOT_IMPLEMENTED();

	}



	void XHWShader_Dx10::unMapConstbuffer(ShaderType::Enum shaderType,
		ConstbufType::Enum bufType)
	{
		X_ASSERT_NOT_IMPLEMENTED();

	}


	bool XHWShader_Dx10::mapConstBuffer(ShaderType::Enum shaderType,
		ConstbufType::Enum bufType, int maxVectors)
	{
		X_ASSERT_NOT_IMPLEMENTED();

		return false;
	}

	bool XHWShader_Dx10::createConstBuffer(ShaderType::Enum shaderType,
		ConstbufType::Enum bufType, int maxVectors)
	{
		X_ASSERT_NOT_IMPLEMENTED();
		return false;
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

#endif 

} // namespace shader

X_NAMESPACE_END
