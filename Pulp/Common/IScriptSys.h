#pragma once

#ifndef _X_SCRIPT_SYS_I_H_
#define _X_SCRIPT_SYS_I_H_

#include "Util\Delegate.h"


X_NAMESPACE_BEGIN(script)


X_DECLARE_ENUM(ScriptMoudles)(
	CORE, 
	SCRIPT, 
	SOUND, 
	IO
);

X_DECLARE_ENUM(ScriptValueType)(
	TNIL, 
	NONE, 
	BOOLEAN, 
	NUMBER, 
	STRING, 
	TABLE, 
	VECTOR, 
	FUNCTION, 
	POINTER, 
	HANDLE, 
	USERDATA
);

class SmartScriptTable;

struct IScriptTableDumpSink;
struct IScriptTableIterator;
struct IScriptTable;

struct XScriptFuncHandle {};
typedef XScriptFuncHandle* HSCRIPTFUNCTION;

// note this is a union not a struct
union ScriptHandle
{
	ScriptHandle() : ptr(0) {};
	ScriptHandle(int i) : id(i) {};
	ScriptHandle(void* p) : ptr(p) {};

	UINT_PTR id;
	void* ptr;
};

struct ScriptValue
{
	typedef ScriptValueType Type;

	ScriptValue(bool value) : type_(Type::BOOLEAN) { b = value; }
	ScriptValue(int value) : type_(Type::NUMBER) { number = (float)value; }
	ScriptValue(unsigned int value) : type_(Type::NUMBER) { number = (float)value; }
	ScriptValue(float value) : type_(Type::NUMBER) { number = value; }
	ScriptValue(const char* value) : type_(Type::STRING) { str = value; }
	ScriptValue(IScriptTable* pTable_);
	ScriptValue(HSCRIPTFUNCTION function) : type_(Type::FUNCTION) { pFunction = function; }
	ScriptValue(ScriptHandle value) : type_(Type::HANDLE) { ptr = value.ptr; }
	ScriptValue(const Vec3f& vec) : type_(Type::VECTOR) { vec3.x = vec.x; vec3.y = vec.y; vec3.z = vec.z; }
	ScriptValue(const SmartScriptTable& value);
	ScriptValue() : type_(Type::NONE), pTable(nullptr) {
	}

	ScriptValue(bool, int) : type_(Type::BOOLEAN) {};
	ScriptValue(int, int) : type_(Type::NUMBER) {};
	ScriptValue(unsigned int, int) : type_(Type::NUMBER) {};
	ScriptValue(float, int) : type_(Type::NUMBER) {};
	ScriptValue(const char*, int) : type_(Type::STRING) {};
	ScriptValue(IScriptTable*, int);
	ScriptValue(HSCRIPTFUNCTION, int) : type_(Type::FUNCTION) {};
	ScriptValue(ScriptHandle value, int) : type_(Type::HANDLE) { X_UNUSED(value); };
	ScriptValue(const Vec3f&, int) : type_(Type::VECTOR) {};
	ScriptValue(const SmartScriptTable &value, int);

	~ScriptValue() {
		Clear();
	}

	X_INLINE void Clear()
	{
		type_ = Type::NONE;
	}

	X_INLINE Type::Enum getType(void) const {
		return type_;
	}

	X_INLINE ScriptValue& operator=(const ScriptValue& rhs);
	X_INLINE bool operator==(const ScriptValue& rhs) const;
	X_INLINE bool operator!=(const ScriptValue& rhs) const;
	X_INLINE void Swap(ScriptValue& value);

	X_INLINE bool CopyTo(bool &value) const { if (type_ == Type::BOOLEAN) { value = b; return true; }; return false; };
	X_INLINE bool CopyTo(int &value) const { if (type_ == Type::NUMBER) { value = (int)number; return true; }; return false; };
	X_INLINE bool CopyTo(unsigned int &value) const { if (type_ == Type::NUMBER) { value = (unsigned int)number; return true; }; return false; };
	X_INLINE bool CopyTo(float &value) const { if (type_ == Type::NUMBER) { value = number; return true; }; return false; };
	X_INLINE bool CopyTo(const char *&value) const { if (type_ == Type::STRING) { value = str; return true; }; return false; };
	X_INLINE bool CopyTo(char *&value) const { if (type_ == Type::STRING) { value = (char*)str; return true; }; return false; };
	X_INLINE bool CopyTo(ScriptHandle &value) const { if (type_ == Type::HANDLE) { value.ptr = const_cast<void*>(ptr); return true; }; return false; };
	X_INLINE bool CopyTo(HSCRIPTFUNCTION &value) const;
	X_INLINE bool CopyTo(Vec3f& value) const { if (type_ == Type::VECTOR) { value.x = vec3.x; value.y = vec3.y; value.z = vec3.z; return true; }; return false; };
	X_INLINE bool CopyTo(IScriptTable*& value) const; // implemented at the end of header
	X_INLINE bool CopyTo(SmartScriptTable& value) const; // implemented at the end of header


	Type::Enum type_;

	union
	{
		bool b;
		float number;
		const char* str;
		const void* ptr;
		IScriptTable* pTable;
		HSCRIPTFUNCTION pFunction;
		struct { float x, y, z; } vec3;
		struct { void * ptr; int nRef; } ud;
	};

};



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


	virtual HSCRIPTFUNCTION GetFunctionPtr(const char* sFuncName) X_ABSTRACT;
	virtual	HSCRIPTFUNCTION GetFunctionPtr(const char* sTableName, const char* sFuncName) X_ABSTRACT;

	virtual HSCRIPTFUNCTION AddFuncRef(HSCRIPTFUNCTION f) X_ABSTRACT;
	virtual bool CompareFuncRef(HSCRIPTFUNCTION f1, HSCRIPTFUNCTION f2) X_ABSTRACT;
	virtual void ReleaseFunc(HSCRIPTFUNCTION f) X_ABSTRACT;


	// Summary
	//   Creates a new IScriptTable table accessible to the scripts.
	// Return Value:
	//	 A  pointer to the created object, with the reference count of 0.
	virtual IScriptTable* CreateTable(bool bEmpty = false) X_ABSTRACT;
	// Get Global value.
	virtual bool GetGlobalAny(const char* Key, ScriptValue& any) X_ABSTRACT;
	// Set Global value to Null.
	virtual void SetGlobalToNull(const char* Key) {
		SetGlobalAny(Key, ScriptValue(ScriptValueType::TNIL)); 
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


struct IFunctionHandler
{
	virtual ~IFunctionHandler() {}
	// Summary:
	//	 Returns pointer to the script system.
	virtual IScriptSys* GetIScriptSystem() X_ABSTRACT;

	virtual void* GetThis() X_ABSTRACT;
	virtual bool GetSelfAny(ScriptValue &any) X_ABSTRACT;

	// Description:
	//    Retrieves the value of the self passed when calling the table.
	// Notes:
	//	  Always the 1st argument of the function.
	template <class T>
	bool GetSelf(T &value)
	{
		ScriptValue any(value, 0);
		return GetSelfAny(any) && any.CopyTo(value);
	}

	// Description:
	//     Returns the function name of the currently called function.
	// Notes:
	//     Use this only used for error reporting.
	virtual const char* GetFuncName() X_ABSTRACT;

	// Description:
	//    Gets the number of parameter at specified index passed by the Lua.
	virtual int GetParamCount() X_ABSTRACT;

	// Description:
	//    Gets the type of the parameter at specified index passed by the Lua.
	virtual ScriptValueType::Enum GetParamType(int nIdx) X_ABSTRACT;

	// Description:
	//    Gets the nIdx param passed by the script.
	// Arguments:
	//    nIdx - 1-based index of the parameter.
	//    val  - Reference to the C++ variable that will store the value.
	virtual bool GetParamAny(int nIdx, ScriptValue &any) X_ABSTRACT;

	// Description:
	//    Get the nIdx param passed by the script.
	// Arguments:
	//    nIdx - 1-based index of the parameter.
	//    val  - Reference to the C++ variable that will store the value.
	template <typename T>
	bool GetParam(int nIdx, T &value)
	{
		ScriptValue any(value, 0);
		return GetParamAny(nIdx, any) && any.CopyTo(value);
	}

	// Description:
	//    
	virtual int EndFunctionAny(const ScriptValue& any) X_ABSTRACT;
	virtual int EndFunctionAny(const ScriptValue& any1, const ScriptValue& any2) X_ABSTRACT;
	virtual int EndFunctionAny(const ScriptValue& any1, const ScriptValue& any2, 
		const ScriptValue& any3) X_ABSTRACT;
	virtual int EndFunction() X_ABSTRACT;

	template <class T>
	int EndFunction(const T &value) {
		return EndFunctionAny(value);
	}
	template <class T1, class T2>
	int EndFunction(const T1 &value1, const T2 &value2) { 
		return EndFunctionAny(value1, value2); 
	}
	template <class T1, class T2, class T3>
	int EndFunction(const T1 &value1, const T2 &value2, const T3 &value3) { 
		return EndFunctionAny(value1, value2, value3); 
	}
	int EndFunctionNull() {
		return EndFunction(); 
	}

};


struct IScriptTableDumpSink
{
	virtual ~IScriptTableDumpSink(){}
	virtual void OnElementFound(const char* name, ScriptValueType::Enum type) X_ABSTRACT;
	virtual void OnElementFound(int nIdx, ScriptValueType::Enum type) X_ABSTRACT;
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
		ScriptValue any(value, 0);
		return GetValueAny(sKey, any) && any.CopyTo(value);
	}

	bool HaveValue(const char * sKey)
	{
		ScriptValue any;
		GetValueAny(sKey, any);

		switch (any.getType())
		{
			case ScriptValueType::NONE:
				return false;
			default:
				return true;
		}
	}


	//	 Sets member value to nil.
	void SetToNull(const char* sKey)  { 
		SetValueAny(sKey, ScriptValue(ScriptValueType::NONE));
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
		SetValueChain(sKey, ScriptValue(ScriptValueType::NONE)); 
	}


	virtual ScriptValueType::Enum GetValueType(const char* sKey) X_ABSTRACT;
	virtual ScriptValueType::Enum GetAtType(int nIdx) X_ABSTRACT;

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
		return any.getType() != ScriptValueType::NONE;
	}

	// Description:
	//    Sets the value of a member variable to nil at the specified index.
	void SetNullAt(int nIndex) { 
		SetAtAny(nIndex, ScriptValue(ScriptValueType::NONE));
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
			sFunctionName(""),
			sFunctionParams(""),
			sGlobalName(""),
			nParamIdOffset(0),
			userDataSize(0),
			pDataBuffer(nullptr),
			pUserDataFunc(nullptr)
		{}

		const char* sFunctionName;			// Name of function.
		const char* sFunctionParams;		// List of parameters (ex "nSlot,vDirection" ).
		const char* sGlobalName;			// Name of global table (ex "Core")
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
		pTable = pSS->CreateTable(bCreateEmpty);
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
		pTable = pSS->CreateTable(bCreateEmpty);
		pTable->addRef();
		return (pTable) ? true : false;
	}

private:
	IScriptTable* pTable;
};

X_INLINE ScriptValue::ScriptValue(IScriptTable* table) :
type_(Type::TABLE)
{
	pTable = table;
	if (pTable)
		pTable->addRef();
}

X_INLINE ScriptValue::ScriptValue(const SmartScriptTable &value) :
type_(Type::TABLE)
{
	pTable = value;
	if (pTable)
		pTable->addRef();
}	

X_INLINE ScriptValue::ScriptValue(IScriptTable* table, int) :
type_(Type::TABLE)
{
	pTable = table;
	if (pTable)
		pTable->addRef();
}

X_INLINE ScriptValue::ScriptValue(const SmartScriptTable &value, int) : 
type_(Type::TABLE)
{
	pTable = value;
	if (pTable)
		pTable->addRef();
}

X_INLINE bool ScriptValue::CopyTo(IScriptTable*& value) const
{
	if (type_ == Type::TABLE) {
		value = pTable;
		return true;
	}
	return false;
}

X_INLINE bool ScriptValue::CopyTo(SmartScriptTable& value) const
{
	if (type_ == Type::TABLE) {
		value = pTable;
		return true;
	}
	return false;
}

X_INLINE ScriptValue& ScriptValue::operator = (const ScriptValue& rhs)
{
	memcpy(this, &rhs, sizeof(ScriptValue));
	return *this;
}

X_INLINE bool ScriptValue::operator == (const ScriptValue& rhs) const
{
	bool result = type_ == rhs.type_;
	if (result)
	{
		switch (type_)
		{
			case Type::BOOLEAN: result = b == rhs.b; break;
			case Type::NUMBER: result = number == rhs.number; break;
			case Type::STRING: result = str == rhs.str; break;
			case Type::VECTOR: result = vec3.x == rhs.vec3.x && vec3.y == rhs.vec3.y && vec3.z == rhs.vec3.z; break;
			case Type::TABLE: result = pTable == rhs.pTable; break;
			case Type::HANDLE: result = ptr == rhs.ptr; break;
			case Type::FUNCTION: result = gEnv->pScriptSys->CompareFuncRef(pFunction, rhs.pFunction); break;
				//		case Type::USERDATA: result = ud.nRef == rhs.ud.nRef && ud.ptr == rhs.ud.ptr; break;

			default:
				X_ASSERT_NOT_IMPLEMENTED();
				break;
		}
	}
	return result;
}

X_INLINE bool ScriptValue::operator != (const ScriptValue& rhs) const 
{
	return !(*this == rhs);
};

X_INLINE void ScriptValue::Swap(ScriptValue& value)
{
	core::Swap(*this,value);
}




X_NAMESPACE_END


#endif // !_X_SCRIPT_SYS_I_H_
