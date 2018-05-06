#include "stdafx.h"
#include "SessionVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(net)

SessionVars::SessionVars()
{
    connectionAttemps_ = 3;
    connectionRetyDelayMs_ = 500;
}


void SessionVars::registerVars(void)
{

    ADD_CVAR_REF("net_connect_attemps", connectionAttemps_, connectionAttemps_, 0, 128, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Number of connection attemps to make");

    ADD_CVAR_REF("net_connect_rety_delay", connectionRetyDelayMs_, connectionRetyDelayMs_, 1, (1000 * 60) * 10, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Time to wait in MS before retrying");

}


X_NAMESPACE_END
