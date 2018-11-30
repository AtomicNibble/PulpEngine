#include "stdafx.h"
#include "SessionVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(net)

SessionVars::SessionVars()
{
    sessionDebug_ = 0;
    lobbyDebug_ = 0;
    drawLobbyDebug_ = 0;
    connectionAttemps_ = 3;
    connectionRetyDelayMs_ = 500;
    joinLobbyTimeoutMs_ = 5000;

    snapDebug_ = 0;
    snapMaxbufferedMs_ = 100;
    snapRateMs_ = 100;
    userCmdRateMs_ = 40;

    waitForPlayers_ = 1;
}


void SessionVars::registerVars(void)
{
    ADD_CVAR_REF("net_session_debug", sessionDebug_, sessionDebug_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable session debug");

    ADD_CVAR_REF("net_lobby_debug", lobbyDebug_, lobbyDebug_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable lobby debug");

    ADD_CVAR_REF("net_draw_lobby_debug", drawLobbyDebug_, drawLobbyDebug_, 0, 2, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Draw lobby debug");

    ADD_CVAR_REF("net_connect_attemps", connectionAttemps_, connectionAttemps_, 0, 128, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Number of connection attemps to make");

    ADD_CVAR_REF("net_connect_retry_delay", connectionRetyDelayMs_, connectionRetyDelayMs_, 1, (1000 * 60) * 10, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Time to wait in MS before retrying");

    ADD_CVAR_REF("net_lobby_join_timeout", joinLobbyTimeoutMs_, joinLobbyTimeoutMs_, 1, (1000 * 60) * 10, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Timeout in MS for joining a lobby");

    // snip snap.
    ADD_CVAR_REF("net_snap_debug", snapDebug_, snapDebug_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Drops the next outgoing snap");

    ADD_CVAR_REF("net_snap_force_drop_next", snapFroceDrop_, 0, 0, 1, core::VarFlag::SYSTEM,
        "Drops the next outgoing snap");

    ADD_CVAR_REF("net_snap_max_buffered_ms", snapMaxbufferedMs_, snapMaxbufferedMs_, 1, 1000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Max ammount of buffered snaps shots in ms");
    ADD_CVAR_REF("net_snap_rate_ms", snapRateMs_, snapRateMs_, 1, 1000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "How many ms between sending snaps");
    ADD_CVAR_REF("net_ucmd_rate_ms", userCmdRateMs_, userCmdRateMs_, 1, 1000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "How many ms between sending usercmds");

    ADD_CVAR_REF("net_wait_for_players", waitForPlayers_, waitForPlayers_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "If the host will wait for players to finish loading");

}


X_NAMESPACE_END
