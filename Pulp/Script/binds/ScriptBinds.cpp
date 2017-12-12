#include "stdafx.h"
#include "ScriptBinds.h"

// the modules
#include "ScriptBinds_core.h"
#include "ScriptBinds_script.h"
#include "ScriptBinds_io.h"


X_NAMESPACE_BEGIN(script)


XScriptBinds::XScriptBinds()
{
}

XScriptBinds::~XScriptBinds()
{

}

void XScriptBinds::Init(IScriptSys* pScriptSystem, ICore* pCore)
{
	modules_.emplace_back(g_ScriptArena, X_NEW(XBinds_Core, g_ScriptArena, "CoreBinds")(pScriptSystem, pCore));
	modules_.emplace_back(g_ScriptArena, X_NEW(XBinds_Script, g_ScriptArena, "ScriptBinds")(pScriptSystem));
	modules_.emplace_back(g_ScriptArena, X_NEW(XBinds_Io, g_ScriptArena, "IoBinds")(pScriptSystem, pCore));
}


void XScriptBinds::Shutdown(void)
{
	modules_.clear();

}


X_NAMESPACE_END