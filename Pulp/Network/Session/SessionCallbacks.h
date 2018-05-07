#pragma once

X_NAMESPACE_BEGIN(net)

class SnapShot;

struct ISessionCallbacks
{
    virtual ~ISessionCallbacks() = default;

    virtual void onLostConnectionToHost(void) X_ABSTRACT;

    virtual void onReciveSnapShot(SnapShot&& snap) X_ABSTRACT;
};


X_NAMESPACE_END