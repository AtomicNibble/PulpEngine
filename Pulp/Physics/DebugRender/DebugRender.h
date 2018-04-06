#pragma once

X_NAMESPACE_DECLARE(engine,
                    class IPrimativeContext;);

X_NAMESPACE_BEGIN(physics)

class DebugRender
{
public:
    DebugRender(engine::IPrimativeContext* pPrimCon);
    ~DebugRender();

public:
    void update(const physx::PxRenderBuffer& debugRenderable);
    void queueForRender(void);
    void clear(void);

private:
    engine::IPrimativeContext* pPrimCon_;
};

X_NAMESPACE_END
