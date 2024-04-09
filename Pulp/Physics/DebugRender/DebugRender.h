#pragma once

X_NAMESPACE_DECLARE(engine,
                    class IPrimitiveContext);

X_NAMESPACE_BEGIN(physics)

class DebugRender
{
public:
    DebugRender(engine::IPrimitiveContext* pPrimCon);
    ~DebugRender();

public:
    void update(const physx::PxRenderBuffer& debugRenderable);
    void queueForRender(void);
    void clear(void);

private:
    engine::IPrimitiveContext* pPrimCon_;
};

X_NAMESPACE_END
