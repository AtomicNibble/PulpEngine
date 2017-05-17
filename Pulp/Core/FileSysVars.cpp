#include "stdafx.h"
#include "FileSysVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(core)

XFileSysVars::XFileSysVars() :
	debug(0),
	QueDebug(0),
	numVirtualDir(0)
{
	core::zero_object(pVirtualDirs);
}

void XFileSysVars::registerVars(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);

	ADD_CVAR_REF("filesys_debug", debug, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Filesystem debug. 0=off 1=on");
	ADD_CVAR_REF("filesys_Quedebug", QueDebug, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Filesystem que debug. 0=off 1=on");

	// create vars for the virtual directories which we then update with the paths once set.
	size_t i;
	core::StackString<64> name;
	for (i = 0; i < FS_MAX_VIRTUAL_DIR; i++)
	{
		name.set("filesys_mod_dir_");
		name.appendFmt("%" PRIuS, i);
		pVirtualDirs[i] = ADD_CVAR_STRING(name.c_str(), "",
			core::VarFlag::SYSTEM |
			core::VarFlag::READONLY |
			core::VarFlag::CPY_NAME,
			"Virtual mod directory");
	}
}



X_NAMESPACE_END
