#include "stdafx.h"
#include "AssetFileExtensions.h"

#include <IModel.h>
#include <IAnimation.h>
#include <IMaterial.h>
#include <ICi.h>
#include <IVideo.h>
#include <IWeapon.h>

X_NAMESPACE_BEGIN(assetDb)

namespace
{
	// provides a nicer way to set all the extensions, without having to remember the index
	// each asset enum is.
	// also is resilent to enum order changes.
	class ExtensionHelper
	{
	public:
		ExtensionHelper() 
		{
			static_assert(AssetType::ENUM_COUNT == 15, "Added new asset type, this code might need updating");

			extensions_.fill("");
			extensions_[AssetType::MODEL] = model::MODEL_FILE_EXTENSION;
			extensions_[AssetType::ANIM] = anim::ANIM_FILE_EXTENSION;
			extensions_[AssetType::MATERIAL] = engine::MTL_B_FILE_EXTENSION;
			extensions_[AssetType::IMG] = texture::CI_FILE_EXTENSION;
			extensions_[AssetType::WEAPON] = game::weapon::WEAPON_FILE_EXTENSION;
			extensions_[AssetType::VIDEO] = video::VIDEO_FILE_EXTENSION;
		}

		inline const char* operator[](AssetType::Enum type) const {
			return extensions_[type];
		}

	private:
		std::array<const char*, AssetType::ENUM_COUNT> extensions_;
	};

	const ExtensionHelper extensions;
}

const char* getAssetTypeExtension(AssetType::Enum type)
{
	return extensions[type];
}

X_NAMESPACE_END