#pragma once

X_NAMESPACE_BEGIN(script)

class XScriptSys;
class XScriptTable;
class XScriptBinds;

// helper for seting up binds, inherit it.
struct XScriptBindsBase
{
public:
    XScriptBindsBase(XScriptSys* pScriptSys);

    virtual void bind(ICore* pCore) X_ABSTRACT;

    void createBindTable(void);

    void setName(const char* pName);
    void setGlobalName(const char* pGlobalName);
    void setParamOffset(int paramIdOffset);
    XScriptTable* getMethodsTable(void);

protected:
    XScriptSys* pScriptSys_;
    XScriptBinds* pBindTable_;
};

class XScriptBinds : public IScriptBinds
{
public:
    XScriptBinds(XScriptSys* pScriptSys);
    virtual ~XScriptBinds() X_FINAL;

    void setName(const char* pName);
    void setGlobalName(const char* pGlobalName) X_FINAL;
    void setParamOffset(int paramIdOffset) X_FINAL;
    IScriptTable* getMethodsTable(void) X_FINAL;
    const char* getGlobalName(void) const;

    void registerFunction(const char* pFuncName, const IScriptTable::ScriptFunction& function) X_FINAL;

protected:
    core::StackString<60> name_;
    XScriptSys* pScriptSys_;
    XScriptTable* pMethodsTable_;
    int paramIdOffset_;
};

X_NAMESPACE_END