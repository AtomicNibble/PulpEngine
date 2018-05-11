#include "stdafx.h"
#include "Session.h"

#include "Lobby.h"
#include "Vars\SessionVars.h"

#include <I3DEngine.h>
#include <IPrimativeContext.h>
#include <IFont.h>
#include <ITimer.h>

X_NAMESPACE_BEGIN(net)

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------

X_DISABLE_WARNING(4355) // 'this': used in base member initializer list

Session::Session(SessionVars& vars, IPeer* pPeer, core::MemoryArenaBase* arena) :
    vars_(vars),
    pPeer_(X_ASSERT_NOT_NULL(pPeer)),
    arena_(X_ASSERT_NOT_NULL(arena)),
    lobbys_{ {
        {vars_, this, pPeer_, LobbyType::Party, arena},
        {vars_, this, pPeer_, LobbyType::Game, arena}
    }}
{
    X_ASSERT(lobbys_[LobbyType::Party].getType() == LobbyType::Party, "Incorrect type")();
    X_ASSERT(lobbys_[LobbyType::Game].getType() == LobbyType::Game, "Incorrect type")();

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

    lobbys_[LobbyType::Party].handleState();
    lobbys_[LobbyType::Game].handleState();

    handleState();
}


void Session::connect(SystemAddress address)
{
    // biiiiitcooooonnneeeecttt!!!!

    quitToMenu();

    // connect
#if 1
    lobbys_[LobbyType::Game].connectTo(address);
    setState(SessionState::ConnectAndMoveToGame);
#else
    lobbys_[LobbyType::Party].connectTo(address);
    setState(SessionState::ConnectAndMoveToParty);
#endif
}


void Session::finishedLoading(void)
{
    // we finished loading.
    // noice.

    lobbys_[LobbyType::Game].finishedLoading();

    auto flags = lobbys_[LobbyType::Game].getMatchFlags();
    if (flags.IsSet(MatchFlag::Online))
    {
        if (lobbys_[LobbyType::Game].isHost())
        {
            if (vars_.waitForPlayers())
            {
                X_LOG0("Session", "Not waiting for players to load");
            }
            else
            {
                setState(SessionState::InGame);
            }
        }
        else
        {
            lobbys_[LobbyType::Game].sendToHost(MessageID::LoadingDone);
        }
    }
    else
    {
        setState(SessionState::InGame);
    }
}

bool Session::hasFinishedLoading(void) const
{
    return lobbys_[LobbyType::Game].hasFinishedLoading();
}

void Session::quitToMenu(void)
{
    if (state_ == SessionState::Idle) {
        return;
    }

    for (int32_t i = 0; i < lobbys_.size(); i++)
    {
        lobbys_[i].reset();
    }

    setState(SessionState::Idle);
}

void Session::createPartyLobby(const MatchParameters& params)
{
    X_ASSERT(state_ == SessionState::Idle, "Must be idle")(state_);

    // host a new 'party' lobby.
    lobbys_[LobbyType::Party].startHosting(params);

    setState(SessionState::CreateAndMoveToPartyLobby);
}

void Session::createMatch(const MatchParameters& params)
{
    // host a new 'game'
    lobbys_[LobbyType::Game].startHosting(params);

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

    lobbys_[LobbyType::Game].sendUserCmd(snap);
}

void Session::sendSnapShot(SnapShot&& snap)
{
    X_ASSERT(state_ == SessionState::InGame, "Should only send snapshot if in game")(state_);

    // too all peers.
    lobbys_[LobbyType::Game].sendSnapShot(snap);
}

const SnapShot* Session::getSnapShot(void)
{
    if (recivedSnaps_.size() == 0) {
        return nullptr;
    }

    auto num = recivedSnaps_.size();

    return &recivedSnaps_[num - 1];
}

const ILobby* Session::getLobby(LobbyType::Enum type) const
{
    return &lobbys_[type];
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

void Session::onReciveSnapShot(SnapShot&& snap)
{
    X_UNUSED(snap);

    auto time = gEnv->pTimer->GetTimeNowReal();

    snap.setTime(time);

    recivedSnaps_.emplace_back(std::move(snap));

    // process the snap shot.
    // not sure where i want to put this logic.
    // i think it been in the network session is fine.
    // rather than core, since snapshots are 'network' related.
    // core should not care if you have a networked game or not.
}


// --------------------------------------

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

// --------------------------------------

bool Session::stateIdle(void)
{

    return true;
}

bool Session::stateCreateAndMoveToPartyLobby(void)
{
    // wait for the lobby to create.
    if (hasLobbyCreateCompleted(lobbys_[LobbyType::Party]))
    {
        // Success
        setState(SessionState::PartyLobbyHost);

        return true;
    }

    return true;
}

bool Session::stateCreateAndMoveToGameLobby(void)
{
    if (hasLobbyCreateCompleted(lobbys_[LobbyType::Game]))
    {
     //   lobbys_[LobbyType::Party].sendMembersToLobby(lobbys_[LobbyType::Game]);

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
    // click start plz.

    return true;
}

bool Session::stateGameLobbyPeer(void)
{
    // start the game already!
    // need a way for the lobby to tell me to start loading.
    // the lobby needs to handle the packet, to check it actually came from the host, no bad peers plz!
    auto& lobby = lobbys_[LobbyType::Game];

    X_ASSERT(lobby.isPeer(), "Should be game lobby peer")(lobby.isPeer(), lobby.isHost());

    if (lobby.shouldStartLoading())
    {
        lobby.beganLoading();

        setState(SessionState::Loading);
    }

    return true;
}

bool Session::stateConnectAndMoveToParty(void)
{
    return handleConnectAndMoveToLobby(lobbys_[LobbyType::Party]);
}

bool Session::stateConnectAndMoveToGame(void)
{
    return handleConnectAndMoveToLobby(lobbys_[LobbyType::Game]);
}

bool Session::stateLoading(void)
{
    // hurry the fuck up!
    auto& gameLobby = lobbys_[LobbyType::Game];

    if (!gameLobby.hasFinishedLoading()) {
        return false;
    }

    // if not online, we should of switched out of this state if hasFinishedLoading().
    if (!gameLobby.getMatchFlags().IsSet(MatchFlag::Online)) {
        X_ASSERT_UNREACHABLE();
    }

    if (gameLobby.isHost())
    {
        if (!gameLobby.allPeersLoaded()) {
            X_LOG0("Session", "Waiting for peers to load..");
            return false;
        }

        X_LOG0("Session", "All peers loaded...");
    }
    else
    {
        // we are a dirty pleb.
        // check the host is still around?
        if (gameLobby.getNumConnectedPeers() < 1)
        {
            X_ERROR("Session", "No peers");
            quitToMenu();
            return false;
        }

     //   return false;
    }

    setState(SessionState::InGame);
    return true;
}

bool Session::stateInGame(void)
{
    // packets, packets, packets!
    return true;
}


// --------------------------------------

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
    if(lobbys_[LobbyType::Game].getMatchFlags().IsSet(MatchFlag::Online))
    {
        X_ASSERT(lobbys_[LobbyType::Game].isHost(), "Cant start loading if we are not the host")(lobbys_[LobbyType::Game].isHost());
        
        lobbys_[LobbyType::Game].sendToPeers(MessageID::LoadingStart);
    }

    // Weeeeeeeeeeee!!
    setState(SessionState::Loading);
}

bool Session::readPackets(void)
{
    Packet* pPacket = nullptr;
    for (pPacket = pPeer_->receive(); pPacket; pPeer_->freePacket(pPacket), pPacket = pPeer_->receive())
    {
      //  X_LOG0("Session", "Recived packet: bitLength: %" PRIu32, pPacket->bitLength);

        core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

        // so we have party and game lobby.
        // need to know what todo.
        // kinda don't gethte point of seperate lobbies currently.
        // other than platform abstraction.
        // rip.

        auto msg = pPacket->getID();

        switch (msg)
        {
            case MessageID::SnapShot:
                onReciveSnapShot(pPacket);
                break;

            case MessageID::AlreadyConnected:
            case MessageID::ConnectionLost:
            case MessageID::DisconnectNotification:

            case MessageID::ConnectionRequestFailed:
            case MessageID::ConnectionBanned:
            case MessageID::ConnectionNoFreeSlots:
            case MessageID::ConnectionRateLimited:
            case MessageID::InvalidPassword:

            case MessageID::ConnectionRequestHandShake:
            case MessageID::ConnectionRequestAccepted:

            case MessageID::LobbyJoinRequest:
            case MessageID::LobbyJoinAccepted:
            case MessageID::LobbyJoinNoFreeSlots:
            case MessageID::LobbyUsersConnected:
            case MessageID::LobbyUsersDiconnected:
            case MessageID::LobbyGameParams:

            case MessageID::LoadingStart:
            case MessageID::LoadingDone:
            case MessageID::InGame:
                sendPacketToLobby(pPacket);
                break; 
          
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
                X_ERROR("Session", "Unhandled message: %s", MessageID::ToString(msg));
                break;
        }

    }

    return true;
}

void Session::onReciveSnapShot(Packet* pPacket)
{
#if 0 // TODO
    X_ASSERT(state_ == SessionState::InGame, "Recived snap shot when not in game")(state_);
#endif

    lobbys_[LobbyType::Game].onReciveSnapShot(pPacket);
}

void Session::sendPacketToLobby(Packet* pPacket)
{
    // so the server accepted us.
    switch (state_)
    {
        case SessionState::ConnectAndMoveToParty:
        case SessionState::PartyLobbyHost:
        case SessionState::PartyLobbyPeer:
           lobbys_[LobbyType::Party].handlePacket(pPacket);
            break;

        case SessionState::ConnectAndMoveToGame:
        case SessionState::GameLobbyHost:
        case SessionState::GameLobbyPeer:
        case SessionState::Loading:
        case SessionState::InGame:
            lobbys_[LobbyType::Game].handlePacket(pPacket);
            break;
        default:
            X_ERROR("Session", "Unhandle state: %s", SessionState::ToString(state_));
            X_ASSERT_UNREACHABLE();
            break;
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

bool Session::isHost(void) const
{
    // not sure what states I will allowthis to be caleld form yet.
    //X_ASSERT(getStatus() == SessionStatus::InGame, "Unexpected status")(getStatus());

    return lobbys_[LobbyType::Game].isHost();
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

const MatchParameters& Session::getMatchParams(void) const
{
    return lobbys_[LobbyType::Game].getMatchParams();
}

void Session::drawDebug(engine::IPrimativeContext* pPrim) const
{
    X_UNUSED(pPrim);

    // draw me some shit.
    Vec2f base0(5.f, 120.f);

    base0.y += lobbys_[LobbyType::Party].drawDebug(base0, pPrim).y;
    base0.y += 20.f;
    lobbys_[LobbyType::Game].drawDebug(base0, pPrim);
}


X_NAMESPACE_END