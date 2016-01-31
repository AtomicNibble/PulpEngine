#include "stdafx.h"
#include "Converter.h"

#include <IModel.h>
#include <IAnimation.h>
#include <IConverterModule.h>

#include <Extension\IPotatoFactory.h>
#include <Extension\PotatoCreateClass.h>


X_NAMESPACE_BEGIN(converter)


Converter::Converter()
{
	
}

Converter::~Converter()
{

}

void Converter::PrintBanner(void)
{
	X_LOG0("Converter", "=================== V0.1 ===================");

}


bool Converter::Convert(AssetType::Enum type, ConvertArgs& args)
{
	if (!EnsureLibLoaded(type)) {
		return false;
	}

	if (type == AssetType::ANIM) {
		return libs_.pAnimLib->Convert(args);
	}
	if (type == AssetType::MODEL) {
		return libs_.pModelLib->Convert(args);
	}

	X_ASSERT_NOT_IMPLEMENTED();
	return false;
}



bool Converter::EnsureLibLoaded(AssetType::Enum type)
{
	// this needs to be more generic.
	// might move to a single interface for all converter libs.

	if (type == AssetType::ANIM) {
		return LoadAnimLib();
	}
	if (type == AssetType::MODEL) {
		return LoadModelLib();
	}

	X_ASSERT_NOT_IMPLEMENTED();
	return false;;
}

bool Converter::LoadAnimLib(void)
{
	if (libs_.pAnimLib) {
		return true;
	}

	if (!IntializeConverterModule("Engine_AnimLib", "Engine_AnimLib")) {
		return false;
	}

	return true;
}

bool Converter::LoadModelLib(void)
{
	if (libs_.pModelLib) {
		return true;
	}

	if (!IntializeConverterModule("Engine_ModelLib", "Engine_ModelLib")) {
		return false;
	}

	return true;
}

bool Converter::IntializeConverterModule(const char* dllName, const char* moduleClassName)
{
	bool res = false;

	core::Path<char> path(dllName);

	path.setExtension(".dll");

#if !defined(X_LIB) 
	void* hModule = LoadDLL(path.c_str());
	if (!hModule) {
		if (gEnv && gEnv->pLog) {
			X_ERROR("Converter", "Failed to load converter module: %s", dllName);
		}
		return false;
	}

#endif // #if !defined(X_LIB) 


	std::shared_ptr<IConverterModule> pModule;
	if (PotatoCreateClassInstance(moduleClassName, pModule))
	{
		res = pModule->Initialize(libs_);
	}
	else
	{
		X_ERROR("Converter", "failed to find interface: %s -> %s", dllName, moduleClassName);
		return false;
	}

	return res;
}

void* Converter::LoadDLL(const char* dllName)
{
	void* hDll = PotatoLoadLibary(dllName);

	if (!hDll) {
		return nullptr;
	}

	ModuleLinkfunc::Pointer pfnModuleInitISystem = reinterpret_cast<ModuleLinkfunc::Pointer>(
		PotatoGetProcAddress(hDll, "LinkModule"));

	if (pfnModuleInitISystem)
	{
		pfnModuleInitISystem(gEnv->pCore, dllName);
	}

	return hDll;
}

X_NAMESPACE_END