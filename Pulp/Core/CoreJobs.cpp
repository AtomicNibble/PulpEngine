#include "stdafx.h"
#include "Core.h"

#include <Threading\JobSystem2.h>

#include <IInput.h>
#include <IFrameData.h>

#include <Platform\DirectoryWatcher.h>


void XCore::Job_DirectoryWatcher(core::V2::JobSystem& jobSys, size_t threadIdx,
    core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys, threadIdx, pJob, pData);

    // will cause  OnFileChange below to get called from this job.
    // which then makes jobs for each change calling Job_OnFileChange
    X_ASSERT_NOT_NULL(pDirWatcher_);

    pDirWatcher_->tick();
}

void XCore::Job_ConsoleUpdates(core::V2::JobSystem& jobSys, size_t threadIdx,
    core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys);
    X_UNUSED(threadIdx);
    X_UNUSED(pJob);
    X_UNUSED(pData);

    if (env_.pConsole) {
        core::FrameTimeData& time = *reinterpret_cast<core::FrameTimeData*>(pData);

        // this should not run any commands as it's just repeating a key
        env_.pConsole->Job_dispatchRepeateInputEvents(time);

        // runs any commands that got submitted via input or config / other things.
        // so basically this is the only place that triggers command callbacks and modified callbacks.
        env_.pConsole->Job_runCmds();
    }
}
