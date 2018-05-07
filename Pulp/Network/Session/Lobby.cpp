#include "stdafx.h"
#include "Lobby.h"
#include "SessionCallbacks.h"
#include "Vars\SessionVars.h"

#include <UserCmd.h>
#include <SnapShot.h>

#include <INetwork.h>

X_NAMESPACE_BEGIN(net)

void LobbyPeer::setConnectionState(ConnectionState::Enum state, core::MemoryArenaBase* arena)
{
    if (state == connectionState) {
        X_ERROR("Lobby", "Peer already in stat: \"%s\"", ConnectionState::ToString(state));
        return;
    }

    if (state == ConnectionState::Free)
    {
        if (state == ConnectionState::Established)
        {
            // bye bye..
        }

        pSnapMan.reset();
    }
    else if (state == ConnectionState::Pending)
    {
        X_ASSERT(connectionState == ConnectionState::Free, "Invalid peer state")(connectionState);

        pSnapMan = core::makeUnique<SnapshotManager>(arena, arena);

    }
    else if (state == ConnectionState::Established)
    {
        X_ASSERT(connectionState == ConnectionState::Pending, "Invalid peer state")(connectionState);

    }
    else
    {
        X_ASSERT_UNREACHABLE();
    }

    connectionState = state;
}

X_INLINE LobbyPeer::ConnectionState::Enum LobbyPeer::getConnectionState(void) const
{
    return connectionState;
}

X_INLINE bool LobbyPeer::isConnected(void) const
{
    return connectionState == ConnectionState::Established;
}

// ------------------------------------------------------

Lobby::Lobby(SessionVars& vars, ISessionCallbacks* pCallbacks, IPeer* pPeer, LobbyType::Enum type, core::MemoryArenaBase* arena) :
    vars_(vars),
    arena_(arena),
    pCallbacks_(X_ASSERT_NOT_NULL(pCallbacks)),
    pPeer_(pPeer),
    type_(type),
    users_(arena),
    peers_(arena)
{
    peers_.setGranularity(8);
    peers_.resize(8);

    state_ = LobbyState::Idle;

    isHost_ = false;
    finishedLoading_ = false;

    hostIdx_ = -1;
}

void Lobby::connectTo(SystemAddress address)
{
    // i would like to connect plz.
    shutdown();

    // Bittttttcoooonnneeeeeeeeectt!!!!
    auto delay = core::TimeVal::fromMS(vars_.connectionRetryDelayMs());

    auto res = pPeer_->connect(address, PasswordStr(), vars_.connectionAttemps(), delay);
    if (res != ConnectionAttemptResult::Started)
    {
        X_ERROR("Lobby", "Failed to connectTo: \"%s\"", ConnectionAttemptResult::ToString(res));
        setState(LobbyState::Error);
        return;
    }

    // add the address as a peer and mark as host.
    hostIdx_ = addPeer(address);
    X_ASSERT(peers_[hostIdx_].getConnectionState() == LobbyPeer::ConnectionState::Pending, "Invalid peer state")(peers_[hostIdx_].getConnectionState());


    setState(LobbyState::Connecting);
}


bool Lobby::handlePacket(Packet* pPacket)
{
    auto id = pPacket->getID();

    switch (id)
    {
        case MessageID::ConnectionRequestAccepted:
            handleConnectionAccepted(pPacket);
            break;

        case MessageID::ConnectionRequestHandShake:
            X_ASSERT(isHost(), "Recived connection handshake when not a host")(isHost());
            handleConnectionHandShake(pPacket);
            break;

        case MessageID::ConnectionRequestFailed:
        case MessageID::ConnectionBanned:
        case MessageID::ConnectionNoFreeSlots:
        case MessageID::ConnectionRateLimited:
        case MessageID::InvalidPassword:
            handleConnectionAttemptFailed(id);
            break;

        case MessageID::ConnectionLost:
        case MessageID::DisconnectNotification:
            handleConnectionLost(pPacket);
            break;

        default:
            break;
    }

    return false;
}

void Lobby::onReciveSnapShot(Packet* pPacket)
{
    // okay so this is a snap shot :O
    X_ASSERT(isPeer(), "Recived snapshot when not a peer")(isPeer());
    X_ASSERT(type_ == LobbyType::Game, "None game lobby recived snapshot")(type_);

    auto& hostPerr = peers_[hostIdx_];

    if (pPacket->guid != hostPerr.guid) {
        NetGuidStr str0, str1;
        X_ERROR("Lobby", "Recived snapshot was not from host peer. Packed: %s Host: %s", pPacket->guid.toString(str0), hostPerr.guid.toString(str1));
        return;
    }

    // TODO pass the snapshot in to the snapshot manager which will handle deltas from the host.
    // which we will then ACK.
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    SnapShot snap(arena_);
    snap.fromBitStream(bs);

    if (!bs.isEos()) {
        X_ERROR("Lobby", "Failed to read all of snapshot");
    }

    // now we need to just give this snapshot to someone o.o
    // i'm in the lobby :(
    // need a way back to session?
    pCallbacks_->onReciveSnapShot(std::move(snap));
}

void Lobby::handleConnectionAccepted(Packet* pPacket)
{
    X_ASSERT(isPeer(), "We should only be connecting to a server if a peer")(isPeer());
    X_ASSERT(pPacket->systemHandle != INVALID_SYSTEM_HANDLE, "invalid handle")(pPacket->systemHandle);

    if (state_ != LobbyState::Connecting)
    {
        X_ERROR("Lobby", "Recived connection accepted when not trying to connect. State: \"%s\"", LobbyState::ToString(state_));
        return;
    }

    // kinda need to know the fucking peer index here SLUT!
    // TODO not assume it's host
    auto& peer = peers_[hostIdx_];
    X_ASSERT(peer.getConnectionState() == LobbyPeer::ConnectionState::Pending, "Invalid peer state")(peer.getConnectionState());

    peer.setConnectionState(LobbyPeer::ConnectionState::Established, arena_);
    peer.systemHandle = pPacket->systemHandle;
    peer.guid = pPacket->guid;

    setState(LobbyState::Idle);
}

void Lobby::handleConnectionHandShake(Packet* pPacket)
{
    // the peer confirmed connnection with us.
    // what a nice little slut

    // but so far it's only passwed the network test / server password.
    // now lets enforce lobby requirements, like been sexy.

    // bitch plz.
    X_ASSERT(isHost(), "Recived connection hand shake when not host")(isHost());

    auto address = pPeer_->getAddressForHandle(pPacket->systemHandle);
    auto peerIdx = addPeer(address);
    
    IPStr strBuf;
    X_LOG0("Lobby", "Peer connected to \"%s\" lobby address \"%s\"", LobbyType::ToString(type_), gEnv->pNet->systemAddressToString(address, strBuf, true));

    auto& peer = peers_[peerIdx];
    peer.systemHandle = pPacket->systemHandle;
    peer.guid = pPacket->guid;
    peer.setConnectionState(LobbyPeer::ConnectionState::Established, arena_); // they have just confirmed connection.

    X_ASSERT(peer.getConnectionState() == LobbyPeer::ConnectionState::Established, "Unexpected connection state")(peer.getConnectionState());

    // sent a state msg?

}

void Lobby::handleConnectionAttemptFailed(MessageID::Enum id)
{
    if (state_ != LobbyState::Connecting)
    {
        X_ERROR("Lobby", "Recived connection failed when not trying to connect. State: \"%s\"", LobbyState::ToString(state_));
        return;
    }

    X_ERROR("Net", "Connection attempt failed: \"%s\"", MessageID::ToString(id));
        

    setState(LobbyState::Error);
}

void Lobby::handleConnectionLost(Packet* pPacket)
{
    if (isHost())
    {
        // bye!
        auto pPeer = findPeer(pPacket->systemHandle);
        if (!pPeer) {
            X_ERROR("Lobby", "Failed to find peer to remove");
            return;
        }

        X_ERROR("Lobby", "Peer lost connection");
        peers_.remove(pPeer);
    }
    else
    {
        // we lost connection to server :(
        X_ERROR("Lobby", "Lost connection to host");

        pCallbacks_->onLostConnectionToHost();
    }
}

void Lobby::sendUserCmd(const UserCmd& cmd)
{
    X_ASSERT(isPeer(), "Can only send user cmd if peer")(isPeer(), isHost());
    const auto& peer = peers_[hostIdx_];

    // do i wnat to compress this or something?
    core::FixedBitStreamStack<1500> bs;
    cmd.writeToBitStream(bs);

    pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::UnReliableSequenced, peer.systemHandle);
}

void Lobby::sendSnapShot(const SnapShot& snap)
{
    X_ASSERT(isHost(), "Can only send snapshot if host")(isPeer(), isHost());

    core::FixedBitStreamStack<1500> bs;
    bs.write(MessageID::SnapShot);
    snap.writeToBitStream(bs);

  //  X_ASSERT_NOT_IMPLEMENTED();

    for (auto& peer : peers_)
    {
        if (!peer.isConnected()) {
            continue;
        }

        if (!peer.loaded) {
      //      continue;
        }

        NetGuidStr str;
        X_LOG0("Lobby", "Sending snap to ", peer.guid.toString(str));

      // for now just send whole snap, later will need to build deltas and shit.
    // how do i know it's a snapshot tho?
        pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::UnReliableSequenced, peer.systemHandle);
      //  peer.pSnapMan->setStateSnap(snap);
    }
}

void Lobby::sendToHost(MessageID::Enum id)
{
    core::FixedBitStreamStack<32> bs;
    bs.write(id);

    sendToHost(bs.data(), bs.sizeInBytes());
}

void Lobby::sendToHost(const uint8_t* pData, size_t lengthInBytes)
{
    X_ASSERT(isPeer() && !isHost(), "Invalid operation")(isPeer(), isHost());
    const auto& peer = peers_[hostIdx_];

    pPeer_->send(pData, lengthInBytes, PacketPriority::High, PacketReliability::Reliable, peer.systemHandle);
}

void Lobby::sendToPeers(MessageID::Enum id)
{
    core::FixedBitStreamStack<32> bs;
    bs.write(id);

    sendToPeers(bs.data(), bs.sizeInBytes());
}

void Lobby::sendToPeers(const uint8_t* pData, size_t lengthInBytes)
{
    X_ASSERT(!isPeer() && isHost(), "Invalid operation")(isPeer(), isHost());
    
    for (auto& peer : peers_)
    {
        if (!peer.isConnected()) {
            continue;
        }

        pPeer_->send(pData, lengthInBytes, PacketPriority::High, PacketReliability::Reliable, peer.systemHandle);
    }
}


bool Lobby::handleState(void)
{
    switch (state_) {
        case LobbyState::Idle:
            return stateIdle();
        case LobbyState::Creating:
            return stateCreating();
        case LobbyState::Connecting:
            return stateConnecting();
        case LobbyState::Error:
            return false;

        default:
            X_NO_SWITCH_DEFAULT_ASSERT;
    }

    return false;
}


const LobbyPeer* Lobby::findPeer(SystemHandle handle) const
{
    auto it = std::find_if(peers_.begin(), peers_.end(), [handle](const LobbyPeer& p) { return p.systemHandle == handle; });
    if (it == peers_.end()) {
        return nullptr;
    }

    return it;
}

int32_t Lobby::addPeer(SystemAddress address)
{
    // X_ASSERT(findPeer(handle) == nullptr, "Peer already exsists")(handle);

    // find free idx.
    int32_t idx = -1;
    for(size_t i=0; i<peers_.size(); i++)
    {
        if (peers_[i].getConnectionState() == LobbyPeer::ConnectionState::Free)
        {
            idx = static_cast<int32_t>(i);
            break;
        }
    }

    // you not be trying to add a peer if no free slots.
    if (idx < 0) {
        X_ASSERT_UNREACHABLE();
        return idx;
    }

    auto& peer = peers_[idx];
    peer.systemHandle = INVALID_SYSTEM_HANDLE;
    peer.systemAddr = address;
    peer.guid = NetGUID();
    peer.setConnectionState(LobbyPeer::ConnectionState::Pending, arena_);

    return idx;
}

void Lobby::setState(LobbyState::Enum state)
{
    X_LOG0("Lobby", "State changed: \"%s\"", LobbyState::ToString(state));

    state_ = state;
}

void Lobby::startHosting(const MatchParameters& params)
{
    params_ = params;

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
    isHost_ = false;
    finishedLoading_ = false;

    hostIdx_ = -1;

    users_.clear();
    
    for (auto& peer : peers_)
    {
        if (peer.getConnectionState() != LobbyPeer::ConnectionState::Free) {
            peer.setConnectionState(LobbyPeer::ConnectionState::Free, arena_);
        }
    }

    setState(LobbyState::Idle);
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

bool Lobby::stateConnecting(void)
{
    // waiting for the response!
    // anything todo, tickle my poo?

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

bool Lobby::allPeersLoaded(void) const
{
    bool loaded = true;

    for (auto& peer : peers_)
    {
        if (!peer.isConnected()) {
            continue;
        }

        loaded &= peer.loaded;
    }

    return loaded;
}

X_NAMESPACE_END