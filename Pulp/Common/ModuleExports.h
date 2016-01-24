#pragma once

#ifndef _X_ENGINE_MOUDLE_EXPORTS_H_
#define _X_ENGINE_MOUDLE_EXPORTS_H_


#include <Core\Platform.h>
#include <icore.h>

#include <Extension\FactoryRegNode.h>
#include <Extension\IPotatoFactoryRegistryImpl.h>

#include <Debugging\InvalidParameterHandler.h>
#include <Debugging\PureVirtualFunctionCallHandler.h>
#include <Debugging\ExceptionHandler.h>
#include <Debugging\AbortHandler.h>
// #include <Debugging\SymbolResolution.h>

#if defined(X_LIB) && !defined(_LAUNCHER)
extern SCoreGlobals* gEnv;
extern core::MallocFreeAllocator* gMalloc;
#else // !X_LIB


SCoreGlobals* gEnv = nullptr;
core::MallocFreeAllocator* gMalloc = nullptr;


#if  !defined(X_LIB)
struct XRegFactoryNode* g_pHeadToRegFactories = 0;
#endif


extern "C" DLL_EXPORT void LinkModule(ICore* pCore, const char* moduleName)
{
	X_UNUSED(moduleName);

//	core::symbolResolution::Startup();
	core::invalidParameterHandler::Startup();
	core::pureVirtualFunctionCallHandler::Startup();
	core::exceptionHandler::Startup();
	core::abortHandler::Startup();

	if (gEnv) // Already registered.
		return;

	if (pCore) {
		gEnv = pCore->GetGlobalEnv();
		gMalloc = pCore->GetGlobalMalloc();
	}

#if  !defined(X_LIB)
	if (pCore)
	{
		IPotatoFactoryRegistryImpl* pGoatFactoryImpl = 
			static_cast<IPotatoFactoryRegistryImpl*>(pCore->GetFactoryRegistry());
		pGoatFactoryImpl->RegisterFactories(g_pHeadToRegFactories);
	}
#endif

}


#endif // !X_LIB

#endif // !_X_ENGINE_MOUDLE_EXPORTS_H_
