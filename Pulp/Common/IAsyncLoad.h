#pragma once

#include <Util\EnumMacros.h>
#include <Util\NamespaceMacros.h>
#include <Util\UniquePointer.h>

X_NAMESPACE_BEGIN(core)


X_DECLARE_ENUM8(LoadStatus) (
	NotLoaded,
	Loading,
	Complete,
	Error
);

class AssetBase;

struct IAssetLoader
{
	virtual ~IAssetLoader() = default;

	virtual bool waitForLoad(AssetBase* passet) X_ABSTRACT;
};

struct IAssetLoadSink
{
	virtual ~IAssetLoadSink() = default;

	virtual bool processData(AssetBase* pAsset, core::UniquePointer<char[]> data, uint32_t dataSize) X_ABSTRACT;
	virtual void onLoadRequestFail(AssetBase* pAsset) X_ABSTRACT;
};

X_NAMESPACE_END