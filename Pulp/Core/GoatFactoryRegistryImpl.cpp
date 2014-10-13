#include "stdafx.h"
#include "GoatFactoryRegistryImpl.h"

#include "Core.h"

#include <Extension\IGoatClass.h>
#include <Extension\IGoatFactory.h>
#include <Extension\FactoryRegNode.h>

#include <algorithm>


X_USING_NAMESPACE;


XGoatFactoryRegistryImpl XGoatFactoryRegistryImpl::s_registry;


XGoatFactoryRegistryImpl& XGoatFactoryRegistryImpl::Access()
{
	return s_registry;
}

XGoatFactoryRegistryImpl::XGoatFactoryRegistryImpl()
{

}


IGoatFactory* XGoatFactoryRegistryImpl::GetFactory(const char* cname) const
{
	if (!cname)
		return 0;

	const FactoryByCName search(cname);
	FactoriesByCNameConstIt it = std::lower_bound(byCName_.begin(), byCName_.end(), search);
	return it != byCName_.end() && !(search < *it) ? (*it).m_ptr : 0;
}



bool XGoatFactoryRegistryImpl::GetInsertionPos(IGoatFactory* pFactory, 
	FactoriesByCNameIt& itPosForCName)
{
	FactoryByCName searchByCName(pFactory);
	FactoriesByCNameIt itForCName 
		= std::lower_bound(byCName_.begin(), byCName_.end(), searchByCName);
	if (itForCName != byCName_.end() && !(searchByCName < *itForCName))
	{
		int error = 1;
		return false;
	}

	itPosForCName = itForCName;
	return true;
}


void XGoatFactoryRegistryImpl::RegisterFactories(const XRegFactoryNode* pFactories)
{
	const XRegFactoryNode* p = pFactories;
	while (p)
	{
		IGoatFactory* pFactory = p->m_pFactory;
		if (pFactory)
		{
			FactoriesByCNameIt itPosForCName;
			if (GetInsertionPos(pFactory, itPosForCName))
			{
				byCName_.insert(itPosForCName, FactoryByCName(pFactory));
				
			//	byCName_.append(FactoryByCName(pFactory));

			}

			p = p->m_pNext;
		}
	}

}

void XGoatFactoryRegistryImpl::UnregisterFactories(const XRegFactoryNode* pFactories)
{
	int goat = 0;

	goat = 5;
}

IGoatFactoryRegistry* XCore::GetFactoryRegistry() const
{
	return &XGoatFactoryRegistryImpl::Access();
}