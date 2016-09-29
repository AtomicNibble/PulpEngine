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

class CONVERTERLIB_EXPORT Converter : public IConverterHost
{
	typedef core::traits::Function<void *(ICore *pSystem, const char *moduleName)> ModuleLinkfunc;
public:
	typedef IConverter::ConvertArgs ConvertArgs;
	typedef IConverter::OutPath OutPath;

public:
	Converter(core::MemoryArenaBase* scratchArea);
	~Converter();

	void PrintBanner(void);

	bool Convert(AssetType::Enum assType, const core::string& name);
	bool ConvertAll(void);
	bool ConvertAll(AssetType::Enum assType);
	bool CleanAll(const char* pMod = nullptr);

	// IConverterHost
	virtual bool GetAssetData(const char* pAssetName, AssetType::Enum assType, core::Array<uint8_t>& dataOut) X_OVERRIDE;
	virtual bool AssetExists(const char* pAssetName, assetDb::AssetType::Enum assType) X_OVERRIDE;
	// ~IConverterHost

private:
	bool CleanMod(assetDb::AssetDB::ModId id, const core::string& name, core::Path<char>& outDir);

	bool Convert_int(AssetType::Enum assType, ConvertArgs& args, const core::Array<uint8_t>& fileData,
		const OutPath& pathOut);

	IConverter* GetConverter(AssetType::Enum assType);
	bool EnsureLibLoaded(AssetType::Enum assType);

	void* LoadDLL(const char* dllName);
	bool IntializeConverterModule(AssetType::Enum assType);
	bool IntializeConverterModule(AssetType::Enum assType, const char* dllName, const char* moduleClassName);

private:
	static void GetOutputPathForAsset(AssetType::Enum assType, const core::string& name,
		const core::Path<char>& modPath, core::Path<char>& pathOut);

private:
	core::MemoryArenaBase* scratchArea_;
	IConverter* converters_[AssetType::ENUM_COUNT];
	assetDb::AssetDB db_;
};


X_NAMESPACE_END