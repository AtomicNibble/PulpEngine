

X_NAMESPACE_BEGIN(render)

namespace shader
{

	X_INLINE const core::string& XShader::getName(void) const
	{ 
		return name_; 
	}

	X_INLINE size_t XShader::numTechs(void) const 
	{
		return techs_.size(); 
	}


} // namespace shader

X_NAMESPACE_END