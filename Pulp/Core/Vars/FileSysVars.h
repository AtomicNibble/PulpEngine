#pragma once

#ifndef _X_FILE_SYSTEM_VARS_H_
#define _X_FILE_SYSTEM_VARS_H_

#include <IFileSys.h>

X_NAMESPACE_BEGIN(core)

struct ICVar;

class XFileSysVars
{
public:
    XFileSysVars();

    void registerVars(void);

public:
    int32_t debug_;
    int32_t queueDebug_;
    int32_t numVirtualDir_;

    int32_t pakMemorySizeLimitMB_;

#if X_ENABLE_FILE_ARTIFICAIL_DELAY
    // artificial delays
    int32_t artOpenDelay_;
    int32_t artReadDelay_;
    int32_t artWriteDelay_;
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

    core::ICVar* pVirtualDirs_[core::FS_MAX_VIRTUAL_DIR];
};

X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_VARS_H_
