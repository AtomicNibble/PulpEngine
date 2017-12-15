#include "stdafx.h"
#include "TableDump.h"


X_NAMESPACE_BEGIN(script)


XScriptTableDumpConsole::XScriptTableDumpConsole()
{

}

XScriptTableDumpConsole::~XScriptTableDumpConsole()
{

}

void XScriptTableDumpConsole::onElementFound(const char* pName, Type::Enum type)
{
	X_LOG0("Script", "%s %s", pName, Type::ToString(type));

}

void XScriptTableDumpConsole::onElementFound(int idx, Type::Enum type)
{
	X_LOG0("Script", "%i> %s", idx, Type::ToString(type));
}



X_NAMESPACE_END