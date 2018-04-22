#pragma once

#include <IPhysics.h>
#include <Time\TimeVal.h>

#include "UserCmds\UserCmd.h"

X_NAMESPACE_DECLARE(model,
                    class XModel;)

X_NAMESPACE_DECLARE(anim,
                    class Animator;)

X_NAMESPACE_DECLARE(engine,
                    struct IRenderEnt;
                    namespace fx {
                        struct IEmitter;
                    })

X_NAMESPACE_BEGIN(game)

namespace weapon
{
    class WeaponDef;
}

namespace entity
{
    typedef uint16_t EntityId;

    struct TransForm : public Transformf
    {
    };

    struct Health
    {
        int32_t hp;
        int32_t max;
    };

    struct SoundObject
    {
        Vec3f offset; // offset of sound object relative to ent's transform.
        core::string occType;
        sound::SndObjectHandle handle;
    };

    struct SoundEnviroment
    {
        // sound::SndObjectHandle handle;
    };

    struct Mesh
    {
        core::string name;
        model::XModel* pModel;
    };

    struct MeshRenderer
    {
        engine::IRenderEnt* pRenderEnt;
    };

    struct MeshCollider
    {
        physics::ActorHandle actor;
    };

    struct DynamicObject
    {
        physics::ActorHandle actor;
    };

    struct Animator
    {
        anim::Animator* pAnimator;
    };

    struct EntName
    {
        core::string name;
    };

    // -----------------------------------

    struct Inventory
    {


        // i will want per ammo type stores at somepoint
        // but the types are data driven.
        // need some sort of type to index shit maybe.
   //     int32_t ammo;

        // you can have multiple weapons in invetory.
        // what do i store these as?
        // well when we switch weapons we need to basically play the putaway anim then switch weapon def.
        // so i guess we just want a list of weapon defs.
        // ideally I want to map weapons to id's
        // lets says the order weapons are loaded don't matter. and we deal with networking later.
        // we get int id's for each loaded asset, can we use that how we want?
        // well i think so.
        void giveAmmo(weapon::AmmoTypeId type, int32_t num) {
            ammo[type] += num;
        }

        int32_t numAmmo(weapon::AmmoTypeId type) const {
            return ammo[type];
        }

        bool hasAmmo(weapon::AmmoTypeId type, int32_t num) const {
            return ammo[type] >= num;
        }

        bool useAmmo(weapon::AmmoTypeId type, int32_t num) {
            if (hasAmmo(type, num)) {
                ammo[type] -= num;
                return true;
            }
            return false;
        }

        int32_t getClipAmmo(int32_t weaponId) const {
            return clip[weaponId];
        }
        void setClipAmmo(int32_t weaponId, int32_t num) {
            clip[weaponId] = num;
        }

        std::bitset<weapon::WEAPON_MAX_LOADED> weapons;
        
    private:
        std::array<int32_t, weapon::WEAPON_MAX_LOADED> clip; // only sync'd when you switch weapon.
        std::array<int32_t, weapon::WEAPON_MAX_AMMO_TYPES> ammo; 
    };

    struct Weapon
    {
        EntityId ownerEnt;

        engine::fx::IEmitter* pFlashEmt;
        engine::fx::IEmitter* pBrassEmt;

        weapon::WeaponDef* pWeaponDef;
        weapon::State::Enum state;
        weapon::StateFlags stateFlags;

        core::TimeVal stateEnd;

        int32_t ammoInClip;
        int32_t ammoType;

        bool attack;
        bool reload;
    };

    struct Velocity
    {
        Vec3f dir;
    };

    struct PhysicsComponent
    {
        physics::ActorHandle actor;
    };

    struct PhysicsTrigger
    {
        physics::ActorHandle actor;
    };

    struct CharacterController
    {
        physics::ICharacterController* pController;
    };

    struct RenderComponent
    {
        engine::IRenderEnt* pRenderEnt;
        model::XModel* pModel;
    };

    struct Rotator
    {
        Vec3f axis;
        float speed;
    };

    struct Mover // move back and forth
    {
        Vec3f start;
        Vec3f end;
        float time;
        float fract;
    };

    struct Emitter
    {
        core::string effect;
        Vec3f offset;

        engine::fx::IEmitter* pEmitter;
    };

    // struct ScriptName
    // {
    // 	const char* pName;
    // };

    struct RenderView
    {
        Vec2f fov;

        Vec3f viewOrg;
        Matrix33f viewAxis;
    };

    struct Player
    {
        X_DECLARE_FLAGS(State)
        (
            Jump,
            Crouch,
            OnGround);

        typedef Flags<State> StateFlags;

        StateFlags state;

        core::TimeVal jumpTime;

        Vec3f eyeOffset;

        Anglesf viewAngles;
        Anglesf cmdAngles;
        Anglesf deltaViewAngles;

        Anglesf viewBobAngles;
        Vec3f viewBob;

        float bobFrac;
        float bobfracsin;
        int32_t bobCycle;

        Vec3f firstPersonViewOrigin;
        Matrix33f firstPersonViewAxis;

        UserCmd oldUserCmd;
        UserCmd userCmd;

        EntityId weaponEnt;
        EntityId armsEnt;
    };

    struct Attached
    {
        EntityId parentEnt;
        model::BoneHandle parentBone; // not always set

        Vec3f offset;
    };

    using EnitiyRegister = ecs::StandardRegistry<EntityId,
        TransForm,
        Health,

        Mesh,
        MeshRenderer,
        MeshCollider,
        DynamicObject,

        SoundObject,
        SoundEnviroment,

        Inventory,
        Weapon,
        Attached,
        Rotator,
        Mover,
        Emitter,

        Animator,
        Velocity,
        RenderComponent,
        PhysicsComponent,
        PhysicsTrigger,
        CharacterController,
        EntName,
        Player>;

    constexpr EnitiyRegister::entity_type INVALID_ENT_ID = EnitiyRegister::INVALID_ID;

} // namespace entity

X_NAMESPACE_END