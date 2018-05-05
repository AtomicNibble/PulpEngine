#include "stdafx.h"
#include "Lobby.h"
#include "SessionCallbacks.h"

#include "UserCmd.h"
#include "Replication\SnapShot.h"

#include <INetwork.h>

X_NAMESPACE_BEGIN(net)


Lobby::Lobby(ISessionCallbacks* pCallbacks, IPeer* pPeer, LobbyType::Enum type, core::MemoryArenaBase* arena) :
    pCallbacks_(X_ASSERT_NOT_NULL(pCallbacks)),
    pPeer_(pPeer),
    type_(type),
    users_(arena),
    peers_(arena)
{
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
    auto res = pPeer_->connect(address, PasswordStr(), 3, core::TimeVal(0.5f));
    if (res != ConnectionAttemptResult::Started)
    {
        X_ERROR("Lobby", "Failed to connectTo: \"%s\"", ConnectionAttemptResult::ToString(res));
        setState(LobbyState::Error);
        return;
    }

    // add the address as a peer and mark as host.
    LobbyPeer peer;
    peer.systemAddr = address;

    auto idx = peers_.emplace_back(std::move(peer));

    hostIdx_ = safe_static_cast<decltype(hostIdx_)>(idx);

    setState(LobbyState::Connecting);
}


bool Lobby::handlePacket(Packet* pPacket)
{
    auto id = pPacket->getID();

    switch (id)
    {
        case MessageID::ConnectionRequestAccepted:
            handleConnectionAccepted(pPacket->systemHandle);
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

void Lobby::handleConnectionAccepted(SystemHandle handle)
{
    X_ASSERT(isPeer(), "We should only be connecting to a server if a peer")(isPeer());
    X_ASSERT(handle != INVALID_SYSTEM_HANDLE, "invalid handle")(handle);

    if (state_ != LobbyState::Connecting)
    {
        X_ERROR("Lobby", "Recived connection accepted when not trying to connect. State: \"%s\"", LobbyState::ToString(state_));
        return;
    }

    // kinda need to know the fucking peer index here SLUT!
    // TODO not assume it's host
    auto& peer = peers_[hostIdx_];

    peer.connectionState = LobbyPeer::ConnectionState::Established;
    peer.systemHandle = handle;

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

    auto& peer = addPeer(pPacket->systemHandle, pPacket->guid);

    X_ASSERT(peer.connectionState == LobbyPeer::ConnectionState::Established, "Unexpected connection state")(peer.connectionState);


    // sent a state msg?

}

void Lobby::handleConnectionAttemptFailed(MessageID::Enum id)
{
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
    //  bs.write(snap); // TODO: actually seralize.

    X_ASSERT_NOT_IMPLEMENTED();

    for (auto& peer : peers_)
    {
        if (!peer.loaded) {
            continue;
        }

        // can i just broadcast this?
        // no, since we will have per user delta.s
        pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::UnReliableSequenced, peer.systemHandle);
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

LobbyPeer& Lobby::addPeer(SystemHandle handle, NetGUID guid)
{
    X_ASSERT(findPeer(handle) == nullptr, "Peer already exsists")(handle);

    LobbyPeer peer;
    peer.systemHandle = handle;
    peer.systemAddr = pPeer_->getAddressForHandle(handle);
    peer.guid = guid;
    peer.connectionState = LobbyPeer::ConnectionState::Established;

    auto idx = peers_.emplace_back(std::move(peer));

    return peers_[idx];
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
    peers_.clear();

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
        loaded &= peer.loaded;
    }

    return loaded;
}

X_NAMESPACE_END