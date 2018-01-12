#pragma once


struct EngineGUID;
struct IEngineFactory;

struct IEngineFactoryRegistry
{
	virtual IEngineFactory* GetFactory(const char* cname) const = 0;
	virtual IEngineFactory* GetFactory(const EngineGUID& guid) const = 0;

protected:
	// prevent explicit destruction from client side (delete, shared_ptr, etc)
	virtual ~IEngineFactoryRegistry() {}
};