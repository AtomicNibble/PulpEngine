#pragma once


#ifndef X_SCRIPT_BINDS_H_
#define X_SCRIPT_BINDS_H_

#include <Containers\FixedArray.h>
#include <String\StackString.h>

X_NAMESPACE_BEGIN(script)


struct IScriptableBase;

class XScriptBinds
{
public:
	XScriptBinds();
	~XScriptBinds();

	void Init(IScriptSys* pScriptSystem, ICore* pCore);
	void Shutdown(void);

private:
	typedef core::FixedArray<IScriptableBase*, ScriptMoudles::ENUM_COUNT> ScriptModels;
	ScriptModels modules_;
};



X_NAMESPACE_END

#endif // !X_SCRIPT_BINDS_H_