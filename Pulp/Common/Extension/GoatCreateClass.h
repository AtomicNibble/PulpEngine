#pragma once

#include <Extension\IGoatFactoryRegistry.h>

template <class Dst, class Src>
Dst interface_cast(Src p)
{
	return std::dynamic_pointer_cast<Dst>(p);
}

template <class T>
bool GoatCreateClassInstance(const char* cname, std::shared_ptr<T>& p)
{
	p = std::shared_ptr<T>();
	IGoatFactoryRegistry* pFactoryReg = gEnv->pCore->GetFactoryRegistry();
	if (pFactoryReg)
	{
		IPotatoFactory* pFactory = pFactoryReg->GetFactory(cname);
		if (pFactory)
		{
			std::shared_ptr<IPotatoClass> pUnk = pFactory->CreateInstance();
			std::shared_ptr<T> pT = std::static_pointer_cast<T>(pUnk);
			if (pT) {
				p = pT;
				return true;
			}
		}
	}
	return false;
}
