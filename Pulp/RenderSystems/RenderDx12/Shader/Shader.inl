

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

	X_INLINE const int32_t XShader::getID(void) const
	{
		return id_;
	}

	X_INLINE void XShader::setID(int32_t id)
	{
		id_ = id;
	}


} // namespace shader

X_NAMESPACE_END