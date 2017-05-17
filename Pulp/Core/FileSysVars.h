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
	int32_t debug;
	int32_t QueDebug;
	int32_t numVirtualDir;

#if X_ENABLE_FILE_ARTIFICAIL_DELAY
	// artificial delays
	int32_t artOpenDelay;
	int32_t artReadDelay;
	int32_t artWriteDelay;
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

	core::ICVar* pVirtualDirs[core::FS_MAX_VIRTUAL_DIR];
private:
	core::StackString<64> virDirVarsNames[core::FS_MAX_VIRTUAL_DIR];
};

X_NAMESPACE_END

#endif // !_X_FILE_SYSTEM_VARS_H_
