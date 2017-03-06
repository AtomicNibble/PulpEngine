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

	ADD_CVAR_REF("net_default_timeout", defaultTimeoutMS_, 5000, 0, 10000000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"The default timeout for connections in MS");

	ADD_CVAR_REF("net_rl_connections_per_ip", rlconnectionsPerIp_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Rate limits connection requests from the same ip");

	ADD_CVAR_REF("net_rl_connections_per_ip_threshold", rlconnectionsPerIpThreshMS_, 100, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Threshold in MS for applying connection rate limiting");

	ADD_CVAR_REF("net_rl_connections_per_ip_ban_time", rlconnectionsPerIpBanTimeMS_, 10000, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"The amount of time in MS the ip is banned for after exceeding threshold. 0=unlimited");

	ADD_CVAR_REF("net_partial_connection_timeout", dropPartialConnectionsMS_, 3000, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"The amount of time in MS before dropping a incomplete connection");
	
	ADD_CVAR_REF("net_ping_interval", pingTimeMS_, 1000, 0, 10000000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Min amount of time between pings");

	ADD_CVAR_REF("net_unreliable_timeout", unreliableTimeoutMS_, 0, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Time in MS before not sending a unreliable msg. 0=never");

	ADD_CVAR_REF("net_unexpected_msg_ban_time", unexpectedMsgBanTime_, 2000, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Time in MS to ban a client that sends a unexpected msg. eg: sending post connection msg before connecting. 0=never");

	ADD_CVAR_REF("net_connection_bandwidth_limit", connectionBSPLimit_, 0, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Limits outgoing bandwidth(BPS) per connection, once the limit is reached traffic is qeued. 0=unlimited");

	// artifical ping / packet loss.
	ADD_CVAR_REF("net_art_packet_loss", artificalPacketLoss_, 0, 0, 100, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Introduce artifical outgoing packet loss, percentage chance. 0=disabled");

	ADD_CVAR_REF("net_art_ping", artificalPing_, 0, 0, 999, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Introduce artifical ping, all messages will have a ping of atleast this value. 0=disabled");

	ADD_CVAR_REF("net_art_ping_variance", artificalPingVariance_, 0, 0, 999, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Introduce artifical ping variance");

	ADD_CVAR_REF("net_ignore_password_if_not_required", ignorePasswordFromClientIfNotRequired_, 1, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"If a client provides as password when connecting, but one is not required allow it to connect");

	// how to add algo names to var desc.
	// needs to be compile time str.
	// soo.. manually make it ;(
	using namespace core::Compression;
	static_assert(Algo::ENUM_COUNT == 7, "Algo enum changed this code needs updating?");

	static_assert(Algo::STORE == 0, "Algo enum order changed, update this code");
	static_assert(Algo::LZ4 == 1, "Algo enum order changed, update this code");
	static_assert(Algo::LZ4HC == 2, "Algo enum order changed, update this code");
	static_assert(Algo::LZMA == 3, "Algo enum order changed, update this code");
	static_assert(Algo::ZLIB == 4, "Algo enum order changed, update this code");
	static_assert(Algo::LZ5 == 5, "Algo enum order changed, update this code");
	static_assert(Algo::LZ5HC == 6, "Algo enum order changed, update this code");


	ADD_CVAR_REF("net_comp_ack_algo", ackCompAlgo_, 0, 0, core::Compression::Algo::ENUM_COUNT - 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Compression algo used for ack's. 0=none, 1=lz4, 2=lz4hc, 3=lzma, 4=zlib, 5=lz5, 6=lz6hc");

	ADD_CVAR_REF("net_comp_packet_algo", compAlgo_, 0, 0, core::Compression::Algo::ENUM_COUNT - 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Compression algo used for data packets. 0=none, 1=lz4, 2=lz4hc, 3=lzma, 4=zlib, 5=lz5, 6=lz6hc");
}

X_NAMESPACE_END