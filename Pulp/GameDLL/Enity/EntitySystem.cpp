#include "stdafx.h"
#include "EntitySystem.h"
#include "Vars\GameVars.h"
#include "UserNetMappings.h"
#include "Weapon\WeaponDef.h"
#include "Weapon\WeaponManager.h"
#include "Multiplayer.h"

#include <Containers\FixedBitStream.h>
#include <String\Json.h>
#include <Hashing\Fnva1Hash.h>
#include <Time\TimeLiterals.h>

#include <UserCmdMan.h>
#include <SnapShot.h>

#include <IFrameData.h>
#include <I3DEngine.h>
#include <IWorld3D.h>
#include <IAnimManager.h>
#include <IEffect.h>

#include <numeric>

using namespace core::Hash::Literals;

X_NAMESPACE_BEGIN(game)

namespace entity
{
    // -----------------------------------------------------------

    EnititySystem::EnititySystem(GameVars& vars, game::weapon::WeaponDefManager& weaponDefs, 
        Multiplayer* pMultiplayer, core::MemoryArenaBase* arena) :
        arena_(arena),
        reg_(&ecsArena_),
        vars_(vars),
        weaponDefs_(weaponDefs),
        pMultiplayer_(pMultiplayer),
        ecsAllocator_(),
        ecsArena_(&ecsAllocator_, "ECSArena"),
        playerSys_(vars.player),
        cameraSys_(vars),
        weaponSys_(vars),
        dtHealth_(arena),
        dtMesh_(arena),
        dtSoundObj_(arena),
        dtRotator_(arena),
        dtMover_(arena),
        dtEmitter_(arena),
        dtDynamicObject_(arena)
    {
        arena->addChildArena(&ecsArena_);

        pPhysics_ = nullptr;
        pPhysScene_ = nullptr;
        p3DWorld_ = nullptr;
        pModelManager_ = nullptr;
        pEffectManager_ = nullptr;

        entIdMap_.fill(INVALID_ENT_ID);
    }

    bool EnititySystem::init(physics::IPhysics* pPhysics, physics::IScene* pPhysScene, engine::IWorld3D* p3DWorld)
    {
        static_assert(decltype(reg_)::NUM_COMP == 18, "More components? add a sensible reserve call");

        reg_.entIdReserve(MAX_ENTS);
        reg_.freelistReserve(64);
        reg_.compReserve<TransForm>(MAX_ENTS);
        reg_.compReserve<Health>(64);
        reg_.compReserve<Mesh>(256);
        reg_.compReserve<MeshRenderer>(256);
        reg_.compReserve<MeshCollider>(256);
        reg_.compReserve<DynamicObject>(256);
        reg_.compReserve<SoundObject>(64);
        reg_.compReserve<Inventory>(net::MAX_PLAYERS);
        reg_.compReserve<Weapon>(64);
        reg_.compReserve<Attached>(16);
        reg_.compReserve<Rotator>(16);
        reg_.compReserve<Mover>(16);
        reg_.compReserve<Emitter>(64);
        reg_.compReserve<NetworkSync>(256);
        reg_.compReserve<Animator>(64);
        reg_.compReserve<CharacterController>(net::MAX_PLAYERS);
        reg_.compReserve<EntName>(16);
        reg_.compReserve<Player>(net::MAX_PLAYERS);

        reg_.setDetroyCallback<EnititySystem, Mesh>(this);
        reg_.setDetroyCallback<EnititySystem, MeshRenderer>(this);
        reg_.setDetroyCallback<EnititySystem, MeshCollider>(this);
        reg_.setDetroyCallback<EnititySystem, DynamicObject>(this);
        reg_.setDetroyCallback<EnititySystem, Weapon>(this);
        reg_.setDetroyCallback<EnititySystem, Emitter>(this);
        reg_.setDetroyCallback<EnititySystem, Animator>(this);
        reg_.setDetroyCallback<EnititySystem, CharacterController>(this);
        reg_.setDetroyCallback<EnititySystem, Player>(this);


        pPhysics_ = X_ASSERT_NOT_NULL(pPhysics);
        pPhysScene_ = X_ASSERT_NOT_NULL(pPhysScene);
        p3DWorld_ = X_ASSERT_NOT_NULL(p3DWorld);
        pModelManager_ = gEnv->p3DEngine->getModelManager();
        pEffectManager_ = gEnv->p3DEngine->getEffectManager();
        auto* pAnimManager = gEnv->p3DEngine->getAnimManager();

        if (!pModelManager_ || !pEffectManager_) {
            return false;
        }

        if (!playerSys_.init(reg_, pPhysScene, p3DWorld)) {
            return false;
        }

        if (!cameraSys_.init()) {
            return false;
        }

        if (!soundSys_.init(reg_)) {
            return false;
        }

        if (!physSys_.init(reg_, pPhysScene_)) {
            return false;
        }

        if (!animatedSys_.init()) {
            return false;
        }

        if (!weaponSys_.init(reg_, pPhysScene, pAnimManager)) {
            return false;
        }

        if (!emitterSys_.init(reg_)) {
            return false;
        }

        if (!meshRendererSys_.init(reg_, p3DWorld)) {
            return false;
        }

        if (!healthSys_.init(reg_)) {
            return false;
        }

        for (uint32_t i = 0; i < net::MAX_PLAYERS; i++) {
            auto id = reg_.create();
            if (id != i) {
                X_ASSERT_UNREACHABLE();
                return false;
            }
        }

        // need to build translators for all the components.
        if (!createTranslatours()) {
            return false;
        }

        return true;
    }

    void EnititySystem::shutdown(void)
    {
        arena_->removeChildArena(&ecsArena_);

        // TODO; cleanup should just happen.
        for (EntityId i = 0; i < net::MAX_PLAYERS; i++) {
            if (reg_.isValid(i) && reg_.has<Player>(i)) {
                removePlayer(i);
            }
        }
    }

    bool EnititySystem::createTranslatours(void)
    {
        // translators.
        ADD_TRANS_MEMBER(dtHealth_, hp);
        ADD_TRANS_MEMBER(dtHealth_, max);

        ADD_TRANS_MEMBER(dtMesh_, name);

        ADD_TRANS_MEMBER(dtSoundObj_, offset);
        dtSoundObj_.initializeFromString("occType", [&](SoundObject& snd, const char* pOccType, size_t length) -> bool {
            snd.occType = sound::OcclusionType::None;

            switch (core::Hash::Fnv1aHash(pOccType, length)) {
                case "None"_fnv1a:
                    snd.occType = sound::OcclusionType::None;
                    break;
                case "SingleRay"_fnv1a:
                    snd.occType = sound::OcclusionType::SingleRay;
                    break;
                case "MultiRay"_fnv1a:
                    snd.occType = sound::OcclusionType::MultiRay;
                    break;
                default:
                    X_ERROR("Ent", "Invalid occlusion type: \"%s\"", pOccType);
                    return false;
            }

            return true;
        });

        ADD_TRANS_MEMBER(dtRotator_, axis);
        ADD_TRANS_MEMBER(dtRotator_, speed);

        ADD_TRANS_MEMBER(dtMover_, start);
        ADD_TRANS_MEMBER(dtMover_, end);
        ADD_TRANS_MEMBER(dtMover_, time);

        ADD_TRANS_MEMBER(dtEmitter_, effect);
        ADD_TRANS_MEMBER(dtEmitter_, offset);

        ADD_TRANS_MEMBER(dtDynamicObject_, kinematic);
        return true;
    }

    void EnititySystem::runUserCmdForPlayer(core::TimeVal dt, const net::UserCmd& cmd, EntityId playerId)
    {
        playerSys_.runUserCmdForPlayer(dt, reg_, weaponDefs_, pModelManager_, p3DWorld_, cmd, playerId);
    }

    void EnititySystem::update(core::FrameData& frame, net::UserCmdMan& userCmdMan, EntityId localPlayerId)
    {
        X_UNUSED(userCmdMan);

        reg_.update(frame.timeInfo.deltas[core::Timer::GAME]);

        if (reg_.numPendingEvents() > 1024) {
            X_WARNING("Ent", "Large number of pending events: %" PRIuS, reg_.numPendingEvents());
        }

        cameraSys_.setActiveEnt(localPlayerId);

        // update the cameras.
        cameraSys_.update(frame, reg_, pPhysScene_);

        playerSys_.update(frame.timeInfo, reg_);

        soundSys_.update(frame, reg_);

        animatedSys_.update(frame.timeInfo, reg_, p3DWorld_);

        physSys_.update(frame, reg_);

        weaponSys_.update(frame, reg_);

        emitterSys_.update(frame, reg_);

        meshRendererSys_.update(frame, reg_);

        healthSys_.update(frame.timeInfo, reg_);


        // do this before the next frame so that it's not simulated in physics.
        // otherwise we can end up with transform updates for deleted ents.
        // even tho we delete the actor the transform buffer has already been populated.
        reg_.cleanupPendingDestroy();
    }

    void EnititySystem::createSnapShot(net::SnapShot& snap)
    {
        physics::ScopedLock lock(pPhysScene_, physics::LockAccess::Read);

        core::FixedBitStreamStack<256> bs; // max-size?

        auto view = reg_.view<NetworkSync, entity::TransForm>();
        for (auto entityId : view)
        {
            // you want to be synced, fuck you!
            auto& netSync = reg_.get<NetworkSync>(entityId);
            X_UNUSED(netSync);

            auto& trans = reg_.get<TransForm>(entityId);
            auto mask = reg_.getComponentMask(entityId);

            bs.write(mask);

            // - data -
            bs.write(trans);

            if (reg_.has<entity::DynamicObject>(entityId))
            {
                auto& col = reg_.get<DynamicObject>(entityId);
                col.writeToSnapShot(pPhysScene_, bs);
            }

            snap.addObject(entityId, bs);

            bs.reset();
        }
    }

    void EnititySystem::applySnapShot(const UserNetMappings& unm, const net::SnapShot& snap)
    {
        // TODO: perf - maybe gather up physics changes and do them in a batch.
        physics::ScopedLock lock(pPhysScene_, physics::LockAccess::Write);

        for (size_t i = 0; i < snap.getNumObjects(); i++)
        {
            net::ObjectID id = snap.getObjectIDByIndex(i);

            // TEMP: temp hack, should really dispatch based on id.
            if (id == net::SnapShot::SNAP_MP_STATE) {
                continue;
            }

            auto bs = snap.getMessageByIndex(i);

            auto remoteEntityId = static_cast<entity::EntityId>(id);

            auto entityId = entIdMap_[remoteEntityId];

            if (bs.isEos())
            {
                // the remotemap should give us a valid ent if we are been told to delete it.
                X_ASSERT(reg_.isValid(entityId), "Enitity id is not valid")(entityId);

                if (entityId < net::MAX_PLAYERS)
                {
                    removePlayer(entityId);
                }
                else
                {
                    destroyEnt(entityId);
                }

                entIdMap_[remoteEntityId] = INVALID_ENT_ID;
                continue;
            }

            // spawn?
            if (entityId == INVALID_ENT_ID)
            {
                if (remoteEntityId < net::MAX_PLAYERS)
                {
                    // spawn a player!
                    // TODO: pos
                    auto pos = Vec3f(-80, -50.f + (remoteEntityId * 50.f), 10);

                    // need to work out if this is local.
                    bool isLocal = unm.localPlayerIdx == remoteEntityId;

                    spawnPlayer(unm, remoteEntityId, pos, isLocal);

                    // player id's always match up.
                    entityId = remoteEntityId;
                }
                else
                {
                    // we make the ent and the normal path should handle adding the components.
                    entityId = reg_.create<entity::TransForm>();
                }

                entIdMap_[remoteEntityId] = entityId;
            }

            X_ASSERT(reg_.isValid(entityId), "Enitity id is not valid")(entityId);

            auto localMask = reg_.getComponentMask(entityId);
            decltype(localMask) mask;
            bs.read(mask);

#if 0
            // components changed?
            if (mask != localMask)
            {
                // well shit on my tits.
                if ((mask & localMask).any())
                {
                    // add
                    X_ASSERT_NOT_IMPLEMENTED();
                }
                else
                {
                    // remove
                    X_ASSERT_NOT_IMPLEMENTED();
                }
            }
#endif


            // we always have transform?
            // for everything else do i just have to check the flags?
            auto& trans = reg_.get<TransForm>(entityId);
            bs.read(trans);

            if (reg_.has<DynamicObject>(mask))
            {
                auto& dyn = reg_.get<DynamicObject>(entityId);
                dyn.readFromSnapShot(pPhysScene_, bs);
            }

            if (reg_.has<Player>(mask))
            {
                auto& ply = reg_.get<Player>(entityId);

                if (ply.pRenderEnt) {
                    p3DWorld_->updateRenderEnt(ply.pRenderEnt, trans);
                }
            }

            if (reg_.has<Inventory>(mask))
            {

            }

            if (reg_.has<MeshRenderer>(mask))
            {
                auto& rend = reg_.get<MeshRenderer>(entityId);
                p3DWorld_->updateRenderEnt(rend.pRenderEnt, trans);
            }

            if (reg_.has<CharacterController>(mask))
            {
                auto& con = reg_.get<CharacterController>(entityId);
                con.pController->setFootPosition(Vec3d(trans.pos));
            }
        }
    }

    // ----------------------------------------------------------

    void EnititySystem::destroy(Mesh& comp)
    {
        pModelManager_->releaseModel(comp.pModel);
    }

    void EnititySystem::destroy(MeshRenderer& comp)
    {
        p3DWorld_->freeRenderEnt(comp.pRenderEnt);
    }

    void EnititySystem::destroy(MeshCollider& comp)
    {
        // is this for static collision only?
        X_ERROR("Ent", "A ent with static collision was removed, collision will remain.");

        if (comp.actor == physics::INVALID_HANLDE) {
            return;
        }

        comp.actor = physics::INVALID_HANLDE;
    }

    void EnititySystem::destroy(DynamicObject& comp)
    {
        if (comp.actor == physics::INVALID_HANLDE) {
            return;
        }

        physics::ScopedLock lock(pPhysScene_, physics::LockAccess::Write);

        pPhysScene_->removeActor(comp.actor);
        pPhysics_->releaseActor(comp.actor);

        comp.actor = physics::INVALID_HANLDE;
    }

    void EnititySystem::destroy(Weapon& comp)
    {
        weaponDefs_.releaseWeaponDef(comp.pWeaponDef);
    }

    void EnititySystem::destroy(Emitter& comp)
    {
        if (!comp.pEmitter) {
            return;
        }

        p3DWorld_->freeEmitter(comp.pEmitter);
    }

    void EnititySystem::destroy(Animator& comp)
    {
        if (comp.pAnimator) {
            X_DELETE(comp.pAnimator, g_gameArena);
        }
    }

    void EnititySystem::destroy(CharacterController& comp)
    {
        X_ASSERT_NOT_NULL(comp.pController);

        physics::ScopedLock lock(pPhysScene_, physics::LockAccess::Write);
        pPhysScene_->releaseCharacterController(comp.pController);
    }

    void EnititySystem::destroy(Player& comp)
    {
        if (comp.armsEnt != entity::INVALID_ID) {
            destroyEnt(comp.armsEnt);
        }

        if (comp.weaponEnt != entity::INVALID_ID) {
            destroyEnt(comp.weaponEnt);
        }
    }

    // ----------------------------------------------------------

    EntityId EnititySystem::createEnt(void)
    {
        auto ent = reg_.create<TransForm>();

        return ent;
    }

    void EnititySystem::destroyEnt(EntityId id)
    {
        reg_.destroy(id);
    }

    void EnititySystem::removePlayer(EntityId id)
    {
        // We must reset for players instead of destroy as the enity slots need to remain reserved.
        reg_.reset(id);
    }

    EntityId EnititySystem::createWeapon(EntityId playerId)
    {
        auto& ply = reg_.get<Player>(playerId);
        auto& inv = reg_.get<Inventory>(playerId);

        model::BoneHandle tagWeapon = model::INVALID_BONE_HANDLE;

        auto armsEnt = reg_.create<TransForm, Mesh, MeshRenderer, Animator>();

        ply.armsEnt = armsEnt;
        {
            auto& trans = reg_.get<TransForm>(armsEnt);
            auto& mesh = reg_.get<Mesh>(armsEnt);
            auto& meshRend = reg_.get<MeshRenderer>(armsEnt);
            auto& an = reg_.get<Animator>(armsEnt);

            trans.pos = Vec3f(-110.f, 16.f, 74.f);

            mesh.pModel = pModelManager_->loadModel("test/arms/view_jap");
            pModelManager_->waitForLoad(mesh.pModel);

            tagWeapon = mesh.pModel->getBoneHandle("tag_weapon");

            an.pAnimator = X_NEW(anim::Animator, g_gameArena, "Animator")(mesh.pModel, g_gameArena);

            engine::RenderEntDesc entDsc;
            entDsc.pModel = mesh.pModel;
            entDsc.trans = trans;
            meshRend.pRenderEnt = p3DWorld_->addRenderEnt(entDsc);
        }

        auto ent = reg_.create<TransForm, Mesh, MeshRenderer, Weapon, Animator, Attached>();
        auto& trans = reg_.get<TransForm>(ent);
        auto& mesh = reg_.get<Mesh>(ent);
        auto& meshRend = reg_.get<MeshRenderer>(ent);
        auto& wpn = reg_.get<Weapon>(ent);
        auto& an = reg_.get<Animator>(ent);
        auto& att = reg_.get<Attached>(ent);
        //		auto& emit = reg_.assign<Emitter>(ent);

        // so in order to make a weapon we need:
        // - weapon def - for info about weapon
        // - viewmodel - to render
        // - animator - to animate model
        // - weapon - to store state

        auto* pWeaponDef = weaponDefs_.loadWeaponDef("test/sw_357");
        weaponDefs_.waitForLoad(pWeaponDef);

        pWeaponDef->waitForLoadDep();

        const char* pViewModel = pWeaponDef->getModelSlot(weapon::ModelSlot::Gun);

        trans.pos = Vec3f(-110.f, 16.f, 30.f);

        // get model.
        mesh.pModel = pModelManager_->loadModel(pViewModel);
        pModelManager_->waitForLoad(mesh.pModel);

        // setup render ent
        engine::RenderEntDesc entDsc;
        entDsc.pModel = mesh.pModel;
        entDsc.trans = trans;
        meshRend.pRenderEnt = p3DWorld_->addRenderEnt(entDsc);

        // add a animator.
        an.pAnimator = X_NEW(anim::Animator, g_gameArena, "Animator")(mesh.pModel, g_gameArena);

        // setup the weapon state.
        const auto ammotTypeId = pWeaponDef->getAmmoTypeId();
        const auto startingAmmo = pWeaponDef->getAmmoSlot(weapon::AmmoSlot::Start);
        const auto clipSize = pWeaponDef->getAmmoSlot(weapon::AmmoSlot::ClipSize);
        const auto clipFill = core::Min(startingAmmo, clipSize);
        const auto ammoForStore = startingAmmo - clipFill;

        wpn.ammoInClip = clipFill;
        wpn.ammoType = ammotTypeId;
        wpn.pWeaponDef = pWeaponDef;
        wpn.state = weapon::State::Holstered;
        wpn.raise();
        wpn.ownerEnt = playerId;
        wpn.stateEnd = core::TimeVal(0.f);

        // full me up.
        inv.giveAmmo(ammotTypeId, ammoForStore);

        {
            engine::EmitterDesc dsc;
            dsc.trans = trans;
            dsc.pEffect = nullptr;
            dsc.looping = false;

            wpn.pFlashEmt = p3DWorld_->addEmmiter(dsc);
            wpn.pBrassEmt = p3DWorld_->addEmmiter(dsc);
        }

        att.parentEnt = armsEnt;
        att.parentBone = tagWeapon;

        return ent;
    }

    void EnititySystem::spawnPlayer(const UserNetMappings& unm, int32_t clientIdx, const Vec3f& pos, bool local)
    {
        X_UNUSED(unm);

        X_ASSERT(clientIdx < net::MAX_PLAYERS, "Client index larger than max player")(clientIdx);

        auto id = static_cast<EntityId>(clientIdx);

        auto& trans = reg_.assign<TransForm>(id);
        auto& player = reg_.assign<Player>(id);
        auto& hp = reg_.assign<Health>(id);
        auto& inv = reg_.assign<Inventory>(id);
        auto& net = reg_.assign<NetworkSync>(id);

        trans.pos = pos;
        player.isLocal = local;

        if (local)
        {
            // arms

        }
        else
        {
            // world model.
            player.pModel = pModelManager_->loadModel("test/anim/char_rus_guard_grachev");
            
            engine::RenderEntDesc entDsc;
            entDsc.pModel = player.pModel;

            player.pRenderEnt = p3DWorld_->addRenderEnt(entDsc);
        }

        X_UNUSED(player, net);
        player.armsEnt = entity::INVALID_ID;
        //		player.eyeOffset = Vec3f(0, 0, 50.f);
        //		player.cameraOrigin = Vec3f(0, 0, 50.f);
        //		player.cameraAxis = Anglesf(0, 0, 0.f);

        hp.hp = 100;
        hp.max = 100;

        auto giveWeapon = [&](const char* pName) {
            auto* pWpn = weaponDefs_.loadWeaponDef(pName);

            weaponDefs_.waitForLoad(pWpn);

            inv.weapons.set(pWpn->getID());
            inv.giveAmmo(pWpn->getAmmoTypeId(), pWpn->getAmmoSlot(weapon::AmmoSlot::Start));
        };

        giveWeapon("test/sw_357");
        giveWeapon("test/mg42");
        giveWeapon("test/raygun");

        player.currentWpn = 1;
        player.targetWpn = 1;

        addController(id);

        // temp, give player a weapon
        player.weaponEnt = createWeapon(id);

        if (pMultiplayer_) {
            pMultiplayer_->playerSpawned(clientIdx);
        }
    }

    bool EnititySystem::addController(EntityId id)
    {
        auto& trans = reg_.get<TransForm>(id);
        
        physics::CapsuleControllerDesc desc;
        desc.radius = 20.f;
        desc.height = vars_.player.normalHeight_;
        desc.climbingMode = physics::CapsuleControllerDesc::ClimbingMode::Easy;
        desc.material = pPhysics_->getDefaultMaterial();
        desc.position = trans.pos;
        desc.upDirection = Vec3f::zAxis();
        desc.maxJumpHeight = vars_.player.jumpHeight_;
        desc.pUserData = (const void*)id;

        physics::ICharacterController* pController;
        {
            physics::ScopedLock lock(pPhysScene_, physics::LockAccess::Write);
            pController = pPhysScene_->createCharacterController(desc);
        }

        if (!pController) {
            return false;
        }

        reg_.assign<CharacterController>(id, pController);
        return true;
    }

    bool EnititySystem::loadEntites(const char* pJsonBegin, const char* pJsonEnd)
    {
        // TODO: bother checking we clear? 

        if (!parseEntites(pJsonBegin, pJsonEnd)) {
            X_ERROR("Ents", "Failed to parse ents");
            return false;
        }

        // now i need to build the id map.
        // i basically need the newst id.
        endOfmapEnts_ = reg_.create();

        // players are not valid yet.
        std::iota(entIdMap_.begin() + net::MAX_PLAYERS, entIdMap_.begin() + endOfmapEnts_, static_cast<EntityId>(net::MAX_PLAYERS));

        return true;
    }

    bool EnititySystem::parseEntites(const char* pJsonBegin, const char* pJsonEnd)
    {
        core::json::MemoryStream ms(pJsonBegin, union_cast<ptrdiff_t>(pJsonEnd - pJsonBegin));
        core::json::EncodedInputStream<core::json::UTF8<>, core::json::MemoryStream> is(ms);

        core::json::Document d;
        if (d.ParseStream<core::json::kParseCommentsFlag>(is).HasParseError()) {
            auto err = d.GetParseError();
            const char* pErrStr = core::json::GetParseError_En(err);
            size_t offset = d.GetErrorOffset();
            size_t line = core::strUtil::LineNumberForOffset(pJsonBegin, pJsonEnd, offset);

            X_ERROR("Ents", "Failed to parse ent desc(%" PRIi32 "): Offset: %" PRIuS " Line: %" PRIuS " Err: %s",
                err, offset, line, pErrStr);
            return false;
        }

        if (d.GetType() != core::json::Type::kObjectType) {
            X_ERROR("Ents", "Unexpected type");
            return false;
        }

        for (auto it = d.MemberBegin(); it != d.MemberEnd(); ++it) {
            auto& v = *it;

            if (v.name == "ents") {
                if (v.value.GetType() != core::json::Type::kArrayType) {
                    X_ERROR("Ents", "Ent data must be array");
                    return false;
                }

                auto ents = v.value.GetArray();
                for (auto& ent : ents) {
                    if (!parseEntDesc(ent)) {
                        X_ERROR("Ents", "Unexpected type");
                        return false;
                    }
                }
            }
            else {
                X_ERROR("Ents", "Unknown ent data member: \"%s\"", v.name.GetString());
                return false;
            }
        }

        return true;
    }

    bool EnititySystem::postLoad(void)
    {
        if (!physSys_.createColliders(reg_, pPhysics_, pPhysScene_)) {
            return false;
        }

        return true;
    }

    template<typename CompnentT>
    bool EnititySystem::parseComponent(DataTranslator<CompnentT>& translator, CompnentT& comp, const core::json::Value& compDesc)
    {
        if (compDesc.GetType() != core::json::Type::kObjectType) {
            X_ERROR("Ents", "Component description must be a object");
            return false;
        }

        for (core::json::Value::ConstMemberIterator it = compDesc.MemberBegin(); it != compDesc.MemberEnd(); ++it) {
            const auto& name = it->name;
            const auto& val = it->value;

            core::StrHash nameHash(name.GetString(), name.GetStringLength());

            switch (val.GetType()) {
                case core::json::Type::kNumberType:

                    if (val.IsInt()) {
                        translator.assignInt(comp, nameHash, val.GetInt());
                    }
                    else if (val.IsBool()) {
                        translator.assignBool(comp, nameHash, val.GetBool());
                    }
                    else if (val.IsFloat()) {
                        translator.assignFloat(comp, nameHash, val.GetFloat());
                    }
                    else {
                        X_ERROR("Ent", "Unknown component numeric type: %" PRIi32, val.GetType());
                        return false;
                    }

                    break;
                case core::json::Type::kStringType:
                    translator.assignString(comp, nameHash, val.GetString(), val.GetStringLength());
                    break;

                case core::json::Type::kObjectType: {
                    if (!val.HasMember("x") || !val.HasMember("y") || !val.HasMember("z")) {
                        X_ERROR("Ents", "Invalid vec3");
                        return false;
                    }

                    Vec3f vec(
                        val["x"].GetFloat(),
                        val["y"].GetFloat(),
                        val["z"].GetFloat());

                    translator.assignVec3(comp, nameHash, vec);
                } break;

                case core::json::Type::kTrueType:
                    translator.assignBool(comp, nameHash, true);
                    break;
                case core::json::Type::kFalseType:
                    translator.assignBool(comp, nameHash, false);
                    break;

                default:
                    X_ERROR("Ent", "Unknown component type: %" PRIi32, val.GetType());
                    return false;
            }
        }

        return true;
    }

    bool EnititySystem::parseEntDesc(core::json::Value& entDesc)
    {
        if (entDesc.GetType() != core::json::Type::kObjectType) {
            X_ERROR("Ents", "Ent description must be a object");
            return false;
        }

        EntityId ent = reg_.create<TransForm>();
        auto& trans = reg_.get<TransForm>(ent);

        for (auto it = entDesc.MemberBegin(); it != entDesc.MemberEnd(); ++it) {
            const auto& name = it->name;
            const auto& value = it->value;

            switch (core::Hash::Fnv1aHash(name.GetString(), name.GetStringLength())) {
                case "Health"_fnv1a: {
                    auto& hp = reg_.assign<Health>(ent);
                    if (!parseComponent(dtHealth_, hp, value)) {
                        return false;
                    }
                    break;
                }
                case "SoundObject"_fnv1a: {
                    auto& snd = reg_.assign<SoundObject>(ent);
                    if (!parseComponent(dtSoundObj_, snd, value)) {
                        return false;
                    }

                    auto sndTrans = trans;
                    sndTrans.pos += snd.offset;

#if X_SOUND_ENABLE_DEBUG_NAMES
                    if (reg_.has<EntName>(ent)) {
                        auto& entName = reg_.get<EntName>(ent);

                        snd.handle = gEnv->pSound->registerObject(sndTrans, entName.name.c_str());
                    }
                    else
#endif // !X_SOUND_ENABLE_DEBUG_NAMES
                    {
                        snd.handle = gEnv->pSound->registerObject(sndTrans);
                    }

#if 1
                    if (snd.occType != sound::OcclusionType::None) {
                        gEnv->pSound->setOcclusionType(snd.handle, snd.occType);
                    }
#else
                    if (snd.occType.isNotEmpty()) {
                        sound::OcclusionType::Enum occ = sound::OcclusionType::None;

                        switch (core::Hash::Fnv1aHash(snd.occType.c_str(), snd.occType.length())) {
                            case "None"_fnv1a:
                                occ = sound::OcclusionType::None;
                                break;
                            case "SingleRay"_fnv1a:
                                occ = sound::OcclusionType::SingleRay;
                                break;
                            case "MultiRay"_fnv1a:
                                occ = sound::OcclusionType::MultiRay;
                                break;
                            default:
                                X_ERROR("Ent", "Invalid occlusion type: \"%s\"", snd.occType.c_str());
                                break;
                        }

                        if (occ != sound::OcclusionType::None) {
                            gEnv->pSound->setOcclusionType(snd.handle, occ);
                        }
                    }
#endif

                    break;
                }

                case "MeshRenderer"_fnv1a: {
                    if (!reg_.has<Mesh>(ent)) {
                        X_ERROR("Ents", "MeshRenderer requires a Mesh component");
                        return false;
                    }

                    auto& mesh = reg_.get<Mesh>(ent);
                    auto& meshRend = reg_.assign<MeshRenderer>(ent);

                    engine::RenderEntDesc entDsc;
                    entDsc.pModel = mesh.pModel;
                    entDsc.trans = trans;

                    meshRend.pRenderEnt = p3DWorld_->addRenderEnt(entDsc);
                    break;
                }

                case "MeshCollider"_fnv1a: {
                    if (!reg_.has<Mesh>(ent)) {
                        X_ERROR("Ents", "MeshCollider requires a Mesh component");
                        return false;
                    }

                    // we just assign the component
                    // then later before we finished loading we iterate these waiting for the models to
                    // finish loading and then create the physics.
                    reg_.assign<MeshCollider>(ent);
                    break;
                }

                case "DynamicObject"_fnv1a: {
                    if (!reg_.has<Mesh>(ent)) {
                        X_ERROR("Ents", "MeshCollider requires a Mesh component");
                        return false;
                    }

                    auto& dyn = reg_.assign<DynamicObject>(ent);

                    if (!parseComponent(dtDynamicObject_, dyn, value)) {
                        return false;
                    }

                    break;
                }

                case "Mesh"_fnv1a: {
                    auto& mesh = reg_.assign<Mesh>(ent);
                    if (!parseComponent(dtMesh_, mesh, value)) {
                        return false;
                    }
                    if (mesh.name.isEmpty()) {
                        X_ERROR("Ents", "Mesh has empty name");
                        return false;
                    }

                    mesh.pModel = pModelManager_->loadModel(mesh.name.c_str());

                    mesh.pModel->waitForLoad(pModelManager_);
                    break;
                }

                case "Animator"_fnv1a: {
                    if (!reg_.has<Mesh>(ent)) {
                        X_ERROR("Ents", "Animator requires a Mesh component");
                        return false;
                    }

                    auto& mesh = reg_.get<Mesh>(ent);

                    auto* pAnimator = X_NEW(anim::Animator, g_gameArena, "Animator")(mesh.pModel, g_gameArena);

                    reg_.assign<Animator>(ent, pAnimator);
                    break;
                }
                case "Rotator"_fnv1a: {
                    auto& rot = reg_.assign<Rotator>(ent);
                    if (!parseComponent(dtRotator_, rot, value)) {
                        return false;
                    }

                    break;
                }
                case "Mover"_fnv1a: {
                    auto& mov = reg_.assign<Mover>(ent);
                    if (!parseComponent(dtMover_, mov, value)) {
                        return false;
                    }

                    mov.fract = 0.f;

                    break;
                }

                case "Emitter"_fnv1a: {
                    auto& emit = reg_.assign<Emitter>(ent);
                    if (!parseComponent(dtEmitter_, emit, value)) {
                        return false;
                    }


                    // we need to create a emmiter on the world.
                    // to play our fx.
                    engine::fx::Effect* pEffect = nullptr;
                    
                    if (emit.effect.isNotEmpty())
                    {
                        pEffect = pEffectManager_->loadEffect(emit.effect);
                    }

                    engine::EmitterDesc dsc;
                    dsc.trans = trans;
                    dsc.trans.pos += emit.offset;
                    dsc.pEffect = pEffect;
                    dsc.looping = true;

                    emit.pEmitter = p3DWorld_->addEmmiter(dsc);
                    break;
                }

                case "NetworkSync"_fnv1a: {
                    reg_.assign<NetworkSync>(ent);
                    break;
                }

                // loosy goosy
                case "origin"_fnv1a: {
                    // x,y,z
                    if (!value.HasMember("x") || !value.HasMember("y") || !value.HasMember("z")) {
                        X_ERROR("Ents", "Invalid origin");
                        return false;
                    }

                    trans.pos.x = value["x"].GetFloat();
                    trans.pos.y = value["y"].GetFloat();
                    trans.pos.z = value["z"].GetFloat();
                } break;
                case "angles"_fnv1a: {
                    Anglesf angles;
                    if (!value.HasMember("p") || !value.HasMember("y") || !value.HasMember("r")) {
                        X_ERROR("Ents", "Invalid angles");
                        return false;
                    }

                    // fuck you make me wet.
                    angles.setPitch(value["p"].GetFloat());
                    angles.setYaw(value["y"].GetFloat());
                    angles.setRoll(value["r"].GetFloat());

                    trans.quat = angles.toQuat();
                } break;

                case "name"_fnv1a: {
                    auto& entName = reg_.assign<EntName>(ent);
                    if (value.GetType() != core::json::Type::kStringType) {
                        X_ERROR("Ents", "Invalid name");
                        return false;
                    }

                    entName.name = core::string(value.GetString(), value.GetStringLength());
                } break;

                default:
                    X_WARNING("Ent", "Unknown ent member: \"%s\"", name.GetString());
                    break;
            }
        }

        return true;
    }

} // namespace entity

X_NAMESPACE_END
