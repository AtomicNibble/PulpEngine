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
