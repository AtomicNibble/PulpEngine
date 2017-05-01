#pragma once


X_NAMESPACE_DECLARE(core,
	struct XFile;
)



X_NAMESPACE_BEGIN(render)

namespace shader
{
	class Sampler
	{
	public:
		Sampler() = default;
		Sampler(const char* pName, int16_t bindPoint, int16_t bindCount);
		Sampler(core::string& name, int16_t bindPoint, int16_t bindCount);

		bool SSave(core::XFile* pFile) const;
		bool SLoad(core::XFile* pFile);

	private:
		core::string name_;
		int16_t bindPoint_;
		int16_t bindCount_;
	};


} // namespace shader

X_NAMESPACE_END