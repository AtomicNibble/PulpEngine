#include "stdafx.h"
#include "Converter.h"

#include <IModel.h>
#include <IAnimation.h>
#include <IConverterModule.h>

#include <Extension\IPotatoFactory.h>
#include <Extension\PotatoCreateClass.h>

// need assetDB.
X_LINK_LIB("engine_AssetDb")

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

bool Converter::Convert(AssetType::Enum assType, core::string& name)
{
	if (!db_.OpenDB()) {
		X_LOG0("Converter", "Failed to open AssetDb");
		return false;
	}

	int32_t assetId = -1;
	if (db_.AssetExsists(assType, name, &assetId)) {
		X_LOG0("Converter", "Asset does not exists");
		return false;
	}

	core::string argsStr;
	if (db_.GetArgsForAsset(assetId, argsStr)) {
		X_LOG0("Converter", "Failed to get conversion args");
		return false;
	}


	return Convert_int(assType, argsStr);
}


bool Converter::Convert_int(AssetType::Enum assType, ConvertArgs& args)
{
	IConverter* pCon = GetConverter(assType);

	if (pCon) {
		return pCon->Convert(args);
	}

	return false;
}



IConverter* Converter::GetConverter(AssetType::Enum assType)
{
	if (!EnsureLibLoaded(assType)) {
		X_LOG0("Converter", "Failed to load convert for asset type: \"%s\"", AssetType::ToString(assType));
		return false;
	}

	return converters_[assType];
}

bool Converter::EnsureLibLoaded(AssetType::Enum assType)
{
	// this needs to be more generic.
	// might move to a single interface for all converter libs.
	if (converters_[assType]) {
		return true;
	}


	return IntializeConverterModule(assType);
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