#include "stdafx.h"
#include "Game.h"

#include <IRender.h>
#include <IConsole.h>
#include <IFrameData.h>

#include <Math\XMatrixAlgo.h>
#include <Containers\FixedFifo.h>
#include <Hashing\Fnva1Hash.h>

// TMP
#include <I3DEngine.h>
#include <IPrimitiveContext.h>
#include <SnapShot.h>

#include <IGui.h>

X_NAMESPACE_BEGIN(game)

using namespace core::string_view_literals;

XGame::XGame(ICore* pCore) :
    arena_(g_gameArena),
    pCore_(pCore),
    pTimer_(nullptr),
    pPeer_(nullptr),
    pSession_(nullptr),
    pRender_(nullptr),
    prevStatus_(net::SessionStatus::Idle),
    world_(arena_),
    userCmdGen_(inputVars_),
    weaponDefs_(arena_),
    pMenuHandler_(nullptr)
{
    lastUserCmdRunTime_.fill(0);
    lastUserCmdRunOnClientTime_.fill(0);
    lastUserCmdRunOnServerTime_.fill(0);

    X_ASSERT_NOT_NULL(pCore);
}

XGame::~XGame()
{
}

void XGame::registerVars(void)
{
    weaponDefs_.registerVars();
    vars_.registerVars();
    inputVars_.registerVars();
}

void XGame::registerCmds(void)
{
    weaponDefs_.registerCmds();

    ADD_COMMAND_MEMBER("map", this, XGame, &XGame::Command_Map, core::VarFlag::SYSTEM, "Loads a map");
    ADD_COMMAND_MEMBER("mainMenu", this, XGame, &XGame::Command_MainMenu, core::VarFlag::SYSTEM, "Return to main menu");

    ADD_COMMAND_MEMBER("uiOpenMenu", this, XGame, &XGame::Cmd_OpenMenu, core::VarFlags::SYSTEM, "Open menu");
    
    ADD_COMMAND_MEMBER("chat", this, XGame, &XGame::Cmd_Chat, core::VarFlags::SYSTEM | core::VarFlags::SINGLE_ARG, "Chat");

}

// ---------------------------------

bool XGame::init(void)
{
    X_LOG0("Game", "init");
    X_ASSERT_NOT_NULL(gEnv->pInput);
    X_ASSERT_NOT_NULL(gEnv->pTimer);
    X_ASSERT_NOT_NULL(gEnv->pRender);
    X_ASSERT_NOT_NULL(gEnv->pNet);
    X_ASSERT_NOT_NULL(gEnv->pPhysics);

    auto* pPhysics = gEnv->pPhysics;

    // init physics.
    // this don't create a scene, that's done later..
    physics::ToleranceScale scale;
    scale.length = physics::SCALE_LENGTH;
    scale.mass = physics::SCALE_MASS;
    scale.speed = physics::SCALE_SPEED;

    if (!pPhysics->init(scale)) {
        X_ERROR("3DEngine", "Failed to setup physics scene");
        return false;
    }

    // setup groups collision filters.
    pPhysics->setGroupCollisionFlag(physics::GroupFlag::AiClip, physics::GroupFlag::Player, false);
    pPhysics->setGroupCollisionFlag(physics::GroupFlag::AiClip, physics::GroupFlag::Ai, true);
    pPhysics->setGroupCollisionFlag(physics::GroupFlag::AiClip, physics::GroupFlag::Vehicle, false);
    pPhysics->setGroupCollisionFlag(physics::GroupFlag::PlayerClip, physics::GroupFlag::Player, false);
    pPhysics->setGroupCollisionFlag(physics::GroupFlag::PlayerClip, physics::GroupFlag::Ai, true);
    pPhysics->setGroupCollisionFlag(physics::GroupFlag::PlayerClip, physics::GroupFlag::Vehicle, false);
    pPhysics->setGroupCollisionFlag(physics::GroupFlag::VehicleClip, physics::GroupFlag::Player, false);
    pPhysics->setGroupCollisionFlag(physics::GroupFlag::VehicleClip, physics::GroupFlag::Ai, false);
    pPhysics->setGroupCollisionFlag(physics::GroupFlag::VehicleClip, physics::GroupFlag::Vehicle, true);


    pTimer_ = gEnv->pTimer;
    pRender_ = gEnv->pRender;

    auto* pMenuMan = gEnv->p3DEngine->getMenuManager();

    pMenuHandler_ = pMenuMan->createMenuHandler();
    pMenuHandler_->openMenu("main"_sv);

    // networking.
    {
        auto* pNet = gEnv->pNet;
        pPeer_ = pNet->createPeer();

        userNetMap_.myGuid = pPeer_->getMyGUID();

        // So I would like to only open the port if we actually want to talk to another pickle.
        // also we should just use default port, but that's a net var :(
        // sounds like the game should not even be dealing with this.
        // The game should just have a session and session will open ports based on what the game asked the session todo.
        // Think this will also mean session should manage own peer instance.
        net::Port basePort = 1337;
        net::Port maxPort = basePort + 10;

        net::SocketDescriptor sd(basePort);
        auto res = pPeer_->init(4, sd);

        while (res == net::StartupResult::SocketPortInUse && sd.getPort() <= maxPort) {
            sd.setPort(sd.getPort() + 1);
            res = pPeer_->init(4, sd);
        }

        if (res != net::StartupResult::Started) {
            X_ERROR("Game", "Failed to setup networking: \"%s\"", net::StartupResult::ToString(res));
            return false;
        }

        pPeer_->setMaximumIncomingConnections(4);
        X_LOG0("Game", "Listening on port ^6%" PRIu16, sd.getPort());

        pSession_ = pNet->createSession(pPeer_, this);
        if (!pSession_) {
            X_ERROR("Game", "Failed to create net session");
            return false;
        }

    }

    auto deimension = gEnv->pRender->getDisplayRes();

    X_ASSERT(deimension.x > 0, "height is not valid")(deimension.x);
    X_ASSERT(deimension.y > 0, "height is not valid")(deimension.y);

    userCmdGen_.init();
    weaponDefs_.init();

    return true;
}

bool XGame::shutDown(void)
{
    X_LOG0("Game", "Shutting Down");

    if (pSession_) {
        gEnv->pNet->deleteSession(pSession_);
    }

    if (pPeer_) {
        gEnv->pNet->deletePeer(pPeer_);
    }

    if (world_) {
        world_.reset();
    }

    if (pMenuHandler_) {
        auto* pMenuMan = gEnv->p3DEngine->getMenuManager();

        pMenuMan->releaseMenuHandler(pMenuHandler_);
    }

    userCmdGen_.shutdown();
    weaponDefs_.shutDown();
    return true;
}

void XGame::release(void)
{
    X_DELETE(this, g_gameArena);
}

bool XGame::asyncInitFinalize(void)
{
    bool allOk = true;

    allOk &= weaponDefs_.asyncInitFinalize();

    return allOk;
}

bool XGame::onInputEvent(const input::InputEvent& event)
{
    // okay!
    // so we decide where to send the input here.
    auto status = pSession_->getStatus();

    if (event.action == input::InputState::RELEASED)
    {
        if (event.keyId == input::KeyId::ESCAPE)
        {
            if (pMenuHandler_)
            {
                if (status == net::SessionStatus::InGame)
                {
                    if (!pMenuHandler_->isActive())
                    {
                        pMenuHandler_->openMenu("pause"_sv);
                        return true;
                    }
                    else
                    {
                        if (pMenuHandler_->back(true)) {
                            return true;
                        }
                    }
                }
                else if (status == net::SessionStatus::Idle)
                {
                    if (pMenuHandler_->back(false)) {
                        return true;
                    }
                }
            }
        }
    }
  

    if (userCmdGen_.onInputEvent(event)) {
        return true;
    }

    return false;
}

bool XGame::update(core::FrameData& frame)
{
    X_PROFILE_BEGIN("Update", core::profiler::SubSys::GAME);
    ttZone(gEnv->ctx, "(Game) Update");

    X_UNUSED(frame);
    // how todo this camera move shit.
    // when the input frames are been called
    // the frame data has valid times.
    // we just don't have the data in the input callback.

    // the real issue is that input callbacks are global events, when this update
    // is a data based call.
    // but i have all the input events in this
    // but they are no use since i don't know if i'm allowed to use them all.
    // i like the input sinks tho
    // as things are registered with priority
    // and each devices gets input events it's allowed to use.
    // the problem is this data not linked to framedata
    // so

    // orth
    Matrix44f orthoProj;
    MatrixOrthoOffCenterRH(
        &orthoProj,
        0.f,
        static_cast<float>(frame.view.displayRes.x),
        static_cast<float>(frame.view.displayRes.y),
        0.f,
        -1e10f,
        1e10
    );

    frame.view.viewMatrixOrtho = Matrix44f::identity();
    frame.view.projMatrixOrtho = orthoProj;
    frame.view.viewProjMatrixOrth = orthoProj * frame.view.viewMatrixOrtho;

    auto width = static_cast<float>(frame.view.displayRes.x);
    auto height = static_cast<float>(frame.view.displayRes.y);

    Vec2f center(width * 0.5f, height * 0.5f);

    pSession_->update();
    pSession_->getSessionInfo(sessionInfo_);

    auto status = sessionInfo_.status;

    auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::GUI);

    bool blockUserCmd = drawMenu(frame, pPrim);

    // this is built all the time currently even if it's not used.
    // since input events are passed to it all the time.
    userCmdGen_.buildUserCmd(blockUserCmd);

    if (status == net::SessionStatus::Idle)
    {
        // main menu :D

        if (prevStatus_ != net::SessionStatus::Idle)
        {
            clearWorld();

            pMenuHandler_->openMenu("main"_sv);

        }
    }
    else if (status == net::SessionStatus::Loading)
    {
        // loading the map :(
        // i need a better way to know this is first call to loading.
        // rather than relying on world.
        // guess could store last state.
        // if not that, num frames since state change?
        if (prevStatus_ != net::SessionStatus::Loading)
        {
            // first frame in loading.
            pMenuHandler_->openMenu("loading"_sv);

            clearWorld();

            // TODO: only create this if we needed it.
            // I create this when loading as the server will send us state for this before we finish loading.
            pMultiplayerGame_ = core::makeUnique<Multiplayer>(arena_, vars_, userNetMap_, pSession_);
        }

        if (!world_)
        {
            auto matchParams = pSession_->getMatchParams();

            if (matchParams.mapName.isEmpty()) {
                X_ERROR("Game", "Match map name is empty");
                pSession_->quitToMenu();
            }
            else
            {
                world_ = core::makeUnique<World>(arena_, vars_, sessionInfo_, gEnv->pPhysics, weaponDefs_, pMultiplayerGame_.get(), arena_);

                if (!world_->loadMap(matchParams.mapName)) {
                    X_ERROR("Game", "Failed to load map");
                    // TODO: quit to menu?
                }
            }
        }

        // so i need to support been loaded, yet still in the loading state, while we wait for peers.
        // to finish loading, this is so the loading screen is still showing and can show pleb progress.
        // where to store this state?
        if (world_)
        {
            if (world_->hasLoaded() && !pSession_->hasFinishedLoading())
            {
                // spawn stuff like players!
                syncLobbyUsers();

                pSession_->finishedLoading();
            }
        }
    }
    else if (status == net::SessionStatus::InGame)
    {
        if (prevStatus_ != net::SessionStatus::InGame)
        {
            serverGameTimeMS_ = 0;
            gameTimeMS_ = 0;

            pMenuHandler_->close();

            userCmdGen_.clearForNewLevel();
        }

        X_ASSERT_NOT_NULL(world_.ptr());
        X_ASSERT_NOT_NULL(pMultiplayerGame_.ptr());

        syncLobbyUsers();

        auto localIdx = userNetMap_.localPlayerIdx;
        entity::EntityId localId = static_cast<entity::EntityId>(localIdx);

        const bool isHost = sessionInfo_.isHost;

        auto& userCmd = userCmdGen_.getCurrentUserCmd();
        {
            userCmd.clientGameTimeMS = gameTimeMS_;
            userCmd.serverGameTimeMS = serverGameTimeMS_;

            if (!isHost) {
                // don't go past snapshot.
                userCmd.serverGameTimeMS = core::Min(userCmd.serverGameTimeMS, netInterpolState_.snapShotEndMS);
            }

            userCmdMan_.addUserCmdForPlayer(localIdx, userCmd);
        }

        // both server and clients update this.
        // but clients get serverGameTimeMS_ set for them.
        gameTimeMS_ += frame.timeInfo.deltas[core::Timer::GAME].GetMilliSecondsAsInt32();

        if (isHost)
        {
            // run user cmds for all the valid players.
            for (int32_t i = 0; i < static_cast<int32_t>(userNetMap_.lobbyUserGuids.size()); i++)
            {
                if (!userNetMap_.lobbyUserGuids[i].isValid()) {
                    continue;
                }

                runUserCmdsForPlayer(frame, i);
            }   
        }
        else
        {
            // send user commands to server.
            pSession_->sendUserCmd(userCmdMan_, localIdx, frame.timeInfo);

            // this will set out interpolation time and maybe apply a new snapshot.
            pSession_->handleSnapShots(frame.timeInfo);

            runUserCmdsForPlayer(frame, localIdx);
        }

        world_->update(frame, userCmdMan_, netInterpolState_, localId);

        if (isHost) {

            // send snapshot after updating world.
            pSession_->sendSnapShot(frame.timeInfo);

            pMultiplayerGame_->update();
        }

        pMultiplayerGame_->drawChat(frame.timeInfo, pPrim);
        pMultiplayerGame_->drawEvents(frame.timeInfo, pPrim);

        // What's the point we all know stu will be at the top :(
        if (userCmd.buttons.IsSet(net::Button::SHOW_SCORES)) {
            pMultiplayerGame_->drawLeaderboard(pPrim);
        }
    }
    else if (status == net::SessionStatus::PartyLobby)
    {
        // party!! at stu's house.
        // open lobby menu?
        if (prevStatus_ != net::SessionStatus::PartyLobby)
        {
            pMenuHandler_->openMenu("lobby"_sv);
        }


    }
    else if (status == net::SessionStatus::GameLobby)
    {
        if (prevStatus_ != net::SessionStatus::GameLobby)
        {
            pMenuHandler_->openMenu("lobby"_sv);
        }


    }
    else if (status == net::SessionStatus::Connecting)
    {
        // ...
    }
    else
    {
        X_ERROR("Game", "Unhandled session status: %s", net::SessionStatus::ToString(status));
    }

    prevStatus_ = status;
    
    drawDebug(pPrim);

    return true;
}

void XGame::onUserCmdReceive(net::NetGUID guid, core::FixedBitStreamBase& bs)
{
    // we got user cmds o.o
    // what if i don't like you HEY!
    auto clientIdx = getPlayerIdxForGuid(guid);

    net::NetGuidStr buf;
    X_ASSERT(clientIdx >= 0, "Failed to get client index for guid \"%s\"", guid.toString(buf))(guid);

    userCmdMan_.readUserCmdFromBs(bs, clientIdx);
}

void XGame::buildSnapShot(net::SnapShot& snap)
{
    ttZone(gEnv->ctx, "(Game/Net) Build Snap");

    snap.setTime(gameTimeMS_);
    snap.setUserCmdTimes(lastUserCmdRunTime_);
    snap.setPlayerGuids(userNetMap_.lobbyUserGuids);

    {
        core::FixedBitStreamStack<512> bs;
        pMultiplayerGame_->writeToSnapShot(bs);

        snap.addObject(net::SnapShot::SNAP_MP_STATE, bs);
    }

    world_->createSnapShot(snap);
}

void XGame::applySnapShot(const net::SnapShot& snap)
{
    ttZone(gEnv->ctx, "(Game/Net) Apply Snap");

    // set the game state.
    {
        X_ASSERT_NOT_NULL(pMultiplayerGame_.ptr());

        net::SnapShot::MsgBitStream bs;
        if (snap.findObjectByID(net::SnapShot::SNAP_MP_STATE, bs))
        {
            pMultiplayerGame_->readFromSnapShot(bs);
        }
    }

    // get the games times the server has run
    lastUserCmdRunTime_ = snap.getUserCmdTimes();

    // logic in following scope must be done before applySnapShot is called.
    {
        userNetMap_.lobbyUserGuids = snap.getPlayerGuids();

        // Have the client work out it's localPlayerIdx.
        if (userNetMap_.localPlayerIdx < 0)
        {
            for (size_t i = 0; i < userNetMap_.lobbyUserGuids.size(); i++)
            {
                if (userNetMap_.myGuid == userNetMap_.lobbyUserGuids[i])
                {
                    userNetMap_.localPlayerIdx = safe_static_cast<int32_t>(i);
                    X_LOG0("Game", "Local player idx: %" PRIi32, userNetMap_.localPlayerIdx);
                    break;
                }
            }
        }
    }

    world_->applySnapShot(userNetMap_, snap);

    auto localLastRunTimeMS = lastUserCmdRunTime_[userNetMap_.localPlayerIdx];

    if (vars_.userCmdClientReplay())
    {
        net::UserCmdMan::UserCmdArr userCmds;
        userCmdMan_.getReadUserCmdsAfterGameTime(userNetMap_.localPlayerIdx, localLastRunTimeMS, userCmds);

        if (userCmds.isNotEmpty())
        {
            int32_t lastCmdMS = localLastRunTimeMS;

            X_LOG0_IF(vars_.userCmdDebug(), "Game", "Replaying %" PRIuS " userCmd(s) since: %" PRIi32, userCmds.size(), localLastRunTimeMS);

            // they are in newest to oldest order
            for (int32_t i = static_cast<int32_t>(userCmds.size()) - 1; i >= 0; i--)
            {
                auto& userCmd = userCmds[i];
                userCmd.flags.Set(net::UserCmdFlag::REPLAY);

                auto deltaMS = userCmd.clientGameTimeMS - lastCmdMS;
                X_ASSERT(deltaMS > 0, "Delta can't be less than 1")(deltaMS);
                auto dt = core::TimeVal::fromMS(deltaMS);

                runUserCmdForPlayer(dt, userCmd, userNetMap_.localPlayerIdx);

                lastCmdMS = userCmd.clientGameTimeMS;
            }
        }
    }
}

bool XGame::handlePacket(net::Packet* pPacket)
{
    X_UNUSED(pPacket);

    auto msg = pPacket->getID();

    switch (msg)
    {
        case net::MessageID::GameChatMsg:
            handleChatMsg(pPacket);
            break;
        case net::MessageID::GameEvent:
            handleGameEvent(pPacket);
            break;

        default:
            return false;
    }

    return true;
}

void XGame::handleChatMsg(net::Packet* pPacket)
{
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    char name[net::MAX_USERNAME_LEN] = {};
    char msg[net::MAX_CHAT_MSG_LEN] = {};

    auto nameLen = core::Min<uint32_t>(net::MAX_USERNAME_LEN, bs.read<uint8_t>());
    bs.read(name, nameLen);

    auto msgLen = core::Min<uint32_t>(net::MAX_CHAT_MSG_LEN, bs.read<uint8_t>());
    bs.read(msg, msgLen);

    X_LOG0("Game", "nameLen %" PRIu32 " msgLen: %" PRIu32, nameLen, msgLen);

    if (sessionInfo_.isHost) {
        pMultiplayerGame_->handleChatMsg(core::string_view(name, nameLen), core::string_view(msg, msgLen));
    }
    else {
        pMultiplayerGame_->addChatLine(core::string_view(name, nameLen), core::string_view(msg, msgLen));
    }
}

void XGame::handleGameEvent(net::Packet* pPacket)
{
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    pMultiplayerGame_->handleEvent(bs);
}

void XGame::setInterpolation(float fraction, int32_t serverGameTimeMS, int32_t ssStartTimeMS, int32_t ssEndTimeMS)
{
    X_ASSERT(!pSession_->isHost(), "Only clients should have interpolation set")();

    netInterpolState_.frac = fraction;
    netInterpolState_.serverGameTimeMS = serverGameTimeMS;
    netInterpolState_.snapShotStartMS = ssStartTimeMS;
    netInterpolState_.snapShotEndMS = ssEndTimeMS;
    serverGameTimeMS_ = serverGameTimeMS;
}

void XGame::runUserCmdsForPlayer(core::FrameData& frame, int32_t playerIdx)
{
    ttZone(gEnv->ctx, "(Game/Net) Run UserCmd");

    auto dt = frame.timeInfo.deltas[core::Timer::GAME];

    // if the player is local
    // we run a user command for them
    if (userNetMap_.localPlayerIdx == playerIdx) {
        auto unread = userCmdMan_.getNumUnreadFrames(playerIdx);
        // should only be one, unless we now support running multiple client frames.
        X_ASSERT(unread == 1, "More than one userCmd for local player")(unread);

        auto& userCmd = userCmdMan_.getUserCmdForPlayer(playerIdx);
        runUserCmdForPlayer(dt, userCmd, playerIdx);
    }
    else 
    {

        if (userCmdMan_.hasUnreadFrames(playerIdx))
        {
            // TODO: this kinda bugs out if the first userCmd is not starting at zero.
            // also timeSinceServerRanLastCmd could be huge if player joins late.
            auto nextCmdClientTimeMS = userCmdMan_.getNextUserCmdClientTimeMSForPlayer(playerIdx);
            auto clientGameTimedelta = nextCmdClientTimeMS - lastUserCmdRunOnClientTime_[playerIdx];
            auto timeSinceServerRanLastCmd = gameTimeMS_ - lastUserCmdRunOnServerTime_[playerIdx];

            int32_t clientTimeRunSoFar = 0;

            // if the client delta is less than server the client is running 'faster'
            // so if the dleta is negative client is running faster
            auto clientServerDelta = clientGameTimedelta - timeSinceServerRanLastCmd;

            X_LOG0_IF(vars_.userCmdDebug(), "Game", "Ply %" PRIi32 " UserCmd clientDelta: %" PRIi32 " serverDelta: %" PRIi32 " delta: %" PRIi32,
                playerIdx, clientGameTimedelta, timeSinceServerRanLastCmd, clientServerDelta);

            if (clientServerDelta <= 1_i32)
            {
                // the client might be running slighty faster than us as there delta is smaller the ours.
                // so process them till they match our delta.
                while (clientTimeRunSoFar < timeSinceServerRanLastCmd && userCmdMan_.hasUnreadFrames(playerIdx))
                {
                    auto& userCmd = userCmdMan_.getUserCmdForPlayer(playerIdx);
                    runUserCmdForPlayer(dt, userCmd, playerIdx);

                    lastUserCmdRunOnClientTime_[playerIdx] = userCmd.clientGameTimeMS;
                    lastUserCmdRunOnServerTime_[playerIdx] = gameTimeMS_;

                    clientTimeRunSoFar += clientGameTimedelta;

                    // log?

                    // update info.
                    if (userCmdMan_.hasUnreadFrames(playerIdx)) {
                        nextCmdClientTimeMS = userCmdMan_.getNextUserCmdClientTimeMSForPlayer(playerIdx);
                        clientGameTimedelta = nextCmdClientTimeMS - lastUserCmdRunOnClientTime_[playerIdx];
                    }
                }
            }
            else
            {
                X_WARNING("Game", "Client delta too large for remote player %" PRIi32 " running last userCmd", playerIdx);

                // the client is probs running slower than us, as it's delta is bigger than servers.
                // we want to just re run the last players command.
                auto userCmd = lastUserCmdRun_[playerIdx];
                runUserCmdForPlayer(dt, userCmd, playerIdx);
            }
        }
        else
        {
            X_WARNING("Game", "no userCmd for remote player %" PRIi32, playerIdx);
            // dam user not sending user commands run an empty command.
            auto userCmd = lastUserCmdRun_[playerIdx];
            runUserCmdForPlayer(dt, userCmd, playerIdx);

            lastUserCmdRunOnServerTime_[playerIdx] = gameTimeMS_;
        }
    }
}

void XGame::runUserCmdForPlayer(core::TimeVal dt, const net::UserCmd& userCmd, int32_t playerIdx)
{
    world_->runUserCmdForPlayer(dt, userCmd, playerIdx);

    lastUserCmdRun_[playerIdx] = userCmd;
    lastUserCmdRunTime_[playerIdx] = userCmd.clientGameTimeMS;
}

bool XGame::drawMenu(core::FrameData& frame, engine::IPrimitiveContext* pPrim)
{
    if (!pMenuHandler_) {
        return false;
    }

    if (!pMenuHandler_->isActive()) {
        return false;
    }

    engine::gui::MenuParams params;
    params.pSession = pSession_;
    pMenuHandler_->update(params, frame, pPrim);
    return true;
}

void XGame::drawDebug(engine::IPrimitiveContext* pPrim)
{
    Vec2f base(5.f, 50.f);

    {
        Vec2f size = pSession_->drawDebug(base, pPrim);
        base.y += size.y;
    }

    if (!vars_.drawSessionInfoDebug() && !vars_.userCmdDrawDebug() && !vars_.drawGameUserDebug()) {
        return;
    }

    font::TextDrawContext con;
    con.col = Col_Whitesmoke;
    con.size = Vec2f(16.f, 16.f);
    con.effectId = 0;
    con.pFont = gEnv->pFontSys->getDefault();
    con.flags.Clear();

    core::StackString512 txt;
    net::NetGuidStr guidStr;

    Vec3f pos(base.x, base.y, 1.f);

    if (vars_.drawSessionInfoDebug()) {
        txt.setFmt("Session: %s\n", net::SessionStatus::ToString(sessionInfo_.status));
        txt.appendFmt("Host: %" PRIi8 "\n", sessionInfo_.isHost);
        txt.appendFmt("PlyIdx: %" PRIi32 " Guid: %s", userNetMap_.localPlayerIdx, userNetMap_.myGuid.toString(guidStr));

        pPrim->drawText(pos, con, txt.begin(), txt.end());
        pos.y += 60.f;
    }

    if (vars_.userCmdDrawDebug()) {
        txt.setFmt("UserCmds:");

        for (int32_t i = 0; i < net::MAX_PLAYERS; i++) {
            txt.appendFmt("\nPly%" PRIi32 " UCmd: %" PRIuS, i, userCmdMan_.getNumUnreadFrames(i));
        }

        pPrim->drawText(pos, con, txt.begin(), txt.end());
        pos.y += (net::MAX_PLAYERS * 16.f) + 30.f;
    }

    if (vars_.drawGameUserDebug()) {
        txt.setFmt("LocalIdx %" PRIi32 " guid: %s\n", userNetMap_.localPlayerIdx, userNetMap_.myGuid.toString(guidStr));

        for (size_t i = 0; i < userNetMap_.lobbyUserGuids.size(); i++) {
            auto userGuid = userNetMap_.lobbyUserGuids[i];
            txt.appendFmt("%" PRIuS " %s\n", i, userGuid.toString(guidStr));
        }

        pPrim->drawText(pos, con, txt.begin(), txt.end());
        pos.y += (net::MAX_PLAYERS * 16.f) + 30.f;
    }
}

void XGame::syncLobbyUsers(void)
{
    core::FixedArray<net::UserInfo, net::MAX_PLAYERS> currentUsers;
    core::FixedFifo<net::UserInfo, net::MAX_PLAYERS> newUsers;

    auto* pLobby = pSession_->getLobby(net::LobbyType::Game);

    // only the host spawns players this way.
    // clients are told to spawn via snapshots.
    if (!sessionInfo_.isHost) {
        return;
    }

    auto numUsers = pLobby->getNumUsers();
    for (int32_t i = 0; i < numUsers; i++)
    {
        net::UserInfo info;
        pLobby->getUserInfoForIdx(i, info);

        X_ASSERT(info.guid.isValid(), "User is no valid")();

        if (!userNetMap_.guidPresent(info.guid))
        {
            newUsers.push(info);
        }
        else
        {
            currentUsers.push_back(info);
        }
    }

    X_ASSERT(static_cast<int32_t>(currentUsers.size()) <= userNetMap_.getNumUsers(),
        "current users is can't be bigger")(currentUsers.size(), userNetMap_.getNumUsers());

    // You still here?
    for (int32_t i = 0; i < static_cast<int32_t>(userNetMap_.lobbyUserGuids.size()); i++)
    {
        auto userGuid = userNetMap_.lobbyUserGuids[i];
        if (!userGuid.isValid()) {
            continue;
        }

        auto it = std::find_if(currentUsers.begin(), currentUsers.end(), [userGuid](const net::UserInfo& ui) {
            return ui.guid == userGuid;
        });

        if (it == currentUsers.end()) {

            net::NetGuidStr buf;
            X_LOG0("Game", "Client left %" PRIi32 " guid: %s", i, userGuid.toString(buf));

            if (pMultiplayerGame_) {
                pMultiplayerGame_->playerLeft(i);
            }

            userNetMap_.resetIndex(i);
            world_->removePlayer(i);
        }
    }

    // add the new users.
    while (newUsers.isNotEmpty())
    {
        // find a free local player slot.
        int32_t plyIdx = userNetMap_.findFreeSlot();

        if (plyIdx == -1) {
            X_ERROR("Game", "Failed to find free player slot for connected player");
            break;
        }

        auto user = newUsers.peek();
        newUsers.pop();

        net::NetGuidStr buf;
        X_LOG0("Game", "Client connected %" PRIi32 " guid: %s", plyIdx, user.guid.toString(buf));

        userNetMap_.addUser(plyIdx, user);
        bool isLocal = plyIdx == userNetMap_.localPlayerIdx;

        userCmdMan_.resetPlayer(plyIdx);

        // spawn!
        world_->spawnPlayer(userNetMap_, plyIdx, isLocal);
    }

}

void XGame::clearWorld(void)
{
    if (world_) {
        world_.reset();
    }

    gEnv->p3DEngine->clearPersistent();

    userNetMap_.reset();

    for (auto& uCmd : lastUserCmdRun_) {
        uCmd.clear();
    }

    lastUserCmdRunTime_.fill(0);
    lastUserCmdRunOnClientTime_.fill(0);
    lastUserCmdRunOnServerTime_.fill(0);
}

int32_t XGame::getPlayerIdxForGuid(net::NetGUID guid) const
{
    return userNetMap_.getPlayerIdxForGuid(guid);
}

void XGame::Command_Map(core::IConsoleCmdArgs* pCmd)
{
    if (pCmd->GetArgCount() != 2) {
        X_WARNING("Game", "map <mapname>");
        return;
    }

    auto mapName = pCmd->GetArg(1);

    // holly moly!!!!
    net::MatchParameters match;
    match.numSlots = 2;
    match.mode = net::GameMode::SinglePlayer;
    match.mapName.set(mapName.begin(), mapName.end());

    // i need to wait for state changes, but don't really want to stall.
    // so i basically need to track state?
    // really this needs to just be a single operation.
    // this is basically automating UI clicks for if you wanted to host a game.
    // basically i just neet to flag that i'm joining a game and track the state.
    // but single player games should be instant.

    auto waitForState = [&](net::SessionStatus::Enum status) {
        while (pSession_->getStatus() != status) {
            pSession_->update();
        }
    };

    pSession_->quitToMenu();
    waitForState(net::SessionStatus::Idle);
    pSession_->createPartyLobby(match);
    waitForState(net::SessionStatus::PartyLobby);
    pSession_->createMatch(match);
    waitForState(net::SessionStatus::GameLobby);
    pSession_->startMatch();

}

void XGame::Command_MainMenu(core::IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    pSession_->quitToMenu();
}

void XGame::Cmd_OpenMenu(core::IConsoleCmdArgs* pArgs)
{
    if (pArgs->GetArgCount() < 2) {
        X_WARNING("Game", "uiOpenMenu <name>");
        return;
    }

    auto menuName = pArgs->GetArg(1);

    pMenuHandler_->close();
    pMenuHandler_->openMenu(menuName);
}

void XGame::Cmd_Chat(core::IConsoleCmdArgs* pCmd)
{
    auto msg = pCmd->GetArg(1);
    if (msg.empty()) {
        return;
    }

    // who you talking to?
    if (!pMultiplayerGame_) {
        // nobody.
        return;
    }

    auto* pLobby = pSession_->getLobby(net::LobbyType::Game);
    
    // want the name of the player.
    core::string_view name = "player"_sv;

    if (userNetMap_.localPlayerIdx >= 0) {
        const auto& netGuid = userNetMap_.getLocalPlayerGUID();

        net::UserInfo info;
        if (pLobby->getUserInfoForGuid(netGuid, info)) {
            name = info.name;
        }
    }
    else {
        name = "server"_sv;
    }

    if (sessionInfo_.isHost) {
        pMultiplayerGame_->handleChatMsg(name, msg);
    }
    else {
        // client sends it to the server, then server will send it us back :D
        Multiplayer::ChatPacketBs bs;
        Multiplayer::buildChatPacket(bs, name, msg);

        pLobby->sendToHost(bs);
    }
}

X_NAMESPACE_END
