#pragma once

#ifndef X_SCRIPT_FUNCTION_HANDLER_H_
#define X_SCRIPT_FUNCTION_HANDLER_H_

X_NAMESPACE_BEGIN(script)

class XFunctionHandler : public IFunctionHandler
{
public:
	XFunctionHandler(XScriptSys* pSS, lua_State* lState, const char* pFuncName, int32_t paramIdOffset);
	~XFunctionHandler() X_FINAL;

	virtual IScriptSys* getIScriptSystem(void) X_FINAL;

	virtual void* getThis(void) X_FINAL;
	virtual const char* getFuncName(void) X_FINAL;
	
	virtual int32_t getParamCount(void) X_FINAL;
	
	virtual Type::Enum getParamType(int idx) X_FINAL;
	virtual bool getSelfAny(ScriptValue& any) X_FINAL;
	virtual bool getParamAny(int idx, ScriptValue &any) X_FINAL;

	virtual int32_t endFunctionAny(const ScriptValue& any) X_FINAL;
	virtual int32_t endFunctionAny(const ScriptValue& any1, const ScriptValue& any2) X_FINAL;
	virtual int32_t endFunctionAny(const ScriptValue& any1, const ScriptValue& any2, const ScriptValue& any3) X_FINAL;
	

private:
	XScriptSys* pSS_;
	lua_State* L_;
	const char* pFuncName_;
	int paramIdOffset_;
};

X_NAMESPACE_END

#endif // !X_SCRIPT_FUNCTION_HANDLER_H_