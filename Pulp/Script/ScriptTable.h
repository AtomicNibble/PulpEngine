#pragma once

#ifndef X_SCRIPT_TABLE_H_
#define X_SCRIPT_TABLE_H_

#include "Util\ReferenceCounted.h"

X_NAMESPACE_BEGIN(script)


class XScriptTable : public IScriptTable
{
	X_PACK_PUSH(1);
	struct CFunctionData
	{
		ScriptFunction pFunction;
		int8_t paramIdOffset;
		const char funcName[1];
	};

	struct UserDataFunctionData
	{
		UserDataFunction::Pointer* pFunction;
		int16_t dataSize;
		int8_t paramIdOffset;
		const char funcName[1];
	};
	X_PACK_POP;

public:
	XScriptTable();
	~XScriptTable() X_OVERRIDE;

	// IScriptTable 
	virtual void addRef(void) X_OVERRIDE{ refCount_++; }
	virtual void release(void) X_OVERRIDE{ if (--refCount_ <= 0) DeleteThis(); }

	virtual IScriptSys* GetScriptSystem() const X_OVERRIDE;
	virtual void Delegate(IScriptTable *pMetatable) X_OVERRIDE;
	virtual void* GetUserDataValue() X_OVERRIDE;


	// Set/Get chain.
	virtual bool BeginSetGetChain() X_OVERRIDE;
	virtual void EndSetGetChain() X_OVERRIDE;

	//////////////////////////////////////////////////////////////////////////
	virtual void SetValueAny(const char *sKey, const ScriptValue& any, bool bChain = false) X_OVERRIDE;
	virtual bool GetValueAny(const char *sKey, ScriptValue& any, bool bChain = false) X_OVERRIDE;

	//////////////////////////////////////////////////////////////////////////
	virtual void SetAtAny(int nIndex, const ScriptValue &any) X_OVERRIDE;
	virtual bool GetAtAny(int nIndex, ScriptValue &any) X_OVERRIDE;

	virtual Type::Enum GetValueType(const char* sKey) X_OVERRIDE;
	virtual Type::Enum GetAtType(int nIdx) X_OVERRIDE;

	// Iteration.
	virtual IScriptTable::Iterator BeginIteration() X_OVERRIDE;
	virtual bool MoveNext(Iterator& iter) X_OVERRIDE;
	virtual void EndIteration(const Iterator& iter) X_OVERRIDE;

	virtual void Clear() X_OVERRIDE;
	virtual int  Count() X_OVERRIDE;
	virtual bool Clone(IScriptTable* pSrcTable, bool bDeepCopy = false, bool bCopyByReference = false) X_OVERRIDE;
	virtual void Dump(IScriptTableDumpSink* p) X_OVERRIDE;

	virtual bool AddFunction(const XUserFunctionDesc& fd) X_OVERRIDE;

	// --------------------------------------------------------------------------
	void CreateNew();
	int  GetRef();
	void Attach();
	void AttachToObject(IScriptTable* so);
	void DeleteThis();

	// Create object from pool.
	// Assign a metatable to a table.
	void SetMetatable(IScriptTable* pMetatable);
	// Push reference of this object to the stack.
	void PushRef();
	// Push reference to specified script table to the stack.
	static void PushRef(IScriptTable* pObj);

public:
	static lua_State* L;
	static XScriptSys* pScriptSystem_;

	static std::set<class XScriptTable*> s_allTables_;

private:
	static int StdCFunction(lua_State* L);
	static int StdCUserDataFunction(lua_State* L);

	static void CloneTable(int srcTable, int trgTable);
	static void CloneTable_r(int srcTable, int trgTable);
	static void ReferenceTable_r(int scrTable, int trgTable);

private:
	int refCount_;
	int luaRef_;

};

X_NAMESPACE_END


#endif // !X_SCRIPT_TABLE_H_