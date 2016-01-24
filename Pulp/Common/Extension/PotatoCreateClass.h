#pragma once

#include <Extension\IPotatoFactoryRegistry.h>

template <class T>
bool PotatoCreateClassInstance(const char* cname, std::shared_ptr<T>& p)
{
	p = std::shared_ptr<T>();
	IPotatoFactoryRegistry* pFactoryReg = gEnv->pCore->GetFactoryRegistry();
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
