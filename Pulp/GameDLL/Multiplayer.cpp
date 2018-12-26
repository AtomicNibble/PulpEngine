#include "stdafx.h"
#include "Multiplayer.h"
#include "UserNetMappings.h"

#include "Vars/GameVars.h"

#include <Containers/FixedBitStream.h>

#include <IFrameData.h>

X_NAMESPACE_BEGIN(game)


Multiplayer::Multiplayer(GameVars& vars) :
    vars_(vars),
    state_(GameState::NONE)
{
    
}

void Multiplayer::update(net::IPeer* pPeer, const UserNetMappings& unm)
{
    // get all the pings yo.
    // this should just map with players.
    // maybe we should wait for spawn.
    for (size_t i = 0; i < unm.lobbyUserGuids.size(); i++)
    {
        if (!unm.lobbyUserGuids[i].isValid()) {
            continue;
        }

        auto sysHandle = unm.sysHandles[i];
        if (sysHandle == net::INVALID_SYSTEM_HANDLE) {
            X_ASSERT(i == unm.localPlayerIdx, "Invalid system handle for none localy player")();
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

void Multiplayer::drawChat(engine::IPrimativeContext* pPrim)
{
    X_UNUSED(pPrim);

    if (chatLines_.isEmpty()) {
        return;
    }

    font::TextDrawContext con;
    con.col = Col_Whitesmoke;
    con.size = Vec2f(18.f, 18.f);
    con.effectId = 0;
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

        con.col.a = CHANTRAIT<uint8_t>::convert(a);

        pPrim->drawText(x, y, con, line.line.begin(), line.line.end());
        y += 20.f;
    }
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

void Multiplayer::addChatLine(core::string line)
{
    if (chatLines_.freeSpace() == 0) {
        chatLines_.pop();
    }

    chatLines_.emplace(line);
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
    float startY = 100.f;

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

    {
        // draw the row backgrounds
        float y = rowsRect.y1;

        for (int32_t i = 0; i < net::MAX_PLAYERS; i++)
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

        for (int32_t i = 0; i < net::MAX_PLAYERS; i++)
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

        for (int32_t i = 0; i < net::MAX_PLAYERS; i++)
        {
            Rectf row(rowsRect);
            row.y1 = y;
            row.y2 = y + rowHeight;

            y += rowHeight + rowPadding;

            // Name - Points - Kills - Headshots - Ping
            auto& ply = playerStates_[i];

            str.setFmt(" %-48s %-12" PRIi32 " %-12" PRIi32 " %-12" PRIi32 " %-12" PRIi32, "Stu", ply.points, ply.kills, ply.headshots, ply.ping);
            pPrim->drawText(row.x1, row.getCenter().y, con, str.begin(), str.end());
        }
    }
}

X_NAMESPACE_END