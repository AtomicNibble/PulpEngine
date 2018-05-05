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

    X_INLINE int32_t debugEnabled(void) const;

private:
    int32_t debug_;

};

X_NAMESPACE_END

#include "SessionVars.inl"