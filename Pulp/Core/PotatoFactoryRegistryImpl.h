#pragma once


#include <Extension\IEngineFactory.h>
#include <Extension\IEngineFactoryRegistryImpl.h>
#include <Containers\FixedArray.h>

#include <Extension\GUID.h>

class XPotatoFactoryRegistryImpl : public IEngineFactoryRegistryImpl
{
public:
	virtual IEngineFactory* GetFactory(const char* cname) const X_OVERRIDE;
	virtual IEngineFactory* GetFactory(const EngineGUID& guid) const X_OVERRIDE;


	virtual void RegisterFactories(const XRegFactoryNode* pFactories) X_OVERRIDE;
	virtual void UnregisterFactories(const XRegFactoryNode* pFactories) X_OVERRIDE;

public:
	static XPotatoFactoryRegistryImpl& Access();

	XPotatoFactoryRegistryImpl();

private:
	struct FactoryByCName
	{
		const char* pCname;
		IEngineFactory* pPtr;

		FactoryByCName() : pCname(nullptr), pPtr(nullptr) {}
		FactoryByCName(const char* cname) : pCname(cname), pPtr(0) { }
		FactoryByCName(IEngineFactory* ptr) : pCname(ptr ? ptr->GetName() : nullptr), pPtr(ptr) {  }
		bool operator <(const FactoryByCName& rhs) const { return strcmp(pCname, rhs.pCname) < 0; }
	};


	struct FactoryByID
	{
		EngineGUID id;
		IEngineFactory* pPtr;

		FactoryByID(EngineGUID id) : id(id), pPtr(nullptr) {  }
		FactoryByID(IEngineFactory* ptr) : id(ptr ? ptr->GetGUID() : EngineGUID()), pPtr(ptr) {  }
		bool operator <(const FactoryByID& rhs) const { if (id != rhs.id) return id < rhs.id; return pPtr < rhs.pPtr; }
	};

	typedef core::FixedArray<FactoryByCName, 20> FactoriesByCName;
	typedef core::FixedArray<FactoryByID, 20> FactoriesByID;

private:

	bool GetInsertionPos(IEngineFactory* pFactory,
		FactoriesByCName::iterator& itPosForCName, FactoriesByID::iterator& itPosForId);

private:
	FactoriesByCName byCName_;
	FactoriesByID byID_;


private:
	static XPotatoFactoryRegistryImpl s_registry;
};