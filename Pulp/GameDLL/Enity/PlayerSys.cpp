#include "stdafx.h"
#include "PlayerSys.h"
#include "Vars\PlayerVars.h"
#include "Weapon\WeaponManager.h"

#include <IFrameData.h>
#include <ITimer.h>
#include <IWorld3D.h>
#include <I3DEngine.h>
#include <IModelManager.h>
#include <INetwork.h>

#include <Hashing\Fnva1Hash.h>


using namespace core::Hash::Literals;
using namespace sound::Literals;

X_NAMESPACE_BEGIN(game)

namespace entity
{
    namespace
    {
        float angleNormalize360(float angle)
        {
            if ((angle >= 360.0f) || (angle < 0.0f)) {
                angle -= floorf(angle * (1.0f / 360.0f)) * 360.0f;
            }
            return angle;
        }

        float angleNormalize180(float angle)
        {
            angle = angleNormalize360(angle);
            if (angle > 180.0f) {
                angle -= 360.0f;
            }
            return angle;
        }

    } // namespace

    PlayerSystem::PlayerSystem(PlayerVars& playerVars) :
        vars_(playerVars),
        pPhysScene_(nullptr)
    {

    }

    bool PlayerSystem::init(physics::IScene* pPhysScene)
    {
        pPhysScene_ = pPhysScene;
        return true;
    }

    void PlayerSystem::update(core::FrameTimeData& timeInfo, ECS& reg)
    {
        X_UNUSED(timeInfo);
        
        auto view = reg.view<Player, TransForm>();
        for (auto playerId : view) {

            if (vars_.drawPosInfo_) {

                auto& trans = reg.get<TransForm>(playerId);
                auto& player = reg.get<Player>(playerId);

                auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::GUI);

                core::StackString256 dbgTxt;
                dbgTxt.appendFmt("pos (%.2f,%.2f,%.2f)\nyaw %.2f pitch: %.2f roll: %.2f",
                    trans.pos.x, trans.pos.y, trans.pos.z,
                    player.viewAngles.yaw(), player.viewAngles.pitch(), player.viewAngles.roll());

                font::TextDrawContext con;
                con.col = Col_Mintcream;
                con.size = Vec2f(24.f, 24.f);
                con.effectId = 0;
                con.pFont = gEnv->pFontSys->getDefault();
                pPrim->drawText(Vec3f(10.f, 200.f + (playerId * 50.f), 0.8f), con, dbgTxt.begin(), dbgTxt.end());
            }
        }

    }

    void PlayerSystem::runUserCmdForPlayer(core::TimeVal dt, ECS& reg,
        weapon::WeaponDefManager& weaponDefs, model::IModelManager* pModelManager,
        engine::IWorld3D* p3DWorld, const net::UserCmd& userCmd, EntityId playerId)
    {
        X_ASSERT(playerId < net::MAX_PLAYERS, "Invalid player id")(playerId, net::MAX_PLAYERS);
        X_ASSERT(reg.has<Player>(playerId), "Not a valid player")(playerId);

        X_UNUSED(p3DWorld);

        auto trans = reg.get<TransForm>(playerId); // copy
        auto& player = reg.get<Player>(playerId);
        auto& state = player.state;

        player.oldUserCmd = player.userCmd;
        player.userCmd = userCmd;

        // re-palying a old command?
//        bool userCmdReplay = userCmd.flags.IsSet(net::UserCmdFlag::REPLAY);
        bool crouched = state.IsSet(Player::State::Crouch);
        bool enterCrouch = userCmd.buttons.IsSet(net::Button::CROUCH) && !crouched;
        bool leaveCrouch = !userCmd.buttons.IsSet(net::Button::CROUCH) && crouched;

        if (enterCrouch) {
            state.Set(Player::State::Crouch);
            crouched = true;

            gEnv->pSound->postEvent(force_hash<"player_brag"_soundId>(), sound::GLOBAL_OBJECT_ID);
        }
        else if (leaveCrouch) {
            state.Remove(Player::State::Crouch);
            crouched = false;
        }

        // jump
        if (userCmd.buttons.IsSet(net::Button::JUMP) && !state.IsSet(Player::State::Jump)) {
            state.Set(Player::State::Jump);
            player.jumpTime = core::TimeVal(0ll);

            gEnv->pSound->postEvent(force_hash<"player_jump"_soundId>(), sound::GLOBAL_OBJECT_ID);
        }

        // update the view angles.
        {
            player.cmdAngles = userCmd.angles;

            for (int32_t i = 0; i < 3; i++) {
                player.viewAngles[i] = angleNormalize180(userCmd.angles[i] + player.deltaViewAngles[i]);
            }

            // clamp.
            float pitch = math<float>::clamp(player.viewAngles.pitch(), vars_.minViewPitch_, vars_.maxViewPitch_);
            player.viewAngles.setPitch(pitch);

            // calculate delta.
            Anglesf delta;
            for (int i = 0; i < 3; i++) {
                delta[i] = player.viewAngles[i] - userCmd.angles[i];
            }

            player.deltaViewAngles = delta;
        }

        if (reg.has<CharacterController>(playerId)) {
            auto& con = reg.get<CharacterController>(playerId);
            const float timeDelta = dt.GetSeconds();
            const float gravity = -100.f;

            float speed = vars_.walkSpeed_;

            if (state.IsSet(Player::State::Crouch)) {
                speed = vars_.crouchSpeed_;
            }
            else if (userCmd.buttons.IsSet(net::Button::RUN)) {
                // TODO: blend in stamina here.
                speed = vars_.runSpeed_;
            }

            Vec3f displacement;

            if (userCmd.moveForwrd != 0) {
                displacement.y += userCmd.moveForwrd * timeDelta * speed;
            }
            if (userCmd.moveRight != 0) {
                displacement.x += userCmd.moveRight * timeDelta * speed;
            }

            if (userCmd.moveForwrd || userCmd.moveRight) {
                static int frames = 0;

                // HACK: untill have animations playing and can que it from notrtracks in anim.
                ++frames;
                if (frames == 15) {
                    frames = 0;
                    gEnv->pSound->postEvent(force_hash<"player_footstep"_soundId>(), sound::GLOBAL_OBJECT_ID);
                }
            }

            auto rotation = player.viewAngles.toMat3(); //  player.firstPersonViewAxis;

            displacement = rotation * displacement;

            // jumping?
            float jumpHeight = 0.f;

            if (state.IsSet(Player::State::Jump)) {
                player.jumpTime += dt;

                const float gJumpGravity = -vars_.jumpHeight_; // -50.f;
                const float jumpTime = player.jumpTime.GetSeconds();
                jumpHeight = gJumpGravity * jumpTime * jumpTime + 30.f * jumpTime;
            }

            auto upDisp = Vec3f::zAxis();

            if (jumpHeight != 0.f) {
                upDisp *= jumpHeight;
            }
            else {
                upDisp *= gravity * timeDelta;
            }

            displacement += upDisp;

            // move the controller.
            {
                physics::ScopedLock lock(pPhysScene_, physics::LockAccess::Write);

                if (enterCrouch) {
                    con.pController->resize(vars_.crouchHeight_);
                }
                else if (leaveCrouch) {
                    con.pController->resize(vars_.normalHeight_);
                }

                auto flags = con.pController->move(displacement, 0.f, timeDelta);
                if (flags.IsSet(physics::CharacterColFlag::DOWN)) {
                    state.Set(Player::State::OnGround);

                    // TEMP: don't allow another jump untill atleast 0.2 seconds have passed.
                    if (player.jumpTime > core::TimeVal(0.2f)) {
                        state.Remove(Player::State::Jump);
                    }
                }
                else {
                    state.Remove(Player::State::OnGround);
                }
            }

            trans.pos = con.pController->getFootPosition();
        }

        updateViewBob(dt, player);

        // who you looking at?
        updateEye(player);
        // calculate first person view.
        calculateFirstPersonView(trans, player);

        // update trans?
        {
            auto& curTrans = reg.get<TransForm>(playerId); // copy
            if (trans != curTrans)
            {
                curTrans = trans;
                reg.dispatch<MsgMove>(playerId);
            }
        }

        if(reg.has<MeshRenderer>(playerId))
        {
            auto& rend = reg.get<MeshRenderer>(playerId);
            p3DWorld->updateRenderEnt(rend.pRenderEnt, trans);
        }

        if (player.weaponEnt != entity::INVALID_ENT_ID) {
            auto& wpn = reg.get<Weapon>(player.weaponEnt);

            // oh shit son.
            // this be like, some crazy weapon switching shit.
            if (userCmd.impulse == net::Impulse::WEAP_NEXT || userCmd.impulse == net::Impulse::WEAP_PREV) {
                X_LOG0("Player", "Change the fucking weapon!");

                // how is this going to work o_o
                // need to holster the weapon, then switch to new one.
                // ugh.
                
                // first find a weapon we can swith to.
                auto& inv = reg.get<Inventory>(playerId);
                auto wpnIdx = player.targetWpn;

                while (1)
                {
                    if (userCmd.impulse == net::Impulse::WEAP_NEXT)
                    {
                        ++wpnIdx;

                        if (wpnIdx >= weapon::WEAPON_MAX_LOADED) {
                            wpnIdx = 0;
                        }
                    }
                    else if (userCmd.impulse == net::Impulse::WEAP_PREV)
                    {
                        --wpnIdx;

                        if (wpnIdx < 0) {
                            wpnIdx = weapon::WEAPON_MAX_LOADED - 1;
                        }
                    }
                    else
                    {
                        X_ASSERT_UNREACHABLE();
                    }

                    // looped?
                    if (wpnIdx == player.targetWpn) {
                        break;
                    }

                    if (!inv.weapons.test(wpnIdx)) {
                        continue;
                    }

                    // gimmy the fooking weapon!
                    auto* pWpnDef = weaponDefs.findWeaponDef(wpnIdx);
                    if (!pWpnDef) {
                        continue;
                    }

                    auto ammoType = pWpnDef->getAmmoTypeId();

                    if (inv.numAmmo(ammoType) > 0 || inv.getClipAmmo(wpnIdx) > 0) {
                        break;
                    }
                }

                if (wpnIdx != player.currentWpn && wpnIdx != player.targetWpn) {
                    player.targetWpn = wpnIdx;

                    auto* pWpnDef = weaponDefs.findWeaponDef(wpnIdx);
                    X_LOG0("Player", "switch to: %s", pWpnDef->getStrSlot(weapon::StringSlot::DisplayName));
                }
            }

            if (userCmd.buttons.IsSet(net::Button::ATTACK)) {
                wpn.attack = true;
            }
            else if (player.oldUserCmd.buttons.IsSet(net::Button::ATTACK)) {
                wpn.attack = false;
            }

            if (player.oldUserCmd.buttons.IsSet(net::Button::RELOAD)) {
                wpn.reload = true;
            }

            if (player.currentWpn != player.targetWpn)
            {
                if (wpn.isReady()) {
                    wpn.holster = true;
                }

                if (wpn.isHolstered())
                {
                    auto& inv = reg.get<Inventory>(playerId);
                    auto& mesh = reg.get<Mesh>(player.weaponEnt);
                    auto& meshRend = reg.get<MeshRenderer>(player.weaponEnt);
                    auto& an = reg.get<Animator>(player.weaponEnt);

                    // save some info about current weapon.
                    {

                        inv.setClipAmmo(player.currentWpn, wpn.ammoInClip);
                    }

                    // now switch and raise.
                    // how do we switch 0_0
                    auto* pWpnDef = weaponDefs.findWeaponDef(player.targetWpn);
                    const auto ammotTypeId = pWpnDef->getAmmoTypeId();

                    X_ASSERT(pWpnDef->getID() == player.targetWpn, "Id mismatch")(pWpnDef->getID(), player.targetWpn);

                    // change the model
                    const char* pViewModel = pWpnDef->getModelSlot(weapon::ModelSlot::Gun);

                    mesh.pModel = pModelManager->loadModel(pViewModel);
                    pModelManager->waitForLoad(mesh.pModel);

                    if (meshRend.pRenderEnt) {
                        p3DWorld->freeRenderEnt(meshRend.pRenderEnt);
                    }

                    engine::RenderEntDesc entDsc;
                    entDsc.pModel = mesh.pModel;
                    entDsc.trans = trans;
                    meshRend.pRenderEnt = p3DWorld->addRenderEnt(entDsc);
     
                    an.pAnimator->setModel(mesh.pModel);


                    auto clipAmmo = inv.getClipAmmo(player.targetWpn);

                    wpn.ammoInClip = clipAmmo;
                    wpn.ammoType = ammotTypeId;
                    wpn.pWeaponDef = pWpnDef;
                    wpn.raise();

                    player.currentWpn = player.targetWpn;
                }
            }
            
        }

        // if we have arms, shove them where we want the gun pos, as gun animations
        // are positioned based on arms.
        // TODO: use `tag_view` in arms?
        if (player.armsEnt != entity::INVALID_ENT_ID) {
            auto& armsTrans = reg.get<TransForm>(player.armsEnt);

            const Vec3f& viewOrigin = player.firstPersonViewOrigin;
            Matrix33f viewAxis = player.firstPersonViewAxis;

            viewAxis.rotate(Vec3f::zAxis(), ::toRadians(90.f));

            const Vec3f gunPos = vars_.gunOffset_;
            Vec3f origin = viewOrigin + (viewAxis * gunPos);

            const Quatf viewAxisQ(viewAxis);
            if (armsTrans.pos != origin || armsTrans.quat != viewAxisQ)
            {
                armsTrans.quat = viewAxisQ;
                armsTrans.pos = origin;
                reg.dispatch<MsgMove>(player.armsEnt);
            }
        }
    }

    void PlayerSystem::updateViewBob(core::TimeVal dt, Player& player)
    {
        auto& userCmd = player.userCmd;

        Vec3f gravity = Vec3f(0.f, 0.f, -1.f);

        Vec3f velocity = Vec3f(0.f, 0.f, 0.f);
        //	velocity = Vec3f(0.f, 20.f, 0.f);

        Vec3f vel = velocity - (velocity * gravity) * gravity;
        float xyspeed = vel.length();

        if (!userCmd.moveForwrd && !userCmd.moveRight || (xyspeed < 5.f)) {
            // reset.
            player.bobCycle = 0;
            player.bobfracsin = 0;
        }
        else {
            float bobmove;

            if (player.state.IsSet(Player::State::Crouch)) {
                bobmove = vars_.crouchBob_;
            }
            else {
                bobmove = vars_.walkBob_;
            }

            auto delta = dt.GetMilliSeconds();
            auto old = player.bobCycle;
            player.bobCycle = (int32_t)(old + bobmove * delta) & 255;
            player.bobfracsin = math<float>::abs(math<float>::sin((player.bobCycle & 127) / 127.f * math<float>::PI));
        }

        player.viewBobAngles = Anglesf::zero();
        player.viewBob = Vec3f::zero();

        auto viewaxis = player.viewAngles.toMat3();

        // add angles based on velocity
        float delta = velocity.dot(viewaxis.getColumn(0));
        player.viewBobAngles[Anglesf::Rotation::PITCH] += delta * vars_.bobRunPitch_;

        delta = velocity.dot(viewaxis.getColumn(1));
        player.viewBobAngles[Anglesf::Rotation::ROLL] -= delta * vars_.bobRunRoll_;

        // add angles based on bob
        // make sure the bob is visible even at low speeds
        float speed = xyspeed > 200 ? xyspeed : 200;

        delta = player.bobfracsin * vars_.bobPitch_ * speed;
        if (player.state.IsSet(Player::State::Crouch)) {
            delta *= 3; // crouching
        }
        player.viewBobAngles[Anglesf::Rotation::PITCH] += delta;
        delta = player.bobfracsin * vars_.bobRoll_ * speed;
        if (player.state.IsSet(Player::State::Crouch)) {
            delta *= 3; // crouching accentuates roll
        }
        player.viewBobAngles[Anglesf::Rotation::ROLL] += delta;

        auto bob = player.bobfracsin * xyspeed * vars_.bobUp_;
        if (bob > 6) {
            bob = 6;
        }

        // up down!
        player.viewBob[2] += bob;
    }

    void PlayerSystem::updateEye(Player& player)
    {
        Vec3f newEye;

        if (player.state.IsSet(Player::State::Crouch)) {
            newEye.z = vars_.crouchViewHeight_;
        }
        else {
            newEye.z = vars_.normalViewHeight_;
        }

        if (newEye != player.eyeOffset) {
            player.eyeOffset = player.eyeOffset * vars_.crouchRate_ + newEye * (1.0f - vars_.crouchRate_);
        }
    }

    void PlayerSystem::calculateFirstPersonView(TransForm& trans, Player& player)
    {
        auto eyePosition = trans.pos + player.eyeOffset + player.viewBob;
        auto angles = player.viewAngles + player.viewBobAngles;

        auto axis = angles.toMat3();
        auto origin = eyePosition;

        player.firstPersonViewAxis = axis;
        player.firstPersonViewOrigin = origin;

        // this seams wrong should be yaw not roll o.o?
        // TODO: these is some issue with the quat to mat, and matrix to quat logic.
        // BROKEN AS FUCK.
        trans.quat = Quatf(Vec3f(0, 0, 1), Quatf(axis).getRoll() + ::toRadians(90.f));
    }

} // namespace entity

X_NAMESPACE_END