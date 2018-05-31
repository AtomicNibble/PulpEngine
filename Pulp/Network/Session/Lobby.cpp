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

void ChatMsg::writeToBitStream(core::FixedBitStreamBase& bs) const
{
    bs.write(userGuid);
    bs.write(dateTimeStamp);
    bs.write(safe_static_cast<int16_t>(msg.length()));
    bs.write(msg.data(), msg.length());
}

void ChatMsg::fromBitStream(core::FixedBitStreamBase& bs)
{
    bs.read(userGuid);
    bs.read(dateTimeStamp);

    int32_t len = bs.read<int16_t>();
    if (len > MAX_CHAT_MSG_LENGTH) {
        len = MAX_CHAT_MSG_LENGTH;
    }

    char buf[MAX_CHAT_MSG_LENGTH];

    bs.read(buf, len);
    msg.assign(buf, len);
}

// ------------------------------------------------------------

LobbyUser::LobbyUser()
{
    peerIdx = -1;
    username.set("Stu");
}

X_INLINE bool LobbyUser::hasPeer(void) const
{
    return peerIdx >= 0;
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

LobbyPeer::LobbyPeer()
{
    reset();
}

void LobbyPeer::reset(void)
{
    connectionState = ConnectionState::Free;

    loaded = false;
    inGame = false;
    pauseSnapShots = false;

    snapHz = 0.f;
    numSnapsSent = 0;
    pSnapMan.reset();

    systemHandle = INVALID_SYSTEM_HANDLE;
    systemAddr = SystemAddress();
    guid = NetGUID();
}

X_INLINE bool LobbyPeer::isConnected(void) const
{
    return connectionState == ConnectionState::Established;
}

X_INLINE LobbyPeer::ConnectionState::Enum LobbyPeer::getConnectionState(void) const
{
    return connectionState;
}

X_INLINE void LobbyPeer::setConnectionState(ConnectionState::Enum state)
{
    connectionState = state;
}


// ------------------------------------------------------

Lobby::Lobby(SessionVars& vars, ISessionCallbacks* pCallbacks, IPeer* pPeer, IGameCallbacks* pGameCallbacks,
        LobbyType::Enum type, core::MemoryArenaBase* arena) :
    vars_(vars),
    arena_(arena),
    pCallbacks_(X_ASSERT_NOT_NULL(pCallbacks)),
    pPeer_(X_ASSERT_NOT_NULL(pPeer)),
    pGameCallbacks_(X_ASSERT_NOT_NULL(pGameCallbacks)),
    type_(type),
    users_(arena),
    peers_(arena),
    chatMsgs_(arena_),
    chatHistory_(arena_)
{
    peers_.setGranularity(MAX_PLAYERS);
    peers_.resize(MAX_PLAYERS);
    users_.setGranularity(MAX_PLAYERS);
    users_.reserve(MAX_PLAYERS);

    reset();
}

void Lobby::reset(void)
{
    X_LOG0_IF(vars_.lobbyDebug(), "Lobby", "reset \"%s\" lobby", LobbyType::ToString(type_));

    isHost_ = false;
    startLoading_ = false;
    finishedLoading_ = false;

    hostIdx_ = -1;

    clearUsers();

    for (auto& peer : peers_)
    {
        if (peer.getConnectionState() != LobbyPeer::ConnectionState::Free) {
            setPeerConnectionState(peer, LobbyPeer::ConnectionState::Free);
        }
    }

    chatMsgs_.clear();
    chatHistory_.clear();

    setState(LobbyState::Idle);
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

bool Lobby::handlePacket(Packet* pPacket)
{
    auto id = pPacket->getID();

    switch (id)
    {
        case MessageID::SnapShot:
            handleSnapShot(pPacket);
            break;
        case MessageID::UserCmd:
            handleUserCmd(pPacket);
            break;

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
            handleConnectionLost(pPacket);
            break;
        case MessageID::DisconnectNotification:
            handleDisconnectNotification(pPacket);
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
        case MessageID::LobbyConnectAndMove:
            handleLobbyConnectAndMove(pPacket);
            break;
        case MessageID::LobbyLeaveGameLobby:
            handleLobbyLeaveGameLobby(pPacket);
            break;

        case MessageID::LoadingStart:
            handleLoadingStart(pPacket);
            break;
        case MessageID::LoadingDone:
            handleLoadingDone(pPacket);
            break;
        case MessageID::InGame:
            handleInGame(pPacket);
            break;
        case MessageID::EndGame:
            handleEndGame(pPacket);
            break;

        case MessageID::LobbyChatMsg:
            handleLobbyChatMsg(pPacket);
            break;
        default:
            break;
    }

    return false;
}

// -------------------------------------------

void Lobby::connectTo(SystemAddress address)
{
    // i would like to connect plz.
    reset();

    // Bittttttcoooonnneeeeeeeeectt!!!!
    auto delay = core::TimeVal::fromMS(vars_.connectionRetryDelayMs());

    auto res = pPeer_->connect(address, PasswordStr(), vars_.connectionAttemps(), delay);

    switch (res)
    {
        case ConnectionAttemptResult::Started:
        case ConnectionAttemptResult::AlreadyConnected:
            break;

        default:
            X_ERROR("Lobby", "Failed to connectTo: \"%s\"", ConnectionAttemptResult::ToString(res));
            setState(LobbyState::Error);
            return;
    }

    // add the address as a peer and mark as host.
    hostIdx_ = addPeer(address);
    X_ASSERT(peers_[hostIdx_].getConnectionState() == LobbyPeer::ConnectionState::Pending, "Invalid peer state")(peers_[hostIdx_].getConnectionState());

    if (res != ConnectionAttemptResult::AlreadyConnected) {
        setState(LobbyState::Connecting);
        return;
    }

    // we are already connected to the remote system we just wanna join the lobby.
    // currently there is going to be a problem when shutting down lobby tho.
    // as it will close the connection in both lobby.

    auto systemHandle = pPeer_->getSystemHandleForAddress(address);
    if (systemHandle == INVALID_SYSTEM_HANDLE) {
        setState(LobbyState::Error);
        return;
    }

    auto guid = pPeer_->getGuidForHandle(systemHandle);
    X_ASSERT(guid != NetGUID(), "Guid is invalid")(guid);

    auto& peer = peers_[hostIdx_];
    peer.systemHandle = systemHandle;
    peer.guid = guid;

    addLocalUsers();

    // ask to join the lobby.
    sendJoinRequestToHost();

    setState(LobbyState::Joining);
}


void Lobby::startHosting(const MatchParameters& params)
{
    params_ = params;

    reset();

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

// -------------------------------------------

void Lobby::sendMembersToLobby(Lobby& destLobby) const
{
    auto dstLobbyType = destLobby.getType();

    sendMembersToLobby(dstLobbyType);
}

void Lobby::sendMembersToLobby(LobbyType::Enum type) const
{
    X_ASSERT(type != type_, "Trying to send member to lobby of same type")(type, type_);

    core::FixedBitStreamStack<0x20> bs;
    bs.write(MessageID::LobbyConnectAndMove);
    bs.write(safe_static_cast<uint8_t>(type_));
    bs.write(safe_static_cast<uint8_t>(type));

    // TODO: inssert remote address.
    SystemAddress sa = pPeer_->getMyBoundAddress();
    sa.writeToBitStream(bs);

    sendToPeers(bs.data(), bs.sizeInBytes());
}

void Lobby::sendPeerToLobby(int32_t peerIdx, LobbyType::Enum type) const
{
    X_ASSERT(peerIdx >= 0, "Invalid peerIdx")(peerIdx);

    auto& peer = peers_[peerIdx];
    if (!peer.isConnected()) {
        X_ERROR("Lobby", "Can't send a none connected peer to lobby");
        return;
    }

    // away with you pleb!
    core::FixedBitStreamStack<0x20> bs;
    bs.write(MessageID::LobbyConnectAndMove);
    bs.write(safe_static_cast<uint8_t>(type_));
    bs.write(safe_static_cast<uint8_t>(type));

    SystemAddress sa = pPeer_->getMyBoundAddress();
    sa.writeToBitStream(bs);

    sendToPeer(peerIdx, bs.data(), bs.sizeInBytes());
}


void Lobby::notifyPeersLeavingGameLobby(void)
{
    X_ASSERT(isHost(), "Can only notify leaving lobby if host")(isPeer(), isHost());
    X_ASSERT(type_ == LobbyType::Party, "Can't only notify leaving game lobby via party lobby")(type_);


    sendToPeers(MessageID::LobbyLeaveGameLobby);
}


// -------------------------------------------

void Lobby::sendChatMsg(core::span<const char> msg)
{
    if (msg.length() > MAX_CHAT_MSG_LENGTH) {
        X_ERROR("Lobby", "Failed to send chat msg it exceeds max length");
        return;
    }

    ChatMsg cm;
    cm.userGuid = pPeer_->getMyGUID();
    cm.dateTimeStamp = core::DateTimeStamp::getSystemDateTime();
    cm.msg.assign(msg.data(), msg.size_bytes());

    // build a BS.
    ChatMsgBs bs;
    bs.write(MessageID::LobbyChatMsg);
    bs.write(safe_static_cast<uint8_t>(type_));
    bs.write<int32_t>(1);
    cm.writeToBitStream(bs);

    if (isPeer())
    {
        sendToHost(bs.data(), bs.sizeInBytes()); // we will display the msg when it comes back.
    }
    else if (isHost())
    {
        // just send via loopback, so all the same logic for incoming chat msg's gets used.
        pPeer_->sendLoopback(bs.data(), bs.sizeInBytes());
    }
    else
    {
        // should you be allowed to talk to yourself? YES!
        X_ERROR("Lobby", "Can't send chat msg neither peer or host");
    }
}

void Lobby::sendUserCmd(const core::FixedBitStreamBase& bs)
{
    X_ASSERT(isPeer(), "Can only send user cmd if peer")(isPeer(), isHost());
    const auto& peer = peers_[hostIdx_];

    pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::UnReliableSequenced, peer.systemHandle);
}

void Lobby::sendSnapShot(const SnapShot& snap)
{
    X_ASSERT(isHost(), "Can only send snapshot if host")(isPeer(), isHost());

    core::FixedBitStreamStack<1500> bs;
    bs.write(MessageID::SnapShot);
    snap.writeToBitStream(bs);

    for (auto& peer : peers_)
    {
        if (!peer.isConnected()) {
            continue;
        }

        if (!peer.loaded) {
            continue;
        }

        NetGuidStr str;
        X_LOG0("Lobby", "Sending snap to %s", peer.guid.toString(str));

        // for now just send whole snap, later will need to build deltas and shit.
        // how do i know it's a snapshot tho?
        pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::UnReliableSequenced, peer.systemHandle);
        //  peer.pSnapMan->setStateSnap(snap);
    }
}

void Lobby::sendToHost(MessageID::Enum id)
{
    MsgIdBs bs;
    bs.write(id);
    bs.write(safe_static_cast<uint8_t>(type_));

    sendToHost(bs.data(), bs.sizeInBytes());
}

void Lobby::sendToHost(const uint8_t* pData, size_t lengthInBytes)
{
    X_ASSERT(isPeer() && !isHost(), "Invalid operation")(isPeer(), isHost());
    const auto& peer = peers_[hostIdx_];

    sendToPeer(hostIdx_, pData, lengthInBytes);
}

void Lobby::sendToPeers(MessageID::Enum id) const
{
    MsgIdBs bs;
    bs.write(id);
    bs.write(safe_static_cast<uint8_t>(type_));

    sendToPeers(bs.data(), bs.sizeInBytes());
}

void Lobby::sendToPeers(const uint8_t* pData, size_t lengthInBytes) const
{
    X_ASSERT(!isPeer() && isHost(), "Invalid operation")(isPeer(), isHost());

    for (auto& peer : peers_)
    {
        if (!peer.isConnected()) {
            continue;
        }

        pPeer_->send(pData, lengthInBytes, PacketPriority::High, PacketReliability::ReliableOrdered, peer.systemHandle, OrderingChannel::SessionMsg);
    }
}

void Lobby::sendToAll(const uint8_t* pData, size_t lengthInBytes)
{
    X_ASSERT(!isPeer() && isHost(), "Invalid operation")(isPeer(), isHost());

    sendToPeers(pData, lengthInBytes);
    pPeer_->sendLoopback(pData, lengthInBytes);
}


void Lobby::sendToPeer(int32_t peerIdx, MessageID::Enum id) const
{
    MsgIdBs bs;
    bs.write(id);
    bs.write(safe_static_cast<uint8_t>(type_));

    sendToPeer(peerIdx, bs.data(), bs.sizeInBytes());
}

void Lobby::sendToPeer(int32_t peerIdx, const uint8_t* pData, size_t lengthInBytes) const
{
    auto& peer = peers_[peerIdx];
    if (!peer.isConnected()) {
        X_ERROR("Lobby", "Can't send a none connected peer a msg");
        return;
    }

    pPeer_->send(pData, lengthInBytes, PacketPriority::High, PacketReliability::ReliableOrdered, peer.systemHandle, OrderingChannel::SessionMsg);
}

// ----------------------------------------------------

void Lobby::setState(LobbyState::Enum state)
{
    X_LOG0_IF(vars_.lobbyDebug(), "Lobby", "State changed: \"%s\"", LobbyState::ToString(state));

    if (state == state_) {
        X_WARNING("Lobby", "Redundant State changed");
    }

    state_ = state;
}

// ----------------------------------------------------

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
    for (size_t i = 0; i<peers_.size(); i++)
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
    setPeerConnectionState(peer, LobbyPeer::ConnectionState::Pending);

    return idx;
}


void Lobby::disconnectPeer(int32_t peerIdx)
{
    X_ASSERT(peerIdx >= 0 && peerIdx < safe_static_cast<int32_t>(peers_.size()), "Invalid peerIdx")(peerIdx, peers_.size());
    X_ASSERT(isHost(), "Should not be trying to diconnectPeer if not host")(isHost(), isPeer());

    X_LOG0("Lobby", "Diconnecting peerIdx: %" PRIi32, peerIdx);

    auto& peer = peers_[peerIdx];

    if (peer.getConnectionState() != LobbyPeer::ConnectionState::Free) {
        setPeerConnectionState(peer, LobbyPeer::ConnectionState::Free);
    }
}

X_INLINE void Lobby::setPeerConnectionState(int32_t peerIdx, LobbyPeer::ConnectionState::Enum state)
{
    auto& peer = peers_[peerIdx];

    setPeerConnectionState(peer, state);
}

void Lobby::setPeerConnectionState(LobbyPeer& peer, LobbyPeer::ConnectionState::Enum newState)
{
    const auto curState = peer.getConnectionState();

    if (newState == curState) {
        X_ERROR("Lobby", "Peer already in stat: \"%s\"", ConnectionState::ToString(newState));
        return;
    }

    if (newState == LobbyPeer::ConnectionState::Free)
    {
        if (curState == LobbyPeer::ConnectionState::Established)
        {
            // tell the user to get fucked?
            // HELL YER!
            
            pPeer_->closeConnection(peer.systemHandle, true, OrderingChannel::Default, PacketPriority::Low);
        }

        // this won't break removing the user below.
        peer.reset();
    }
    else if (newState == LobbyPeer::ConnectionState::Pending)
    {
        X_ASSERT(curState == LobbyPeer::ConnectionState::Free, "Invalid peer state")(curState);

        peer.pSnapMan = core::makeUnique<SnapshotManager>(arena_, arena_);
    }
    else if (newState == LobbyPeer::ConnectionState::Established)
    {
        X_ASSERT(curState == LobbyPeer::ConnectionState::Pending, "Invalid peer state")(curState);

    }
    else
    {
        X_ASSERT_UNREACHABLE();
    }

    peer.stateChangeTime = gEnv->pTimer->GetTimeNowReal();
    peer.setConnectionState(newState);

    if (isHost() && newState == LobbyPeer::ConnectionState::Free) 
    {
        size_t numRemoved = removeUsersWithDisconnectedPeers();
        if (numRemoved == 0)
        {
            X_WARNING("Lobby", "No users where removed after disconnecting peer");
        }
    }
}

// ---------------------------------------------------------------

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
            else
            {
                // currently no use case.
                X_ASSERT_NOT_IMPLEMENTED();
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
                user.peerIdx = peerIdx; // mark this as host.
            }
            else
            {
                user.peerIdx = -1; // peer indexes aree not valid on clients.
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

    UserInfoBs bs;
    bs.write(MessageID::LobbyUsersConnected);
    bs.write(safe_static_cast<uint8_t>(type_));
    bs.write<int32_t>(num);

    for (int32_t i = startIdx; i < numUsers; i++)
    {
        users_[i].writeToBitStream(bs);
    }

    // send the msg to all peers except skip peer.
    for (int32_t i = 0; i < static_cast<int32_t>(peers_.size()); i++)
    {
        if (i == skipPeer) {
            continue;
        }

        auto& peer = peers_[i];
        if (!peer.isConnected()) {
            continue;
        }

        sendToPeer(i, bs.data(), bs.sizeInBytes());
    }
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
    user.username.appendFmt("_%" PRIx32, core::Process::getCurrentID());
    users_.emplace_back(user);

    X_ASSERT(users_.isNotEmpty(), "User list empty")(users_.size());
}

void Lobby::clearUsers(void)
{
    users_.clear();
}

size_t Lobby::removeUsersWithDisconnectedPeers(void)
{
    X_ASSERT(isHost(), "Host only logic")(isHost());

    NetGUIDArr guids;

    // find any users that peers are no longer connected.
    for (auto& user : users_)
    {
        // us?
        if (!user.hasPeer()) {
            continue;
        }

        auto& peer = peers_[user.peerIdx];
        if (!peer.isConnected()) {
            // gone :(
            guids.push_back(user.guid);
        }
    }
 
    removeUsersByGuid(guids);

    return guids.size();
}

void Lobby::removeUsersByGuid(const NetGUIDArr& ids)
{
    if (ids.isEmpty()) {
        return;
    }

    for (auto id : ids)
    {
        bool removed = false;

        for (size_t i = 0; i < users_.size(); i++)
        {
            auto& user = users_[i];
            if (user.guid == id)
            {
                removed = users_.removeIndex(i);
                break;
            }
        }
    
        if (!removed)
        {
            NetGuidStr buf;
            X_WARNING("Lobby", "Failed to find user with guid: %s for removal", id.toString(buf));
        }
    }

    // tell the bitchez?
    if (isHost())
    {
        NetGUIDBs bs;
        bs.write(MessageID::LobbyUsersDiconnected);
        bs.write(safe_static_cast<uint8_t>(type_));
        bs.write(safe_static_cast<int32_t>(ids.size()));
        bs.write(ids.data(), ids.size());
        sendToPeers(bs.data(), bs.sizeInBytes());
    }
}

// ----------------------------------------------------

void Lobby::pushChatMsg(ChatMsg&& msg)
{
    if (isHost()) 
    {
        if (chatHistory_.size() >= MAX_CHAT_MSGS) {
            chatHistory_.pop();
        }

        chatHistory_.push(msg);
    }

    if (chatMsgs_.size() >= MAX_CHAT_MSGS) {
        chatMsgs_.pop();
    }

    chatMsgs_.emplace(std::move(msg));
}

void Lobby::sendChatMsgToPeers(const ChatMsg& msg)
{
    X_ASSERT(isHost(), "Trying to broadcast chat msg when not host")(isPeer(), isHost());

    ChatMsgBs bs;
    bs.write(MessageID::LobbyChatMsg);
    bs.write(safe_static_cast<uint8_t>(type_));
    bs.write(1_i32);
    msg.writeToBitStream(bs);

    sendToPeers(bs.data(), bs.sizeInBytes());
}

void Lobby::sendChatHistoryToPeer(int32_t peerIdx)
{
    X_ASSERT(isHost(), "Trying to send chat history when not host")(isPeer(), isHost());
    if (!chatHistory_.isNotEmpty()) {
        return;
    }

    if (peerIdx < 0) {
        X_ERROR("Lobby", "Can't send chat history invalid peer index");
        return;
    }

    X_LOG0("Lobby", "Sending chat log to new peer, num: %" PRIuS, chatHistory_.size());

    auto& peer = peers_[peerIdx];
    
    // TODO: pack multiple msg's that fit in single BS.
    for (size_t i = 0; i < chatHistory_.size(); i++)
    {
        ChatMsgBs bs;
        bs.write(MessageID::LobbyChatMsg);
        bs.write(safe_static_cast<uint8_t>(type_));
        bs.write(1_i32);

        auto& msg = chatHistory_[i];
        msg.writeToBitStream(bs);

        sendToPeer(peerIdx, bs.data(), bs.sizeInBytes());
    }
}

// ----------------------------------------------------

void Lobby::handleSnapShot(Packet* pPacket)
{
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

void Lobby::handleUserCmd(Packet* pPacket)
{
    X_ASSERT(isHost(), "Recived usercmd when not host")(isPeer(), isHost());

    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Failed to find peer for incomming userCmd");
        return;
    }

    // pass back to game.
    // this will only happen during a call to handle state, so the game knows
    // when this callback can happen.
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);
    pGameCallbacks_->onUserCmdReceive(pPacket->guid, bs);
}


void Lobby::handleConnectionAccepted(Packet* pPacket)
{
    X_ASSERT(isPeer(), "We should only be connecting to a server if a peer")(isPeer(), isHost());

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
    sendJoinRequestToHost();

    // todo wait for reponse.
    setState(LobbyState::Joining);
}

void Lobby::handleConnectionHandShake(Packet* pPacket)
{
    // the peer confirmed connnection with us.
    // what a nice little slut.
    X_ASSERT(isHost(), "Recived connection hand shake when not host")(isHost());

    if (getNumFreeUserSlots() == 0) {
       X_WARNING("Lobby", "Rejected peer, lobby is full. Total Slots: %" PRIi32 , params_.numSlots); // owned.
        
        MsgIdBs bs;
        bs.write(MessageID::LobbyJoinNoFreeSlots);
        bs.write(safe_static_cast<uint8_t>(type_));
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
        X_ERROR("Lobby", "Recived connection failed when not trying to connect. State: \"%s\" Msg: \"%s\"", 
            LobbyState::ToString(state_), MessageID::ToString(id));
        return;
    }

    X_ERROR("Lobby", "Connection attempt failed: \"%s\"", MessageID::ToString(id));
        
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

        X_ERROR("Lobby", "Peer lost connection to \"%s\"  lobby", LobbyType::ToString(type_));

        setPeerConnectionState(peerIdx, LobbyPeer::ConnectionState::Free);
    }
    else
    {
        // we lost connection to server :(
        X_ERROR("Lobby", "Lost connection to host");

        pCallbacks_->onLostConnectionToHost();
    }
}

void Lobby::handleDisconnectNotification(Packet* pPacket)
{
    if (isHost())
    {
        // bye!
        auto peerIdx = findPeerIdx(pPacket->systemHandle);
        if (peerIdx < 0) {
            X_ERROR("Lobby", "Failed to find peer to remove");
            return;
        }

        X_LOG0_IF(vars_.lobbyDebug(), "Lobby", "Peer %" PRIi32 " disconnected", peerIdx);

        setPeerConnectionState(peerIdx, LobbyPeer::ConnectionState::Free);
    }
    else
    {
        // we lost connection to server :(
        X_ERROR("Lobby", "Host disconnected from us");

        pCallbacks_->onLostConnectionToHost();
    }
}

void Lobby::handleLobbyJoinRequest(Packet* pPacket)
{
    X_ASSERT(isHost(), "Should only recive LobbyJoinRequest if host")(isPeer(), isHost());

    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {

        // this peer is likley already connected to another lobby.
        // and they want to join this one also.
        handleConnectionHandShake(pPacket);

        peerIdx = findPeerIdx(pPacket->systemHandle);
        if (peerIdx < 0) {
            X_ERROR("Lobby", "Failed to add peer for JoinRequest");
            return;
        }
    }

    auto& peer = peers_[peerIdx];
    if (peer.getConnectionState() != LobbyPeer::ConnectionState::Pending) {
        X_ERROR("Lobby", "Recived join request for peer not in pending state. State: \"%s\"", LobbyPeer::ConnectionState::ToString(peer.getConnectionState()));
        return;
    }

    X_LOG0("Lobby", "Accepted join request from peerIdx: %" PRId32, peerIdx);

    {
        core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);
        auto type = bs.read<LobbyType::Enum>();
        X_ASSERT(type == type_, "Recived Lobby packet with type mismatch")(type_, type);
        
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
        setPeerConnectionState(peer, LobbyPeer::ConnectionState::Established);
    }

    // send match params and full user list.
    {
        UserInfoBs bs;
        bs.write(MessageID::LobbyJoinAccepted);
        bs.write(safe_static_cast<uint8_t>(type_));

        params_.writeToBitStream(bs);
        addUsersToBs(bs);

        sendToPeer(peerIdx, bs.data(), bs.sizeInBytes());
    }

    {
        auto sessionStatus = pCallbacks_->getStatus();

        if (type_ == LobbyType::Party)
        {
            // if a user just connected to our party lobby, and we are in gamelobby have them join us.
            if (sessionStatus >= SessionStatus::GameLobby)
            {
                // we are in game, join the orgy!
                sendPeerToLobby(peerIdx, LobbyType::Game);
            }
        }
        else if (type_ == LobbyType::Game)
        {
            // if we are 'inGame' have new player join us.
            // basically join in progress.
            if (sessionStatus >= SessionStatus::Loading)
            {
                sendToPeer(peerIdx, MessageID::LoadingStart);
            }
        }
    }

    // send some recent chat history.
    sendChatHistoryToPeer(peerIdx);
}

void Lobby::handleLobbyJoinAccepted(Packet* pPacket)
{
    X_ASSERT(isPeer(), "Should only recive LobbyJoinAccepted if peer")(isPeer(), isHost());
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    auto type = bs.read<LobbyType::Enum>();
    X_ASSERT(type == type_, "Recived Lobby packet with type mismatch")(type_, type);

    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Recived join accepted from a unknown peer");
        return;
    }

    // should be host.
    X_ASSERT(peerIdx == hostIdx_, "Recoved join accepted from a peer that's not host")( peerIdx, hostIdx_);

    auto& host = peers_[peerIdx];
    X_ASSERT(host.getConnectionState() == LobbyPeer::ConnectionState::Pending, "Unexpected connection state")(host.getConnectionState());
    setPeerConnectionState(host, LobbyPeer::ConnectionState::Established);

    clearUsers();

    params_.fromBitStream(bs);
    addUsersFromBs(bs, peerIdx);

    setState(LobbyState::Idle);
}

void Lobby::handleLobbyUsersConnected(Packet* pPacket)
{
    X_ASSERT(isPeer(), "Recived user-con list when not peer")(isPeer(), isHost());

    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);
    auto type = bs.read<LobbyType::Enum>();
    X_ASSERT(type == type_, "Recived Lobby packet with type mismatch")(type_, type);

    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Recived user list from a unknown peer");
        return;
    }

    addUsersFromBs(bs, peerIdx);
}

void Lobby::handleLobbyUsersDiconnected(Packet* pPacket)
{
    X_ASSERT(isPeer(), "Recived user-decon list when not peer")(isPeer(), isHost());

    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    auto type = bs.read<LobbyType::Enum>();
    X_ASSERT(type == type_, "Recived Lobby packet with type mismatch")(type_, type);

    // users have left ;(
    // WEAK!
    const auto num = bs.read<int32_t>();
    if (num < 0 || num > MAX_PLAYERS) {
        X_ERROR("Lobby", "Recived invalud user diconnect msg, num users: %" PRIi32, num);
        return;
    }

    NetGUIDArr guids;
    guids.resize(num);
    bs.read(guids.data(), guids.size());

    for (auto& id : guids)
    {
        NetGuidStr buf;
        X_LOG0("Lobby", "User Diconnected: %s", id.toString(buf));
    }

    removeUsersByGuid(guids);
}

void Lobby::handleLobbyGameParams(Packet* pPacket)
{
    X_ASSERT(isPeer(), "Recived GameParams when not peer")(isPeer(), isHost());

    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    auto type = bs.read<LobbyType::Enum>();
    X_ASSERT(type == type_, "Recived Lobby packet with type mismatch")(type_, type);

    params_.fromBitStream(bs);
}

void Lobby::handleLobbyConnectAndMove(Packet* pPacket)
{
    X_ASSERT(isPeer(), "Recived ConnectAndMove when not peer")(isPeer(), isHost());

    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    auto type = bs.read<LobbyType::Enum>();
    X_ASSERT(type == type_, "Recived Lobby packet with type mismatch")(type_, type);

    // Where do you want me!
    auto destType = bs.read<LobbyType::Enum>();
    
    X_ASSERT(destType < LobbyType::ENUM_COUNT, "Recived Lobby packet with invalid type")(destType);
    X_ASSERT(destType > type, "Invalid lobby transition")(destType, type);

    SystemAddress sa;
    sa.fromBitStream(bs);

    pCallbacks_->connectAndMoveToLobby(destType, sa);
}

void Lobby::handleLobbyLeaveGameLobby(Packet* pPacket)
{
    X_ASSERT(type_ == LobbyType::Party, "Recived leave game not from party")(type_);
    X_ASSERT(isPeer(), "Recived LeaveGameLobby when not peer")(isPeer(), isHost());


    // only reason i handle packet in here, is for validation really.
    // but also might easy some 3rd party lobby intergation.

    pCallbacks_->leaveGameLobby();
}

void Lobby::handleLoadingStart(Packet* pPacket)
{
    X_ASSERT(type_ == LobbyType::Game, "None game lobby recived loading start")(type_);
    X_ASSERT(isPeer(), "Recived LoadingStart when not peer")(isPeer(), isHost());

    // stu will take all your loads.
    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Recived loading start from a unknown peer");
        return;
    }

    if (peerIdx != hostIdx_) {
        NetGuidStr buf;
        X_ERROR("Lobby", "Recived loading start from a peer that's not host. guid: %s", peers_[peerIdx].guid.toString(buf));
        return;
    }

    // start loading..
    // need to tell the peer to start loading!
    startLoading_ = true;
}

void Lobby::handleLoadingDone(Packet* pPacket)
{
    X_ASSERT(type_ == LobbyType::Game, "None game lobby recived loading done")(type_);
    X_ASSERT(isHost(), "Recived LoadingDone when not host")(isPeer(), isHost());

    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Recived loading done from a unknown peer");
        return;
    }

    auto& peer = peers_[peerIdx];
    if (peer.loaded) {
        X_WARNING("Lobby", "Peer %" PRIi32 " was already marked as loaded", peerIdx);
    }
    
    X_LOG0_IF(vars_.lobbyDebug(), "Lobby", "Peer %" PRIi32 " loaded", peerIdx);

    peer.loaded = true;
}

void Lobby::handleInGame(Packet* pPacket)
{
    X_ASSERT(type_ == LobbyType::Game, "None game lobby recived inGame")(type_);

    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Recived InGame from a unknown peer");
        return;
    }

    auto& peer = peers_[peerIdx];
    if (peer.inGame) {
        X_WARNING("Lobby", "Peer %" PRIi32 " was already marked as in game", peerIdx);
    }

    peer.inGame = true;
}

void Lobby::handleEndGame(Packet* pPacket)
{
    X_ASSERT(type_ == LobbyType::Game, "None game lobby recived EndGame")(type_);
    X_ASSERT(isPeer(), "Recived EndGame when not peer")(isPeer(), isHost());

    auto peerIdx = findPeerIdx(pPacket->systemHandle);
    if (peerIdx < 0) {
        X_ERROR("Lobby", "Recived EndGame from a unknown peer");
        return;
    }

    if (peerIdx != hostIdx_) {
        NetGuidStr buf;
        X_ERROR("Lobby", "Recived EndGame from a peer that's not host. guid: %s", peers_[peerIdx].guid.toString(buf));
        return;
    }

    // the host ended the game rip.
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    auto early = bs.read<bool>();

    pCallbacks_->endGame(early);
}

void Lobby::handleLobbyChatMsg(Packet* pPacket)
{
    X_ASSERT(isHost() || isPeer(), "Recived chat msg when not peer or host")(isPeer(), isHost());

    // meow, meow meow
    core::FixedBitStreamNoneOwning bs(pPacket->begin(), pPacket->end(), true);

    int32_t num = bs.read<int32_t>();
    if (num <= 0) {
        X_ERROR("Lobby", "Recived chat packet with zero msg's");
        return;
    }
 
    X_LOG0("Lobby", "Recived %" PRIi32 " chat msg(s)", num);

    for (int32_t i = 0; i < num; i++)
    {
        ChatMsg cm;
        cm.fromBitStream(bs);
        if (cm.msg.length() > MAX_CHAT_MSG_LENGTH) {
            X_ERROR("Lobby", "Recived oversized chat msg");
            return;
        }

        // re stamp it based on time recived.
        if (isHost())
        {
            cm.dateTimeStamp = core::DateTimeStamp::getSystemDateTime();

            sendChatMsgToPeers(cm);
        }

        pushChatMsg(std::move(cm));
    }
}

// -----------------------------------------------------------


void Lobby::sendJoinRequestToHost(void)
{
    X_ASSERT(hostIdx_ != -1, "Hostidx invalid")(hostIdx_);

    auto& peer = peers_[hostIdx_];
    X_ASSERT(peer.getConnectionState() == LobbyPeer::ConnectionState::Pending, "Invalid peer state")(peer.getConnectionState());

    UserInfoBs bs;
    bs.write(MessageID::LobbyJoinRequest);
    bs.write(safe_static_cast<uint8_t>(type_));

    addUsersToBs(bs);
    
    // we don't use sendToPeer/host as they check connection is established.
    pPeer_->send(bs.data(), bs.sizeInBytes(), PacketPriority::High, PacketReliability::Reliable, peer.systemHandle, OrderingChannel::SessionMsg);
}

// -----------------------------------------------------------


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

// -----------------------------------------------------------

void Lobby::initStateLobbyHost(void)
{
    // we are hosting
    isHost_ = true;

    addLocalUsers();

    hostAddress_ = users_.front().address;

    // done?
}

// -----------------------------------------------------------

bool Lobby::hasActivePeers(void) const
{
    return std::any_of(peers_.begin(), peers_.end(), [](const LobbyPeer& p) {
        return p.getConnectionState() != LobbyPeer::ConnectionState::Free;
    });
}

bool Lobby::allPeersLoaded(void) const
{
    bool loaded = true;

    for (auto& peer : peers_) {
        if (!peer.isConnected()) {
            continue;
        }

        loaded &= peer.loaded;
    }

    return loaded;
}

bool Lobby::allPeersInGame(void) const
{
    bool inGame = true;

    for (auto& peer : peers_) {
        if (!peer.isConnected()) {
            continue;
        }

        inGame &= peer.inGame;
    }

    return inGame;
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

const char* Lobby::getUserName(LobbyUserHandle handle) const
{
    size_t idx = static_cast<size_t>(handle);

    // Stu? is that you?
    return users_[idx].username.c_str();
}


bool Lobby::getUserInfoForIdx(size_t idx, UserInfo& info) const
{
    auto& user = users_[idx];

    info.pName = user.username.c_str();
    info.peerIdx = user.peerIdx;
    info.guid = user.guid;
    return true;
}


bool Lobby::getUserInfo(LobbyUserHandle handle, UserInfo& info) const
{
    size_t idx = static_cast<size_t>(handle);
    auto& user = users_[idx];

    info.pName = user.username.c_str();
    info.peerIdx = user.peerIdx;
    info.guid = user.guid;
    return true;
}

bool Lobby::tryPopChatMsg(ChatMsg& msg)
{
    if (chatMsgs_.isEmpty()) {
        return false;
    }

    msg = chatMsgs_.peek();
    chatMsgs_.pop();
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

    float width = 750.f;
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

        static_assert(std::is_same<uint16_t, decltype(peer.systemHandle)>::value, "format specifier needs updating");

        txt.appendFmt("\n^5Peer%" PRIuS "^7 State: ^1%s^7 loaded: ^1%" PRIu8 "^7 inGame: ^1%" PRIu8 "^7 SysHandle: ^1%" PRIu16 "^7 numSnaps: ^1%" PRIi32 "^7", 
            i, LobbyPeer::ConnectionState::ToString(peer.getConnectionState()), peer.loaded, peer.inGame, peer.systemHandle, peer.numSnapsSent);
    }

    pPrim->drawText(base.x + 2.f, base.y + 2.f, con, txt.begin(), txt.end());
    txt.clear();
    txt.append("\n\nUsers:");

    for (size_t i = 0; i < users_.size(); i++)
    {
        auto& user = users_[i];

        static_assert(std::is_same<int32_t, decltype(user.peerIdx)>::value, "format specifier needs updating");
        NetGuidStr buf;

        txt.appendFmt("\n^5User%" PRIuS "^7 \"%s\" PeerIdx: ^1%" PRIi32 "^7 guid: ^1%s^7 Addr: \"%s\"",
            i,  user.username.c_str(), user.peerIdx, user.guid.toString(buf), pNet->systemAddressToString(user.address, ipStr, true));
    }


    pPrim->drawText(base.x + 2.f, base.y + 30.f + (peers_.size() * 16.f), con, txt.begin(), txt.end());

    return Vec2f(width, height);
}

X_NAMESPACE_END