#pragma once

X_NAMESPACE_DECLARE(core,
                    struct XFile;)

X_NAMESPACE_BEGIN(script)

class XScriptTableDumpConsole : public IScriptTableDumpSink
{
public:
    XScriptTableDumpConsole();
    ~XScriptTableDumpConsole() X_OVERRIDE;

    void onElementFound(const char* pName, Type::Enum type) X_OVERRIDE;
    void onElementFound(int idx, Type::Enum type) X_OVERRIDE;
};

struct XRecursiveLuaDumpToFile : public lua::IRecursiveLuaDump
{
    XRecursiveLuaDumpToFile(core::XFile& file);
    ~XRecursiveLuaDumpToFile() X_FINAL;

    void onElement(int level, const char* sKey, int nKey, ScriptValue& value) X_FINAL;
    void onBeginTable(int level, const char* sKey, int nKey) X_FINAL;
    void onEndTable(int level) X_FINAL;

private:
    const char* GetOffsetStr(int level);
    const char* GetKeyStr(const char* pKey, int key);

private:
    core::XFile& file_;

    char levelOffset_[1024];
    char keyStr_[32];
    size_t size_;
};

bool dumpStateToFile(lua_State* L, core::string_view fileName);

X_NAMESPACE_END
