#pragma once


#include <IConverterModule.h>
#include <String\CmdArgs.h>

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_DECLARE(anim,
struct 	IAnimLib
)

X_NAMESPACE_DECLARE(model,
struct IModelLib
)

X_NAMESPACE_BEGIN(converter)


typedef assetDb::AssetDB::AssetType AssetType;

class Converter
{
	typedef core::traits::Function<void *(ICore *pSystem, const char *moduleName)> ModuleLinkfunc;
public:
	typedef core::CmdArgs<4096, wchar_t> ConvertArgs;

public:
	Converter();
	~Converter();

	void PrintBanner(void);

	bool Convert(AssetType::Enum type, ConvertArgs& args);


private:
	IConverter* GetConverter(AssetType::Enum type);
	bool EnsureLibLoaded(AssetType::Enum type);

	void* LoadDLL(const char* dllName);
	bool IntializeConverterModule(AssetType::Enum assType);
	bool IntializeConverterModule(AssetType::Enum assType, const char* dllName, const char* moduleClassName);

private:
	IConverter* converters_[AssetType::ENUM_COUNT];
};


X_NAMESPACE_END