#pragma once

#include <Extension\IEngineFactoryRegistry.h>

template<class T>
bool EngineCreateClassInstance(const char* cname, std::shared_ptr<T>& p)
{
    p = std::shared_ptr<T>();
    IEngineFactoryRegistry* pFactoryReg = gEnv->pCore->GetFactoryRegistry();
    if (pFactoryReg) {
        IEngineFactory* pFactory = pFactoryReg->GetFactory(cname);
        if (pFactory && pFactory->ClassSupports(EngineIdOf<T>())) {
            IEngineUnknown* pInstance = pFactory->CreateInstance();
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

template<class T>
bool EngineCreateClassInstance(const EngineGUID& id, std::shared_ptr<T>& p)
{
    p = std::shared_ptr<T>();
    IEngineFactoryRegistry* pFactoryReg = gEnv->pCore->GetFactoryRegistry();
    if (pFactoryReg) {
        IEngineFactory* pFactory = pFactoryReg->GetFactory(id);
        if (pFactory && pFactory->ClassSupports(EngineIdOf<T>())) {
            IEngineUnknown* pInstance = pFactory->CreateInstance();
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
