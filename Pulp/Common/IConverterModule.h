#pragma once


#ifndef X_CONVETER_MODULE_I_H_
#define X_CONVETER_MODULE_I_H_

#include <Extension\IPotatoUnknown.h>
#include <Containers\Array.h>

#include <IAssetDb.h>

X_NAMESPACE_DECLARE(anim,
struct 	IAnimLib
)

X_NAMESPACE_DECLARE(model,
struct IModelLib
)


struct IConverter;
struct IConverterModule : public IPotatoUnknown
{
	POTATO_INTERFACE_DECLARE(IConverterModule, 0x695c4e33, 0x4481, 0x45a7, 0x91, 0xe8, 0xb6, 0x4b, 0x67, 0x13, 0xe1, 0x6d );

	virtual const char* GetName(void) X_ABSTRACT;

	virtual IConverter* Initialize(void) X_ABSTRACT;
	virtual bool ShutDown(IConverter* pCon) X_ABSTRACT;
};

struct IConverterHost
{
	typedef core::Array<uint8_t> DataArr;

	virtual ~IConverterHost() {}

	virtual bool GetAssetData(int32_t assetId, DataArr& dataOut) X_ABSTRACT;
	virtual bool GetAssetData(const char* pAssetName, assetDb::AssetType::Enum assType, DataArr& dataOut) X_ABSTRACT;
	virtual bool AssetExists(const char* pAssetName, assetDb::AssetType::Enum assType) X_ABSTRACT;
	virtual bool UpdateAssetThumb(int32_t assetId, Vec2i thumbDim, Vec2i srcDim, const DataArr& data) X_ABSTRACT;

	// get global conversion settings data.
	virtual bool getConversionProfileData(assetDb::AssetType::Enum type, core::string& strOut) X_ABSTRACT;

	virtual IConverter* GetConverter(assetDb::AssetType::Enum assType) X_ABSTRACT;

	virtual core::MemoryArenaBase* getScratchArena(void) X_ABSTRACT;
};

struct IConverter
{
	typedef core::string ConvertArgs;
	typedef core::Path<char> OutPath;

	virtual ~IConverter() {}

	// gets the file extension this converter outputs with.
	virtual const char* getOutExtension(void) const X_ABSTRACT;

	virtual bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_ABSTRACT;

	// thumbs disabled for all types by default.
	virtual bool thumbGenerationSupported(void) const { return false; }
	virtual bool CreateThumb(IConverterHost& host, int32_t assetId, Vec2i targetDim) {
		X_UNUSED(host);
		X_UNUSED(assetId);
		X_UNUSED(targetDim);
		X_ASSERT_UNREACHABLE();
		return false;
	};
};




#endif // !X_CONVETER_MODULE_I_H_