#include "stdafx.h"
#include "EnityComponents.h"

#include <Containers/FixedBitStream.h>

#include <IPhysics.h>

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


    void DynamicObject::readFromSnapShot(physics::IScene* pScene, core::FixedBitStreamBase& bs)
    {
        physics::RigidBodyProps props;
        bs.read(props);

        pScene->setRigidBodyProps(actor, props);
    }
    
    void DynamicObject::writeToSnapShot(physics::IScene* pScene, core::FixedBitStreamBase& bs)
    {
        physics::RigidBodyProps props;
        pScene->getRigidBodyProps(actor, props);
        
        bs.write(props);
    }

} //namespace entity

X_NAMESPACE_END

