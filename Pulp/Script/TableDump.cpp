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
	X_LOG0("Script", "\"%s\" ^6%s", pName, Type::ToString(type));

}

void XScriptTableDumpConsole::onElementFound(int idx, Type::Enum type)
{
	X_LOG0("Script", "%i ^6%s", idx, Type::ToString(type));
}



X_NAMESPACE_END