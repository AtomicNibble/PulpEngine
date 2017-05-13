#include "stdafx.h"
#include "ProfilerVars.h"

#include <IConsole.h>

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)


X_NAMESPACE_BEGIN(core)

namespace profiler
{


	ProfilerVars::ProfilerVars()
	{
		profilerPause_ = 0;
		drawProfileInfo_ = 0;
		jobSysThreadMS_ = 16;

	}

	void ProfilerVars::RegisterVars(void)
	{

		ADD_CVAR_REF("profile_pause", profilerPause_, profilerPause_, 0, 1, core::VarFlag::SYSTEM,
			"Pause the profiler collection");

		ADD_CVAR_REF("profile_draw", drawProfileInfo_, drawProfileInfo_, 0, 1,
			core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Display profiler info. (visible items enabled via profile_draw_* vars)");

		ADD_CVAR_REF("profile_vis_ms", jobSysThreadMS_, jobSysThreadMS_, 1, 48, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
			"Number of MS to visualize");

	}



} // namespace profiler

X_NAMESPACE_END