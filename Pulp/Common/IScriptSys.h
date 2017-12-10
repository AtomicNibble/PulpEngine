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


// the only reason this is uinptr is so it's a diffrent type than int32_t.
// we onlt need 32bit's for the data, but need it to be a strong type.
typedef uintptr_t ScriptFunctionHandle;

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

	X_INLINE bool CopyTo(bool& value) const;
	X_INLINE bool CopyTo(int32_t& value) const;
	X_INLINE bool CopyTo(uint32_t& value) const;
	X_INLINE bool CopyTo(float& value) const;
	X_INLINE bool CopyTo(const char* &value) const;
	X_INLINE bool CopyTo(char* &value) const;
	X_INLINE bool CopyTo(Handle &value) const;
	X_INLINE bool CopyTo(ScriptFunctionHandle &value) const;
	X_INLINE bool CopyTo(Vec3f& value) const;
	X_INLINE bool CopyTo(IScriptTable*& value) const;
	X_INLINE bool CopyTo(SmartScriptTable& value) const; 


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


#else


struct IScriptSys : public core::IEngineSysBase
{
	virtual ~IScriptSys(){};

	virtual void Update(void) X_ABSTRACT;

	virtual bool ExecuteFile(const char* FileName, bool bRaiseError, bool bForceReload) X_ABSTRACT;
	virtual bool UnLoadScript(const char* FileName) X_ABSTRACT;
	virtual void UnloadScripts() X_ABSTRACT;
	virtual bool ReloadScript(const char* FileName, bool raiseError) X_ABSTRACT;
	virtual bool ReloadScripts() X_ABSTRACT;
	virtual void ListLoadedScripts(void) X_ABSTRACT;
	virtual void LogStackTrace(void) X_ABSTRACT;

	virtual void SetGlobalAny(const char* Key, const ScriptValue& val) X_ABSTRACT;


	virtual ScriptFunctionHandle GetFunctionPtr(const char* sFuncName) X_ABSTRACT;
	virtual	ScriptFunctionHandle GetFunctionPtr(const char* sTableName, const char* sFuncName) X_ABSTRACT;

	virtual ScriptFunctionHandle AddFuncRef(ScriptFunctionHandle f) X_ABSTRACT;
	virtual bool CompareFuncRef(ScriptFunctionHandle f1, ScriptFunctionHandle f2) X_ABSTRACT;
	virtual void ReleaseFunc(ScriptFunctionHandle f) X_ABSTRACT;


	virtual IScriptTable* CreateTable(bool bEmpty = false) X_ABSTRACT;
	// Get Global value.
	virtual bool GetGlobalAny(const char* Key, ScriptValue& any) X_ABSTRACT;
	// Set Global value to Null.
	virtual void SetGlobalToNull(const char* Key) {
		SetGlobalAny(Key, ScriptValue(Type::NIL)); 
	}


	// Set Global value.
	template <class T>
	void SetGlobalValue(const char* Key, const T& value) {
		SetGlobalAny(Key, ScriptValue(value));
	}

	// Get Global value.
	template <class T>
	bool GetGlobalValue(const char* Key, T& value)
	{
		ScriptValue any(value, 0);
		return GetGlobalAny(Key, any) && any.CopyTo(value);
	}


	virtual IScriptTable* CreateUserData(void* ptr, size_t size) X_ABSTRACT;

	virtual void OnScriptError(const char* fmt, ...) X_ABSTRACT;
};

#endif

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

	X_INLINE int endFunction(void);
	X_INLINE int endFunctionNull(void);

	template <class T>
	X_INLINE int endFunction(const T &value);
	template <class T1, class T2>
	X_INLINE int endFunction(const T1 &value1, const T2 &value2);
	template <class T1, class T2, class T3>
	X_INLINE int endFunction(const T1 &value1, const T2 &value2, const T3 &value3);

};


struct IScriptTableDumpSink
{
	virtual ~IScriptTableDumpSink(){}
	virtual void OnElementFound(const char* name, Type::Enum type) X_ABSTRACT;
	virtual void OnElementFound(int nIdx, Type::Enum type) X_ABSTRACT;
};


struct IScriptTableIterator
{
	virtual ~IScriptTableIterator(){}
	virtual void AddRef();
	// Summary:
	//	 Decrements reference delete script table iterator.
	virtual void Release();

	virtual bool Next(ScriptValue &var);
};

struct IScriptTable
{
	virtual ~IScriptTable(){}
	typedef core::Delegate<int(IFunctionHandler*)> ScriptFunction;
	typedef core::traits::Function < int(IFunctionHandler* pH, void* pBuffer, int size)> UserDataFunction;
	// typedef int(*UserDataFunction)(IFunctionHandler* pH, void *pBuffer, int nSize);


	virtual IScriptSys* GetScriptSystem() const X_ABSTRACT;

	virtual void addRef(void) X_ABSTRACT;
	virtual void release(void) X_ABSTRACT;

	virtual void Delegate(IScriptTable *pObj) X_ABSTRACT;
	virtual void* GetUserDataValue() X_ABSTRACT;


	//	 Sets the value of a table member.
	virtual void SetValueAny(const char *sKey, const ScriptValue &any, bool bChain = false) X_ABSTRACT;
	//	 Gets the value of a table member.
	virtual bool GetValueAny(const char *sKey, ScriptValue &any, bool bChain = false) X_ABSTRACT;

	//	 Sets value of a table member.
	template <class T> 
	void SetValue(const char *sKey, const T &value) { 
		SetValueAny(sKey, value); 
	}

	//	 Gets value of a table member.
	template <class T>
	bool GetValue(const char *sKey, T &value) {
		ScriptValue any(ValueType<T>::Type);
		return GetValueAny(sKey, any) && any.CopyTo(value);
	}

	bool HaveValue(const char * sKey)
	{
		ScriptValue any;
		GetValueAny(sKey, any);

		switch (any.getType())
		{
			case Type::NONE:
				return false;
			default:
				return true;
		}
	}


	//	 Sets member value to nil.
	void SetToNull(const char* sKey)  { 
		SetValueAny(sKey, ScriptValue(Type::NONE));
	}

	// Summary:
	//	 Sets/Gets Chain.
	// Notes:
	//	 Is a faster version when doing a big amount of SetValue/GetValue.
	virtual bool BeginSetGetChain() X_ABSTRACT;
	virtual void EndSetGetChain() X_ABSTRACT;

	//	 Sets value of a table member.
	template <class T> 
	void SetValueChain(const char *sKey, const T &value) { 
		SetValueAny(sKey, value, true);
	}
	//	 Gets value of a table member.
	template <class T> 
	bool GetValueChain(const char *sKey, T &value) {
		ScriptValue any(value, 0);
		return GetValueAny(sKey, any, true) && any.CopyTo(value);
	}
	void SetToNullChain(const char *sKey)  {
		SetValueChain(sKey, ScriptValue(Type::NONE)); 
	}


	virtual Type::Enum GetValueType(const char* sKey) X_ABSTRACT;
	virtual Type::Enum GetAtType(int nIdx) X_ABSTRACT;

	// Description:
	//    Sets the value of a member variable at the specified index
	//    this means that you will use the object as vector into the script.
	virtual void SetAtAny(int nIndex, const ScriptValue& any) X_ABSTRACT;

	// Description:
	//    Gets the value of a member variable at the specified index.
	virtual bool GetAtAny(int nIndex, ScriptValue& any) X_ABSTRACT;

	// Description:
	//    Sets the value of a member variable at the specified index.
	template <class T> 
	void SetAt(int nIndex, const T& value) { 
		SetAtAny(nIndex, value);
	}

	// Description:
	//    Gets the value of a member variable at the specified index.
	template <class T>
	bool GetAt(int nIndex, T& value)
	{
		ScriptValue any(value, 0);
		return GetAtAny(nIndex, any) && any.CopyTo(value);
	}

	bool HaveAt(int elem)
	{
		ScriptValue any;
		GetAtAny(elem, any);
		return any.getType() != Type::NONE;
	}

	// Description:
	//    Sets the value of a member variable to nil at the specified index.
	void SetNullAt(int nIndex) { 
		SetAtAny(nIndex, ScriptValue(Type::NONE));
	}

	// Description:
	//	 Adds value at next available index.
	template <class T> 
	void PushBack(const T& value)
	{
		int nNextPos = Count() + 1;
		SetAtAny(nNextPos, value);
	}

	// Summary:
	// Iteration over table parameters.
	struct Iterator
	{
		ScriptValue value;
		ScriptValue key;
		int internal;
	};

	virtual IScriptTable::Iterator BeginIteration() X_ABSTRACT;
	virtual bool MoveNext(Iterator &iter) X_ABSTRACT;
	virtual void EndIteration(const Iterator &iter) X_ABSTRACT;

	// Summary:
	//	 Clears the table,removes all the entries in the table.
	virtual void Clear() X_ABSTRACT;
	// Summary:
	//	 Gets the count of elements into the object.
	virtual int Count() X_ABSTRACT;

	// Description:
	//    Produces a copy of the src table.
	// Arguments
	//    pSrcTable - Source table to clone from.
	//    bDeepCopy - Defines if source table is cloned recursively or not,
	//                if bDeepCopy is false Only does shallow copy (no deep copy, table entries are not cloned hierarchically).
	//                If bDeepCopy is true, all sub tables are also cloned recursively.
	//                If bDeepCopy is true and bCopyByReference is true, the table structure is copied but the tables are left empty and the metatable is set to point at the original table.
	virtual bool Clone(IScriptTable* pSrcTable, bool bDeepCopy = false, bool bCopyByReference = false) X_ABSTRACT;

	virtual void Dump(IScriptTableDumpSink* p) X_ABSTRACT;


	struct XUserFunctionDesc
	{
		XUserFunctionDesc() :
			pFunctionName(""),
			pFunctionParams(""),
			pGlobalName(""),
			nParamIdOffset(0),
			userDataSize(0),
			pDataBuffer(nullptr),
			pUserDataFunc(nullptr)
		{}

		const char* pFunctionName;			// Name of function.
		const char* pFunctionParams;		// List of parameters (ex "nSlot,vDirection" ).
		const char* pGlobalName;			// Name of global table (ex "Core")
		ScriptFunction function;			// Pointer to simple function.
		int   nParamIdOffset;				// Offset of the parameter to accept as 1st function argument.
		int   userDataSize;
		void* pDataBuffer;
		UserDataFunction::Pointer pUserDataFunc;
	};

	
	virtual bool AddFunction(const XUserFunctionDesc& fd) X_ABSTRACT;
};


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
