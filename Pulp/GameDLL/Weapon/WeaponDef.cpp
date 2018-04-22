#include "stdafx.h"
#include "WeaponDef.h"

#include <IFileSys.h>
#include <I3DEngine.h>
#include <IAnimManager.h>

X_NAMESPACE_BEGIN(game)

namespace weapon
{
    WeaponDef::WeaponDef(core::string& name) :
        AssetBase(name, assetDb::AssetType::WEAPON),
        ammoTypeId_(weapon::INVALID_AMMO_TYPE)
    {
        soundHashes_.fill(0);
        icons_.fill(nullptr);
        animations_.fill(nullptr);
        effects_.fill(nullptr);
    }

    bool WeaponDef::processData(core::XFile* pFile)
    {
        if (pFile->readObj(hdr_) != sizeof(hdr_)) {
            return false;
        }

        if (!hdr_.isValid()) {
            X_ERROR("WeaponDef", "Header is invalid: \"%s\"", getName().c_str());
            return false;
        }

        data_ = core::makeUnique<uint8_t[]>(g_gameArena, hdr_.dataSize);
        if (pFile->read(data_.get(), hdr_.dataSize) != hdr_.dataSize) {
            return false;
        }

        auto* pAnimManager = gEnv->p3DEngine->getAnimManager();
        auto* pMaterialMan = gEnv->p3DEngine->getMaterialManager();
        auto* pEffectMan = gEnv->p3DEngine->getEffectManager();

        for (uint32_t i = 0; i < AnimSlot::ENUM_COUNT; i++) {
            if (hdr_.animSlots[i] != 0) {
                animations_[i] = pAnimManager->loadAnim(getAnimSlot(static_cast<AnimSlot::Enum>(i)));
            }
        }

        for (uint32_t i = 0; i < IconSlot::ENUM_COUNT; i++) {
            if (hdr_.iconSlots[i] != 0) {
                icons_[i] = pMaterialMan->loadMaterial(getIconSlot(static_cast<IconSlot::Enum>(i)));
            }
        }

        for (uint32_t i = 0; i < EffectSlot::ENUM_COUNT; i++) {
            if (hdr_.effectSlots[i] != 0) {
                effects_[i] = pEffectMan->loadEffect(getEffectSlot(static_cast<EffectSlot::Enum>(i)));
            }
        }

        // build the sound event hashes.
        for (uint32_t i = 0; i < SoundSlot::ENUM_COUNT; i++) {
            if (hdr_.sndSlots[i] != 0) {
                soundHashes_[i] = sound::getIDFromStr(getSoundSlot(static_cast<SoundSlot::Enum>(i)));
            }
        }

        // wait for them to load.
        for (uint32_t i = 0; i < AnimSlot::ENUM_COUNT; i++) {
            if (animations_[i] != nullptr) {
                pAnimManager->waitForLoad(animations_[i]);
            }
        }

        for (uint32_t i = 0; i < IconSlot::ENUM_COUNT; i++) {
            if (icons_[i] != nullptr) {
                pMaterialMan->waitForLoad(icons_[i]);
            }
        }

        for (uint32_t i = 0; i < EffectSlot::ENUM_COUNT; i++) {
            if (hdr_.effectSlots[i] != 0) {
                pEffectMan->waitForLoad(effects_[i]);
            }
        }

        return true;
    }

    void WeaponDef::assignProps(const WeaponDef& oth)
    {
        X_UNUSED(oth);
        X_ASSERT_NOT_IMPLEMENTED();
    }

} // namespace weapon

X_NAMESPACE_END