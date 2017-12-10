#include "stdafx.h"
#include "TableDump.h"


X_NAMESPACE_BEGIN(script)


XScriptTableDumpConsole::XScriptTableDumpConsole()
{

}

XScriptTableDumpConsole::~XScriptTableDumpConsole()
{

}

void XScriptTableDumpConsole::OnElementFound(const char* name, Type::Enum type)
{
	X_LOG0("Script", "%s %s", name, Type::ToString(type));

}

void XScriptTableDumpConsole::OnElementFound(int idx, Type::Enum type)
{
	X_LOG0("Script", "%i> %s", idx, Type::ToString(type));
}



X_NAMESPACE_END