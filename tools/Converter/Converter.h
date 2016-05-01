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
	typedef IConverter::ConvertArgs ConvertArgs;
	typedef IConverter::OutPath OutPath;

public:
	Converter();
	~Converter();

	void PrintBanner(void);

	bool Convert(AssetType::Enum assType, const core::string& name);

private:
	bool Convert_int(AssetType::Enum assType, ConvertArgs& args, const core::Array<uint8_t>& fileData,
		const OutPath& pathOut);

	IConverter* GetConverter(AssetType::Enum assType);
	bool EnsureLibLoaded(AssetType::Enum assType);

	void* LoadDLL(const char* dllName);
	bool IntializeConverterModule(AssetType::Enum assType);
	bool IntializeConverterModule(AssetType::Enum assType, const char* dllName, const char* moduleClassName);

private:
	static void GetOutputPathForAsset(AssetType::Enum assType, const core::string& name, core::Path<char>& pathOut);

private:
	IConverter* converters_[AssetType::ENUM_COUNT];
	assetDb::AssetDB db_;
};


X_NAMESPACE_END