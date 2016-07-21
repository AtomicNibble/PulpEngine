#pragma once

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)


X_NAMESPACE_BEGIN(render)

namespace shader
{

	class ShaderVars
	{
	public:
		ShaderVars();
		~ShaderVars() = default;

		void RegisterVars(void);

		X_INLINE bool writeMergedSource(void) const;
		X_INLINE bool asyncCompile(void) const;

	private:
		int32_t writeMergedSource_;
		int32_t asyncShaderCompile_;

	};


} // namespace shader

X_NAMESPACE_END


#include "ShaderVars.inl"