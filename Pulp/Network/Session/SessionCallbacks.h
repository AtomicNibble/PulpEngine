#pragma once

X_NAMESPACE_BEGIN(net)

class SnapShot;

struct ISessionCallbacks
{
    virtual ~ISessionCallbacks() = default;

    virtual SessionStatus::Enum getStatus(void) const X_ABSTRACT;

    virtual ConnectionAttemptResult::Enum connectToPeer(LobbyType::Enum type, SystemAddress sa) X_ABSTRACT;
    virtual void closeConnection(LobbyType::Enum type, SystemHandle systemHandle) X_ABSTRACT;

    virtual void onLostConnectionToHost(LobbyType::Enum type) X_ABSTRACT;

    virtual void onReceiveSnapShot(SnapShot&& snap) X_ABSTRACT;

    virtual void connectAndMoveToLobby(LobbyType::Enum type, SystemAddress sa) X_ABSTRACT;
    virtual void peerJoinedLobby(LobbyType::Enum type, SystemHandle systemHandle) X_ABSTRACT;

    virtual void leaveGameLobby(void) X_ABSTRACT;

    virtual void endGame(bool early) X_ABSTRACT;
};


X_NAMESPACE_END