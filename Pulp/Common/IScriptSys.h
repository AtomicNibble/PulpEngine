#pragma once

#ifndef _X_SCRIPT_SYS_I_H_
#define _X_SCRIPT_SYS_I_H_

#include "Util\Delegate.h"

X_NAMESPACE_BEGIN(script)

static const char* X_SCRIPT_FILE_EXTENSION = "lua";


X_DECLARE_ENUM(Moudles)(
	GLOBAL,
	CORE, 
	SCRIPT, 
	SOUND, 
	GAME,
	PHYSICS,
	NETWORK,
	VIDEO,
	IO
);

X_DECLARE_ENUM(Type)(
	NIL, 
	BOOLEAN, 
	POINTER, 
	NUMBER, 
	STRING, 
	TABLE, 
	FUNCTION, 
	USERDATA,
	HANDLE, 
	VECTOR, 
	NONE
);


class SmartScriptTable;

struct IScriptTableDumpSink;
struct IScriptTableIterator;
struct IScriptTable;


// the only reason this is a custom type instead of a 32bit int.
// if for type resolution in tempaltes / overloads. if we had strong types could use that.
struct ScriptFunctionHandleType {};
typedef ScriptFunctionHandleType* ScriptFunctionHandle;

static const ScriptFunctionHandle INVALID_HANLDE = 0;

// note this is a union not a struct
// Used for storing full range int's in lua.
union Handle
{
	Handle() : pPtr(0) {}
	Handle(size_t i) : id(i) {}
	Handle(void* p) : pPtr(p) {}

	size_t id;
	void* pPtr;
};


template<typename T>
struct ValueType {

};

template<>
struct ValueType<bool> {
	static const Type::Enum Type = Type::BOOLEAN;
};
template<>
struct ValueType<int32_t> {
	static const Type::Enum Type = Type::NUMBER;
};
template<>
struct ValueType<uint32_t> {
	static const Type::Enum Type = Type::NUMBER;
};
template<>
struct ValueType<float> {
	static const Type::Enum Type = Type::NUMBER;
};
template<>
struct ValueType<double> {
	static const Type::Enum Type = Type::NUMBER;
};
template<>
struct ValueType<const char*> {
	static const Type::Enum Type = Type::STRING;
};

template<>
struct ValueType<ScriptFunctionHandle> {
	static const Type::Enum Type = Type::FUNCTION;
};
template<>
struct ValueType<Handle> {
	static const Type::Enum Type = Type::HANDLE;
};
template<>
struct ValueType<Vec3f> {
	static const Type::Enum Type = Type::VECTOR;
};

template<>
struct ValueType<IScriptTable*> {
	static const Type::Enum Type = Type::TABLE;
};
template<>
struct ValueType<SmartScriptTable> {
	static const Type::Enum Type = Type::TABLE;
};


struct ScriptValue
{
	X_INLINE ScriptValue(bool value);
	X_INLINE ScriptValue(int32_t value);
	X_INLINE ScriptValue(uint32_t value);
	X_INLINE ScriptValue(float value);
	X_INLINE ScriptValue(double value);
	X_INLINE ScriptValue(const char* pValue);
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
	X_INLINE bool copyTo(const char* &value) const;
	X_INLINE bool copyTo(char* &value) const;
	X_INLINE bool copyTo(Handle &value) const;
	X_INLINE bool copyTo(ScriptFunctionHandle &value) const;
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
		struct { const char* pStr; int32_t len; } str_;
		struct { float x, y, z; } vec3_;
		struct { void* pPtr; int32_t ref; } ud_;
	};
};


#if 1


struct IScriptSys : public core::IEngineSysBase
{
	virtual ~IScriptSys() {};

	virtual void Update(void) X_ABSTRACT;

	virtual bool runScriptInSandbox(const char* pBegin, const char* pEnd) X_ABSTRACT;

	// you must release function handles.
	virtual ScriptFunctionHandle getFunctionPtr(const char* pFuncName) X_ABSTRACT;
	virtual	ScriptFunctionHandle getFunctionPtr(const char* pTableName, const char* pFuncName) X_ABSTRACT;
	virtual bool compareFuncRef(ScriptFunctionHandle f1, ScriptFunctionHandle f2) X_ABSTRACT;
	virtual void releaseFunc(ScriptFunctionHandle f) X_ABSTRACT;

	virtual IScriptTable* createTable(bool bEmpty = false) X_ABSTRACT;

	// set values.
	virtual void setGlobalValue(const char* pKey, const ScriptValue& any) X_ABSTRACT;
	virtual bool getGlobalValue(const char* pKey, ScriptValue& value) X_ABSTRACT;

	template <class T>
	X_INLINE void setGlobalValue(const char* pKey, const T& value);
	
	X_INLINE void setGlobalToNull(const char* pKey);

	// Get Global value.
	template <class T>
	X_INLINE bool getGlobalValue(const char* pKey, T& value);

	virtual IScriptTable* createUserData(void* ptr, size_t size) X_ABSTRACT;

	virtual void onScriptError(const char* fmt, ...) X_ABSTRACT;
};


struct IFunctionHandler
{
	virtual ~IFunctionHandler() {}

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

	template <class T>
	X_INLINE bool getSelf(T& value);

	template <typename T>
	X_INLINE bool getParam(int32_t idx, T &value);

	template <typename T>
	X_INLINE bool getParam(T& value);
	template <typename T, typename T2>
	X_INLINE bool getParam(T& value1, T2& value2);
	template <typename T, typename T2, typename T3>
	X_INLINE bool getParam(T& value1, T2& value2, T3& value3);
	template <typename T, typename T2, typename T3, typename T4>
	X_INLINE bool getParam(T& value1, T2& value2, T3& value3, T4& value4);


	X_INLINE int32_t endFunction(void) const;
	X_INLINE int32_t endFunctionNull(void) const;

	template <class T>
	X_INLINE int endFunction(const T& value);
	template <class T1, class T2>
	X_INLINE int endFunction(const T1& value1, const T2& value2);
	template <class T1, class T2, class T3>
	X_INLINE int endFunction(const T1& value1, const T2& value2, const T3& value3);

};


#else

#endif



struct IScriptTableDumpSink
{
	virtual ~IScriptTableDumpSink(){}
	virtual void OnElementFound(const char* name, Type::Enum type) X_ABSTRACT;
	virtual void OnElementFound(int nIdx, Type::Enum type) X_ABSTRACT;
};

/*

struct IScriptTableIterator
{
	virtual ~IScriptTableIterator(){}
	virtual void AddRef();
	// Summary:
	//	 Decrements reference delete script table iterator.
	virtual void Release();

	virtual bool Next(ScriptValue &var);
};
*/

struct ScriptFunctionDesc
{
	typedef core::Delegate<int(IFunctionHandler*)> ScriptFunction;
	typedef core::traits::Function<int(IFunctionHandler* pH, void* pBuffer, int32_t size)> UserDataFunction;

	X_INLINE ScriptFunctionDesc();

	const char* pFunctionName;			// Name of function.
	const char* pFunctionParams;		// List of parameters (ex "nSlot,vDirection" ).
	const char* pGlobalName;			// Name of global table (ex "Core")
	ScriptFunction function;			// Pointer to simple function.
	int   paramIdOffset;				// Offset of the parameter to accept as 1st function argument.
	int   userDataSize;
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

	virtual ~IScriptTable(){}

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

	virtual void clear(void) X_ABSTRACT; // clears the table, removes all the entries in the table.
	virtual size_t count(void) X_ABSTRACT; // gets the count of elements into the object.

	// member iteration.
	virtual IScriptTable::Iterator begin(void) X_ABSTRACT;
	virtual bool next(Iterator &iter) X_ABSTRACT;
	virtual void end(const Iterator &iter) X_ABSTRACT;


	// Description:
	//    Produces a copy of the src table.
	// Arguments
	//    bDeepCopy - Defines if source table is cloned recursively or not,
	//                if bDeepCopy is false Only does shallow copy (no deep copy, table entries are not cloned hierarchically).
	//                If bDeepCopy is true, all sub tables are also cloned recursively.
	//                If bDeepCopy is true and bCopyByReference is true, the table structure is copied but the tables are left empty and the metatable is set to point at the original table.
	virtual bool clone(IScriptTable* pSrcTable, bool bDeepCopy = false, bool bCopyByReference = false) X_ABSTRACT;

	virtual void dump(IScriptTableDumpSink* p) X_ABSTRACT;

	virtual bool addFunction(const ScriptFunctionDesc& fd) X_ABSTRACT;

	// -------------- helpers --------------

	// push to next available index.
	template <class T>
	X_INLINE void pushBack(const T& value);

	template <class T> 
	X_INLINE void setValue(const char* pKey, const T &value);

	template <class T>
	X_INLINE void setValue(int idx, const T& value);

	template <class T>
	X_INLINE void setValueChain(const char* pKey, const T &value);

	//	 Gets value of a table member.
	template <class T>
	X_INLINE bool getValue(const char* pKey, T& value);

	template <class T>
	X_INLINE bool getValue(int idx, T& value);

	template <class T> 
	X_INLINE bool getValueChain(const char* pKey, T &value);

	
	X_INLINE bool haveValue(const char* pKey);
	X_INLINE bool haveValue(int idx);

	X_INLINE void setToNull(const char* pKey);
	X_INLINE void setToNull(int idx);
	X_INLINE void setToNullChain(const char* pKey);
};

class XScriptableBase
{
public:
	typedef IScriptTable::ScriptFunction ScriptFunction;

public:
	X_INLINE XScriptableBase();
	X_INLINE virtual ~XScriptableBase();

	X_INLINE virtual void init(IScriptSys* pSS, ICore* pCore, int paramIdOffset = 0);
	X_INLINE void setGlobalName(const char* GlobalName);

	X_INLINE IScriptTable* getMethodsTable(void);

protected:
	X_INLINE void registerGlobal(const char* pName, float value);
	X_INLINE void registerGlobal(const char* pName, int value);

	X_INLINE void registerFunction(const char* pFuncName, const IScriptTable::ScriptFunction& function);

protected:
	core::StackString<60> name_;
	ICore* pCore_;
	IScriptSys* pScriptSys_;
	IScriptTable* pMethodsTable_;
	int paramIdOffset_;
};


#define SCRIPT_CHECK_PARAMETERS(_n) \
if (pH->getParamCount() != _n) \
{  \
	pH->getIScriptSystem()->onScriptError("[%s] %d arguments passed, " #_n " expected)", __FUNCTION__, pH->getParamCount()); \
	return pH->endFunction();  \
}

#define SCRIPT_CHECK_PARAMETERS_MIN(_n) \
if (pH->getParamCount() < _n) \
{  \
	pH->getIScriptSystem()->onScriptError("[%s] %d arguments passed, at least " #_n " expected)", __FUNCTION__, pH->getParamCount()); \
	return pH->endFunction(); \
}


// ===========================================================
class SmartScriptTable
{
public:
	SmartScriptTable() : pTable(nullptr) {};
	SmartScriptTable(const SmartScriptTable& st)
	{
		pTable = st.pTable;
		if (pTable) 
			pTable->addRef();
	}
	SmartScriptTable(IScriptTable* newp)
	{
		if (newp) 
			newp->addRef();
		pTable = newp;
	}

	// Copy operator.
	SmartScriptTable& operator=(IScriptTable* newp)
	{
		if (newp) 
			newp->addRef();
		if (pTable) 
			pTable->release();
		pTable = newp;
		return *this;
	}
	// Copy operator.
	SmartScriptTable& operator=(const SmartScriptTable& st)
	{
		if (st.pTable) 
			st.pTable->addRef();
		if (pTable) 
			pTable->release();
		pTable = st.pTable;
		return *this;
	}

	explicit SmartScriptTable(IScriptSys* pSS, bool bCreateEmpty = false)
	{
		pTable = pSS->createTable(bCreateEmpty);
		pTable->addRef();
	}
	~SmartScriptTable() {
		if (pTable) 
			pTable->release();
	}

	// Casts
	IScriptTable* operator->() const { return pTable; }
	IScriptTable* operator*() const { return pTable; }
	operator const IScriptTable*() const { return pTable; }
	operator IScriptTable*() const { return pTable; }
	operator bool() const { return (pTable != nullptr); }

	// Boolean comparasions.
	bool operator ! () const { return pTable == nullptr; };
	bool operator ==(const IScriptTable* p2) const { return pTable == p2; };
	bool operator ==(IScriptTable* p2) const  { return pTable == p2; };
	bool operator !=(const IScriptTable* p2) const { return pTable != p2; };
	bool operator !=(IScriptTable* p2) const { return pTable != p2; };
	bool operator < (const IScriptTable* p2) const { return pTable < p2; };
	bool operator >(const IScriptTable* p2) const { return pTable > p2; };

	IScriptTable* GetPtr() const { return pTable; }

	bool Create(IScriptSys* pSS, bool bCreateEmpty = false)
	{
		if (pTable)
			pTable->release();
		pTable = pSS->createTable(bCreateEmpty);
		pTable->addRef();
		return (pTable) ? true : false;
	}

private:
	IScriptTable* pTable;
};
	


X_NAMESPACE_END

#include "IScriptSys.inl"

#endif // !_X_SCRIPT_SYS_I_H_
