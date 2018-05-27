#pragma once

X_NAMESPACE_BEGIN(net)

class SnapShot;

struct ISessionCallbacks
{
    virtual ~ISessionCallbacks() = default;

    virtual SessionStatus::Enum getStatus(void) const X_ABSTRACT;

    virtual void onLostConnectionToHost(void) X_ABSTRACT;

    virtual void onReciveSnapShot(SnapShot&& snap) X_ABSTRACT;

    virtual void connectAndMoveToLobby(LobbyType::Enum type, SystemAddress sa) X_ABSTRACT;

    virtual void leaveGameLobby(void) X_ABSTRACT;

    virtual void endGame(bool early) X_ABSTRACT;
};


X_NAMESPACE_END