#pragma once

#include <IPhysics.h>
#include <Time\TimeVal.h>

#include <UserCmd.h>
#include <MetaTableMacros.h>

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

class CompTable;

namespace weapon
{
    class WeaponDef;
}

namespace entity
{
    typedef uint16_t EntityId;

    struct NetworkSync
    {
    };

    struct TransForm : public Transformf
    {
        ADD_META()
    };

    struct Health
    {
        ADD_META()

        int32_t hp;
        int32_t max;
    };

    struct SoundObject
    {
        ADD_META()

        Vec3f offset; // offset of sound object relative to ent's transform.
        sound::OcclusionType::Enum occType;
        sound::SndObjectHandle handle;
    };

    struct SoundEnviroment
    {
        // sound::SndObjectHandle handle;
    };

    struct Mesh
    {
        ADD_META()

        // core::StackString<64> name;
        // TODO replace this with networked string.
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
        Animator() = default;
        Animator(anim::Animator* pAnimator_) : 
            pAnimator(pAnimator_)
        {
        }

        anim::Animator* pAnimator;
    };

    struct EntName
    {
        core::string name;
    };

    // -----------------------------------

    struct Inventory
    {
        ADD_META()

        Inventory();
            
        static constexpr int32_t MAX_AMMO_PER_TYPE = 500;

        typedef uint16_t AmmoInt;

        static_assert(std::numeric_limits<AmmoInt>::max() >= weapon::WEAPON_MAX_AMMO, "Can't store max ammo value");
        static_assert(std::numeric_limits<AmmoInt>::max() >= weapon::WEAPON_MAX_CLIP, "Can't store max clip ammo value");

        // you can have multiple weapons in invetory.
        // what do i store these as?
        // well when we switch weapons we need to basically play the putaway anim then switch weapon def.
        // so i guess we just want a list of weapon defs.
        // ideally I want to map weapons to id's
        // lets says the order weapons are loaded don't matter. and we deal with networking later.
        // we get int id's for each loaded asset, can we use that how we want?
        // well i think so.
        void giveAmmo(weapon::AmmoTypeId type, int32_t num)
        {
            ammo[type] = safe_static_cast<AmmoInt>(core::Min(ammo[type] + num, MAX_AMMO_PER_TYPE));
        }

        int32_t numAmmo(weapon::AmmoTypeId type) const
        {
            return ammo[type];
        }

        bool hasAmmo(weapon::AmmoTypeId type, int32_t num) const
        {
            return ammo[type] >= num;
        }

        bool useAmmo(weapon::AmmoTypeId type, int32_t num)
        {
            if (hasAmmo(type, num)) {
                ammo[type] -= safe_static_cast<AmmoInt>(num);
                return true;
            }

            X_ASSERT_UNREACHABLE();
            return false;
        }

        int32_t getClipAmmo(int32_t weaponId) const
        {
            X_ASSERT(clip[weaponId] >= 0, "Invalid ammo count")(clip[weaponId]);
            return clip[weaponId];
        }
        void setClipAmmo(int32_t weaponId, int32_t num)
        {
            X_ASSERT(num >= 0, "Invalid ammo count")(num);
            clip[weaponId] = safe_static_cast<AmmoInt>(num);
        }

        std::bitset<weapon::WEAPON_MAX_LOADED> weapons;

    private:
        std::array<int16_t, weapon::WEAPON_MAX_LOADED> clip; // only sync'd when you switch weapon.
        std::array<int16_t, weapon::WEAPON_MAX_AMMO_TYPES> ammo;
    };

    struct Weapon
    {
        Weapon();

        X_INLINE bool isReady(void) const
        {
            return state == weapon::State::Idle;
        }
        X_INLINE bool isHolstered(void) const
        {
            return state == weapon::State::Holstered;
        }
        X_INLINE void raise(void)
        {
            holster = false;
        }

        EntityId ownerEnt;

        engine::fx::IEmitter* pFlashEmt;
        engine::fx::IEmitter* pBrassEmt;

        weapon::WeaponDef* pWeaponDef;
        weapon::State::Enum state;
        weapon::StateFlags stateFlags;

        core::TimeVal stateEnd;

        int32_t ammoInClip;
        weapon::AmmoTypeId ammoType;

        bool attack;
        bool reload;
        bool holster;
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
        CharacterController(physics::ICharacterController* pCon) :
            pController(pCon)
        {
        }

        physics::ICharacterController* pController;
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
        ADD_META()

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

        net::UserCmd oldUserCmd;
        net::UserCmd userCmd;

        EntityId weaponEnt;
        EntityId armsEnt;

        int32_t currentWpn;
        int32_t targetWpn;
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

        NetworkSync,

        Animator,
        Velocity,
        PhysicsComponent,
        PhysicsTrigger,
        CharacterController,
        EntName,
        Player>;

    constexpr EnitiyRegister::entity_type INVALID_ENT_ID = EnitiyRegister::INVALID_ID;

} // namespace entity

X_NAMESPACE_END
