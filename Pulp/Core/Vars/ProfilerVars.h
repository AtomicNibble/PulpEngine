#pragma once


X_NAMESPACE_BEGIN(core)

namespace profiler
{

	class ProfilerVars
	{
	public:
		ProfilerVars();
		~ProfilerVars() = default;

		void RegisterVars(void);

		X_INLINE bool isPaused(void) const;
		X_INLINE bool drawProfileInfo(void) const;


	private:
		int32_t profilerPause_;
		int32_t drawProfileInfo_;


		int32_t drawProfileInfoWhenConsoleExpaned_;
		int32_t drawSubsystems_;
		int32_t drawMemInfo_;
		int32_t drawStats_;
		int32_t drawFrameTimeBar_;
	};



} // namespace profiler

X_NAMESPACE_END

#include "ProfilerVars.inl"