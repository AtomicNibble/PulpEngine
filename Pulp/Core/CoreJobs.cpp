#include "stdafx.h"
#include "Core.h"

#include <Threading\JobSystem2.h>

#include <IInput.h>
#include <IFrameData.h>

namespace
{
    struct FileChangeJobData
    {
        core::Path<char> name;
    };

} // namespace

void XCore::Job_DirectoryWatcher(core::V2::JobSystem& jobSys, size_t threadIdx,
    core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys);
    X_UNUSED(threadIdx);
    X_UNUSED(pJob);
    X_UNUSED(pData);

    // will cause  OnFileChange below to get called from this job.
    // which then makes jobs for each change calling Job_OnFileChange
    dirWatcher_.tick();
}

void XCore::Job_OnFileChange(core::V2::JobSystem& jobSys, size_t threadIdx,
    core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys);
    X_UNUSED(threadIdx);
    X_UNUSED(pJob);
    FileChangeJobData* pFileChangeData = reinterpret_cast<FileChangeJobData*>(pData);

    const core::Path<char>& name = pFileChangeData->name;
    const char* pFileName = name.fileName();
    const char* pExtention = name.extension(false);

    if (pExtention) {
        auto it = hotReloadExtMap_.find(X_CONST_STRING(pExtention));
        if (it != hotReloadExtMap_.end()) {
            it->second->Job_OnFileChange(jobSys, name);
        }
        else {
#if X_DEBUG
            // before we log a warning check to see if it's in the hotreload ignore list.
            if (HotRelodIgnoreArr::invalid_index == hotReloadIgnores_.find(core::string(pExtention))) {
                X_WARNING("hotReload", "file extension '%s' has no reload handle.", pExtention);
            }
#endif // !X_DEBUG
        }
    }
    else {
        if (this->dirWatcher_.isDebugEnabled()) {
            X_LOG1("hotReload", "No file extension ignoring: %s", pFileName);
        }
    }

    X_DELETE(pFileChangeData, g_coreArena);
}

void XCore::Job_PostInputFrame(core::V2::JobSystem& jobSys, size_t threadIdx,
    core::V2::Job* pJob, void* pData)
{
    X_UNUSED(jobSys);
    X_UNUSED(threadIdx);
    X_UNUSED(pJob);

    core::FrameData& frameData = *reinterpret_cast<core::FrameData*>(pData);

    if (env_.pInput) {
        // during the running of this is when command and Var callbacks will be run.
        env_.pInput->Job_PostInputFrame(jobSys, frameData);
    }
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

// XDirectoryWatcherListener

bool XCore::OnFileChange(core::XDirectoryWatcher::Action::Enum action,
    const char* name, const char* oldName, bool isDirectory)
{
    X_UNUSED(oldName);

    if (isDirectory) {
        return false;
    }

    if (action == core::XDirectoryWatcher::Action::MODIFIED) {
        FileChangeJobData* pData = X_NEW(FileChangeJobData, g_coreArena, "FileChangeJobData");
        pData->name.set(name);

        core::V2::JobSystem& jobSys = *env_.pJobSys;
        core::V2::Job* pJob = jobSys.CreateMemberJob<XCore>(this, &XCore::Job_OnFileChange, pData JOB_SYS_SUB_ARG(core::profiler::SubSys::CORE));
        jobSys.Run(pJob);
    }

    return true;
}

// ~XDirectoryWatcherListener