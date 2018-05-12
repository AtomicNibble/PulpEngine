#include "stdafx.h"
#include "Game.h"

#include <INetwork.h>
#include <IRender.h>
#include <IConsole.h>
#include <IFrameData.h>

#include <Math\XMatrixAlgo.h>

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
    world_(arena_),
    localClientId_(entity::INVALID_ID),
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

    // register some vars
    //	ADD_CVAR_REF_VEC3("cam_pos", cameraPos_, s_DefaultCamPosition, core::VarFlag::CHEAT,
    //		"camera position");
    //	ADD_CVAR_REF_VEC3("cam_angle", cameraAngle_, s_DefaultCamAngle, core::VarFlag::CHEAT,
    //		"camera angle(radians)");

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

        pPeer->setMaximumIncomingConnections(4);

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

        X_LOG0("Game", "Listening on port ^6%" PRIu16, sd.getPort());

        if (!pNet->createSession(pPeer)) {
            X_ERROR("Game", "Failed to create net session");
            return false;
        }

        pSession_ = X_ASSERT_NOT_NULL(pNet->getSession());
    }

    auto deimension = gEnv->pRender->getDisplayRes();

    X_ASSERT(deimension.x > 0, "height is not valid")(deimension.x);
    X_ASSERT(deimension.y > 0, "height is not valid")(deimension.y);

    cam_.setFrustum(deimension.x, deimension.y, DEFAULT_FOV, 1.f, 2048.f);

    // fiuxed for now, will match network id or something later
    localClientId_ = 0;

    userCmdGen_.init();
    weaponDefs_.init();


    return true;
}

bool XGame::shutDown(void)
{
    X_LOG0("Game", "Shutting Down");

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

    if (status == net::SessionStatus::Idle)
    {
        // main menu :D
        if (world_) {
            world_.reset();
        }

        static float val = 0.f;

        val += frame.timeInfo.deltas[core::ITimer::Timer::UI].GetSeconds() * 0.5f;

        float t = (math<float>::sin(val) + 1.f) * 0.5f;

        Color col = Col_White;

        con.col = col.lerp(t, Col_Red);
        pPrim->drawText(Vec3f(center.x, 75, 1.f), con, "Insert fancy main menu here");
    }
    else if (status == net::SessionStatus::Loading)
    {
        // loading the map :(
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
            world_->spawnPlayer(0);

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

        world_->update(frame, userCmdMan_);

        // if we are host we make snapshot.
        if (pSession_->isHost())
        {
            net::SnapShot snap(arena_);
            world_->createSnapShot(frame, snap);

            pSession_->sendSnapShot(std::move(snap));
        }
        else
        {
            // send userCmd?
            auto usrCmd = userCmdMan_.getUserCmdForPlayer(0);
            pSession_->sendUserCmd(usrCmd);

            auto* pSnap = pSession_->getSnapShot();
            if (pSnap)
            {
                world_->applySnapShot(frame, pSnap);
            }
        }
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

    {

        con.col = Col_Crimson;
        con.size = Vec2f(24.f, 24.f);
        con.flags.Clear();

        core::StackString256 txt;
        txt.appendFmt("Session: %s\n", net::SessionStatus::ToString(status));
        txt.appendFmt("Host: %" PRIi8, pSession_->isHost());

        pPrim->drawText(Vec3f(5.f, 50.f, 1.f), con, txt.begin(), txt.end());
    }


    Angles<float> goat;

    auto quat = goat.toQuat();
    auto foward = goat.toForward();

    userCmdGen_.buildUserCmd();
    auto& userCmd = userCmdGen_.getCurrentUsercmd();

    // so i want to store the input for what ever player we are but maybe I don't event have a level yet.
    // but we will likley want players before we have a level
    // for lobbies and stuff.
    // i kinda wanna clear all ents when you change level, which is why it's part of the world currently.
    // but makes it annoying to persist shit.
    // what if we just have diffrent registries?
    // one for players lol.
    // or should all this logic only happen if you in a bucket?

    userCmdMan_.addUserCmdForPlayer(localClientId_, userCmd);

    // do we actually have a player?
    // aka is a level loaded shut like that.
    // if not nothing todo.
    if (world_) {
    //    world_->update(frame, userCmdMan_);
    }
    else {
        // orth
    //    Matrix44f orthoProj;
    //    MatrixOrthoOffCenterRH(&orthoProj, 0, frame.view.viewport.getWidthf(), frame.view.viewport.getHeightf(), 0, -1e10f, 1e10);
    //
    //    frame.view.viewMatrixOrtho = Matrix44f::identity();
    //    frame.view.projMatrixOrtho = orthoProj;
    //    frame.view.viewProjMatrixOrth = orthoProj * frame.view.viewMatrixOrtho;
    }

    //	ProcessInput(frame.timeInfo);

    //	cam_.setAngles(cameraAngle_);
    //	cam_.setPosition(cameraPos_);

    // orth
    Matrix44f orthoProj;
    MatrixOrthoOffCenterRH(&orthoProj, 0, frame.view.viewport.getWidthf(), frame.view.viewport.getHeightf(), 0, -1e10f, 1e10);

    frame.view.viewMatrixOrtho = Matrix44f::identity();
    frame.view.projMatrixOrtho = orthoProj;
    frame.view.viewProjMatrixOrth = orthoProj * frame.view.viewMatrixOrtho;

    return true;
}

void XGame::ProcessInput(core::FrameTimeData& timeInfo)
{
    X_UNUSED(timeInfo);

    const float speed = 250.f;
    const float timeScale = speed * timeInfo.deltas[core::ITimer::Timer::GAME].GetSeconds();

    for (const auto& e : inputEvents_) {
        Vec3f posDelta;

        // rotate.
        switch (e.keyId) {
            case input::KeyId::MOUSE_X:
                cameraAngle_.z += -(e.value * 0.002f);
                continue;
            case input::KeyId::MOUSE_Y:
                cameraAngle_.x += -(e.value * 0.002f);
                continue;
            default:
                break;
        }

        float scale = 1.f;
        if (e.modifiers.IsSet(input::InputEvent::ModiferType::LSHIFT)) {
            scale = 2.f;
        }

        scale *= timeScale;

#if 0
		switch (e.keyId)
		{
			// forwards.
		case input::KeyId::W:
			posDelta.y = scale;
			break;
			// backwards
		case input::KeyId::A:
			posDelta.x = -scale;
			break;

			// Left
		case input::KeyId::S:
			posDelta.y = -scale;
			break;
			// right
		case input::KeyId::D:
			posDelta.x = scale;
			break;

			// up / Down
		case input::KeyId::Q:
			posDelta.z = -scale;
			break;
		case input::KeyId::E:
			posDelta.z = scale;
			break;

		default:
			continue;
		}
#endif

        // I want movement to be relative to the way the camera is facing.
        // so if i'm looking 90 to the right the direction also needs to be rotated.
        Matrix33f angle = Matrix33f::createRotation(cameraAngle_);

        cameraPos_ += (angle * posDelta);
    }

    inputEvents_.clear();
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