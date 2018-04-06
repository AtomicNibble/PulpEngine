#include "stdafx.h"
#include "PotatoFactoryRegistryImpl.h"

#include "Core.h"

#include <Extension\IEngineUnknown.h>
#include <Extension\IEngineFactory.h>
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

IEngineFactory* XPotatoFactoryRegistryImpl::GetFactory(const char* cname) const
{
    if (!cname) {
        return nullptr;
    }

    const FactoryByCName search(cname);
    auto it = std::lower_bound(byCName_.begin(), byCName_.end(), search);
    return it != byCName_.end() && !(search < *it) ? (*it).pPtr : nullptr;
}

IEngineFactory* XPotatoFactoryRegistryImpl::GetFactory(const EngineGUID& guid) const
{
    const FactoryByID search(guid);

    auto it = std::lower_bound(byID_.begin(), byID_.end(), search);
    return it != byID_.end() && !(search < *it) ? (*it).pPtr : nullptr;
}

bool XPotatoFactoryRegistryImpl::GetInsertionPos(IEngineFactory* pFactory,
    FactoriesByCName::iterator& itPosForCName, FactoriesByID::iterator& itPosForId)
{
    FactoryByCName searchByCName(pFactory);
    auto itForCName = std::lower_bound(byCName_.begin(), byCName_.end(), searchByCName);
    if (itForCName != byCName_.end() && !(searchByCName < *itForCName)) {
        X_ASSERT_UNREACHABLE();
        return false;
    }

    FactoryByID searchByID(pFactory);
    auto itForId = std::lower_bound(byID_.begin(), byID_.end(), searchByID);
    if (itForId != byID_.end() && !(searchByID < *itForId)) {
        X_ASSERT_UNREACHABLE();
        return false;
    }

    itPosForCName = itForCName;
    itPosForId = itForId;
    return true;
}

void XPotatoFactoryRegistryImpl::RegisterFactories(const XRegFactoryNode* pFactories)
{
    const XRegFactoryNode* p = pFactories;
    while (p) {
        IEngineFactory* pFactory = p->pFactory;
        if (pFactory) {
            FactoriesByCName::iterator itPosForCName;
            FactoriesByID::iterator itPosForCNameID;
            if (GetInsertionPos(pFactory, itPosForCName, itPosForCNameID)) {
                byCName_.insert(itPosForCName, FactoryByCName(pFactory));
                byID_.insert(itPosForCNameID, FactoryByID(pFactory));
            }

            p = p->pNext;
        }
    }
}

void XPotatoFactoryRegistryImpl::UnregisterFactories(const XRegFactoryNode* pFactories)
{
    X_UNUSED(pFactories);
    X_ASSERT_NOT_IMPLEMENTED();
}

IEngineFactoryRegistry* XCore::GetFactoryRegistry() const
{
    return &XPotatoFactoryRegistryImpl::Access();
}