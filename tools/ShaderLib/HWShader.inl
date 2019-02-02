

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

    X_INLINE XHWShader::LockType& XHWShader::getLock(void)
    {
        return lock_;
    }

    X_INLINE const core::string& XHWShader::getName(void) const
    {
        return name_;
    }
    X_INLINE const core::string& XHWShader::getEntryPoint(void) const
    {
        return entryPoint_;
    }

    X_INLINE const core::string& XHWShader::getShaderSource(void) const
    {
        return sourceFile_;
    }

    X_INLINE ShaderStatus::Enum XHWShader::getStatus(void) const
    {
        return status_;
    }
    X_INLINE PermatationFlags XHWShader::getPermFlags(void) const
    {
        return permFlags_;
    }
    X_INLINE ILFlags XHWShader::getILFlags(void) const
    {
        return ILFlags_;
    }
    X_INLINE ShaderType::Enum XHWShader::getType(void) const
    {
        return type_;
    }
    X_INLINE InputLayoutFormat::Enum XHWShader::getILFormat(void) const
    {
        return IlFmt_;
    }

    X_INLINE int32_t XHWShader::getNumRenderTargets(void) const
    {
        return numRenderTargets_;
    }
    X_INLINE int32_t XHWShader::getNumSamplers(void) const
    {
        return safe_static_cast<int32_t>(samplers_.size());
    }
    X_INLINE int32_t XHWShader::getNumTextures(void) const
    {
        return safe_static_cast<int32_t>(textures_.size());
    }
    X_INLINE int32_t XHWShader::getNumConstantBuffers(void) const
    {
        return safe_static_cast<int32_t>(cbuffers_.size());
    }
    X_INLINE int32_t XHWShader::getNumBuffers(void) const
    {
        return safe_static_cast<int32_t>(buffers_.size());
    }
    X_INLINE int32_t XHWShader::getNumInputParams(void) const
    {
        return numInputParams_;
    }
    X_INLINE int32_t XHWShader::getNumInstructions(void) const
    {
        return numInstructions_;
    }
    X_INLINE XHWShader::HashVal XHWShader::getInputBindHash(void) const
    {
        return inputBindHash_;
    }
    X_INLINE CompileFlags XHWShader::getCompileFlags(void) const
    {
        return compileFlags_;
    }

    X_INLINE ErrorInfo XHWShader::getErrorInfo(void) const
    {
        return errInfo_;
    }

#if X_ENABLE_RENDER_SHADER_RELOAD

    X_INLINE int32_t XHWShader::getCompileCount(void) const
    {
        return compileCount_;
    }

#endif // !X_ENABLE_RENDER_SHADER_RELOAD

    X_INLINE bool XHWShader::isValid(void) const
    {
        return status_ == ShaderStatus::Ready;
    }

    X_INLINE bool XHWShader::isILFmtValid(void) const
    {
        return IlFmt_ != InputLayoutFormat::Invalid;
    }

    X_INLINE bool XHWShader::hasFailedtoCompile(void) const
    {
        return status_ == ShaderStatus::FailedToCompile;
    }

    X_INLINE bool XHWShader::isCompiling(void) const
    {
        return status_ == ShaderStatus::Compiling || status_ == ShaderStatus::AsyncCompileDone;
    }

    X_INLINE void XHWShader::markStale(void)
    {
        if (isValid()) {
            status_ = ShaderStatus::NotCompiled;
        }
    }

    X_INLINE const XHWShader::CBufferArr& XHWShader::getCBuffers(void) const
    {
        return cbuffers_;
    }

    X_INLINE XHWShader::CBufferArr& XHWShader::getCBuffers(void)
    {
        return cbuffers_;
    }

    X_INLINE const XHWShader::BufferArr& XHWShader::getBuffers(void) const
    {
        return buffers_;
    }

    X_INLINE XHWShader::BufferArr& XHWShader::getBuffers(void)
    {
        return buffers_;
    }

    X_INLINE const XHWShader::SamplerArr& XHWShader::getSamplers(void) const
    {
        return samplers_;
    }

    X_INLINE XHWShader::SamplerArr& XHWShader::getSamplers(void)
    {
        return samplers_;
    }

    X_INLINE const XHWShader::TextureArr& XHWShader::getTextures(void) const
    {
        return textures_;
    }

    X_INLINE XHWShader::TextureArr& XHWShader::getTextures(void)
    {
        return textures_;
    }

    X_INLINE const XHWShader::ByteArr& XHWShader::getShaderByteCode(void) const
    {
        return bytecode_;
    }

} // namespace shader

X_NAMESPACE_END
