#pragma once

#ifndef X_SCRIPT_FUNCTION_HANDLER_H_
#define X_SCRIPT_FUNCTION_HANDLER_H_

X_NAMESPACE_BEGIN(script)

class XFunctionHandler : public IFunctionHandler
{
public:
	XFunctionHandler(XScriptSys* pSS, lua_State* lState, const char* sFuncName, int paramIdOffset) 
	{
		pSS_ = pSS;
		L = lState;
		sFuncName_ = sFuncName;
		paramIdOffset_ = paramIdOffset;
	}
	~XFunctionHandler() X_OVERRIDE;

	virtual IScriptSys* GetIScriptSystem() X_OVERRIDE;

	virtual void* GetThis() X_OVERRIDE;
	virtual bool GetSelfAny(ScriptValue &any) X_OVERRIDE;

	virtual const char* GetFuncName() X_OVERRIDE;

	virtual int GetParamCount() X_OVERRIDE;
	virtual Type::Enum GetParamType(int nIdx) X_OVERRIDE;


	virtual bool GetParamAny(int nIdx, ScriptValue &any) X_OVERRIDE;

	virtual int EndFunctionAny(const ScriptValue& any) X_OVERRIDE;
	virtual int EndFunctionAny(const ScriptValue& any1, const ScriptValue& any2) X_OVERRIDE;
	virtual int EndFunctionAny(const ScriptValue& any1, const ScriptValue& any2,
		const ScriptValue& any3) X_OVERRIDE;
	virtual int EndFunction() X_OVERRIDE;

private:
	XScriptSys* pSS_;
	lua_State* L;
	const char* sFuncName_;
	int paramIdOffset_;
};

X_NAMESPACE_END

#endif // !X_SCRIPT_FUNCTION_HANDLER_H_