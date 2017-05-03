
X_NAMESPACE_BEGIN(render)

namespace shader
{
	X_INLINE void Texture::setName(const core::string& name)
	{
		name_ = name;
	}

	X_INLINE void Texture::setName(const char* pName)
	{
		name_ = pName;
	}

	X_INLINE const core::string& Texture::getName(void) const
	{
		return name_;
	}

} // namespace shader

X_NAMESPACE_END