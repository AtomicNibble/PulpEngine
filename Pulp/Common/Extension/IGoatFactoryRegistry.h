#pragma once



struct IGoatFactory;


struct IGoatFactoryRegistry
{
	virtual IGoatFactory* GetFactory(const char* cname) const = 0;

//	virtual void IterateFactories(const CryInterfaceID& iid, ICryFactory** pFactories, size_t& numFactories) const = 0;

protected:
	// prevent explicit destruction from client side (delete, shared_ptr, etc)
	virtual ~IGoatFactoryRegistry() {}
};