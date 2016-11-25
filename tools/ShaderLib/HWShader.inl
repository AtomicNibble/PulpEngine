

X_NAMESPACE_BEGIN(render)

namespace shader
{
	X_INLINE const int32_t XHWShader::getID(void) const
	{
		return id_;
	}

	X_INLINE void XHWShader::setID(int32_t id)
	{
		id_ = id;
	}

	X_INLINE const core::string& XHWShader::getName(void) const {
		return name_;
	}
	X_INLINE const core::string& XHWShader::getSourceFileName(void) const {
		return sourceFileName_;
	}
	X_INLINE const core::string& XHWShader::getEntryPoint(void) const {
		return entryPoint_;
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

	X_INLINE int32_t XHWShader::getNumRenderTargets(void) const {
		return numRenderTargets_;
	}
	X_INLINE int32_t XHWShader::getNumSamplers(void) const {
		return numSamplers_;
	}
	X_INLINE int32_t XHWShader::getNumTextures(void) const {
		return numTextures_;
	}
	X_INLINE int32_t XHWShader::getNumConstantBuffers(void) const {
		return safe_static_cast<int32_t>(cbuffers_.size());
	}
	X_INLINE int32_t XHWShader::getNumInputParams(void) const {
		return numInputParams_;
	}
	X_INLINE int32_t XHWShader::getSourceCrc32(void) const {
		return sourceCrc32_;
	}
	X_INLINE int32_t XHWShader::getD3DCompileFlags(void) const {
		return D3DCompileflags_;
	}


	X_INLINE bool XHWShader::isValid(void) const
	{
		return status_ == ShaderStatus::ReadyToRock;
	}

	X_INLINE bool XHWShader::isILFmtValid(void) const
	{
		return 	IlFmt_ != InputLayoutFormat::Invalid;
	}

	X_INLINE bool XHWShader::FailedtoCompile(void) const
	{
		return status_ == ShaderStatus::FailedToCompile;
	}

	X_INLINE bool XHWShader::isCompiling(void) const
	{
		return status_ == ShaderStatus::Compiling || status_ == ShaderStatus::AsyncCompileDone;
	}

	X_INLINE const XHWShader::CBufferArr& XHWShader::getCBuffers(void) const
	{
		return cbuffers_;
	}

	X_INLINE XHWShader::CBufferArr& XHWShader::getCBuffers(void)
	{
		return cbuffers_;
	}

	X_INLINE const core::Array<uint8_t>& XHWShader::getShaderByteCode(void) const
	{
		return bytecode_;
	}


} // namespace shader

X_NAMESPACE_END
