#include "stdafx.h"
#include "TableDump.h"


X_NAMESPACE_BEGIN(script)


XScriptTableDumpConsole::XScriptTableDumpConsole()
{

}

XScriptTableDumpConsole::~XScriptTableDumpConsole()
{

}

void XScriptTableDumpConsole::OnElementFound(const char* name, ScriptValueType::Enum type)
{
	X_LOG0("Script", "%s %s", name, ScriptValueType::ToString(type));

}

void XScriptTableDumpConsole::OnElementFound(int idx, ScriptValueType::Enum type)
{
	X_LOG0("Script", "%i> %s", idx, ScriptValueType::ToString(type));
}



X_NAMESPACE_END