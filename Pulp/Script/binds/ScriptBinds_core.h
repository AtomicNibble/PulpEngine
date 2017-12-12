#pragma once

#ifndef X_SCRIPT_BINDS_CORE_H_
#define X_SCRIPT_BINDS_CORE_H_

X_NAMESPACE_DECLARE(core, 
struct IConsole;
struct ITimer;
)

#ifdef DrawText
#undef DrawText
#endif // !DrawText

X_NAMESPACE_BEGIN(script)

class XBinds_Core : public XScriptableBase
{
public:
	XBinds_Core(IScriptSys* pSS, ICore* pCore);
	~XBinds_Core() X_OVERRIDE;

private:
	void init(IScriptSys* pSS, ICore* pCore);

	int GetDvarInt(IFunctionHandler* pH);
	int GetDvarFloat(IFunctionHandler* pH);
	int GetDvar(IFunctionHandler* pH);
	int SetDvar(IFunctionHandler* pH);

	int Log(IFunctionHandler* pH);
	int Warning(IFunctionHandler* pH);
	int Error(IFunctionHandler* pH);


private:
	core::IConsole* pConsole_;
	core::ITimer* pTimer_;
};

X_NAMESPACE_END

#endif // !X_SCRIPT_BINDS_CORE_H_