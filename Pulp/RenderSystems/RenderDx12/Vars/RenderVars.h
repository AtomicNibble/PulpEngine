#pragma once

X_NAMESPACE_DECLARE(core,
                    struct ICVar;)

X_NAMESPACE_BEGIN(render)

class RenderVars
{
public:
    RenderVars();
    ~RenderVars() = default;

    void registerVars(void);

    X_INLINE bool varsRegisterd(void) const; // check if vars are init
    X_INLINE bool enableDebugLayer(void) const;
    X_INLINE const Colorf& getClearCol(void) const;

    void setNativeRes(const Vec2i& res);

private:
    bool varsRegisterd_;
    bool _pad[3];

    int32_t debugLayer_;
    Colorf clearColor_;

    core::ICVar* pNativeRes_;
};

X_NAMESPACE_END

#include "RenderVars.inl"