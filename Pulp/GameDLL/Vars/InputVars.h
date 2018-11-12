#pragma once

X_NAMESPACE_DECLARE(core,
    struct ICVar;)

X_NAMESPACE_BEGIN(game)

class InputVars
{
public:
    InputVars();
    ~InputVars() = default;

    void registerVars(void);

    X_INLINE bool toggleRun(void) const;
    X_INLINE bool toggleCrouch(void) const;
    X_INLINE bool toggleZoom(void) const;

private:
    int32_t toggleRun_;
    int32_t toggleCrouch_;
    int32_t toggleZoom_;
};

X_NAMESPACE_END

#include "InputVars.inl"
