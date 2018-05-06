#pragma once

#include <ICompression.h>

X_NAMESPACE_DECLARE(core,
    struct ICVar)

X_NAMESPACE_BEGIN(net)

class SessionVars
{
public:
    SessionVars();
    ~SessionVars() = default;

    void registerVars(void);

    X_INLINE int32_t connectionAttemps(void) const;
    X_INLINE int32_t connectionRetryDelayMs(void) const;
    X_INLINE bool snapFroceDrop(void);

private:
    int32_t connectionAttemps_;
    int32_t connectionRetyDelayMs_;
    int32_t snapFroceDrop_;

};

X_NAMESPACE_END

#include "SessionVars.inl"