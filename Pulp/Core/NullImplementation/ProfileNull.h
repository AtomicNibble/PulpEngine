#pragma once



X_NAMESPACE_BEGIN(core)

namespace profiler
{

	class ProfileNull : public IProfileReg
	{
		~ProfileNull() X_FINAL = default;

		void AddStartupProfileData(XProfileData* pData) X_FINAL;
		void AddProfileData(XProfileData* pData) X_FINAL;
	};

} // namespace profiler

X_NAMESPACE_END


