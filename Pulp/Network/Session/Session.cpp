#include "stdafx.h"
#include "Session.h"


#include <I3DEngine.h>
#include <IPrimativeContext.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(net)

Lobby::Lobby(IPeer* pPeer, LobbyType::Enum type, core::MemoryArenaBase* arena) :
    pPeer_(pPeer),
    users_(arena),
    peers_(arena)
{
    type_ = type;
    state_ = LobbyState::Idle;

    isHost_ = false;
    finishedLoading_ = false;

}

void Lobby::sendUserCmd(const UserCmd& snap)
{
    X_ASSERT(isPeer(), "Can only send user cmd if peer")(isPeer(), isHost());

    // send it to the host!
    X_ASSERT_NOT_IMPLEMENTED();

}

void Lobby::sendSnapShot(const SnapShot& snap)
{
    X_ASSERT(isHost(), "Can only send snapshot if host")(isPeer(), isHost());

    for (auto& peer : peers_)
    {
        // send!
        X_UNUSED(peer);
        X_ASSERT_NOT_IMPLEMENTED();
    }
}


bool Lobby::handleState(void)
{

    switch (state_) {
        case LobbyState::Idle:
            return stateIdle();
        case LobbyState::Creating:
            return stateCreating();

        default:
            X_NO_SWITCH_DEFAULT_ASSERT;
    }

    return false;
}

void Lobby::setState(LobbyState::Enum state)
{
    state_ = state;
}

void Lobby::startHosting(const MatchParameters& parms)
{
    params_ = params_;

    shutdown();

    {
        // if we have a remote lobby, like on a server.
        // would need to dispatch request and wait.
        // for now just instant readdy.

    }

    setState(LobbyState::Creating);
}

void Lobby::finishedLoading(void)
{
    finishedLoading_ = true;
}

void Lobby::shutdown(void)
{


}

bool Lobby::stateIdle(void)
{
    // anything..?

    return true;
}

bool Lobby::stateCreating(void)
{
    // here we check if lobby has failed to create / finished.
    // for now it's instantly ready.

    // setup state of the new lobby.
    initStateLobbyHost();

    setState(LobbyState::Idle);
    return true;
}

void Lobby::initStateLobbyHost(void)
{
    // we are hosting
    isHost_ = true;


    // need to add myself to peer list.
    clearUsers();

    auto localGuid = pPeer_->getMyGUID();
    SystemAddress address;

    LobbyUser user;
    user.guid = localGuid;
    user.address = address;
    users_.emplace_back(user);

    hostAddress_ = address;

    // done?
}

void Lobby::clearUsers(void)
{
    users_.clear();
}

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------

Session::Session(IPeer* pPeer, core::MemoryArenaBase* arena) :
    pPeer_(X_ASSERT_NOT_NULL(pPeer)),
    arena_(X_ASSERT_NOT_NULL(arena)),
    partyLobby_(pPeer_, LobbyType::Party, arena),
    gameLobby_(pPeer_, LobbyType::Game, arena)
{

}

void Session::finishedLoading(void)
{
    // we finished loading.
    // noice.

    gameLobby_.finishedLoading();

    auto flags = gameLobby_.getMatchFlags();
    if (flags.IsSet(MatchFlag::Online))
    {
        if (gameLobby_.isHost())
        {
            setState(SessionState::InGame);
        }
        else
        {
            // TODO: send a packet to the host telling them we loaded.
            // also eat a pickle, while we wait. yum.
            X_ASSERT_NOT_IMPLEMENTED();
        }
    }
    else
    {
        setState(SessionState::InGame);
    }
}


void Session::quitToMenu(void)
{
    // TODO: leave any lobby.

    setState(SessionState::Idle);
}

void Session::createPartyLobby(const MatchParameters& parms)
{
    // host a new 'party' lobby.
    partyLobby_.startHosting(parms);

    setState(SessionState::CreateAndMoveToPartyLobby);
}

void Session::createMatch(const MatchParameters& parms)
{
    // host a new 'game'
    gameLobby_.startHosting(parms);

    setState(SessionState::CreateAndMoveToGameLobby);
}

void Session::startMatch(void)
{
    X_ASSERT(state_ == SessionState::GameLobbyHost, "Can't start match without been a game host")(state_);

    startLoading();
}

void Session::runUpdate(void)
{

    handleState();
}

void Session::sendUserCmd(const UserCmd& snap)
{
    gameLobby_.sendUserCmd(snap);
}

void Session::sendSnapShot(const SnapShot& snap)
{
    // too all peers.
    gameLobby_.sendSnapShot(snap);
}

bool Session::handleState(void)
{
    // so this is like the state machine for the game.
    // we have diffrent logic depending on your current state.
    pPeer_->runUpdate();

    switch (state_)
    {
        case SessionState::Idle:
            return stateIdle();
        case SessionState::CreateAndMoveToPartyLobby:
            return stateCreateAndMoveToPartyLobby();
        case SessionState::CreateAndMoveToGameLobby:
            return stateCreateAndMoveToGameLobby();
        case SessionState::PartyLobbyHost:
            return statePartyLobbyHost();
        case SessionState::PartyLobbyPeer:
            return statePartyLobbyPeer();
        case SessionState::GameLobbyHost:
            return stateGameLobbyHost();
        case SessionState::GameLobbyPeer:
            return stateGameLobbyPeer();
        case SessionState::Connecting:
            return stateConnecting();
        case SessionState::Loading:
            return stateLoading();
        case SessionState::InGame:
            return stateInGame();

        default:
            X_NO_SWITCH_DEFAULT_ASSERT;
    }

    return false;
}


void Session::setState(SessionState::Enum state)
{
    state_ = state;
}

bool Session::stateIdle(void)
{

    return true;
}

bool Session::stateConnecting(void)
{

    return true;
}

bool Session::stateCreateAndMoveToPartyLobby(void)
{
    // wait for the lobby to create.
    if (hasLobbyCreateCompleted(partyLobby_))
    {
        // Success
        setState(SessionState::PartyLobbyHost);

        return true;
    }

    return readPackets();
}

bool Session::stateCreateAndMoveToGameLobby(void)
{
    if (hasLobbyCreateCompleted(gameLobby_))
    {
     //   partyLobby_.sendMembersToLobby(gameLobby_);

        // Success
        setState(SessionState::GameLobbyHost);
        return true;
    }

    return true;
}

bool Session::statePartyLobbyHost(void)
{
    return readPackets();
}

bool Session::statePartyLobbyPeer(void)
{
    return readPackets();
}

bool Session::stateGameLobbyHost(void)
{
    return readPackets();
}

bool Session::stateGameLobbyPeer(void)
{
    return readPackets();
}

bool Session::stateLoading(void)
{
    // hurry the fuck up!
    if (!gameLobby_.hasFinishedLoading()) {
        return false;
    }

    // if not online, we should of switched out of this state if hasFinishedLoading().
    if (gameLobby_.getMatchFlags().IsSet(MatchFlag::Online)) {
        X_ASSERT_UNREACHABLE();
    }

    if (gameLobby_.isHost())
    {
        // TODO: wait for the plebs to finish loading.
        X_ASSERT_NOT_IMPLEMENTED();
        return false;
    }
    else
    {
        // we are a dirty pleb.
        // check the host is still around?
        // if not exit to menu with error.
        X_ASSERT_NOT_IMPLEMENTED();
        return false;
    }

    setState(SessionState::InGame);
    return true;
}

bool Session::stateInGame(void)
{
    // packets, packets, packets!
    return readPackets();
}


bool Session::hasLobbyCreateCompleted(Lobby& lobby)
{
    if (lobby.getState() == LobbyState::Idle) {
        return true;
    }

    // check for failed state.

    return false;
}

void Session::startLoading(void)
{
    // should only be called if we are the host.
    {
        // TODO: tell the connected plebs to start loading.

    }

    // Weeeeeeeeeeee!!
    setState(SessionState::Loading);
}

bool Session::readPackets(void)
{
    Packet* pPacket = nullptr;
    for (pPacket = pPeer_->receive(); pPacket; pPeer_->freePacket(pPacket), pPacket = pPeer_->receive())
    {
        X_LOG0("Session", "Recived packet: bitLength: %" PRIu32, pPacket->bitLength);

        core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

        switch (pPacket->getID())
        {
            case MessageID::ConnectionRequestAccepted:
            {
                X_LOG0("Session", "Connected to server");
                pleb_ = pPacket->systemHandle;
                break;
            }
            case MessageID::ChatMsg:
            {
                // it's a chat msg.
                static char buf[256] = { '\0' };

                core::zero_object(buf);

                auto len = bs.read<int16_t>();
                bs.read(buf, len);

                X_LOG0("Char", "Msg: %s", buf);

                auto* pPRim = gEnv->p3DEngine->getPrimContext(engine::PrimContext::PERSISTENT);
                font::TextDrawContext con;
                con.col = Col_Red;
                con.pFont = gEnv->pFontSys->GetDefault();

                Matrix33f ang = Matrix33f::createRotation(Vec3f(1.f, 0.f, 0.f), ::toRadians(-90.f));

                static int32_t num = 0;

                pPRim->drawText(Vec3f(0.f, 0.f, 80.f - (num * 18.f)), ang, con, buf);

                num++;
                break;
            }

            default:
                break;
        }

    }

    return true;
}


void Session::sendChatMsg(const char* pMsg)
{
    core::FixedBitStreamStack<256> bs;

    auto len = core::strUtil::strlen(pMsg);
    auto len16 = safe_static_cast<uint16_t>(len);

    bs.write(MessageID::ChatMsg);
    bs.write(len16);
    bs.write(pMsg, len);

    pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::Reliable, pleb_);

}

SessionStatus::Enum Session::getStatus(void) const
{
    static_assert(SessionState::ENUM_COUNT == 10, "Enums changed?");
    static_assert(SessionStatus::ENUM_COUNT == 6, "Enums changed?");

    switch(state_)
    {
        case SessionState::Idle:
            return SessionStatus::Idle;
        case SessionState::CreateAndMoveToPartyLobby:
            return SessionStatus::Connecting;
        case SessionState::CreateAndMoveToGameLobby:
            return SessionStatus::Connecting;
        case SessionState::PartyLobbyHost:
            return SessionStatus::PartyLobby;
        case SessionState::PartyLobbyPeer:
            return SessionStatus::PartyLobby;
        case SessionState::GameLobbyHost:
            return SessionStatus::GameLobby;
        case SessionState::GameLobbyPeer:
            return SessionStatus::GameLobby;
        case SessionState::Connecting:
            return SessionStatus::Connecting;
        case SessionState::Loading:
            return SessionStatus::Loading;
        case SessionState::InGame:
            return SessionStatus::InGame;
        default:
            X_NO_SWITCH_DEFAULT_ASSERT;
    }

    return SessionStatus::Idle;
}

X_NAMESPACE_END