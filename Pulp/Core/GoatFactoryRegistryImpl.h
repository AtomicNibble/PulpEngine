#pragma once


#include <Extension\IGoatFactory.h>
#include <Extension\IGoatFactoryRegistryImpl.h>
#include <Containers\FixedArray.h>

#include <vector>

class XGoatFactoryRegistryImpl : public IGoatFactoryRegistryImpl
{
public:
	virtual IPotatoFactory* GetFactory(const char* cname) const;


	virtual void RegisterFactories(const XRegFactoryNode* pFactories);
	virtual void UnregisterFactories(const XRegFactoryNode* pFactories);

public:
	static XGoatFactoryRegistryImpl& Access();

	XGoatFactoryRegistryImpl();

private:
	struct FactoryByCName
	{
		const char* m_cname;
		IPotatoFactory* m_ptr;

		FactoryByCName() : m_cname(nullptr), m_ptr(nullptr) {}
		FactoryByCName(const char* cname) : m_cname(cname), m_ptr(0) { }
		FactoryByCName(IPotatoFactory* ptr) : m_cname(ptr ? ptr->GetName() : 0), m_ptr(ptr) {  }
		bool operator <(const FactoryByCName& rhs) const { return strcmp(m_cname, rhs.m_cname) < 0; }
	};

	// typedef std::vector<FactoryByCName> FactoriesByCName;
	typedef core::FixedArray<FactoryByCName,16> FactoriesByCName;
	typedef FactoriesByCName::iterator FactoriesByCNameIt;
	typedef FactoriesByCName::const_iterator FactoriesByCNameConstIt;


	bool GetInsertionPos(IPotatoFactory* pFactory,
		FactoriesByCNameIt& itPosForCName);

private:
	FactoriesByCName byCName_;


private:
	static XGoatFactoryRegistryImpl s_registry;
};