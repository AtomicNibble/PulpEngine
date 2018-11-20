#include "stdafx.h"
#include "NetVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(net)

NetVars::NetVars()
{
}

void NetVars::registerVars(void)
{
    core::ConsoleVarFunc del;

    ADD_CVAR_REF("net_debug", debug_, 0, 0, 2, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable net debug msg's 1=enabled 2=verbose");

    ADD_CVAR_REF("net_debug_datagram", debugDataGram_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable net debug msg's for datagrams");

    ADD_CVAR_REF("net_debug_ignored", debugIgnored_, 1, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable debug msg's for ignored packets.");

    ADD_CVAR_REF("net_debug_ack", debugAck_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable net ack debug msg's");

    ADD_CVAR_REF("net_debug_nack", debugNACk_, 1, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable net nack msg's");

    ADD_CVAR_REF("net_debug_socket", debugSockets_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable socket debug logging");

    del.Bind<NetVars, &NetVars::Var_OnDefaultTimeoutChanged>(this);

    ADD_CVAR_REF("net_default_timeout", defaultTimeoutMS_, 10000, 0, 10000000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "The default timeout for connections in MS")
        ->SetOnChangeCallback(del);

    ADD_CVAR_REF("net_rl_connections_per_ip", rlconnectionsPerIp_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Rate limits connection requests from the same ip");

    ADD_CVAR_REF("net_rl_connections_per_ip_threshold", rlconnectionsPerIpThreshMS_, 100, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Threshold in MS for applying connection rate limiting");

    ADD_CVAR_REF("net_rl_connections_per_ip_ban_time", rlconnectionsPerIpBanTimeMS_, 10000, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "The amount of time in MS the ip is banned for after exceeding threshold. 0=unlimited");

    ADD_CVAR_REF("net_partial_connection_timeout", dropPartialConnectionsMS_, 3000, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "The amount of time in MS before dropping a incomplete connection");

    del.Bind<NetVars, &NetVars::Var_OnPingTimeChanged>(this);

    ADD_CVAR_REF("net_ping_interval", pingTimeMS_, 1000, 0, 10000000, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Min amount of time between pings")
        ->SetOnChangeCallback(del);

    ADD_CVAR_REF("net_unreliable_timeout", unreliableTimeoutMS_, 0, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Time in MS before not sending a unreliable msg. 0=never");

    ADD_CVAR_REF("net_unexpected_msg_ban_time", unexpectedMsgBanTime_, 2000, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Time in MS to ban a client that sends a unexpected msg. eg: sending post connection msg before connecting. 0=never");

    ADD_CVAR_REF("net_connection_bandwidth_limit", connectionBSPLimit_, 0, 0, std::numeric_limits<int32_t>::max(), core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Limits outgoing bandwidth(BPS) per connection, once the limit is reached traffic is qeued. 0=unlimited");

    del.Bind<NetVars, &NetVars::Var_OnArtificalNetworkChanged>(this);

    // artifical ping / packet loss.
    ADD_CVAR_REF("net_art", artificalNetwork_, 0, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable artifical network. This just enabled vars like 'net_art_packet_loss'")
        ->SetOnChangeCallback(del);

    ADD_CVAR_REF("net_art_packet_loss", artificalPacketLoss_, 0.f, 0.f, 100.f, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Introduce artifical outgoing packet loss, percentage chance.");

    ADD_CVAR_REF("net_art_ping", artificalPing_, 0, 0, 999, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Introduce artifical ping, all messages will have a ping of atleast this value. 0=disabled");

    ADD_CVAR_REF("net_art_ping_variance", artificalPingVariance_, 0, 0, 999, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Introduce artifical ping variance");

    // why is this even a thing lol?
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

void NetVars::Var_OnDefaultTimeoutChanged(core::ICVar* pVar)
{
    // below ping time?
    int32_t val = pVar->GetInteger();

    if (val < pingTimeMS_) {
        X_WARNING("Net", "New default timeout time is shorter than ping time");
    }
}

void NetVars::Var_OnPingTimeChanged(core::ICVar* pVar)
{
    int32_t val = pVar->GetInteger();

    if (val > defaultTimeoutMS_) {
        X_WARNING("Net", "New min ping time is longer than default timeout time");
    }
}

void NetVars::Var_OnArtificalNetworkChanged(core::ICVar* pVar)
{
    int32_t val = pVar->GetInteger();
    if (val) {
        // warn that's it's enabled, hopefully it stops someone spending ages trying to debug some network issue.
        // to find artifical network is enabled hehhe.
        X_WARNING("Net", "Artifical network is enabled");
    }
}

X_NAMESPACE_END