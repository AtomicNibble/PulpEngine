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
	/* CONVERTERLIB_EXPORT <- shows all my privates in export list :/ */
	: public IConverterHost
{
	typedef core::traits::Function<void *(ICore *pSystem, const char *moduleName)> ModuleLinkfunc;
public:
	typedef IConverter::ConvertArgs ConvertArgs;
	typedef IConverter::OutPath OutPath;

public:
	CONVERTERLIB_EXPORT Converter(assetDb::AssetDB& db, core::MemoryArenaBase* scratchArea);
	CONVERTERLIB_EXPORT ~Converter();

	CONVERTERLIB_EXPORT void PrintBanner(void);

	// enables force conversion so assets are rebuilt even if not stale.
	CONVERTERLIB_EXPORT void forceConvert(bool force);

	CONVERTERLIB_EXPORT bool Convert(AssetType::Enum assType, const core::string& name);
	CONVERTERLIB_EXPORT bool Convert(int32_t modId);
	CONVERTERLIB_EXPORT bool Convert(int32_t modId, AssetType::Enum assType);
	CONVERTERLIB_EXPORT bool Convert(AssetType::Enum assType);
	CONVERTERLIB_EXPORT bool ConvertAll(void);
	CONVERTERLIB_EXPORT bool CleanAll(const char* pMod = nullptr);

	// IConverterHost
	CONVERTERLIB_EXPORT virtual bool GetAssetData(int32_t assetId, core::Array<uint8_t>& dataOut) X_OVERRIDE;
	CONVERTERLIB_EXPORT virtual bool GetAssetData(const char* pAssetName, AssetType::Enum assType, core::Array<uint8_t>& dataOut) X_OVERRIDE;
	CONVERTERLIB_EXPORT virtual bool AssetExists(const char* pAssetName, assetDb::AssetType::Enum assType) X_OVERRIDE;
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
	void GetOutputPathForAsset(AssetType::Enum assType, const core::string& name,
		const core::Path<char>& modPath, core::Path<char>& pathOut);

private:
	core::MemoryArenaBase* scratchArea_;
	IConverter* converters_[AssetType::ENUM_COUNT];
	assetDb::AssetDB& db_;

	bool forceConvert_;
	bool _pad[3];
};


X_NAMESPACE_END