#pragma once

#ifndef X_SCRIPT_BINDS_SCRIPT_H_
#define X_SCRIPT_BINDS_SCRIPT_H_

X_NAMESPACE_BEGIN(script)

class XBinds_Script : public XScriptableBase
{
public:
	XBinds_Script();
	~XBinds_Script() X_OVERRIDE;

	void init(IScriptSys* pSS, ICore* pCore, int paramIdOffset = 0) X_OVERRIDE;

	int Load(IFunctionHandler* pH);
	int ReLoad(IFunctionHandler* pH);
	int UnLoad(IFunctionHandler* pH);

	int ListLoaded(IFunctionHandler* pH);

private:
};

X_NAMESPACE_END

#endif // !X_SCRIPT_BINDS_SCRIPT_H_