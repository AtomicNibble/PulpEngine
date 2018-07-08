#include "stdafx.h"
#include "EnityComponents.h"


X_NAMESPACE_BEGIN(game)

namespace entity
{

    Inventory::Inventory()
    {
        clip.fill(0);
        ammo.fill(0);
    }

    Weapon::Weapon()
    {
        ownerEnt = EnitiyRegister::INVALID_ID;

        pFlashEmt = nullptr;
        pBrassEmt = nullptr;

        pWeaponDef = nullptr;

        ammoInClip = 0;
        ammoType = 0;

        attack = false;
        reload = false;
        holster = true;
    }

} //namespace entity

X_NAMESPACE_END

