#include "stdafx.h"
#include "Game.h"

#include <INetwork.h>
#include <IRender.h>
#include <IConsole.h>
#include <IFrameData.h>

#include <Math\XMatrixAlgo.h>
#include <Containers\FixedFifo.h>

// TMP
#include <I3DEngine.h>
#include <IPrimativeContext.h>
#include <SnapShot.h>

X_NAMESPACE_BEGIN(game)

namespace
{
    static const Vec3f s_DefaultCamPosition(0, -150, 150);
    static const Vec3f s_DefaultCamAngle(toRadians(-45.f), 0, toRadians(0.f));

} // namespace

XGame::XGame(ICore* pCore) :
    arena_(g_gameArena),
    pCore_(pCore),
    pTimer_(nullptr),
    pSession_(nullptr),
    pRender_(nullptr),
    pFovVar_(nullptr),
    prevStatus_(net::SessionStatus::Idle),
    world_(arena_),
    weaponDefs_(arena_)
{

    X_ASSERT_NOT_NULL(pCore);
}

XGame::~XGame()
{
}

void XGame::registerVars(void)
{
	weaponDefs_.registerVars();
    vars_.registerVars();

    pFovVar_ = ADD_CVAR_FLOAT("cam_fov", ::toDegrees(DEFAULT_FOV), 0.01f, ::toDegrees(math<float>::PI),
        core::VarFlag::SAVE_IF_CHANGED, "camera fov");

    core::ConsoleVarFunc del;
    del.Bind<XGame, &XGame::OnFovChanged>(this);
    pFovVar_->SetOnChangeCallback(del);
}

void XGame::registerCmds(void)
{
	weaponDefs_.registerCmds();

    ADD_COMMAND_MEMBER("map", this, XGame, &XGame::Command_Map, core::VarFlag::SYSTEM, "Loads a map");
    ADD_COMMAND_MEMBER("mainMenu", this, XGame, &XGame::Command_MainMenu, core::VarFlag::SYSTEM, "Return to main menu");
    
}

bool XGame::init(void)
{
    X_LOG0("Game", "init");
    X_ASSERT_NOT_NULL(gEnv->pInput);
    X_ASSERT_NOT_NULL(gEnv->pTimer);
    X_ASSERT_NOT_NULL(gEnv->pRender);
    X_ASSERT_NOT_NULL(gEnv->pNet);

    pTimer_ = gEnv->pTimer;
    pRender_ = gEnv->pRender;

    // networking.
    {
        auto* pNet = gEnv->pNet;
        auto* pPeer = pNet->createPeer();
        
        myGuid_  = pPeer->getMyGUID();

        net::Port basePort = 1337;
        net::Port maxPort = basePort + 10;

        net::SocketDescriptor sd(basePort);
        auto res = pPeer->init(2, sd);

        while (res == net::StartupResult::SocketPortInUse && sd.getPort() <= maxPort) {
            sd.setPort(sd.getPort() + 1);
            res = pPeer->init(2, sd);
        }

        if (res != net::StartupResult::Started) {
            X_ERROR("Game", "Failed to setup networking: \"%s\"", net::StartupResult::ToString(res));
            return false;
        }

        pPeer->setMaximumIncomingConnections(4);
        X_LOG0("Game", "Listening on port ^6%" PRIu16, sd.getPort());

        pSession_ = pNet->createSession(pPeer, this);
        if (!pSession_) {
            X_ERROR("Game", "Failed to create net session");
            return false;
        }
    }

    auto deimension = gEnv->pRender->getDisplayRes();

    X_ASSERT(deimension.x > 0, "height is not valid")(deimension.x);
    X_ASSERT(deimension.y > 0, "height is not valid")(deimension.y);

    cam_.setFrustum(deimension.x, deimension.y, DEFAULT_FOV, 1.f, 2048.f);

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

    if (pFovVar_) {
        pFovVar_->Release();
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
    MatrixOrthoOffCenterRH(&orthoProj, 0, frame.view.viewport.getWidthf(), frame.view.viewport.getHeightf(), 0, -1e10f, 1e10);

    frame.view.viewMatrixOrtho = Matrix44f::identity();
    frame.view.projMatrixOrtho = orthoProj;
    frame.view.viewProjMatrixOrth = orthoProj * frame.view.viewMatrixOrtho;


    font::TextDrawContext con;
    con.col = Col_Whitesmoke;
    con.size = Vec2f(36.f, 36.f);
    con.effectId = 0;
    con.pFont = gEnv->pFontSys->GetDefault();
    con.flags.Set(font::DrawTextFlag::CENTER);
    con.flags.Set(font::DrawTextFlag::CENTER_VER);

    auto width = frame.view.viewport.getWidthf();
    auto height = frame.view.viewport.getHeightf();

    Vec2f center(width * 0.5f, height * 0.5f);

    pSession_->update();

    auto status = pSession_->getStatus();

    auto* pPrim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::GUI);

    pSession_->drawDebug(pPrim);

    userCmdGen_.buildUserCmd();

    if (status == net::SessionStatus::Idle)
    {
        // main menu :D
        clearWorld();

        auto val = frame.timeInfo.ellapsed[core::ITimer::Timer::UI].GetSeconds();

        float t = (math<float>::sin(val) + 1.f) * 0.5f;

        Color col = Col_White;

        con.col = col.lerp(t, Col_Red);
        pPrim->drawText(Vec3f(center.x, 75, 1.f), con, "Insert fancy main menu here");
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
                world_ = core::makeUnique<World>(arena_, vars_, gEnv->pPhysics, userCmdMan_, weaponDefs_, arena_);

                if (!world_->loadMap(matchParams.mapName)) {
                    X_ERROR("Game", "Failed to load map");
                }
            }
        }

        // so i need to support been loaded, yet sillt in the loading state, while we wait for shity peers.
        // to finish loading, this is so the loading screen is still showing and can show pleb progress.
        // where to store this state?
        if (world_->hasLoaded() && !pSession_->hasFinishedLoading())
        {
            // spawn stuff like players!
            syncLobbyUsers();

            pSession_->finishedLoading();
        }

        // draw some shitty load screen?
        pPrim->drawQuad(0.f, 0.f, width, height, Col_Black);

        con.col = Col_Whitesmoke;
        pPrim->drawText(Vec3f(center.x, center.y, 1.f), con, L"loading");

        const float barWidth = 400.f;
        const float barHeight = 10.f;
        float progress = 0.3f;

        Vec2f barPos(center);
        barPos.x -= barWidth * 0.5f;
        barPos.y += 30.f;

        pPrim->drawQuad(barPos.x, barPos.y, barWidth * progress, barHeight, Col_Red);
        pPrim->drawRect(barPos.x, barPos.y, barWidth, barHeight, Col_Red);
    }
    else if (status == net::SessionStatus::InGame)
    {
        X_ASSERT_NOT_NULL(world_.ptr());

        syncLobbyUsers();

        auto localIdx = getLocalClientIdx();
        entity::EntityId localId = static_cast<entity::EntityId>(localIdx);

        {
            auto& userCmd = userCmdGen_.getCurrentUsercmd();
            userCmd.gameTime = frame.timeInfo.ellapsed[core::ITimer::Timer::GAME];
            userCmdMan_.addUserCmdForPlayer(localIdx, userCmd);
        }

        // if we are host we make snapshot.
        if (pSession_->isHost())
        {
            // run user cmds for all the valid players.
            for (int32_t i = 0; i < lobbyUserGuids_.size(); i++)
            {
                if (!lobbyUserGuids_[i].isValid()) {
                    continue;
                }

                // meow !
                if (userCmdMan_.hasUnreadFrames(i))
                {
                    auto& userCmd = userCmdMan_.getUserCmdForPlayer(i);

                    world_->runUserCmdForPlayer(frame, userCmd, i);
                }
                else
                {
                    X_WARNING("Game", "no user cmds for player: %" PRIi32, i);
                }
            }

            net::SnapShot snap(arena_);
            world_->createSnapShot(frame, snap);

            pSession_->sendSnapShot(std::move(snap));
        }
        else
        {
            // we send N cmds, but this should be rate limited by 'net_ucmd_rate_ms'

            core::FixedBitStreamStack<(sizeof(net::UserCmd) * net::MAX_USERCMD_SEND) + 0x100> bs;
            bs.write(net::MessageID::UserCmd);
            userCmdMan_.writeUserCmdToBs(bs, net::MAX_USERCMD_SEND, localIdx);

            pSession_->sendUserCmd(bs);

            auto* pSnap = pSession_->getSnapShot();
            if (pSnap)
            {
                world_->applySnapShot(frame, pSnap);
            }

            // run me some user cmds!
            auto& userCmd = userCmdMan_.getUserCmdForPlayer(localIdx);
            auto unread = userCmdMan_.getNumUnreadFrames(localIdx);
            
            if (userCmd.moveForwrd)
            {
                X_LOG0("Game", "client move forward");
            }

            X_LOG0_EVERY_N(60, "Goat", "Unread %i", unread);
            
            world_->runUserCmdForPlayer(frame, userCmd, localIdx);
        }

        world_->update(frame, userCmdMan_, localId);
    }
    else if (status == net::SessionStatus::PartyLobby)
    {
        // party!! at stu's house.


    }
    else if (status == net::SessionStatus::GameLobby)
    {
        auto* pLobby = pSession_->getLobby(net::LobbyType::Game);
        auto numUsers = pLobby->getNumUsers();
        auto freeSlots = pLobby->getNumFreeUserSlots();
        auto hostIdx = pLobby->getHostPeerIdx();
        auto& params = pLobby->getMatchParams();

        net::ChatMsg msg;
        while (pLobby->tryPopChatMsg(msg))
        {
            core::DateTimeStamp::Description timeStr;
            
            X_LOG0("Chat", "%s: \"%s\"", msg.dateTimeStamp.toString(timeStr), msg.msg.c_str());
        }

        con.col = Col_Floralwhite;
        con.size = Vec2f(24.f, 24.f);
        con.flags.Clear();

        // who#s in my lobbyyyyy!!
        core::StackString512 txt;
        txt.setFmt("---- GameLobby(%" PRIuS "/%" PRIuS ") ----\n", numUsers, numUsers + freeSlots);
        
        for (size_t i = 0; i < numUsers; i++)
        {
            auto handle = pLobby->getUserHandleForIdx(i);
            
            net::UserInfo info;
            pLobby->getUserInfo(handle, info);

            bool isHost = (hostIdx == info.peerIdx);

            txt.appendFmt("\n%s ^8%s ^7peerIdx: ^8%" PRIi32 "^7", isHost ? "H" : "P", info.pName, info.peerIdx);
        }

        pPrim->drawQuad(800.f, 200.f, 320.f + 320.f, 200.f, Color8u(40,40,40,100));
        pPrim->drawText(Vec3f(802.f, 202.f, 1.f), con, txt.begin(), txt.end());

        txt.setFmt("Options:\nSlots: %" PRIi32 "\nMap: \"%s\"", params.numSlots, params.mapName.c_str());

        pPrim->drawText(Vec3f(1240.f, 202.f, 1.f), con, txt.begin(), txt.end());

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

    if (vars_.userCmdDrawDebug())
    {
        core::StackString256 txt;

        txt.appendFmt("UserCmds:");

        for (int32_t i = 0; i < net::MAX_PLAYERS; i++)
        {
            if (userCmdMan_.hasUnreadFrames(i))
            {
                txt.appendFmt("\nPly%" PRIi32 "Unread UCmd: %" PRIuS, i, userCmdMan_.getNumUnreadFrames(i));
            }
        }

        pPrim->drawText(Vec3f(5.f, 500.f, 1.f), con, txt.begin(), txt.end());

    }

    return true;
}

void XGame::onUserCmdReceive(net::NetGUID guid, core::FixedBitStreamBase& bs)
{
    // we got user cmds o.o
    // what if i don't like you HEY!
    auto clientIdx = getPlayerIdxForGuid(guid);

    userCmdMan_.readUserCmdToBs(bs, clientIdx);
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
    for (int32_t i = 0; i < lobbyUserGuids_.size(); i++)
    {
        auto userGuid = lobbyUserGuids_[i];
        if (!userGuid.isValid()) {
            continue;
        }

        if (currentUsers.find(userGuid) == decltype(currentUsers)::invalid_index) {
            lobbyUserGuids_[i] = net::NetGUID();

            world_->removePlayer(i);
        }
    }

    // add the new users.
    while (newUsers.isNotEmpty())
    {
        // find a free local player slot.
        int32_t plyIdx = -1;
        for (int32_t i = 0; i < lobbyUserGuids_.size(); i++)
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

        // spawn!
        world_->spawnPlayer(plyIdx);
    }

}

void XGame::clearWorld(void)
{
    if (world_) {
        world_.reset();
    }

    lobbyUserGuids_.fill(net::NetGUID());
}

int32_t XGame::getLocalClientIdx(void) const
{
    for (int32_t i = 0; i < lobbyUserGuids_.size(); i++)
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


void XGame::OnFovChanged(core::ICVar* pVar)
{
    float fovDegress = pVar->GetFloat();
    float fov = ::toRadians(fovDegress);

    cam_.setFov(fov);
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
    match.numSlots = 1;
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


X_NAMESPACE_END