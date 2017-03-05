#include "stdafx.h"
#include "NetVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(net)

NetVars::NetVars()
{
	debug_ = 0;

}


void NetVars::registerVars(void)
{

	ADD_CVAR_REF("net_debug", debug_, 0, 0, 2, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enable net debug msg's 1=enabled 2=verbose");

	ADD_CVAR_REF("net_socket_debug", debugSockets_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enable socket debug logging");

	ADD_CVAR_REF("net_default_timeout", debugSockets_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"The default timeout for connections");

	ADD_CVAR_REF("net_rl_connections_per_ip", rlconnectionsPerIp_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Rate limits connection requests from the same ip");

	ADD_CVAR_REF("net_rl_connections_per_ip_threshold", rlconnectionsPerIpThreshMS_, 100, 0, 10000000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Threshold in MS for applying connection rate limiting");

	ADD_CVAR_REF("net_rl_connections_per_ip_ban_time", rlconnectionsPerIpBanTimeMS_, 10000, 0, 10000000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"The amount of time in MS the ip is banned for after exceeding threshold. 0=unlimited");

	ADD_CVAR_REF("net_partial_connection_timeout", dropPartialConnectionsMS_, 3000, 0, 10000000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"The amount of time in MS before dropping a incomplete connection");
	
	ADD_CVAR_REF("net_ping_interval", pingTimeMS_, 1000, 0, 10000000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Min amount of time between pings");

	ADD_CVAR_REF("net_unreliable_timeout", unreliableTimeoutMS_, 0, 0, 10000000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Time in MS before not sending a unreliable msg. 0=never");

	ADD_CVAR_REF("net_connection_bandwidth_limit", connectionBSPLimit_, 0, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Limits outgoing bandwidth(BPS) per connection, once the limit is reached traffic is qeued. 0=unlimited");


}

X_NAMESPACE_END