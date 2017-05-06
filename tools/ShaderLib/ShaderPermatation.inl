

X_NAMESPACE_BEGIN(render)

namespace shader
{



	X_INLINE bool ShaderPermatation::isStageSet(ShaderType::Enum type) const
	{
		return stages_[type] != nullptr;
	}

	X_INLINE XHWShader* ShaderPermatation::getStage(ShaderType::Enum type) const
	{
		return stages_[type];
	}

	X_INLINE InputLayoutFormat::Enum ShaderPermatation::getILFmt(void) const
	{
		return IlFmt_;
	}

	X_INLINE const CBufLinksArr& ShaderPermatation::getCbufferLinks(void) const
	{
		return cbLinks_;
	}

	X_INLINE const ShaderPermatation::SamplerArr& ShaderPermatation::getSamplers(void) const
	{
		if (!isStageSet(ShaderType::Pixel)) {
			static SamplerArr nill(g_rendererArena);
			return nill;
		}

		return getStage(ShaderType::Pixel)->getSamplers();
	}

	X_INLINE const typename ShaderPermatation::HWShaderStagesArr& ShaderPermatation::getStages(void) const
	{
		return stages_;
	}

} // namespace shader

X_NAMESPACE_END