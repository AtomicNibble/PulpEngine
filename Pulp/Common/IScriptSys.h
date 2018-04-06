#pragma once

#ifndef _X_SCRIPT_SYS_I_H_
#define _X_SCRIPT_SYS_I_H_

#include <IAsyncLoad.h>

#include <Assets\AssetBase.h>
#include <Util\Delegate.h>
#include <String\StringRange.h>

X_NAMESPACE_BEGIN(script)

static const char* SCRIPT_FILE_EXTENSION = "lua";

static const uint32_t SCRIPT_MAX_LOADED = 256;

X_DECLARE_ENUM(Moudles)
(
    Global,
    Core,
    Script,
    Sound,
    Game,
    Physics,
    Network,
    Video,
    Io);

X_DECLARE_ENUM(Type)
(
    Nil,
    Boolean,
    Pointer,
    Number,
    String,
    Table,
    Function,
    Userdata,
    Handle,
    Vector,
    None);

class SmartScriptTable;

struct IScriptTableDumpSink;
struct IScriptTableIterator;
struct IScriptTable;
struct IScriptBinds;

// the only reason this is a custom type instead of a 32bit int.
// if for type resolution in tempaltes / overloads. if we had strong types could use that.
struct ScriptFunctionHandleType
{
};
typedef ScriptFunctionHandleType* ScriptFunctionHandle;

static const ScriptFunctionHandle INVALID_HANLDE = 0;

// note this is a union not a struct
// Used for storing full range int's in lua.
union Handle
{
    Handle() :
        pPtr(0)
    {
    }
    Handle(size_t i) :
        id(i)
    {
    }
    Handle(void* p) :
        pPtr(p)
    {
    }

    size_t id;
    void* pPtr;
};

template<typename T>
struct ValueType
{
};

template<>
struct ValueType<bool>
{
    static const Type::Enum Type = Type::Boolean;
};
template<>
struct ValueType<int32_t>
{
    static const Type::Enum Type = Type::Number;
};
template<>
struct ValueType<uint32_t>
{
    static const Type::Enum Type = Type::Number;
};
template<>
struct ValueType<float>
{
    static const Type::Enum Type = Type::Number;
};
template<>
struct ValueType<double>
{
    static const Type::Enum Type = Type::Number;
};
template<>
struct ValueType<const char*>
{
    static const Type::Enum Type = Type::String;
};
template<>
struct ValueType<core::StringRange<char>>
{
    static const Type::Enum Type = Type::String;
};

template<>
struct ValueType<ScriptFunctionHandle>
{
    static const Type::Enum Type = Type::Function;
};
template<>
struct ValueType<Handle>
{
    static const Type::Enum Type = Type::Handle;
};
template<>
struct ValueType<Vec3f>
{
    static const Type::Enum Type = Type::Vector;
};

template<>
struct ValueType<IScriptTable*>
{
    static const Type::Enum Type = Type::Table;
};
template<>
struct ValueType<SmartScriptTable>
{
    static const Type::Enum Type = Type::Table;
};

struct ScriptValue
{
    X_INLINE ScriptValue(bool value);
    X_INLINE ScriptValue(int32_t value);
    X_INLINE ScriptValue(uint32_t value);
    X_INLINE ScriptValue(float value);
    X_INLINE ScriptValue(double value);
    X_INLINE ScriptValue(const char* pValue);
    X_INLINE ScriptValue(core::StringRange<char>& value);
    X_INLINE ScriptValue(IScriptTable* pTable_);
    X_INLINE ScriptValue(ScriptFunctionHandle function);
    X_INLINE ScriptValue(Handle value);
    X_INLINE ScriptValue(const Vec3f& vec);
    X_INLINE ScriptValue(const SmartScriptTable& value);
    X_INLINE ScriptValue(Type::Enum type);
    X_INLINE ScriptValue();
    X_INLINE ~ScriptValue();

    X_INLINE void clear();
    X_INLINE Type::Enum getType(void) const;

    X_INLINE ScriptValue& operator=(const ScriptValue& rhs);
    X_INLINE bool operator==(const ScriptValue& rhs) const;
    X_INLINE bool operator!=(const ScriptValue& rhs) const;
    X_INLINE void swap(ScriptValue& value);

    X_INLINE bool copyTo(bool& value) const;
    X_INLINE bool copyTo(int32_t& value) const;
    X_INLINE bool copyTo(uint32_t& value) const;
    X_INLINE bool copyTo(float& value) const;
    X_INLINE bool copyTo(const char*& value) const;
    X_INLINE bool copyTo(char*& value) const;
    X_INLINE bool copyTo(core::StringRange<char>& value) const;
    X_INLINE bool copyTo(Handle& value) const;
    X_INLINE bool copyTo(ScriptFunctionHandle& value) const;
    X_INLINE bool copyTo(Vec3f& value) const;
    X_INLINE bool copyTo(IScriptTable*& value) const;
    X_INLINE bool copyTo(SmartScriptTable& value) const;

public:
    Type::Enum type_;

    union
    {
        bool bool_;
        double number_;
        const void* pPtr_;
        IScriptTable* pTable_;
        ScriptFunctionHandle pFunction_;
        // const char* pStr_;
        struct
        {
            const char* pStr;
            int32_t len;
        } str_;
        struct
        {
            float x, y, z;
        } vec3_;
        struct
        {
            void* pPtr;
            int32_t ref;
        } ud_;
    };
};

X_ENSURE_SIZE(ScriptValue, 24);

struct IScript
{
    virtual ~IScript() = default;
};

struct IScriptSys : public core::IEngineSysBase
    , public core::IAssetLoader
{
    using core::IAssetLoader::waitForLoad;

    virtual ~IScriptSys() = default;

    virtual void Update(void) X_ABSTRACT;

    virtual bool runScriptInSandbox(const char* pBegin, const char* pEnd) X_ABSTRACT;

    virtual IScript* findScript(const char* pFileName) X_ABSTRACT;
    virtual IScript* loadScript(const char* pFileName) X_ABSTRACT;

    virtual bool waitForLoad(IScript* pScript) X_ABSTRACT; // returns true if load succeed.

    // you must release function handles.
    virtual ScriptFunctionHandle getFunctionPtr(const char* pFuncName) X_ABSTRACT;
    virtual ScriptFunctionHandle getFunctionPtr(const char* pTableName, const char* pFuncName) X_ABSTRACT;
    virtual bool compareFuncRef(ScriptFunctionHandle f1, ScriptFunctionHandle f2) X_ABSTRACT;
    virtual void releaseFunc(ScriptFunctionHandle f) X_ABSTRACT;

    virtual IScriptTable* createTable(bool empty = false) X_ABSTRACT;
    virtual IScriptBinds* createScriptBind(void) X_ABSTRACT;

    // set values.
    virtual void setGlobalValue(const char* pKey, const ScriptValue& any) X_ABSTRACT;
    virtual bool getGlobalValue(const char* pKey, ScriptValue& value) X_ABSTRACT;

    template<class T>
    X_INLINE void setGlobalValue(const char* pKey, const T& value);

    X_INLINE void setGlobalToNull(const char* pKey);

    // Get Global value.
    template<class T>
    X_INLINE bool getGlobalValue(const char* pKey, T& value);

    // Call api
    //
    // beginCall(...);
    // pushFuncArg("meow");
    // pushFuncArg(2);
    // endCall();
    virtual bool call(ScriptFunctionHandle f) X_ABSTRACT;

    virtual bool beginCall(ScriptFunctionHandle f) X_ABSTRACT;
    virtual bool beginCall(const char* pFunName) X_ABSTRACT;
    virtual bool beginCall(const char* pTableName, const char* pFunName) X_ABSTRACT;
    virtual bool beginCall(IScriptTable* pTable, const char* pFunName) X_ABSTRACT;

    virtual void pushCallArg(const ScriptValue& any) X_ABSTRACT;

    virtual bool endCall(void) X_ABSTRACT;
    virtual bool endCall(ScriptValue& value) X_ABSTRACT;

    virtual IScriptTable* createUserData(void* ptr, size_t size) X_ABSTRACT;

    virtual void onScriptError(const char* fmt, ...) X_ABSTRACT;
    virtual void logCallStack(void) X_ABSTRACT;
};

struct IFunctionHandler
{
    virtual ~IFunctionHandler() = default;

    virtual IScriptSys* getIScriptSystem(void) X_ABSTRACT;

    virtual void* getThis(void) X_ABSTRACT;
    virtual const char* getFuncName(void) X_ABSTRACT;

    virtual int32_t getParamCount(void) X_ABSTRACT;

    virtual Type::Enum getParamType(int32_t idx) X_ABSTRACT;
    virtual bool getSelfAny(ScriptValue& any) X_ABSTRACT;
    virtual bool getParamAny(int32_t idx, ScriptValue& any) X_ABSTRACT;

    virtual int32_t endFunctionAny(const ScriptValue& any) X_ABSTRACT;
    virtual int32_t endFunctionAny(const ScriptValue& any1, const ScriptValue& any2) X_ABSTRACT;
    virtual int32_t endFunctionAny(const ScriptValue& any1, const ScriptValue& any2, const ScriptValue& any3) X_ABSTRACT;

    template<class T>
    X_INLINE bool getSelf(T& value);

    template<typename T>
    X_INLINE bool getParam(int32_t idx, T& value);

    template<typename T>
    X_INLINE bool getParam(T& value);
    template<typename T, typename T2>
    X_INLINE bool getParam(T& value1, T2& value2);
    template<typename T, typename T2, typename T3>
    X_INLINE bool getParam(T& value1, T2& value2, T3& value3);
    template<typename T, typename T2, typename T3, typename T4>
    X_INLINE bool getParam(T& value1, T2& value2, T3& value3, T4& value4);

    X_INLINE int32_t endFunction(void) const;
    X_INLINE int32_t endFunctionNull(void) const;

    template<class T>
    X_INLINE int endFunction(const T& value);
    template<class T1, class T2>
    X_INLINE int endFunction(const T1& value1, const T2& value2);
    template<class T1, class T2, class T3>
    X_INLINE int endFunction(const T1& value1, const T2& value2, const T3& value3);
};

struct IScriptTableDumpSink
{
    virtual ~IScriptTableDumpSink() = default;
    virtual void onElementFound(const char* pName, Type::Enum type) X_ABSTRACT;
    virtual void onElementFound(int idx, Type::Enum type) X_ABSTRACT;
};

struct ScriptFunctionDesc
{
    typedef core::Delegate<int(IFunctionHandler*)> ScriptFunction;
    typedef core::traits::Function<int(IFunctionHandler* pH, void* pBuffer, int32_t size)> UserDataFunction;

    X_INLINE ScriptFunctionDesc();

    const char* pFunctionName;   // Name of function.
    const char* pFunctionParams; // List of parameters (ex "nSlot,vDirection" ).
    const char* pGlobalName;     // Name of global table (ex "Core")
    ScriptFunction function;     // Pointer to simple function.
    int paramIdOffset;           // Offset of the parameter to accept as 1st function argument.
    int userDataSize;
    void* pDataBuffer;
    UserDataFunction::Pointer pUserDataFunc;
};

struct IScriptTable
{
    typedef ScriptFunctionDesc::ScriptFunction ScriptFunction;
    typedef ScriptFunctionDesc::UserDataFunction UserDataFunction;

    struct Iterator
    {
        ScriptValue value;
        ScriptValue key;
        int internal;
    };

public:
    virtual ~IScriptTable() = default;

    virtual IScriptSys* getIScriptSystem(void) const X_ABSTRACT;

    virtual void addRef(void) X_ABSTRACT;
    virtual void release(void) X_ABSTRACT;

    virtual void* getUserData(void) X_ABSTRACT;

    virtual void setValueAny(const char* pKey, const ScriptValue& any, bool bChain = false) X_ABSTRACT;
    virtual bool getValueAny(const char* pKey, ScriptValue& any, bool bChain = false) X_ABSTRACT;

    virtual void setValueAny(int32_t idx, const ScriptValue& any) X_ABSTRACT;
    virtual bool getValueAny(int32_t idx, ScriptValue& any) X_ABSTRACT;

    virtual Type::Enum getValueType(const char* pKey) X_ABSTRACT;
    virtual Type::Enum getValueType(int32_t idx) X_ABSTRACT;

    virtual bool beginChain(void) X_ABSTRACT;
    virtual void endChain(void) X_ABSTRACT;

    virtual void clear(void) X_ABSTRACT;   // clears the table, removes all the entries in the table.
    virtual size_t count(void) X_ABSTRACT; // gets the count of elements into the object.

    virtual void* getThis(void) X_ABSTRACT;

    virtual void setMetatable(IScriptTable* pMetatable) X_ABSTRACT; // Assign a metatable to a table.

    // member iteration.
    virtual IScriptTable::Iterator begin(void) X_ABSTRACT;
    virtual bool next(Iterator& iter) X_ABSTRACT;
    virtual void end(const Iterator& iter) X_ABSTRACT;

    // Description:
    //    Produces a copy of the src table.
    // Arguments
    //    bDeepCopy - Defines if source table is cloned recursively or not,
    //                if bDeepCopy is false Only does shallow copy (no deep copy, table entries are not cloned hierarchically).
    //                If bDeepCopy is true, all sub tables are also cloned recursively.
    //                If bDeepCopy is true and bCopyByReference is true, the table structure is copied but the tables are left empty and the metatable is set to point at the original table.
    virtual bool clone(IScriptTable* pSrcTable, bool bDeepCopy = false, bool bCopyByReference = false) X_ABSTRACT;

    virtual void dump(IScriptTableDumpSink* pSink) X_ABSTRACT;

    virtual bool addFunction(const ScriptFunctionDesc& fd) X_ABSTRACT;

    // -------------- helpers --------------

    // push to next available index.
    template<class T>
    X_INLINE void pushBack(const T& value);

    template<class T>
    X_INLINE void setValue(const char* pKey, const T& value);

    template<class T>
    X_INLINE void setValue(int idx, const T& value);

    template<class T>
    X_INLINE void setValueChain(const char* pKey, const T& value);

    //	 Gets value of a table member.
    template<class T>
    X_INLINE bool getValue(const char* pKey, T& value);

    template<class T>
    X_INLINE bool getValue(int idx, T& value);

    template<class T>
    X_INLINE bool getValueChain(const char* pKey, T& value);

    X_INLINE bool haveValue(const char* pKey);
    X_INLINE bool haveValue(int idx);

    X_INLINE void setToNull(const char* pKey);
    X_INLINE void setToNull(int idx);
    X_INLINE void setToNullChain(const char* pKey);
};

struct IScriptBinds
{
    typedef IScriptTable::ScriptFunction ScriptFunction;

    virtual ~IScriptBinds() = default;

    virtual void setGlobalName(const char* pGlobalName) X_ABSTRACT;
    virtual void setParamOffset(int paramIdOffset) X_ABSTRACT;
    virtual IScriptTable* getMethodsTable(void) X_ABSTRACT;

    virtual void registerFunction(const char* pFuncName, const IScriptTable::ScriptFunction& function) X_ABSTRACT;
};

struct IScriptBindsBase
{
public:
    X_INLINE IScriptBindsBase(IScriptSys* pScriptSys);

    X_INLINE IScriptTable* getMethodsTable(void);

    X_INLINE void createBindTable(void);
    X_INLINE void setGlobalName(const char* pGlobalName);

protected:
    IScriptSys* pScriptSys_;
    IScriptBinds* pBindTable_;
};

#define X_SCRIPT_BIND(classname, func)                    \
                                                          \
    {                                                     \
        script::IScriptBinds::ScriptFunction Delegate;    \
        Delegate.Bind<classname, &classname::func>(this); \
        pBindTable_->registerFunction(#func, Delegate);   \
    }

#define SCRIPT_CHECK_PARAMETERS(_n)                                                                                              \
                                                                                                                                 \
    if (pH->getParamCount() != _n)                                                                                               \
                                                                                                                                 \
    {                                                                                                                            \
        pH->getIScriptSystem()->onScriptError("[%s] %d arguments passed, " #_n " expected)", __FUNCTION__, pH->getParamCount()); \
        return pH->endFunction();                                                                                                \
    }

#define SCRIPT_CHECK_PARAMETERS_MIN(_n)                                                                                                   \
                                                                                                                                          \
    if (pH->getParamCount() < _n)                                                                                                         \
                                                                                                                                          \
    {                                                                                                                                     \
        pH->getIScriptSystem()->onScriptError("[%s] %d arguments passed, at least " #_n " expected)", __FUNCTION__, pH->getParamCount()); \
        return pH->endFunction();                                                                                                         \
    }

// ===========================================================
class SmartScriptTable
{
public:
    X_INLINE SmartScriptTable();
    X_INLINE SmartScriptTable(const SmartScriptTable& st);
    X_INLINE SmartScriptTable(SmartScriptTable&& st);
    X_INLINE SmartScriptTable(IScriptTable* pNew);
    X_INLINE explicit SmartScriptTable(IScriptSys* pSS, bool createEmpty);
    X_INLINE ~SmartScriptTable();

    X_INLINE SmartScriptTable& operator=(IScriptTable* pNew);
    X_INLINE SmartScriptTable& operator=(const SmartScriptTable& st);
    X_INLINE SmartScriptTable& operator=(SmartScriptTable&& st);

    // Casts
    X_INLINE IScriptTable* operator->() const;
    X_INLINE IScriptTable* operator*() const;
    X_INLINE operator const IScriptTable*() const;
    X_INLINE operator IScriptTable*() const;
    X_INLINE operator bool() const;

    // Boolean comparasions.
    X_INLINE bool operator!() const;
    X_INLINE bool operator==(const IScriptTable* p2) const;
    X_INLINE bool operator==(IScriptTable* p2) const;
    X_INLINE bool operator!=(const IScriptTable* p2) const;
    X_INLINE bool operator!=(IScriptTable* p2) const;
    X_INLINE bool operator<(const IScriptTable* p2) const;
    X_INLINE bool operator>(const IScriptTable* p2) const;

    X_INLINE IScriptTable* getPtr(void) const;

    X_INLINE bool create(IScriptSys* pSS, bool createEmpty);

private:
    IScriptTable* pTable_;
};

class SmartScriptFunction
{
    X_NO_COPY(SmartScriptFunction);
    X_NO_ASSIGN(SmartScriptFunction);

public:
    X_INLINE SmartScriptFunction();
    X_INLINE SmartScriptFunction(IScriptSys* pSS, ScriptFunctionHandle func);
    X_INLINE SmartScriptFunction(SmartScriptFunction&& other);
    X_INLINE ~SmartScriptFunction();

    X_INLINE SmartScriptFunction& operator=(SmartScriptFunction&& other);

    X_INLINE operator ScriptFunctionHandle() const;
    X_INLINE ScriptFunctionHandle get(void) const;

    X_INLINE void swap(SmartScriptFunction& other);
    X_INLINE void reset(void);
    X_INLINE void reset(IScriptSys* pSS, ScriptFunctionHandle func);

private:
    ScriptFunctionHandle func_;
    IScriptSys* pSS_;
};

X_NAMESPACE_END

#include "IScriptSys.inl"

#endif // !_X_SCRIPT_SYS_I_H_
