#include "stdafx.h"
#include "FileSysVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(core)

XFileSysVars::XFileSysVars() :
    debug_(0),
    queueDebug_(0),
    numVirtualDir_(0),
    pakMemorySizeLimitMB_(64)
{
#if X_ENABLE_FILE_ARTIFICAIL_DELAY
    artOpenDelay_ = 0;
    artReadDelay_ = 0;
    artWriteDelay_ = 0;
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

    core::zero_object(pVirtualDirs_);
}

void XFileSysVars::registerVars(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pConsole);

    ADD_CVAR_REF("filesys_debug", debug_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Filesystem debug. 0=off 1=on");
    ADD_CVAR_REF("filesys_queue_debug", queueDebug_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Filesystem que debug. 0=off 1=on");

    ADD_CVAR_REF("filesys_pak_memory_load_max_size", pakMemorySizeLimitMB_, 64, 0, 1024, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Max size of pak's loaded into memory in MB");

#if X_ENABLE_FILE_ARTIFICAIL_DELAY
    ADD_CVAR_REF("filesys_art_open_delay", artOpenDelay_, artOpenDelay_, 0, 100000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Introduce artificial file Open delay(ms)");

    ADD_CVAR_REF("filesys_art_read_delay", artReadDelay_, artReadDelay_, 0, 10000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Introduce artificial file read delay(ms)");

    ADD_CVAR_REF("filesys_art_write_delay", artWriteDelay_, artWriteDelay_, 0, 100000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Introduce artificial file write delay(ms)");
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

    // create vars for the virtual directories which we then update with the paths once set.
    size_t i;
    core::StackString<64> name;
    for (i = 0; i < FS_MAX_VIRTUAL_DIR; i++) {
        name.set("filesys_mod_dir_");
        name.appendFmt("%" PRIuS, i);
        pVirtualDirs_[i] = ADD_CVAR_STRING(name.c_str(), "",
            core::VarFlag::SYSTEM | core::VarFlag::READONLY | core::VarFlag::CPY_NAME,
            "Virtual mod directory");
    }
}

X_NAMESPACE_END
