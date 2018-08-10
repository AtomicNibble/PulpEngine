#pragma once

#ifndef _X_ENGINE_MOUDLE_EXPORTS_H_
#define _X_ENGINE_MOUDLE_EXPORTS_H_

#include <Core\Platform.h>
#include <icore.h>

#include <Extension\FactoryRegNode.h>
#include <Extension\IEngineFactoryRegistryImpl.h>

#include <Debugging\InvalidParameterHandler.h>
#include <Debugging\PureVirtualFunctionCallHandler.h>
#include <Debugging\AbortHandler.h>
// #include <Debugging\SymbolResolution.h>

#if defined(X_LIB) && !defined(_LAUNCHER)
extern CoreGlobals* gEnv;
extern core::MallocFreeAllocator* gMalloc;
#else // !X_LIB

CoreGlobals* gEnv = nullptr;
core::MallocFreeAllocator* gMalloc = nullptr;

#if !defined(X_LIB)
struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;
#endif

#if defined(_LAUNCHER)
extern "C" void LinkModule(ICore* pCore, const char* moduleName)
#else
extern "C" DLL_EXPORT void LinkModule(ICore* pCore, const char* moduleName)
#endif
{
    X_UNUSED(moduleName);

//	core::symbolResolution::Startup();
#ifndef X_NO_DEBUG_HANDLERS
    core::invalidParameterHandler::Startup();
    core::pureVirtualFunctionCallHandler::Startup();
    core::abortHandler::Startup();
#endif // !X_NO_DEBUG_HANDLERS

    if (gEnv) { // Already registered.
        return;
    }

    if (pCore) {
        gEnv = pCore->GetGlobalEnv();
        gMalloc = pCore->GetGlobalMalloc();
    }

#if !defined(X_LIB)
    if (pCore) {
        IEngineFactoryRegistryImpl* pGoatFactoryImpl = static_cast<IEngineFactoryRegistryImpl*>(pCore->GetFactoryRegistry());
        pGoatFactoryImpl->RegisterFactories(g_pHeadToRegFactories);
    }
#endif
}

#endif // !X_LIB

#endif // !_X_ENGINE_MOUDLE_EXPORTS_H_
