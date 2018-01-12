#pragma once

#include "IEngineFactoryRegistry.h"

struct EngineGUID;
struct XRegFactoryNode;

struct IEngineFactoryRegistryImpl : public IEngineFactoryRegistry
{
	virtual IEngineFactory* GetFactory(const char* cname) const = 0;
	virtual IEngineFactory* GetFactory(const EngineGUID& guid) const = 0;


	virtual void RegisterFactories(const XRegFactoryNode* pFactories) = 0;
	virtual void UnregisterFactories(const XRegFactoryNode* pFactories) = 0;

protected:
	// prevent explicit destruction from client side (delete, shared_ptr, etc)
	virtual ~IEngineFactoryRegistryImpl() {}
};