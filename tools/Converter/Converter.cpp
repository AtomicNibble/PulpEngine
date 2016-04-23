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
	core::zero_object(converters_);
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
	IConverter* pCon = GetConverter(type);

	if (pCon) {
		return pCon->Convert(args);
	}

	return false;
}



IConverter* Converter::GetConverter(AssetType::Enum type)
{
	if (!EnsureLibLoaded(type)) {
		return false;
	}

	return converters_[type];
}

bool Converter::EnsureLibLoaded(AssetType::Enum type)
{
	// this needs to be more generic.
	// might move to a single interface for all converter libs.
	if (converters_[type]) {
		return true;
	}


	return IntializeConverterModule(type);
}


bool Converter::IntializeConverterModule(AssetType::Enum assType)
{
	const char* pAssTypeStr = nullptr;
	
	if (assType == AssetType::ANIM) {
		pAssTypeStr = "Anim";
	}
	else if (assType == AssetType::MODEL) {
		pAssTypeStr = "Model";
	}
	else {
		X_ASSERT_UNREACHABLE();
	}

	core::StackString<64> dllName("Engine_");
	dllName.append(pAssTypeStr);
	dllName.append("Lib");

	core::StackString<64> className(dllName);

	return IntializeConverterModule(assType, dllName.c_str(), className.c_str());
}

bool Converter::IntializeConverterModule(AssetType::Enum assType, const char* dllName, const char* moduleClassName)
{
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
		converters_[assType] = pModule->Initialize();
	}
	else
	{
		X_ERROR("Converter", "failed to find interface: %s -> %s", dllName, moduleClassName);
		return false;
	}

	return converters_[assType] != nullptr;
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