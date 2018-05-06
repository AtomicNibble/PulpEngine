#include "stdafx.h"
#include "Session.h"

#include "Lobby.h"

#include <I3DEngine.h>
#include <IPrimativeContext.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(net)

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------

X_DISABLE_WARNING(4355) // 'this': used in base member initializer list

Session::Session(SessionVars& vars, IPeer* pPeer, core::MemoryArenaBase* arena) :
    vars_(vars),
    pPeer_(X_ASSERT_NOT_NULL(pPeer)),
    arena_(X_ASSERT_NOT_NULL(arena)),
    partyLobby_(vars_, this, pPeer_, LobbyType::Party, arena),
    gameLobby_(vars_, this, pPeer_, LobbyType::Game, arena)
{

    state_ = SessionState::Idle;

}

X_ENABLE_WARNING(4355)

void Session::update(void)
{
    X_PROFILE_BEGIN("Session", core::profiler::SubSys::NETWORK);

    // do a network tick.
    pPeer_->runUpdate();

    // potentially we have now recived packets.
    readPackets();

    partyLobby_.handleState();
    gameLobby_.handleState();

    handleState();
}


void Session::connect(SystemAddress address)
{
    // biiiiitcooooonnneeeecttt!!!!

    quitToMenu();

    // connect
    partyLobby_.connectTo(address);

    setState(SessionState::ConnectAndMoveToParty);
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
            gameLobby_.sendToHost(MessageID::LoadingDone);
        }
    }
    else
    {
        setState(SessionState::InGame);
    }
}


void Session::quitToMenu(void)
{
    if (state_ == SessionState::Idle) {
        return;
    }

    // TODO: leave any lobby.
    partyLobby_.shutdown();
    gameLobby_.shutdown();

    setState(SessionState::Idle);
}

void Session::createPartyLobby(const MatchParameters& params)
{
    X_ASSERT(state_ == SessionState::Idle, "Must be idle")(state_);

    // host a new 'party' lobby.
    partyLobby_.startHosting(params);

    setState(SessionState::CreateAndMoveToPartyLobby);
}

void Session::createMatch(const MatchParameters& params)
{
    // host a new 'game'
    gameLobby_.startHosting(params);

    setState(SessionState::CreateAndMoveToGameLobby);
}

void Session::startMatch(void)
{
    X_ASSERT(state_ == SessionState::GameLobbyHost, "Can't start match without been a game host")(state_);

    startLoading();
}

void Session::sendUserCmd(const UserCmd& snap)
{
    X_ASSERT(state_ == SessionState::InGame, "Should only send user cmd if in game")(state_);

    gameLobby_.sendUserCmd(snap);
}

void Session::sendSnapShot(const SnapShot& snap)
{
    X_ASSERT(state_ == SessionState::InGame, "Should only send snapshot if in game")(state_);

    // too all peers.
    gameLobby_.sendSnapShot(snap);
}

bool Session::handleState(void)
{
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
        case SessionState::ConnectAndMoveToParty:
            return stateConnectAndMoveToParty();
        case SessionState::ConnectAndMoveToGame:
            return stateConnectAndMoveToGame();
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
    if (state_ == state) {
        X_WARNING("Session", "redundant state change: \"%s\"", SessionState::ToString(state));
        return;
    }

    X_LOG0("Session", "State changed: \"%s\" -> \"%s\"", SessionState::ToString(state_), SessionState::ToString(state));

    state_ = state;
}

void Session::onLostConnectionToHost(void)
{
    // rip
    X_LOG0("Session", "Lost connection to host");

    quitToMenu();
}

bool Session::stateIdle(void)
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

    return true;
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
    return true;
}

bool Session::statePartyLobbyPeer(void)
{
    return true;
}

bool Session::stateGameLobbyHost(void)
{
    return true;
}

bool Session::stateGameLobbyPeer(void)
{
    return true;
}

bool Session::stateConnectAndMoveToParty(void)
{
    return handleConnectAndMoveToLobby(partyLobby_);
}

bool Session::stateConnectAndMoveToGame(void)
{
    return handleConnectAndMoveToLobby(gameLobby_);
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
        if (!gameLobby_.allPeersLoaded()) {
            return false;
        }
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
    return true;
}


bool Session::hasLobbyCreateCompleted(Lobby& lobby)
{
    if (lobby.getState() == LobbyState::Idle) {
        return true;
    }

    // check for failed state.

    return false;
}

bool Session::handleConnectAndMoveToLobby(Lobby& lobby)
{
    if (lobby.getState() == LobbyState::Error) {
        X_ERROR("Session", "Failed to connect to lobby");
        handleConnectionFailed(lobby);
        return true;
    }

    // busy.
    if (lobby.getState() != LobbyState::Idle) {
        return true;
    }

    X_LOG0("Session", "Connected to \"%s\" lobby", LobbyType::ToString(lobby.getType()));

    // we connected!
    switch (lobby.getType())
    {
        case LobbyType::Game:
            setState(SessionState::GameLobbyPeer);
            break;
        case LobbyType::Party:
            setState(SessionState::PartyLobbyPeer);
            break;
        default:
            X_NO_SWITCH_DEFAULT_ASSERT;
    }

    return true;
}

void Session::handleConnectionFailed(Lobby& lobby)
{
    X_ASSERT(state_ == SessionState::ConnectAndMoveToGame || state_ == SessionState::ConnectAndMoveToParty, "Unexpected state")();

    quitToMenu();
}

void Session::startLoading(void)
{
    // should only be called if we are the host.
    if(gameLobby_.getMatchFlags().IsSet(MatchFlag::Online))
    {
        X_ASSERT(gameLobby_.isHost(), "Cant start loading if we are not the host")(gameLobby_.isHost());
        
        gameLobby_.sendToPeers(MessageID::LoadingStart);
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

        // so we have party and game lobby.
        // need to know what todo.
        // kinda don't gethte point of seperate lobbies currently.
        // other than platform abstraction.
        // rip.

        auto msg = pPacket->getID();

        switch (msg)
        {
            case MessageID::AlreadyConnected:
            case MessageID::ConnectionLost:
            case MessageID::DisconnectNotification:
                sendPacketToLobby(pPacket);
                break; 

            case MessageID::ConnectionRequestFailed:
            case MessageID::ConnectionBanned:
            case MessageID::ConnectionNoFreeSlots:
            case MessageID::ConnectionRateLimited:
            case MessageID::InvalidPassword:
            {
                onConnectionFailure(pPacket);
                break;
            }

            case MessageID::ConnectionRequestHandShake:
            case MessageID::ConnectionRequestAccepted:
            {
                onConnectionFinalize(pPacket);
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

void Session::sendPacketToLobby(Packet* pPacket)
{
    // so the server accepted us.
    switch (state_)
    {
        case Potato::net::SessionState::PartyLobbyHost:
        case Potato::net::SessionState::PartyLobbyPeer:
           partyLobby_.handlePacket(pPacket);
            break;
        case Potato::net::SessionState::GameLobbyHost:
        case Potato::net::SessionState::GameLobbyPeer:
            break;
            gameLobby_.handlePacket(pPacket);
        default:
            X_ASSERT_UNREACHABLE();
            break;
    }
}


void Session::onConnectionFailure(Packet* pPacket)
{
    // so the server accepted us.
    if (state_ == SessionState::ConnectAndMoveToParty)
    {
        partyLobby_.handlePacket(pPacket);
    }
    else if (state_ == SessionState::ConnectAndMoveToGame)
    {
        gameLobby_.handlePacket(pPacket);
    }
    else
    {
        X_ASSERT_UNREACHABLE();
    }
}


void Session::onConnectionFinalize(Packet* pPacket)
{
    // so the server accepted us.
    if (state_ == SessionState::ConnectAndMoveToParty)
    {
        partyLobby_.handlePacket(pPacket);
    }
    else if (state_ == SessionState::ConnectAndMoveToGame)
    {
        gameLobby_.handlePacket(pPacket);
    }
    else
    {
        X_ASSERT_UNREACHABLE();
    }
}

void Session::sendChatMsg(const char* pMsg)
{
    core::FixedBitStreamStack<256> bs;

    auto len = core::strUtil::strlen(pMsg);
    auto len16 = safe_static_cast<uint16_t>(len);

    bs.write(MessageID::ChatMsg);
    bs.write(len16);
    bs.write(pMsg, len);

 //   pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::Reliable, pleb_);

}

const MatchParameters& Session::getMatchParams(void) const
{
    return gameLobby_.getMatchParams();
}


SessionStatus::Enum Session::getStatus(void) const
{
    static_assert(SessionState::ENUM_COUNT == 11, "Enums changed?");
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
        case SessionState::ConnectAndMoveToParty:
            return SessionStatus::Connecting;
        case SessionState::ConnectAndMoveToGame:
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