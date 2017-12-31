#pragma once


#include <Extension\IPotatoFactory.h>
#include <Extension\IPotatoFactoryRegistryImpl.h>
#include <Containers\FixedArray.h>

#include <Extension\GUID.h>

class XPotatoFactoryRegistryImpl : public IPotatoFactoryRegistryImpl
{
public:
	virtual IPotatoFactory* GetFactory(const char* cname) const X_OVERRIDE;
	virtual IPotatoFactory* GetFactory(const PotatoGUID& guid) const X_OVERRIDE;


	virtual void RegisterFactories(const XRegFactoryNode* pFactories) X_OVERRIDE;
	virtual void UnregisterFactories(const XRegFactoryNode* pFactories) X_OVERRIDE;

public:
	static XPotatoFactoryRegistryImpl& Access();

	XPotatoFactoryRegistryImpl();

private:
	struct FactoryByCName
	{
		const char* pCname;
		IPotatoFactory* pPtr;

		FactoryByCName() : pCname(nullptr), pPtr(nullptr) {}
		FactoryByCName(const char* cname) : pCname(cname), pPtr(0) { }
		FactoryByCName(IPotatoFactory* ptr) : pCname(ptr ? ptr->GetName() : nullptr), pPtr(ptr) {  }
		bool operator <(const FactoryByCName& rhs) const { return strcmp(pCname, rhs.pCname) < 0; }
	};


	struct FactoryByID
	{
		PotatoGUID id;
		IPotatoFactory* pPtr;

		FactoryByID(PotatoGUID id) : id(id), pPtr(nullptr) {  }
		FactoryByID(IPotatoFactory* ptr) : id(ptr ? ptr->GetGUID() : PotatoGUID()), pPtr(ptr) {  }
		bool operator <(const FactoryByID& rhs) const { if (id != rhs.id) return id < rhs.id; return pPtr < rhs.pPtr; }
	};

	typedef core::FixedArray<FactoryByCName, 20> FactoriesByCName;
	typedef core::FixedArray<FactoryByID, 20> FactoriesByID;

private:

	bool GetInsertionPos(IPotatoFactory* pFactory,
		FactoriesByCName::iterator& itPosForCName, FactoriesByID::iterator& itPosForId);

private:
	FactoriesByCName byCName_;
	FactoriesByID byID_;


private:
	static XPotatoFactoryRegistryImpl s_registry;
};