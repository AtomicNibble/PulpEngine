#pragma once

#ifndef X_SCRIPT_BINDS_SCRIPT_H_
#define X_SCRIPT_BINDS_SCRIPT_H_

#include "ScriptableBase.h"

X_NAMESPACE_BEGIN(script)

class XBinds_Script : public XScriptableBase, public IScriptableBase
{
public:
	XBinds_Script(IScriptSys* pScriptSystem, ICore* pCore);
	~XBinds_Script() X_OVERRIDE;

	int Load(IFunctionHandler* pH);
	int ReLoad(IFunctionHandler* pH);
	int UnLoad(IFunctionHandler* pH);

	int ListLoaded(IFunctionHandler* pH);

private:
	IScriptSys* pScriptSystem_;
	ICore* pCore_;
};

X_NAMESPACE_END

#endif // !X_SCRIPT_BINDS_SCRIPT_H_