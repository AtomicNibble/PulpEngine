#pragma once



struct IPotatoFactory;


struct IPotatoFactoryRegistry
{
	virtual IPotatoFactory* GetFactory(const char* cname) const = 0;

protected:
	// prevent explicit destruction from client side (delete, shared_ptr, etc)
	virtual ~IPotatoFactoryRegistry() {}
};