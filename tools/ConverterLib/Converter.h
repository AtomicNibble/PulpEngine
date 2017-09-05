#pragma once


#include <IConverterModule.h>
#include <String\CmdArgs.h>
#include <Platform\Module.h>

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

	typedef std::array<IConverterModule*, AssetType::ENUM_COUNT> ConverterModuleInterfacesArr;

public:
	typedef IConverter::ConvertArgs ConvertArgs;
	typedef IConverter::OutPath OutPath;

public:
	CONVERTERLIB_EXPORT Converter(assetDb::AssetDB& db, core::MemoryArenaBase* scratchArea);
	CONVERTERLIB_EXPORT ~Converter();

	CONVERTERLIB_EXPORT void PrintBanner(void);
	CONVERTERLIB_EXPORT bool Init(void);

	// enables force conversion so assets are rebuilt even if not stale.
	CONVERTERLIB_EXPORT void forceConvert(bool force);
	CONVERTERLIB_EXPORT bool setConversionProfiles(const core::string& profileName);


	CONVERTERLIB_EXPORT bool Convert(AssetType::Enum assType, const core::string& name);
	CONVERTERLIB_EXPORT bool Convert(assetDb::ModId modId);
	CONVERTERLIB_EXPORT bool Convert(assetDb::ModId modId, AssetType::Enum assType);
	CONVERTERLIB_EXPORT bool Convert(AssetType::Enum assType);
	CONVERTERLIB_EXPORT bool ConvertAll(void);
	CONVERTERLIB_EXPORT bool CleanAll(const char* pMod = nullptr);
	CONVERTERLIB_EXPORT bool CleanAll(assetDb::ModId modId);

	// generates thumbs for assets that don't have thumbs already and we support auto thumb generation.
	CONVERTERLIB_EXPORT bool GenerateThumbs(void);

	// IConverterHost
	CONVERTERLIB_EXPORT virtual bool GetAssetArgs(assetDb::AssetId assetId, ConvertArgs& args) X_FINAL;
	CONVERTERLIB_EXPORT virtual bool GetAssetData(assetDb::AssetId assetId, DataArr& dataOut) X_FINAL;
	CONVERTERLIB_EXPORT virtual bool GetAssetData(const char* pAssetName, AssetType::Enum assType, DataArr& dataOut) X_FINAL;
	CONVERTERLIB_EXPORT virtual bool AssetExists(const char* pAssetName, assetDb::AssetType::Enum assType, assetDb::AssetId* pIdOut = nullptr) X_FINAL;
	CONVERTERLIB_EXPORT virtual bool UpdateAssetThumb(assetDb::AssetId assetId, Vec2i thumbDim, Vec2i srcDim, const DataArr& data, core::Compression::Algo::Enum algo, core::Compression::CompressLevel::Enum lvl) X_FINAL;
	CONVERTERLIB_EXPORT virtual bool UpdateAssetThumb(assetDb::AssetId assetId, Vec2i thumbDim, Vec2i srcDim, const DataArr& compressedData) X_FINAL;

	CONVERTERLIB_EXPORT virtual bool getConversionProfileData(assetDb::AssetType::Enum type, core::string& strOut) X_FINAL;

	CONVERTERLIB_EXPORT virtual IConverter* GetConverter(assetDb::AssetType::Enum assType) X_FINAL;
	CONVERTERLIB_EXPORT virtual physics::IPhysLib* GetPhsicsLib(void) X_FINAL;

	CONVERTERLIB_EXPORT virtual core::MemoryArenaBase* getScratchArena(void) X_FINAL;
	// ~IConverterHost

private:
	bool loadConversionProfiles(const core::string& profileName);
	void clearConversionProfiles(void);

	bool CleanMod(assetDb::AssetDB::ModId id, const core::string& name, const core::Path<char>& outDir);

	bool GenerateThumb(AssetType::Enum assType, const core::string& name);
	bool Convert_int(AssetType::Enum assType, assetDb::AssetId assetId, ConvertArgs& args, const OutPath& pathOut);

//	IConverter* GetConverter(AssetType::Enum assType);
	bool EnsureLibLoaded(AssetType::Enum assType);

	bool IntializeConverterModule(AssetType::Enum assType);
	bool IntializeConverterModule(AssetType::Enum assType, const char* dllName, const char* moduleClassName);
	void UnloadConverters(void);

private:
	static void GetOutputPathForAssetType(AssetType::Enum assType,
		const core::Path<char>& modPath, core::Path<char>& pathOut);

	void GetOutputPathForAsset(AssetType::Enum assType, const core::string& name,
		const core::Path<char>& modPath, core::Path<char>& pathOut);

private:
	core::MemoryArenaBase* scratchArea_;
	assetDb::AssetDB& db_;

	// physics converter is special just like you.
	physics::IPhysLib* pPhysLib_;
	IConverterModule* pPhysConverterMod_;

	IConverter* converters_[AssetType::ENUM_COUNT];
	core::string conversionProfiles_[AssetType::ENUM_COUNT];

	bool forceConvert_;
	bool _pad[3];

	ConverterModuleInterfacesArr converterModules_;
};


X_NAMESPACE_END