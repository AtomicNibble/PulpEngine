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
	modules_.append(X_NEW(XBinds_Core, g_ScriptArena, "CoreBinds")(pScriptSystem, pCore));
	modules_.append(X_NEW(XBinds_Script, g_ScriptArena, "ScriptBinds")(pScriptSystem, pCore));
	modules_.append(X_NEW(XBinds_Sound, g_ScriptArena, "SoundBinds")(pScriptSystem, pCore));
	modules_.append(X_NEW(XBinds_Io, g_ScriptArena, "IoBinds")(pScriptSystem, pCore));

}


void XScriptBinds::Shutdown(void)
{
	ScriptModels::iterator it = modules_.begin();

	for (; it != modules_.end();)
	{
		IScriptableBase* pBase = *it;

		++it;

		if (pBase)
		{
			X_DELETE(pBase,g_ScriptArena);
		}
	}

}


X_NAMESPACE_END