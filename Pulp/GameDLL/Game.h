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

X_NAMESPACE_DECLARE(render,
    struct IRender);
X_NAMESPACE_DECLARE(core,
                    struct ICVar;
                    struct IConsoleCmdArgs;);

X_NAMESPACE_BEGIN(game)

class XGame : public IGame
{
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

private:
    void ProcessInput(core::FrameTimeData& timeInfo);

    void OnFovChanged(core::ICVar* pVar);

    void Command_Map(core::IConsoleCmdArgs* Cmd);

private:
    core::MemoryArenaBase* arena_;

    ICore* pCore_;
    render::IRender* pRender_;
    core::ITimer* pTimer_;
    core::ICVar* pFovVar_;

    Vec3f cameraPos_;
    Vec3f cameraAngle_;

    input::InputEventBuffer inputEvents_;

    XCamera cam_;

private:
    GameVars vars_;

    entity::EntityId localClientId_;

    core::UniquePointer<World> world_;

    UserCmdGen userCmdGen_;
    UserCmdMan userCmdMan_;

    weapon::WeaponDefManager weaponDefs_;
};

X_NAMESPACE_END

#endif // !X_GAME_H_