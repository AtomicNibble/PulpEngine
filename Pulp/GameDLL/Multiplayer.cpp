#include "stdafx.h"
#include "Multiplayer.h"
#include "UserNetMappings.h"

#include <Containers/FixedBitStream.h>

#include <IFrameData.h>

X_NAMESPACE_BEGIN(game)


Multiplayer::Multiplayer() :
    state_(GameState::NONE)
{
    chatTime_ = core::TimeVal::fromMS(8000);
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
        if (pPeer->getPingInfo(sysHandle, pingInfo)) {
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

void Multiplayer::draw(core::FrameTimeData& time, engine::IPrimativeContext* pPrim)
{
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

    for (const auto& line : chatLines_)
    {
        // fade out last 10%?
        float percent = line.ellapsed.GetMilliSeconds() / chatTime_.GetMilliSeconds();

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

}

void Multiplayer::writeToSnapShot(core::FixedBitStreamBase& bs)
{
    bs.write(playerStates_.data(), playerStates_.size());

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

    while (chatLines_.isNotEmpty() && chatLines_.peek().ellapsed > chatTime_) {
        chatLines_.pop();
    }
}

X_NAMESPACE_END