#include "stdafx.h"
#include "NetVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(net)

NetVars::NetVars()
{
	debug_ = 0;

}


void NetVars::registerVars(void)
{

	ADD_CVAR_REF("net_debug", debug_, 0, 0, 2, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enable net debug msg's 1=enabled 2=verbose");

}

X_NAMESPACE_END