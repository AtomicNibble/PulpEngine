#include "stdafx.h"
#include "ScriptBinds.h"

// the modules
#include "ScriptBinds_core.h"
#include "ScriptBinds_script.h"
#include "ScriptBinds_sound.h"
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
	modules_.emplace_back(g_ScriptArena, X_NEW(XBinds_Core, g_ScriptArena, "CoreBinds")());
	modules_.emplace_back(g_ScriptArena, X_NEW(XBinds_Script, g_ScriptArena, "ScriptBinds")());
//	modules_.emplace_back(g_ScriptArena, X_NEW(XBinds_Sound, g_ScriptArena, "SoundBinds")());
	modules_.emplace_back(g_ScriptArena, X_NEW(XBinds_Io, g_ScriptArena, "IoBinds")());

	for (auto& m : modules_) {
		m->init(pScriptSystem, pCore, 0);
	}

}


void XScriptBinds::Shutdown(void)
{
	modules_.clear();

}


X_NAMESPACE_END