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
		if (pFactory && pFactory->ClassSupports(PotatoIdOf<T>()))
		{
			IPotatoUnknown* pInstance = pFactory->CreateInstance();
			T* pTInst = static_cast<T*>(pInstance);
			std::shared_ptr<T> pT = std::shared_ptr<T>(pTInst);
			if (pT) {
				p = pT;
				return true;
			}
		}
	}
	return false;
}


template <class T>
bool PotatoCreateClassInstance(const PotatoGUID& id, std::shared_ptr<T>& p)
{
	p = std::shared_ptr<T>();
	IPotatoFactoryRegistry* pFactoryReg = gEnv->pCore->GetFactoryRegistry();
	if (pFactoryReg)
	{
		IPotatoFactory* pFactory = pFactoryReg->GetFactory(id);
		if (pFactory && pFactory->ClassSupports(PotatoIdOf<T>()))
		{
			IPotatoUnknown* pInstance = pFactory->CreateInstance();
			T* pTInst = static_cast<T*>(pInstance);
			std::shared_ptr<T> pT = std::shared_ptr<T>(pTInst);
			if (pT) {
				p = pT;
				return true;
			}
		}
	}
	return false;
}
