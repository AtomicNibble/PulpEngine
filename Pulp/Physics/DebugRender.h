#pragma once

X_NAMESPACE_DECLARE(render,
	struct IRenderAux;
);

X_NAMESPACE_BEGIN(physics)

class DebugRender 
{

public:
	DebugRender(render::IRenderAux* pAuxRender);
	~DebugRender();


public:
	void update(const physx::PxRenderBuffer& debugRenderable);
	void queueForRender(void);
	void clear(void);


private:
	render::IRenderAux* pAuxRender_;
};


X_NAMESPACE_END
