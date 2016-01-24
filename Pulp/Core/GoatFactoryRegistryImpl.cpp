#include "stdafx.h"
#include "GoatFactoryRegistryImpl.h"

#include "Core.h"

#include <Extension\IPotatoClass.h>
#include <Extension\IPotatoFactory.h>
#include <Extension\FactoryRegNode.h>

#include <algorithm>


X_USING_NAMESPACE;


XPotatoFactoryRegistryImpl XPotatoFactoryRegistryImpl::s_registry;


XPotatoFactoryRegistryImpl& XPotatoFactoryRegistryImpl::Access()
{
	return s_registry;
}

XPotatoFactoryRegistryImpl::XPotatoFactoryRegistryImpl()
{

}


IPotatoFactory* XPotatoFactoryRegistryImpl::GetFactory(const char* cname) const
{
	if (!cname)
		return 0;

	const FactoryByCName search(cname);
	FactoriesByCNameConstIt it = std::lower_bound(byCName_.begin(), byCName_.end(), search);
	return it != byCName_.end() && !(search < *it) ? (*it).m_ptr : 0;
}



bool XPotatoFactoryRegistryImpl::GetInsertionPos(IPotatoFactory* pFactory,
	FactoriesByCNameIt& itPosForCName)
{
	FactoryByCName searchByCName(pFactory);
	FactoriesByCNameIt itForCName 
		= std::lower_bound(byCName_.begin(), byCName_.end(), searchByCName);
	if (itForCName != byCName_.end() && !(searchByCName < *itForCName))
	{
		return false;
	}

	itPosForCName = itForCName;
	return true;
}


void XPotatoFactoryRegistryImpl::RegisterFactories(const XRegFactoryNode* pFactories)
{
	const XRegFactoryNode* p = pFactories;
	while (p)
	{
		IPotatoFactory* pFactory = p->m_pFactory;
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

void XPotatoFactoryRegistryImpl::UnregisterFactories(const XRegFactoryNode* pFactories)
{
	X_UNUSED(pFactories);
	X_ASSERT_NOT_IMPLEMENTED();
}

IPotatoFactoryRegistry* XCore::GetFactoryRegistry() const
{
	return &XPotatoFactoryRegistryImpl::Access();
}