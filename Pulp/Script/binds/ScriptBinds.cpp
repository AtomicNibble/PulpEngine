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
	modules_.emplace_back(X_NEW(XBinds_Core, g_ScriptArena, "CoreBinds")(), g_ScriptArena);
	modules_.emplace_back(X_NEW(XBinds_Script, g_ScriptArena, "ScriptBinds")(), g_ScriptArena);
	modules_.emplace_back(X_NEW(XBinds_Sound, g_ScriptArena, "SoundBinds")(), g_ScriptArena);
	modules_.emplace_back(X_NEW(XBinds_Io, g_ScriptArena, "IoBinds")(), g_ScriptArena);

	for (auto& m : modules_) {
		m->Init(pScriptSystem, pCore, 0);
	}

}


void XScriptBinds::Shutdown(void)
{
	modules_.clear();

}


X_NAMESPACE_END