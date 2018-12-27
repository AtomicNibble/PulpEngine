#pragma once

#ifndef X_GAME_H_
#define X_GAME_H_

#include <IGame.h>
#include <IInput.h>
#include <ITimer.h>

#include "Level\Level.h"
#include "Vars\GameVars.h"
#include "Vars\InputVars.h"
#include "Multiplayer.h"
#include "UserNetMappings.h"

#include "UserCmds\UserCmdGen.h"
#include <UserCmdMan.h>

#include "Weapon\WeaponManager.h"

X_NAMESPACE_DECLARE(net,
    struct IPeer;
    struct ISession);

X_NAMESPACE_DECLARE(render,
    struct IRender);

X_NAMESPACE_DECLARE(core,
                    struct ICVar;
                    struct IConsoleCmdArgs);

X_NAMESPACE_DECLARE(engine,
namespace gui
{
    struct IMenuHandler;
}
);


X_NAMESPACE_BEGIN(game)

class XGame : public IGame, net::IGameCallbacks
{
    using PlayerEntsArr = std::array<entity::EntityId, net::MAX_PLAYERS>;
    using PlayerGuidArr = std::array<net::NetGUID, net::MAX_PLAYERS>;
    using PlayerTimeMSArr = std::array<int32_t, net::MAX_PLAYERS>;
    using PlayerUserCmdArr = std::array<net::UserCmd, net::MAX_PLAYERS>;

    struct NetInterpolationInfo
    {
        NetInterpolationInfo() :
            frac(0.f),
            serverGameTimeMS(0),
            snapShotStartMS(0),
            snapShotEndMS(0)
        {
        }

        float frac;
        int32_t serverGameTimeMS;
        int32_t snapShotStartMS;
        int32_t snapShotEndMS;
    };

public:
    XGame(ICore* pCore);
    ~XGame() X_FINAL;

    // IGame
    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    bool init(void) X_FINAL;
    bool shutDown(void) X_FINAL;
    void release(void) X_FINAL;

    bool asyncInitFinalize(void) X_FINAL;

    bool update(core::FrameData& frame) X_FINAL;
    bool onInputEvent(const input::InputEvent& event) X_FINAL;
    // ~IGame

private:

    // IGameCallbacks
    void onUserCmdReceive(net::NetGUID guid, core::FixedBitStreamBase& bs) X_FINAL;
    void buildSnapShot(net::SnapShot& snap) X_FINAL;
    void applySnapShot(const net::SnapShot& snap) X_FINAL;
    // ~IGameCallbacks
    
private:
    void setInterpolation(float fraction, int32_t serverGameTimeMS, int32_t ssStartTimeMS, int32_t ssEndTimeMS) X_FINAL;

    void runUserCmdsForPlayer(core::FrameData& frame, int32_t playerIdx);
    void runUserCmdForPlayer(core::TimeVal dt, const net::UserCmd& userCmd, int32_t playerIdx);
    
    bool drawMenu(core::FrameData& frame, engine::IPrimativeContext* pPrim);
    void drawDebug(engine::IPrimativeContext* pPrim);

    void syncLobbyUsers(void);
    void clearWorld(void);

    int32_t getPlayerIdxForGuid(net::NetGUID guid) const;

    void Command_Map(core::IConsoleCmdArgs* Cmd);
    void Command_MainMenu(core::IConsoleCmdArgs* Cmd);
    void Cmd_OpenMenu(core::IConsoleCmdArgs* Cmd);
    void Cmd_Chat(core::IConsoleCmdArgs* Cmd);

private:
    core::MemoryArenaBase* arena_;

    ICore* pCore_;
    net::IPeer* pPeer_;
    net::ISession* pSession_;
    render::IRender* pRender_;
    core::ITimer* pTimer_;

private:
    GameVars vars_;
    InputVars inputVars_;

    net::SessionStatus::Enum prevStatus_;

    UserNetMappings userNetMap_;

    core::UniquePointer<World> world_;

    UserCmdGen userCmdGen_;
    net::UserCmdMan userCmdMan_;
    
    weapon::WeaponDefManager weaponDefs_;

    engine::gui::IMenuHandler* pMenuHandler_;

    core::UniquePointer<Multiplayer> pMultiplayerGame_;

    // 
    NetInterpolationInfo netInterpolInfo_;
    int32_t serverGameTimeMS_;  // only set on clients.
    int32_t gameTimeMS_;

    // TODO: maybe move to network module

    // the last usecmd ran, used to tell peer what server has processed.
    // on the client this is updated by snapshot.
    PlayerUserCmdArr lastUserCmdRun_;
    PlayerTimeMSArr lastUserCmdRunTime_;
    PlayerTimeMSArr lastUserCmdRunOnClientTime_;
    PlayerTimeMSArr lastUserCmdRunOnServerTime_;
};

X_NAMESPACE_END

#endif // !X_GAME_H_