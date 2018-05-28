#pragma once

#ifndef X_GAME_H_
#define X_GAME_H_

#include <IGame.h>
#include <IInput.h>
#include <ITimer.h>

#include "Level\Level.h"
#include "Vars\GameVars.h"

#include "UserCmds\UserCmds.h"
#include "UserCmds\UserCmdMan.h"

#include "Weapon\WeaponManager.h"

X_NAMESPACE_DECLARE(net,
    struct ISession);

X_NAMESPACE_DECLARE(render,
    struct IRender);

X_NAMESPACE_DECLARE(core,
                    struct ICVar;
                    struct IConsoleCmdArgs);

X_NAMESPACE_BEGIN(game)

class XGame : public IGame, net::IGameCallbacks
{
    using PlayerEntsArr = std::array<entity::EntityId, net::MAX_PLAYERS>;
    using PlayerGuidArr = std::array<net::NetGUID, net::MAX_PLAYERS>;

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
    // ~IGame

    // IGameCallbacks
    void onUserCmdReceive(net::NetGUID guid, core::FixedBitStreamBase& bs) X_FINAL;

    // ~IGameCallbacks

private:
    void ProcessInput(core::FrameTimeData& timeInfo);

    int32_t getLocalClientIdx(void) const;
    int32_t getPlayerIdxForGuid(net::NetGUID guid) const;

    void OnFovChanged(core::ICVar* pVar);
    void Command_Map(core::IConsoleCmdArgs* Cmd);
    void Command_MainMenu(core::IConsoleCmdArgs* Cmd);

private:
    core::MemoryArenaBase* arena_;

    ICore* pCore_;
    net::ISession* pSession_;
    render::IRender* pRender_;
    core::ITimer* pTimer_;
    core::ICVar* pFovVar_;

    Vec3f cameraPos_;
    Vec3f cameraAngle_;

    input::InputEventBuffer inputEvents_;

    XCamera cam_;

private:
    GameVars vars_;

    net::SessionStatus::Enum prevStatus_;
    net::NetGUID myGuid_;
    PlayerGuidArr lobbyUserGuids_;

    core::UniquePointer<World> world_;

    UserCmdGen userCmdGen_;
    UserCmdMan userCmdMan_;

    weapon::WeaponDefManager weaponDefs_;
};

X_NAMESPACE_END

#endif // !X_GAME_H_