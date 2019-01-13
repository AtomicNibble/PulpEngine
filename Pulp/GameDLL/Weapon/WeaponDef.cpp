#include "stdafx.h"
#include "WeaponDef.h"

#include <IFileSys.h>
#include <I3DEngine.h>
#include <IAnimManager.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
    WeaponDef::WeaponDef(core::string_view name) :
        AssetBase(name, assetDb::AssetType::WEAPON),
        ammoTypeId_(weapon::INVALID_AMMO_TYPE),
		pHdr_(nullptr)
    {
        soundHashes_.fill(0);
        icons_.fill(nullptr);
        animations_.fill(nullptr);
        effects_.fill(nullptr);
    }

	bool WeaponDef::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
    {
        if (dataSize < sizeof(WeaponHdr)) {
            return false;
        }

		auto& hdr = *reinterpret_cast<WeaponHdr*>(data.get());

        if (!hdr.isValid()) {
            X_ERROR("WeaponDef", "Header is invalid: \"%s\"", getName().c_str());
            return false;
        }

        auto* pAnimManager = gEnv->p3DEngine->getAnimManager();
        auto* pMaterialMan = gEnv->p3DEngine->getMaterialManager();
        auto* pEffectMan = gEnv->p3DEngine->getEffectManager();

		// Need to set this for getAnimSlot, etc.. to work.
		pHdr_ = reinterpret_cast<WeaponHdr*>(data.get());
		data_ = std::move(data);

        for (uint32_t i = 0; i < AnimSlot::ENUM_COUNT; i++) {
            if (hdr.animSlots[i] != 0) {
                animations_[i] = pAnimManager->loadAnim(core::string_view(getAnimSlot(static_cast<AnimSlot::Enum>(i))));
            }
        }

        for (uint32_t i = 0; i < IconSlot::ENUM_COUNT; i++) {
            if (hdr.iconSlots[i] != 0) {
                icons_[i] = pMaterialMan->loadMaterial(core::string_view(getIconSlot(static_cast<IconSlot::Enum>(i))));
            }
        }

        for (uint32_t i = 0; i < EffectSlot::ENUM_COUNT; i++) {
            if (hdr.effectSlots[i] != 0) {
                effects_[i] = pEffectMan->loadEffect(core::string_view(getEffectSlot(static_cast<EffectSlot::Enum>(i))));
            }
        }

        // build the sound event hashes.
        for (uint32_t i = 0; i < SoundSlot::ENUM_COUNT; i++) {
            if (hdr.sndSlots[i] != 0) {
                soundHashes_[i] = sound::getIDFromStr(getSoundSlot(static_cast<SoundSlot::Enum>(i)));
            }
        }

        return true;
    }

    void WeaponDef::assignProps(const WeaponDef& oth)
    {
		soundHashes_ = oth.soundHashes_;
		icons_ = oth.icons_;
		animations_ = oth.animations_;
		effects_ = oth.effects_;
		ammoTypeId_ = oth.ammoTypeId_;
		pHdr_ = oth.pHdr_;
    }

    bool WeaponDef::waitForLoadDep(void) const
    {
		X_ASSERT_NOT_NULL(pHdr_);

        auto* p3DEngine = gEnv->p3DEngine;
        auto* pAnimManager = p3DEngine->getAnimManager();
        auto* pMaterialMan = p3DEngine->getMaterialManager();
        auto* pEffectMan = p3DEngine->getEffectManager();

        for (uint32_t i = 0; i < AnimSlot::ENUM_COUNT; i++) {
            if (animations_[i] != nullptr) {
                if (!pAnimManager->waitForLoad(animations_[i])) {
                    return false;
                }
            }
        }

        for (uint32_t i = 0; i < IconSlot::ENUM_COUNT; i++) {
            if (icons_[i] != nullptr) {
                if (!pMaterialMan->waitForLoad(icons_[i])) {
                    return false;
                }
            }
        }

        for (uint32_t i = 0; i < EffectSlot::ENUM_COUNT; i++) {
            if (pHdr_->effectSlots[i] != 0) {
                if (!pEffectMan->waitForLoad(effects_[i])) {
                    return false;
                }
            }
        }

        return true;
    }

} // namespace weapon

X_NAMESPACE_END