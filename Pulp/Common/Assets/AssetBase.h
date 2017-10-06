#pragma once

#include <IAsyncLoad.h>

X_NAMESPACE_BEGIN(core)


class AssetBase
{
public:
	AssetBase(core::string& name);

	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);

	X_INLINE core::LoadStatus::Enum getStatus(void) const;
	X_INLINE bool isLoaded(void) const;
	X_INLINE bool loadFailed(void) const;
	X_INLINE void setStatus(core::LoadStatus::Enum status);
	X_INLINE bool waitForLoad(IAssetLoader* pManager);

	X_INLINE const core::string& getName(void) const;

protected:
	core::string name_;
	int32_t id_;
	core::LoadStatus::Enum status_;
};


X_INLINE AssetBase::AssetBase(core::string& name) :
	name_(name)
{

}

X_INLINE const int32_t AssetBase::getID(void) const
{
	return id_;
}

X_INLINE void AssetBase::setID(int32_t id)
{
	id_ = id;
}

X_INLINE core::LoadStatus::Enum AssetBase::getStatus(void) const
{
	return status_;
}

X_INLINE bool AssetBase::isLoaded(void) const
{
	return status_ == core::LoadStatus::Complete;
}

X_INLINE bool AssetBase::loadFailed(void) const
{
	return status_ == core::LoadStatus::Error;
}

X_INLINE void AssetBase::setStatus(core::LoadStatus::Enum status)
{
	status_ = status;
}

X_INLINE bool AssetBase::waitForLoad(IAssetLoader* pManager)
{
	if (isLoaded()) {
		return true;
	}

	return pManager->waitForLoad(this);
}

X_INLINE const core::string& AssetBase::getName(void) const
{
	return name_;
}



X_NAMESPACE_END
