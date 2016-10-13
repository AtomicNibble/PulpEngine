#pragma once


#ifndef X_CONVETER_MODULE_I_H_
#define X_CONVETER_MODULE_I_H_

#include <Extension\IPotatoClass.h>
#include <Containers\Array.h>

#include <IAssetDb.h>

X_NAMESPACE_DECLARE(anim,
struct 	IAnimLib
)

X_NAMESPACE_DECLARE(model,
struct IModelLib
)


struct IConverter;
struct IConverterModule : public IPotatoClass
{
	virtual const char* GetName(void) X_ABSTRACT;

	virtual IConverter* Initialize(void) X_ABSTRACT;
	virtual bool ShutDown(IConverter* pCon) X_ABSTRACT;
};

struct IConverterHost
{
	virtual ~IConverterHost() {}

	virtual bool GetAssetData(const char* pAssetName, assetDb::AssetType::Enum assType, core::Array<uint8_t>& dataOut) X_ABSTRACT;
	virtual bool AssetExists(const char* pAssetName, assetDb::AssetType::Enum assType) X_ABSTRACT;
};

struct IConverter
{
	typedef core::string ConvertArgs;
	typedef core::Path<char> OutPath;

	virtual ~IConverter() {}

	// gets the file extension this converter outputs with.
	virtual const char* getOutExtension(void) const X_ABSTRACT;

	virtual bool Convert(IConverterHost& host, ConvertArgs& args, const core::Array<uint8_t>& fileData,
		const OutPath& destPath) X_ABSTRACT;
};




#endif // !X_CONVETER_MODULE_I_H_