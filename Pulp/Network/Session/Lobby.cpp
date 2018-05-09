#include "stdafx.h"
#include "Lobby.h"
#include "SessionCallbacks.h"
#include "Vars\SessionVars.h"

#include <UserCmd.h>
#include <SnapShot.h>

#include <ITimer.h>
#include <INetwork.h>

#include <Platform\SystemInfo.h>
#include <Util\Process.h>

// debug drawing
#include <IPrimativeContext.h>
#include <IFont.h>

X_NAMESPACE_BEGIN(net)

void MatchParameters::writeToBitStream(core::FixedBitStreamBase& bs) const
{
    bs.write(numSlots);
    bs.write(flags);
    bs.write(mode);
    bs.write(mapName);
}

void MatchParameters::fromBitStream(core::FixedBitStreamBase& bs)
{
    bs.read(numSlots);
    bs.read(flags);
    bs.read(mode);
    bs.read(mapName);
}

// ------------------------------------------------------------

LobbyUser::LobbyUser()
{
    peerIdx = -1;
    username.set("Stu");
}

void LobbyUser::writeToBitStream(core::FixedBitStreamBase& bs) const
{
    bs.write(guid);
    bs.write(address);
    bs.write(peerIdx);
    bs.write(username);
}

void LobbyUser::fromBitStream(core::FixedBitStreamBase& bs)
{
    bs.read(guid);
    bs.read(address);
    bs.read(peerIdx);
    bs.read(username);
}

// ------------------------------------------------------------

void LobbyPeer::reset(void)
{
    connectionState = ConnectionState::Free;

    loaded = false;
    inGame = false;

    snapHz = 0.f;
    numSnapsSent = 0;
    pSnapMan.reset();

    systemHandle = INVALID_SYSTEM_HANDLE;
    systemAddr = SystemAddress();
    guid = NetGUID();
}

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

        reset();
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

    stateChangeTime = gEnv->pTimer->GetTimeNowReal();
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
    peers_.setGranularity(MAX_PLAYERS);
    peers_.resize(MAX_PLAYERS);
    users_.setGranularity(MAX_PLAYERS);
    users_.reserve(MAX_PLAYERS);

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
        case MessageID::LobbyJoinNoFreeSlots:
            handleConnectionAttemptFailed(id);
            break;

        case MessageID::ConnectionLost:
        case MessageID::DisconnectNotification:
            handleConnectionLost(pPacket);
            break;

        case MessageID::LobbyJoinRequest:
            handleLobbyJoinRequest(pPacket);
            break;
        case MessageID::LobbyJoinAccepted:
            handleLobbyJoinAccepted(pPacket);
            break;
        case MessageID::LobbyUsersConnected:
            handleLobbyUsersConnected(pPacket);
            break;
        case MessageID::LobbyUsersDiconnected:
            handleLobbyUsersDiconnected(pPacket);
            break;
        case MessageID::LobbyGameParams:
            handleLobbyGameParams(pPacket);
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

    if (state_ != LobbyState::Connecting)
    {
        X_ERROR("Lobby", "Recived connection accepted when not trying to connect. State: \"%s\"", LobbyState::ToString(state_));
        return;
    }

    // check it's the host?
    X_ASSERT(hostIdx_ != -1, "Hostidx invalid")(hostIdx_);

    // how can i know this is the host?
    // guess by system address?
    auto& peer = peers_[hostIdx_];
    X_ASSERT(peer.getConnectionState() == LobbyPeer::ConnectionState::Pending, "Invalid peer state")(peer.getConnectionState());
    peer.systemHandle = pPacket->systemHandle;
    peer.guid = pPacket->guid;

    addLocalUsers();

    // ask to join the lobby.
    core::FixedBitStreamStack<0x200> bs;
    bs.write(MessageID::LobbyJoinRequest);

    addUsersToBs(bs);
    pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::Reliable, pPacket->systemHandle);

    // todo wait for reponse.
    setState(LobbyState::Joining);
}


void Lobby::handleConnectionHandShake(Packet* pPacket)
{
    // the peer confirmed connnection with us.
    // what a nice little slut.
    X_ASSERT(isHost(), "Recived connection hand shake when not host")(isHost());

    if (isFull()) {
       X_WARNING("Lobby", "Rejected peer, looby is full");
        
        core::FixedBitStreamStack<32> bs;
        bs.write(MessageID::LobbyJoinNoFreeSlots);
        pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::Reliable, pPacket->systemHandle);
        return;
    }
  
    auto address = pPeer_->getAddressForHandle(pPacket->systemHandle);
    auto peerIdx = addPeer(address);
    
    IPStr strBuf;
    X_LOG0("Lobby", "Peer connected to \"%s\" lobby address \"%s\"", LobbyType::ToString(type_), gEnv->pNet->systemAddressToString(address, strBuf, true));

    auto& peer = peers_[peerIdx];
    peer.systemHandle = pPacket->systemHandle;
    peer.guid = pPacket->guid;
 
    // we now wait for a 'LobbyJoinRequest' or timeout.
    X_ASSERT(peer.getConnectionState() == LobbyPeer::ConnectionState::Pending, "Unexpected connection state")(peer.getConnectionState());
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
        auto peerIdx = findPeerIdx(pPacket->systemHandle);
        if (peerIdx < 0) {
            X_ERROR("Lobby", "Failed to find peer to remove");
            return;
        }

        X_ERROR("Lobby", "Peer lost connection");

        // need to remove users for this peer also.
        // basically any time we disconnect a peer we need to tell the other peers, rip
        for (auto it = users_.begin(); it != users_.end();)
        {
            if (it->peerIdx == peerIdx) {
                it = users_.erase(it);
            }
            else {
                ++it;
            }
        }

        peers_[peerIdx].setConnectionState(LobbyPeer::ConnectionState::Free, arena_);
    }
    else
    {
        // we lost connection to server :(
        X_ERROR("Lobby", "Lost connection to host");

        pCallbacks_->onLostConnectionToHost();
    }
}

void Lobby::handleLobbyJoinRequest(Packet* pPacket)
{
    X_ASSERT(isHost(), "Should only recive LobbyJoinRequest if host")(isPeer(), isHost());

    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Recived join request from a unknown peer");
        return;
    }

    auto& peer = peers_[peerIdx];
    if (peer.getConnectionState() != LobbyPeer::ConnectionState::Pending) {
        X_ERROR("Lobby", "Recived join request for peer not in pending state. State: \"%s\"", LobbyPeer::ConnectionState::ToString(peer.getConnectionState()));
        return;
    }

    X_LOG0("Lobby", "Accepted join request from peerIdx: %" PRId32, peerIdx);

    {
        core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

        auto numUsers = users_.size();

        // the peer sent us user info, what a cunt!
        addUsersFromBs(bs, peerIdx);

        if (numUsers == users_.size())
        {
            X_ERROR("Lobby", "User sent zero user info in join request");
            disconnectPeer(peerIdx);
            return;
        }

        // This peer is now connected.
        peer.setConnectionState(LobbyPeer::ConnectionState::Established, arena_);
    }

    // send match params and full user list.
    core::FixedBitStreamStack<0x400> bs;
    bs.write(MessageID::LobbyJoinAccepted);

    params_.writeToBitStream(bs);
    addUsersToBs(bs);

    pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::Reliable, pPacket->systemHandle);
}

void Lobby::handleLobbyJoinAccepted(Packet* pPacket)
{
    X_ASSERT(isPeer(), "Should only recive LobbyJoinAccepted if peer")(isPeer(), isHost());
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Recived join accepted from a unknown peer");
        return;
    }

    // should be host.
    X_ASSERT(peerIdx == hostIdx_, "Recoved join accepted from a peer that's not host")( peerIdx, hostIdx_);

    auto& host = peers_[peerIdx];
    X_ASSERT(host.getConnectionState() == LobbyPeer::ConnectionState::Pending, "Unexpected connection state")(host.getConnectionState());
    host.setConnectionState(LobbyPeer::ConnectionState::Established, arena_);

    clearUsers();

    params_.fromBitStream(bs);
    addUsersFromBs(bs, peerIdx);

    setState(LobbyState::Idle);
}

void Lobby::handleLobbyUsersConnected(Packet* pPacket)
{
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);
    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Recived user list from a unknown peer");
        return;
    }

    addUsersFromBs(bs, peerIdx);
}

void Lobby::handleLobbyUsersDiconnected(Packet* pPacket)
{
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);


}

void Lobby::handleLobbyGameParams(Packet* pPacket)
{
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    params_.fromBitStream(bs);
}


void Lobby::addUsersFromBs(core::FixedBitStreamBase& bs, int32_t peerIdx)
{
    const auto numUsersStart = getNumUsers();
    int32_t numUsers = bs.read<int32_t>();

    X_ASSERT(numUsers < MAX_PLAYERS, "Recived more users than max players")(numUsers, MAX_PLAYERS);

    for (int32_t i = 0; i < numUsers; i++)
    {
        LobbyUser user;
        user.fromBitStream(bs);

        X_LOG0("Lobby", "Adding user: \"%s\"", user.username.c_str());

        if (isHost())
        {
            // user list is from this peer, so set index.
            if (peerIdx != -1)
            {
                user.address = peers_[peerIdx].systemAddr;
                user.peerIdx = peerIdx;
            }
        }
        else
        {
            X_ASSERT(peerIdx == hostIdx_, "Should only recive user list from host")(peerIdx, hostIdx_);

            // only the host's user address need patching.
            // since the host will have set the address for all other users.
            if (user.peerIdx == -1)
            {
                user.address = peers_[peerIdx].systemAddr;
            }
        }

        users_.emplace_back(std::move(user));
    }

    if (isHost())
    {
        sendNewUsersToPeers(peerIdx, numUsersStart, numUsers);
    }
}

void Lobby::addUsersToBs(core::FixedBitStreamBase& bs) const
{
    bs.write(safe_static_cast<int32_t>(users_.size()));
    for (const auto& user : users_)
    {
        user.writeToBitStream(bs);
    }
}

void Lobby::sendNewUsersToPeers(int32_t skipPeer, int32_t startIdx, int32_t num) const
{
    auto numUsers = getNumUsers();

    if (startIdx >= numUsers) {
        X_WARNING("Lobby", "Skipping sending user diff to peers, no new users");
        return;
    }

    const int32_t diff = numUsers - startIdx;
    if (diff != num) {
        X_WARNING("Lobby", "Sending less users than added. num %" PRIi32 " diff %" PRIi32, num, diff);
    }

    X_LOG0("Lobby", "Sending %" PRId32 " new users to peers", diff);

    core::FixedBitStreamStack<1500> bs;
    bs.write(MessageID::LobbyUsersConnected);
    bs.write<int32_t>(num);

    for (int32_t i = startIdx; i < numUsers; i++)
    {
        users_[i].writeToBitStream(bs);
    }

    // send the msg to all peers except skip peer.
    for (size_t i = 0; i < peers_.size(); i++)
    {
        if (i == skipPeer) {
            continue;
        }

        auto& peer = peers_[i];
        if(!peer.isConnected()) {
            continue;
        }

        pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::Reliable, peer.systemHandle);
    }
}

void Lobby::sendUserCmd(const UserCmd& cmd)
{
    X_ASSERT(isPeer(), "Can only send user cmd if peer")(isPeer(), isHost());
    const auto& peer = peers_[hostIdx_];

    // do i want to compress this or something?
    core::FixedBitStreamStack<1500> bs;
    bs.write(MessageID::UserCmd);
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
        case LobbyState::Joining:
            return stateJoining();
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

int32_t Lobby::findPeerIdx(SystemHandle handle) const
{
    auto it = std::find_if(peers_.begin(), peers_.end(), [handle](const LobbyPeer& p) { return p.systemHandle == handle; });
    if (it == peers_.end()) {
        return -1;
    }

    return safe_static_cast<int32_t>(std::distance(peers_.begin(), it));
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


void Lobby::disconnectPeer(int32_t peerIdx)
{
    X_ASSERT(peerIdx >= 0 && peerIdx < safe_static_cast<int32_t>(peers_.size()), "Invalid peerIdx")(peerIdx, peers_.size());
    X_ASSERT(isHost(), "Should not be trying to diconnectPeer if not host")(isHost(), isPeer());

    X_LOG0("Lobby", "Diconnecting peerIdx: %" PRIi32, peerIdx);

    auto& peer = peers_[peerIdx];

    if (peer.getConnectionState() != LobbyPeer::ConnectionState::Free) {
        peer.setConnectionState(LobbyPeer::ConnectionState::Free, arena_);
    }
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

    clearUsers();
    
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

bool Lobby::stateJoining(void)
{
    // waiting for the response!
    // anything todo, tickle my poo?

    return true;
}

void Lobby::initStateLobbyHost(void)
{
    // we are hosting
    isHost_ = true;

    addLocalUsers();

    hostAddress_ = users_.front().address;

    // done?
}

void Lobby::addLocalUsers(void)
{
    clearUsers();

    core::SysInfo::UserNameStr nameStr;
    core::SysInfo::GetUserName(nameStr);

    // Lerrooooooooooooy JENKINS!!! (is a twat? O_O)
    char buffer[128];

    auto localGuid = pPeer_->getMyGUID();
    SystemAddress address;

    LobbyUser user;
    user.guid = localGuid;
    user.address = address;
    user.username.set(core::strUtil::Convert(nameStr, buffer));
    user.username.appendFmt("_%" PRIx32, core::Process::GetCurrentID());
    users_.emplace_back(user);

    X_ASSERT(users_.isNotEmpty(), "User list empty")(users_.size());
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

int32_t Lobby::getNumConnectedPeers(void) const
{
    int32_t num = 0;
    for (auto& peer : peers_)
    {
        num += static_cast<int32_t>(peer.isConnected());
    }

    return num;
}

int32_t Lobby::getNumConnectedPeersInGame(void) const
{
    int32_t num = 0;
    for (auto& peer : peers_)
    {
        num += static_cast<int32_t>(peer.isConnected() && peer.inGame);
    }

    return num;
}

int32_t Lobby::getHostPeerIdx(void) const
{
    return hostIdx_;
}

int32_t Lobby::getNumUsers(void) const
{
    return safe_static_cast<int32_t>(users_.size());
}

LobbyUserHandle Lobby::getUserHandleForIdx(size_t idx) const
{
    return static_cast<LobbyUserHandle>(idx);
}

const char* Lobby::getUserName(LobbyUserHandle handle) const
{
    size_t idx = static_cast<size_t>(handle);

    return users_[idx].username.c_str();
}

bool Lobby::getUserInfo(LobbyUserHandle handle, UserInfo& info) const
{
    size_t idx = static_cast<size_t>(handle);
    auto& user = users_[idx];

    info.pName = user.username.c_str();
    info.peerIdx = user.peerIdx;


    return true;
}

Vec2f Lobby::drawDebug(Vec2f base, engine::IPrimativeContext* pPrim) const
{
    X_UNUSED(pPrim);

    font::TextDrawContext con;
    con.col = Col_Whitesmoke;
    con.size = Vec2f(16.f, 16.f);
    con.effectId = 0;
    con.pFont = gEnv->pFontSys->GetDefault();

    float width = 700.f;
    float height = 300.f;

    const auto numConnected = getNumConnectedPeers();
    const auto numInGame = getNumConnectedPeersInGame();

    pPrim->drawQuad(base, width, height, Color8u(20, 20, 20, 60));

    auto* pNet = gEnv->pNet;
    IPStr ipStr;

    core::StackString<2048> txt;
    txt.setFmt("Lobby - Type: ^1%s^7 State: ^1%s^7 HostIdx: ^1%" PRIi32 "^7 isHost: ^1%" PRIu8 "^7\n",
        LobbyType::ToString(type_), LobbyState::ToString(state_), hostIdx_, isHost_);

    txt.appendFmt("HostAddr: \"%s\" Connected: ^1%" PRIi32 "^7 inGame: ^1%" PRIi32 "^7\n",
        pNet->systemAddressToString(hostAddress_, ipStr, true), numConnected, numInGame);

    for (size_t i = 0; i < peers_.size(); i++)
    {
        auto& peer = peers_[i];

        static_assert(std::is_same<uint16_t, SystemHandle>::value, "SystemHandle format specifier needs updating");

        txt.appendFmt("\n^5Peer%" PRIuS "^7 State: ^1%s^7 loaded: ^1%" PRIu8 "^7 inGame: ^1%" PRIu8 "^7 SysHandle: ^1%" PRIu16 "^7 numSnaps: ^1%" PRIi32 "^7", 
            i, LobbyPeer::ConnectionState::ToString(peer.getConnectionState()), peer.loaded, peer.inGame, peer.systemHandle, peer.numSnapsSent);
    }

    txt.append("\n\nUsers:");


    for (size_t i = 0; i < users_.size(); i++)
    {
        auto& user = users_[i];

        txt.appendFmt("\n^5User%" PRIuS "^7 Name: \"%s\" PeerIdx: ^1%" PRIi32 "^7 Address: \"%s\"",
            i,  user.username.c_str(), user.peerIdx, pNet->systemAddressToString(user.address, ipStr, true));
    }


    pPrim->drawText(base.x + 2.f, base.y + 2.f, con, txt.begin(), txt.end());

    return Vec2f(width, height);
}

X_NAMESPACE_END