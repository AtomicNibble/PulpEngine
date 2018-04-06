#pragma once

X_NAMESPACE_DECLARE(core,
                    struct ICVar;)

X_NAMESPACE_BEGIN(engine)

class MaterialVars
{
public:
    MaterialVars();
    ~MaterialVars() = default;

    void registerVars(void);

    X_INLINE int32_t maxActiveLoadReq(void) const;

private:
    int32_t maxActiveLoadReq_;
};

X_NAMESPACE_END

#include "MaterialVars.inl"