
X_NAMESPACE_BEGIN(render)

namespace shader
{
	X_INLINE void Sampler::setName(const core::string& name)
	{
		name_ = name;
	}

	X_INLINE void Sampler::setName(const char* pName)
	{
		name_ = pName;
	}

	X_INLINE const core::string& Sampler::getName(void) const
	{
		return name_;
	}

} // namespace shader

X_NAMESPACE_END