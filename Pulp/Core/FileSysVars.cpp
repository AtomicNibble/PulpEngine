#include "stdafx.h"
#include "FileSysVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(core)

XFileSysVars::XFileSysVars() :
	debug(0),
	QueDebug(0),
	numVirtualDir(0)
{
#if X_ENABLE_FILE_ARTIFICAIL_DELAY
	artOpenDelay = 3000;
	artReadDelay = 0;
	artWriteDelay = 0;
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

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

#if X_ENABLE_FILE_ARTIFICAIL_DELAY
	ADD_CVAR_REF("filesys_art_open_delay", artOpenDelay, artOpenDelay, 0, 100000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Introduce artificial file Open delay(ms)");

	ADD_CVAR_REF("filesys_art_read_delay", artReadDelay, artReadDelay, 0, 10000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Introduce artificial file read delay(ms)");
	
	ADD_CVAR_REF("filesys_art_write_delay", artWriteDelay, artWriteDelay, 0, 100000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Introduce artificial file write delay(ms)");
#endif // !X_ENABLE_FILE_ARTIFICAIL_DELAY

	// create vars for the virtual directories which we then update with the paths once set.
	for (size_t i = 0; i < FS_MAX_VIRTUAL_DIR; i++)
	{
		virDirVarsNames[i].set("filesys_mod_dir_");
		virDirVarsNames[i].appendFmt("%" PRIuS, i);
		pVirtualDirs[i] = ADD_CVAR_STRING(virDirVarsNames[i].c_str(), "",
			core::VarFlag::SYSTEM |
			core::VarFlag::READONLY,
			"Virtual mod directory");
	}
}



X_NAMESPACE_END
