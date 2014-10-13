#pragma once


#ifndef X_SCRIPT_TABLE_DUMP_H
#define X_SCRIPT_TABLE_DUMP_H

X_NAMESPACE_BEGIN(script)


class XScriptTableDumpConsole : public IScriptTableDumpSink
{
public:
	XScriptTableDumpConsole();
	~XScriptTableDumpConsole() X_OVERRIDE;

	void OnElementFound(const char* name, ScriptValueType::Enum type) X_OVERRIDE;
	void OnElementFound(int idx, ScriptValueType::Enum type) X_OVERRIDE;
};

X_NAMESPACE_END

#endif // !X_SCRIPT_TABLE_DUMP_H