#include "stdafx.h"
#include "Game.h"

#include <INetwork.h>
#include <IRender.h>
#include <IConsole.h>
#include <IFrameData.h>

#include <Math\XMatrixAlgo.h>
#include <Containers\FixedFifo.h>
#include <Hashing\Fnva1Hash.h>

// TMP
#include <I3DEngine.h>
#include <IPrimativeContext.h>
#include <SnapShot.h>

#include <IGui.h>

X_NAMESPACE_BEGIN(game)

XGame::XGame(ICore* pCore) :
    arena_(g_gameArena),
    pCore_(pCore),
    pTimer_(nullptr),
    pPeer_(nullptr),
    pSession_(nullptr),
    pRender_(nullptr),
    prevStatus_(net::SessionStatus::Idle),
    localPlayerIdx_(-1),
    world_(arena_),
    userCmdGen_(inputVars_),
    weaponDefs_(arena_),
    pMenuHandler_(nullptr)
{
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
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::AiClip, physics::GroupFlag::Player, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::AiClip, physics::GroupFlag::Ai, true);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::AiClip, physics::GroupFlag::Vehicle, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::PlayerClip, physics::GroupFlag::Player, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::PlayerClip, physics::GroupFlag::Ai, true);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::PlayerClip, physics::GroupFlag::Vehicle, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::VehicleClip, physics::GroupFlag::Player, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::VehicleClip, physics::GroupFlag::Ai, false);
    pPhysics->SetGroupCollisionFlag(physics::GroupFlag::VehicleClip, physics::GroupFlag::Vehicle, true);


    pTimer_ = gEnv->pTimer;
    pRender_ = gEnv->pRender;

    auto* pMenuMan = gEnv->p3DEngine->getMenuManager();

    pMenuHandler_ = pMenuMan->createMenuHandler();
    pMenuHandler_->openMenu("main");

    // networking.
    {
        auto* pNet = gEnv->pNet;
        pPeer_ = pNet->createPeer();

        myGuid_ = pPeer_->getMyGUID();

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
                        pMenuHandler_->openMenu("pause");
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
    X_UNUSED(frame);
    // how todo this camera move shit.
    // when the input frames are been called
    // the frame data has valid times.
    // we just don't have the data in the input callback.

    // the real issue is that input callbacks are global events, when this update
    // is a data based call.
    // but i have all the input events in this
    // but they are no use since i don't know if i'm allowed to use them all.
    // i likethe input sinks tho
    // as things are registerd with priority
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


    font::TextDrawContext con;
    con.col = Col_Whitesmoke;
    con.size = Vec2f(36.f, 36.f);
    con.effectId = 0;
    con.pFont = gEnv->pFontSys->getDefault();
    con.flags.Set(font::DrawTextFlag::CENTER);
    con.flags.Set(font::DrawTextFlag::CENTER_VER);

    auto width = static_cast<float>(frame.view.displayRes.x);
    auto height = static_cast<float>(frame.view.displayRes.y);

    Vec2f center(width * 0.5f, height * 0.5f);

    pSession_->update();

    auto status = pSession_->getStatus();

    auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::GUI);

    bool blockUserCmd = drawMenu(frame, pPrim);

    pSession_->drawDebug(pPrim);

    // this is built all the time currently even if it's not used.
    // since input events are passed to it all the time.
    userCmdGen_.buildUserCmd(blockUserCmd);

    if (status == net::SessionStatus::Idle)
    {
        // main menu :D

        if (prevStatus_ != net::SessionStatus::Idle)
        {
            clearWorld();

            pMenuHandler_->openMenu("main");

        }


#if 0
        auto val = frame.timeInfo.ellapsed[core::ITimer::Timer::UI].GetSeconds();

        float t = (math<float>::sin(val) + 1.f) * 0.5f;

        Color col = Col_Red;
        con.col = col.lerp(t, Col_White);
        pPrim->drawText(Vec3f(center.x, 75, 1.f), con, "Insert fancy main menu here");
#endif
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
            // frist frame in loading.
            pMenuHandler_->openMenu("loading");

            clearWorld();
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
                world_ = core::makeUnique<World>(arena_, vars_, gEnv->pPhysics, weaponDefs_, arena_);

                if (!world_->loadMap(matchParams.mapName)) {
                    X_ERROR("Game", "Failed to load map");
                    // TODO: quite to menu?
                }
            }
        }

        // so i need to support been loaded, yet sillt in the loading state, while we wait for shity peers.
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

            pMenuHandler_->close();

            userCmdGen_.clearForNewLevel();
        }

        X_ASSERT_NOT_NULL(world_.ptr());

        auto ellapsedMS = safe_static_cast<int32_t>(frame.timeInfo.deltas[core::ITimer::Timer::GAME].GetMilliSecondsAsInt64());

        serverGameTimeMS_ += ellapsedMS;

        syncLobbyUsers();

        auto localIdx = getLocalClientIdx();
        entity::EntityId localId = static_cast<entity::EntityId>(localIdx);

        const bool isHost = pSession_->isHost();

        {
            auto& userCmd = userCmdGen_.getCurrentUserCmd();
            userCmd.clientGameTimeMS = serverGameTimeMS_; // just make this server game time for now.
            userCmd.serverGameTimeMS = serverGameTimeMS_;

            if (!isHost) {
                // don't go past snapshot.
                userCmd.serverGameTimeMS = core::Min(userCmd.serverGameTimeMS, netInterpolInfo_.snapShotEndMS);
            }

            userCmdMan_.addUserCmdForPlayer(localIdx, userCmd);
        }

        // if we are host we make snapshot.
        if (isHost)
        {
            // run user cmds for all the valid players.
            for (int32_t i = 0; i < static_cast<int32_t>(lobbyUserGuids_.size()); i++)
            {
                if (!lobbyUserGuids_[i].isValid()) {
                    continue;
                }

                runUserCmdsForPlayer(frame, i);
            }   

        }
        else
        {
            // send user commands to server.
            pSession_->sendUserCmd(userCmdMan_, localIdx, frame.timeInfo);

            auto* pSnap = pSession_->getSnapShot();
            if (pSnap)
            {
                world_->applySnapShot(frame, pSnap);

                // get the games times the server has run
                lastUserCmdRunTime_ = pSnap->getUserCmdTimes();

                auto localLastRunTime = lastUserCmdRunTime_[localPlayerIdx_];

                // get all the userCmds we need to replay.
                net::UserCmdMan::UserCmdArr userCmds;
                userCmdMan_.getReadUserCmdsAfterGameTime(localPlayerIdx_, localLastRunTime, userCmds);
             
                for (auto& userCmd : userCmds)
                {
                    runUserCmdForPlayer(frame, userCmd, localPlayerIdx_);
                }
            }
            
            runUserCmdsForPlayer(frame, localIdx);
        }

        world_->update(frame, userCmdMan_, localId);

        if (isHost) {

            // send snapshot after updating world.
            if (pSession_->shouldSendSnapShot(frame.timeInfo)) {
                net::SnapShot snap(arena_);

                snap.setUserCmdTimes(lastUserCmdRunTime_);

                world_->createSnapShot(frame, snap);

                pSession_->sendSnapShot(snap);
            }
        }
    }
    else if (status == net::SessionStatus::PartyLobby)
    {
        // party!! at stu's house.
        // open lobby menu?
        if (prevStatus_ != net::SessionStatus::PartyLobby)
        {
            pMenuHandler_->openMenu("lobby");
        }


    }
    else if (status == net::SessionStatus::GameLobby)
    {
        if (prevStatus_ != net::SessionStatus::GameLobby)
        {
            pMenuHandler_->openMenu("lobby");
        }


    }
    else if (status == net::SessionStatus::Connecting)
    {
        // ...
    }
    else
    {
        X_ERROR("Game", "Unhandle session status: %s", net::SessionStatus::ToString(status));
    }

    prevStatus_ = status;
    
#if 0
    {
        con.col = Col_Crimson;
        con.size = Vec2f(24.f, 24.f);
        con.flags.Clear();

        net::NetGuidStr buf;
        core::StackString256 txt;
        txt.appendFmt("Session: %s\n", net::SessionStatus::ToString(status));
        txt.appendFmt("Host: %" PRIi8 "\n", pSession_->isHost());
        txt.appendFmt("PlyIdx: %" PRIi32 " Guid: %s", getLocalClientIdx(), myGuid_.toString(buf));

        pPrim->drawText(Vec3f(5.f, 50.f, 1.f), con, txt.begin(), txt.end());
    }
#endif

    if (vars_.userCmdDrawDebug())
    {
        core::StackString256 txt;

        txt.appendFmt("UserCmds:");

        for (int32_t i = 0; i < net::MAX_PLAYERS; i++)
        {
            txt.appendFmt("\nPly%" PRIi32 " UCmd: %" PRIuS, i, userCmdMan_.getNumUnreadFrames(i));
        }
        
        con.flags.Clear();

        pPrim->drawText(Vec3f(5.f, 500.f, 1.f), con, txt.begin(), txt.end());

    }

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

void XGame::setInterpolation(int32_t serverGameTimeMS, int32_t ssStartTimeMS, int32_t ssEndTimeMS, float fraction)
{
    netInterpolInfo_.frac = fraction;
    netInterpolInfo_.snapShotStartMS = ssStartTimeMS;
    netInterpolInfo_.snapShotEndMS = ssEndTimeMS;
    netInterpolInfo_.serverGameTimeMS = serverGameTimeMS;
    serverGameTimeMS_ = serverGameTimeMS;
}

void XGame::runUserCmdsForPlayer(core::FrameData& frame, int32_t playerIdx)
{
    // if the player is local
    // we run a user command for them
    if (localPlayerIdx_ == playerIdx) {
        auto unread = userCmdMan_.getNumUnreadFrames(playerIdx);
        X_ASSERT(unread > 0, "No user commans for local player")(unread);

        auto& userCmd = userCmdMan_.getUserCmdForPlayer(playerIdx);
        runUserCmdForPlayer(frame, userCmd, playerIdx);
    }
    else 
    {

        if (userCmdMan_.hasUnreadFrames(playerIdx))
        {
            auto nextCmdClientTimeMS = userCmdMan_.getNextUserCmdClientTimeMSForPlayer(playerIdx);
            auto clientGameTimedelta = nextCmdClientTimeMS - lastUserCmdRunOnClientTime_[playerIdx];
            auto timeSinceServerRanLastCmd = serverGameTimeMS_ - lastUserCmdRunOnServerTime_[playerIdx];

            int32_t clientTimeRunSoFar = 0;

            // if the client delta is less than server the client is running 'faster'
            // so if the dleta is negative client is running faster
            auto clientServerDelta = clientGameTimedelta - timeSinceServerRanLastCmd;

            X_LOG0("Game", "Ply %" PRIi32 " UserCmd clientDelta: %" PRIi32 " serverDelta: %" PRIi32 " delta: %" PRIi32,
                playerIdx, clientGameTimedelta, timeSinceServerRanLastCmd, clientServerDelta);

            if (clientServerDelta <= 1_i32)
            {
                // the client might be running slighty faster than us as there delta is smaller the ours.
                // so process them till they match our delta.
                while (clientTimeRunSoFar < timeSinceServerRanLastCmd && userCmdMan_.hasUnreadFrames(playerIdx))
                {
                    auto& userCmd = userCmdMan_.getUserCmdForPlayer(playerIdx);
                    runUserCmdForPlayer(frame, userCmd, playerIdx);

                    lastUserCmdRunOnClientTime_[playerIdx] = userCmd.clientGameTimeMS;
                    lastUserCmdRunOnServerTime_[playerIdx] = serverGameTimeMS_;

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
                runUserCmdForPlayer(frame, userCmd, playerIdx);
            }
        }
        else
        {
            X_WARNING("Game", "no userCmd for remote player %" PRIi32, playerIdx);
            // dam slut no sending user commads run a empty command.
            auto userCmd = lastUserCmdRun_[playerIdx];
            runUserCmdForPlayer(frame, userCmd, playerIdx);

            lastUserCmdRunOnServerTime_[playerIdx] = serverGameTimeMS_;
        }
    }
}

void XGame::runUserCmdForPlayer(core::FrameData& frame, const net::UserCmd& userCmd, int32_t playerIdx)
{
    world_->runUserCmdForPlayer(frame, userCmd, playerIdx);

    lastUserCmdRun_[playerIdx] = userCmd;
    lastUserCmdRunTime_[playerIdx] = userCmd.clientGameTimeMS;
}

bool XGame::drawMenu(core::FrameData& frame, engine::IPrimativeContext* pPrim)
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


void XGame::syncLobbyUsers(void)
{
    core::FixedArray<net::NetGUID, net::MAX_PLAYERS> currentUsers;
    core::FixedFifo<net::NetGUID, net::MAX_PLAYERS> newUsers;

    auto* pLobby = pSession_->getLobby(net::LobbyType::Game);

    auto numUsers = pLobby->getNumUsers();
    for (int32_t i = 0; i < numUsers; i++)
    {
        net::UserInfo info;
        pLobby->getUserInfoForIdx(i, info);

        X_ASSERT(info.guid.isValid(), "User is no valid")();

        auto it = std::find_if(lobbyUserGuids_.begin(), lobbyUserGuids_.end(), [info](const net::NetGUID& guid) {
            return guid == info.guid;
        });

        if (it == lobbyUserGuids_.end())
        {
            newUsers.push(info.guid);
        }
        else
        {
            currentUsers.push_back(info.guid);
        }
    }

    // You still here?
    for (int32_t i = 0; i < static_cast<int32_t>(lobbyUserGuids_.size()); i++)
    {
        auto userGuid = lobbyUserGuids_[i];
        if (!userGuid.isValid()) {
            continue;
        }

        if (currentUsers.find(userGuid) == decltype(currentUsers)::invalid_index) {

            net::NetGuidStr buf;
            X_LOG0("Game", "Client left %" PRIi32 " guid: %s", i, userGuid.toString(buf));

            lobbyUserGuids_[i] = net::NetGUID();
            world_->removePlayer(i);
        }
    }

    // add the new users.
    while (newUsers.isNotEmpty())
    {
        // find a free local player slot.
        int32_t plyIdx = -1;
        for (int32_t i = 0; i < static_cast<int32_t>(lobbyUserGuids_.size()); i++)
        {
            if (!lobbyUserGuids_[i].isValid())
            {
                plyIdx = i;
                break;
            }
        }

        if (plyIdx == -1)
        {
            X_ERROR("Game", "Failed to find free player slot for connected player");
            break;
        }

        auto userGuid = newUsers.peek();
        newUsers.pop();

        net::NetGuidStr buf;
        X_LOG0("Game", "Client connected %" PRIi32 " guid: %s", plyIdx, userGuid.toString(buf));

        lobbyUserGuids_[plyIdx] = userGuid;

        userCmdMan_.resetPlayer(plyIdx);

        auto isLocal = myGuid_ == userGuid;

        if (isLocal) {
            localPlayerIdx_ = plyIdx;
        }

        // spawn!
        world_->spawnPlayer(plyIdx, isLocal);
    }

}

void XGame::clearWorld(void)
{
    if (world_) {
        world_.reset();
    }

    lobbyUserGuids_.fill(net::NetGUID());

    for (auto& uCmd : lastUserCmdRun_) {
        uCmd.clear();
    }
    lastUserCmdRunOnClientTime_.fill(0);
    lastUserCmdRunOnServerTime_.fill(0);
}

int32_t XGame::getLocalClientIdx(void) const
{
    for (int32_t i = 0; i < static_cast<int32_t>(lobbyUserGuids_.size()); i++)
    {
        if (myGuid_ == lobbyUserGuids_[i])
        {
            return i;
        }
    }

    // return 0?
    // X_ASSERT_UNREACHABLE();
    return -1;
}

int32_t XGame::getPlayerIdxForGuid(net::NetGUID guid) const
{
    auto it = std::find(lobbyUserGuids_.begin(), lobbyUserGuids_.end(), guid);
    if (it != lobbyUserGuids_.end()) {
        auto idx = std::distance(lobbyUserGuids_.begin(), it);
        return safe_static_cast<int32_t>(idx);
    }

    return -1;
}

void XGame::Command_Map(core::IConsoleCmdArgs* pCmd)
{
    if (pCmd->GetArgCount() != 2) {
        X_WARNING("Game", "map <mapname>");
        return;
    }

    const char* pMapName = pCmd->GetArg(1);

    // holly moly!!!!
    net::MatchParameters match;
    match.numSlots = 2;
    match.mode = net::GameMode::SinglePlayer;
    match.mapName.set(pMapName);

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

    auto* pMenuName = pArgs->GetArg(1);

    pMenuHandler_->close();
    pMenuHandler_->openMenu(pMenuName);
}

X_NAMESPACE_END
