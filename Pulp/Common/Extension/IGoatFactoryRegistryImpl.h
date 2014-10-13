#pragma once

#include "IGoatFactoryRegistry.h"

struct XRegFactoryNode;

struct IGoatFactoryRegistryImpl : public IGoatFactoryRegistry
{
	virtual IGoatFactory* GetFactory(const char* cname) const = 0;



	virtual void RegisterFactories(const XRegFactoryNode* pFactories) = 0;
	virtual void UnregisterFactories(const XRegFactoryNode* pFactories) = 0;

protected:
	// prevent explicit destruction from client side (delete, shared_ptr, etc)
	virtual ~IGoatFactoryRegistryImpl() {}
};