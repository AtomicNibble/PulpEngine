#pragma once

#include "IPotatoFactoryRegistry.h"

struct XRegFactoryNode;

struct IPotatoFactoryRegistryImpl : public IPotatoFactoryRegistry
{
	virtual IPotatoFactory* GetFactory(const char* cname) const = 0;



	virtual void RegisterFactories(const XRegFactoryNode* pFactories) = 0;
	virtual void UnregisterFactories(const XRegFactoryNode* pFactories) = 0;

protected:
	// prevent explicit destruction from client side (delete, shared_ptr, etc)
	virtual ~IPotatoFactoryRegistryImpl() {}
};