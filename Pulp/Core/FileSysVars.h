#pragma once

#ifndef _X_FILE_SYSTEM_VARS_H_
#define _X_FILE_SYSTEM_VARS_H_

#include <IFileSys.h>
#include <IConsole.h>

struct XFileSysVars
{
	XFileSysVars() :
		debug(0),
		numVirtualDir(0)
	{
		core::zero_object(pVirtualDirs);
	}

	int32_t debug;
	int32_t numVirtualDir;
	
	core::ICVar* pVirtualDirs[core::FS_MAX_VIRTUAL_DIR];
};

#endif // !_X_FILE_SYSTEM_VARS_H_
