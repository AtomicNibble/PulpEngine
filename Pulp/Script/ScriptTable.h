#pragma once

#ifndef X_SCRIPT_TABLE_H_
#define X_SCRIPT_TABLE_H_

#include <Util\ReferenceCounted.h>
#include <Util\ToggleChecker.h>

X_NAMESPACE_BEGIN(script)

class XScriptTable : public IScriptTable
{
    X_PACK_PUSH(1);
    struct CFunctionData
    {
        ScriptFunction pFunction;
        int8_t paramIdOffset;
        char funcName[1];

        X_INLINE static constexpr size_t requiredSize(size_t nameSize)
        {
            return sizeof(CFunctionData) + nameSize;
        }
    };

    struct UserDataFunctionData
    {
        UserDataFunction::Pointer pFunction;
        uint8_t nameSize;
        int8_t paramIdOffset;
        int16_t dataSize;
        char funcName[1];

        X_INLINE static constexpr size_t requiredSize(size_t nameSize, size_t dataSize)
        {
            return sizeof(UserDataFunctionData) + nameSize + dataSize;
        }

        X_INLINE void* getBuffer(void) const
        {
            return (void*)(reinterpret_cast<const char*>(this) + sizeof(*this) + nameSize);
        }
    };
    X_PACK_POP;

public:
    XScriptTable();
    ~XScriptTable() X_FINAL;

    // IScriptTable
    void addRef(void) X_FINAL;
    void release(void) X_FINAL;

    IScriptSys* getIScriptSystem(void) const X_FINAL;

    void* getUserData(void) X_FINAL;

    void setValueAny(const char* pKey, const ScriptValue& any, bool bChain = false) X_FINAL;
    bool getValueAny(const char* pKey, ScriptValue& any, bool bChain = false) X_FINAL;

    void setValueAny(int32_t idx, const ScriptValue& any) X_FINAL;
    bool getValueAny(int32_t idx, ScriptValue& any) X_FINAL;

    Type::Enum getValueType(const char* pKey) X_FINAL;
    Type::Enum getValueType(int32_t idx) X_FINAL;

    bool beginChain(void) X_FINAL;
    void endChain(void) X_FINAL;

    void clear(void) X_FINAL;   // clears the table, removes all the entries in the table.
    size_t count(void) X_FINAL; // gets the count of elements into the object.

    void setMetatable(IScriptTable* pMetatable) X_FINAL; // Assign a metatable to a table.

    void* getThis(void) X_FINAL;

    // member iteration.
    IScriptTable::Iterator begin(void) X_FINAL;
    bool next(Iterator& iter) X_FINAL;
    void end(const Iterator& iter) X_FINAL;

    bool clone(IScriptTable* pSrcTable, bool bDeepCopy = false, bool bCopyByReference = false) X_FINAL;
    void dump(IScriptTableDumpSink* p) X_FINAL;

    bool addFunction(const ScriptFunctionDesc& fd) X_FINAL;

    // --------------------------------------------------------------------------

    void createNew(void);
    void deleteThis(void);

    int32_t getRef(void) const;
    void attach(IScriptTable* pSO); // push that table and aattach.
    void attach(void);              // attaches table to value at top of stack.
    void pushRef(void);

public:
    static void pushRef(IScriptTable* pObj);

private:
    static int32_t s_CFunction(lua_State* L);
    static int32_t s_CUserDataFunction(lua_State* L);

    static void CloneTable(int srcTable, int trgTable);
    static void CloneTable_r(int srcTable, int trgTable);
    static void ReferenceTable_r(int scrTable, int trgTable);

public:
    static lua_State* L;
    static XScriptSys* pScriptSystem_;
    // static std::set<class XScriptTable*> s_allTables_;

private:
    int32_t luaRef_;
    int32_t refCount_;

#if X_DEBUG
    core::ToggleChecker setChainActive_;
#endif // !X_DEBUG
};

X_NAMESPACE_END

#endif // !X_SCRIPT_TABLE_H_