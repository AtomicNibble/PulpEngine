#include "stdafx.h"
#include "Multiplayer.h"
#include "UserNetMappings.h"

#include "Vars/GameVars.h"

#include <Containers/FixedBitStream.h>
#include <String/Format.h>

#include <IFrameData.h>
#include <Ilocalisation.h>


X_NAMESPACE_BEGIN(game)

using namespace core::Literals;

namespace
{

    float getAlpha(float percent)
    {
        // this really best way :S ?
        const float fadeStart = 0.85f;

        float a = 1.f;
        if (percent >= fadeStart) {
            a = (percent - fadeStart);
            a = a / (1.f - fadeStart);
            a = 1.f - a;
            if (a < 0.f) {
                a = 0.f;
            }
        }

        return a;
    }

} // namespace


Multiplayer::Multiplayer(GameVars& vars, const UserNetMappings& netMappings, net::ISession* pSession) :
    vars_(vars),
    netMappings_(netMappings),
    pSession_(pSession),
    state_(GameState::NONE)
{
    
}

void Multiplayer::update(net::IPeer* pPeer)
{
    // get all the pings yo.
    // this should just map with players.
    // maybe we should wait for spawn.
    for (int32_t i = 0; i < static_cast<int32_t>(netMappings_.lobbyUserGuids.size()); i++)
    {
        if (!netMappings_.lobbyUserGuids[i].isValid()) {
            continue;
        }

#if 0
        playerStates_[i].points++;

        if (playerStates_[i].points % 16 == 0) {
            playerStates_[i].kills++;
        }

        if (playerStates_[i].points % 32 == 0) {
            playerStates_[i].headshots++;
        }
#endif

        // what's the ping of the local player?

        // remote peer pings
        auto sysHandle = netMappings_.sysHandles[i];
        if (sysHandle == net::INVALID_SYSTEM_HANDLE) {
            X_ASSERT(i == netMappings_.localPlayerIdx, "Invalid system handle for none localy player")();
            continue;
        }

        net::PingInfo pingInfo;
        if (!pPeer->getPingInfo(sysHandle, pingInfo)) {
            continue;
        }

        playerStates_[i].ping = pingInfo.avg;
    }

    switch (state_)
    {
        case GameState::GAME:
            break;

        case GameState::SCOREBOARD:
            break;
    }
}

void Multiplayer::drawChat(core::FrameTimeData& time, engine::IPrimativeContext* pPrim)
{
    // TODO: maybe seperate this out.
    updateChat(time.deltas[core::Timer::UI]);

    drawChat(pPrim);
}

void Multiplayer::drawEvents(core::FrameTimeData& time, engine::IPrimativeContext* pPrim)
{
    // TODO: maybe seperate this out.
    updateEvents(time.deltas[core::Timer::UI]);

    drawEvents(pPrim);
}

void Multiplayer::readFromSnapShot(core::FixedBitStreamBase& bs)
{
    bs.read(playerStates_.data(), playerStates_.size());
    bs.read(state_);
}

void Multiplayer::writeToSnapShot(core::FixedBitStreamBase& bs)
{
    // TODO: variable length encode player score and shit?
    bs.write(playerStates_.data(), playerStates_.size());
    bs.write(state_);
}

void Multiplayer::playerSpawned(int32_t localIndex)
{
    // hellow you little shit!
    const auto& netGuid = netMappings_.lobbyUserGuids[localIndex];
    if (!netGuid.isValid()) {
        X_ERROR("Game", "Spawned players guid is not valid: %" PRIi32, localIndex);
    }

    // fucking goat muncher!
    auto* pLobby = pSession_->getLobby(net::LobbyType::Game);

    net::UserInfo info;
    if (!pLobby->getUserInfoForGuid(netGuid, info)) {
        // oh dear, tut tut.
        X_WARNING("Game", "Failed to get name for spawned player for index: %" PRIi32, localIndex);
    }

    auto fmt = gEnv->pLocalisation->getString("#str_game_ply_joined"_strhash);

    // mmm.
    core::StackString256 str;
    core::format::format_to(str, fmt, info.name);

    addEventLine(core::string_view(str.begin(), str.length()));
}

void Multiplayer::playerDeath(int32_t playerIdx, entity::EntityId attacker)
{
    playerStates_[playerIdx].deaths++;

    if (attacker < net::MAX_PLAYERS) {
        playerStates_[attacker].kills++;
    }

    // kill yourself?
    if (playerIdx == attacker) {
        postEvent(Event::PLY_DIED, playerIdx, attacker);
    }
    else {
        postEvent(Event::PLY_KILLED, playerIdx, attacker);
    }
}

void Multiplayer::playerLeft(int32_t localIndex)
{
    postEvent(Event::PLY_LEFT, localIndex, 0);
}

void Multiplayer::handleChatMsg(core::string_view name, core::string_view msg)
{
    X_ASSERT(pSession_->isHost(), "Should only be called on host")();

    addChatLine(name, msg);

    // send to peers.
    ChatPacketBs bs;
    buildChatPacket(bs, name, msg);

    auto* pLobby = pSession_->getLobby(net::LobbyType::Game);
    pLobby->sendToPeers(bs);
}

void Multiplayer::addChatLine(core::string_view name, core::string_view msg)
{
    if (chatLines_.freeSpace() == 0) {
        chatLines_.pop();
    }

    core::StackString256 str;
    str.setFmt("%.*s: %.*s", name.length(), name.data(), msg.length(), msg.data());

    chatLines_.emplace(core::string_view(str.begin(), str.length()));
}

void Multiplayer::addEventLine(core::string_view line)
{
    if (eventLines_.freeSpace() == 0) {
        eventLines_.pop();
    }

    eventLines_.emplace(line);
}

void Multiplayer::drawChat(engine::IPrimativeContext* pPrim)
{
    if (chatLines_.isEmpty()) {
        return;
    }

    font::TextDrawContext con;
    con.col = Col_Whitesmoke;
    con.size = Vec2f(18.f, 18.f);
    con.effectId = 1;
    con.pFont = gEnv->pFontSys->getDefault();

    float height = 20.f;

    // TODO: select center?
    float x = 5.f;
    float y = 500.f - (height * chatLines_.size());

    auto chatTime = core::TimeVal::fromMS(vars_.chatMsgLifeMS());

    for (const auto& line : chatLines_)
    {
        // fade out last 10%?
        float percent = line.ellapsed.GetMilliSeconds() / chatTime.GetMilliSeconds();

        con.col.a = CHANTRAIT<uint8_t>::convert(getAlpha(percent));

        pPrim->drawText(x, y, con, line.line.begin(), line.line.end());
        y += 20.f;
    }
}

void Multiplayer::drawEvents(engine::IPrimativeContext* pPrim)
{
    if (eventLines_.isEmpty()) {
        return;
    }

    font::TextDrawContext con;
    con.col = Col_Whitesmoke;
    con.size = Vec2f(18.f, 18.f);
    con.effectId = 1;
    con.pFont = gEnv->pFontSys->getDefault();

    float height = 20.f;

    // TODO: select center?
    float x = 5.f;
    float y = 920.f - (height * eventLines_.size());

    auto chatTime = core::TimeVal::fromMS(vars_.chatMsgLifeMS());

    for (const auto& line : eventLines_)
    {
        // fade out last 10%?
        float percent = line.ellapsed.GetMilliSeconds() / chatTime.GetMilliSeconds();

        con.col.a = CHANTRAIT<uint8_t>::convert(getAlpha(percent));

        pPrim->drawText(x, y, con, line.line.begin(), line.line.end());
        y += 20.f;
    }
}

void Multiplayer::updateChat(core::TimeVal dt)
{
    if (chatLines_.isEmpty()) {
        return;
    }

    // fade lines.
    for (auto& line : chatLines_) {
        line.ellapsed += dt;
    }

    auto chatTime = core::TimeVal::fromMS(vars_.chatMsgLifeMS());

    while (chatLines_.isNotEmpty() && chatLines_.peek().ellapsed > chatTime) {
        chatLines_.pop();
    }
}

void Multiplayer::updateEvents(core::TimeVal dt)
{
    if (eventLines_.isEmpty()) {
        return;
    }

    // fade lines.
    for (auto& line : eventLines_) {
        line.ellapsed += dt;
    }

    auto chatTime = core::TimeVal::fromMS(vars_.chatMsgLifeMS());

    while (eventLines_.isNotEmpty() && eventLines_.peek().ellapsed > chatTime) {
        eventLines_.pop();
    }
}

void Multiplayer::handleEvent(core::FixedBitStreamBase& bs)
{
    auto evt = bs.read<Event::Enum>();
    auto param0 = bs.read<uint32_t>();
    auto param1 = bs.read<uint32_t>();

    postEvent(evt, param0, param1);
}

void Multiplayer::postEvent(Event::Enum evt, int32_t param0, int32_t param1)
{
    X_UNUSED(param0, param1);

    switch (evt)
    {
        case Event::PLY_LEFT: {
            const auto& guid = netMappings_.lobbyUserGuids[param0];
            
            auto* pLobby = pSession_->getLobby(net::LobbyType::Game);

            core::string_view name = pLobby->getUserNameForGuid(guid);

            auto fmt = gEnv->pLocalisation->getString("#str_game_ply_left"_strhash);

            core::StackString256 str;
            core::format::format_to(str, fmt, name);

            addEventLine(core::string_view(str.begin(), str.length()));
            break;
        }

        case Event::PLY_DIED: {


            break;
        }
        case Event::PLY_KILLED: {
            const auto& plyGuid = netMappings_.lobbyUserGuids[param0];
            const auto& KillerGuid = netMappings_.lobbyUserGuids[param1];

            auto* pLobby = pSession_->getLobby(net::LobbyType::Game);

            core::string_view diedName = pLobby->getUserNameForGuid(plyGuid);
            core::string_view killerName = pLobby->getUserNameForGuid(KillerGuid);

            auto fmt = gEnv->pLocalisation->getString("#str_game_ply_killed"_strhash);

            core::StackString256 str;
            core::format::format_to(str, fmt, diedName, killerName);

            addEventLine(core::string_view(str.begin(), str.length()));
            break;
        }

        default:
            X_ASSERT_UNREACHABLE();
            return;
    }

    if (pSession_->isHost())
    {
        EventPacketBs bs;
        bs.write(net::MessageID::GameEvent);
        bs.write(evt);
        bs.write(param0);
        bs.write(param1);

        auto* pLobby = pSession_->getLobby(net::LobbyType::Game);
        pLobby->sendToPeers(bs);
    }
}

void Multiplayer::buildChatPacket(ChatPacketBs& bs, core::string_view name, core::string_view msg)
{
    const uint8_t nameLen = safe_static_cast<uint8_t>(core::Min<size_t>(net::MAX_USERNAME_LEN, name.length()));
    const uint8_t msgLen = safe_static_cast<uint8_t>(core::Min<size_t>(net::MAX_CHAT_MSG_LEN, msg.length()));

    bs.write(net::MessageID::GameChatMsg);
    bs.write(nameLen);
    bs.write(name.data(), nameLen);
    bs.write(msgLen);
    bs.write(msg.data(), msgLen);
}

void Multiplayer::drawLeaderboard(engine::IPrimativeContext* pPrim)
{
    // want some rows that are fixed size maybe?
    // but centered in srreen.
    // think this is something we should just scale based on res.
    // 800 X 600 as base.

    constexpr float baseWidth = 600.f;

    constexpr float screenWidth = 1680;
    // constexpr float screenHeight = 1050.f;

    constexpr float scale = screenWidth / baseWidth;

    constexpr float titleHeight = 30.f;
    constexpr float rowWidth = 480.f * scale;
    constexpr float rowHeight = 16.f * scale;
    constexpr float rowPadding = 4.f * scale;
    constexpr float padding = 8.f;

    float rowsHeight = ((rowHeight + rowPadding) * net::MAX_PLAYERS) - rowPadding;
    float height = padding + titleHeight + padding + rowsHeight + padding;
    float width = rowWidth + (padding * 2);

    float startX = (screenWidth - width) * 0.5f;
    float startY = 160.f;

    Rectf r(
        startX,
        startY,
        startX + width,
        startY + height
    );

    Rectf titleRect(
        startX + padding,
        startY + padding,
        startX + padding + rowWidth,
        startY + padding + titleHeight
    );

    Rectf rowsRect(
        startX + padding,
        titleRect.y2 + padding,
        startX + padding + rowWidth,
        titleRect.y2 + padding + rowsHeight
    );

    // temp maybe?^
 //   pPrim->drawQuad(r, Color8u(100, 100, 100, 128));
    pPrim->drawQuad(titleRect, Color8u(80, 20, 20, 128));
 //   pPrim->drawQuad(rowsRect, Color8u(20,20,20,128));

    struct LeaderBoardInfo
    {
        PlayerState ps;
        core::StackString<net::MAX_USERNAME_LEN> name;
    };

    core::FixedArray<LeaderBoardInfo, net::MAX_PLAYERS> activeStates;

    auto* pLobby = pSession_->getLobby(net::LobbyType::Game);

    for (int32_t i = 0; i < net::MAX_PLAYERS; i++)
    {
        if (!netMappings_.lobbyUserGuids[i].isValid()) {
            continue;
        }

        LeaderBoardInfo lbi;
        lbi.ps = playerStates_[i];

        net::UserInfo info;
        if (pLobby->getUserInfoForGuid(netMappings_.lobbyUserGuids[i], info)) {
            lbi.name.append(info.name.data(), info.name.length());
        }
        else {
            lbi.name.set("<error>");
        }

        activeStates.emplace_back(lbi);
    }

    {
        // draw the row backgrounds
        float y = rowsRect.y1;

        for (size_t i = 0; i < activeStates.size(); i++)
        {
            Rectf row(rowsRect);
            row.y1 = y;
            row.y2 = y + rowHeight;

            y += rowHeight + rowPadding;

            pPrim->drawQuad(row, Color8u(10, 10, 10, 192));
        }
    }

    {
        // draw the row borders
        float y = rowsRect.y1;

        for (size_t i = 0; i < activeStates.size(); i++)
        {
            Rectf row(rowsRect);
            row.y1 = y;
            row.y2 = y + rowHeight;

            y += rowHeight + rowPadding;

            pPrim->drawRect(row, Color8u(60, 60, 60, 10));
        }
    }

    // Draw text now.
    font::TextDrawContext con;
    con.col = Col_Whitesmoke;
    con.size = Vec2f(24.f, 24.f);
    con.effectId = 0;
    con.pFont = gEnv->pFontSys->getDefault();


    core::StackString512 str;
    str.setFmt(" %-48s %-12s %-12s %-12s %-12s", "Name", "Points", "Kills", "HeadShots", "Ping");

    // Title
    con.effectId = 1;
    pPrim->drawText(titleRect.x1, titleRect.y1, con, str.begin(), str.end());

    con.effectId = 0;

    {
        float y = rowsRect.y1;

        con.flags.Set(font::DrawTextFlag::CENTER_VER);

        for (auto& lbi : activeStates)
        {
            Rectf row(rowsRect);
            row.y1 = y;
            row.y2 = y + rowHeight;

            y += rowHeight + rowPadding;

            // Name - Points - Kills - Headshots - Ping
            auto ply = lbi.ps;

            str.setFmt(" %-48s %-12" PRIi32 " %-12" PRIi32 " %-12" PRIi32 " %-12" PRIi32, lbi.name.c_str(), ply.points, ply.kills, ply.headshots, ply.ping);
            pPrim->drawText(row.x1, row.getCenter().y, con, str.begin(), str.end());
        }
    }
}

X_NAMESPACE_END