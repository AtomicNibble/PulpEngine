#pragma once

X_NAMESPACE_BEGIN(net)

struct ISessionCallbacks
{
    virtual ~ISessionCallbacks() = default;

    virtual void onLostConnectionToHost(void) X_ABSTRACT;
};


X_NAMESPACE_END