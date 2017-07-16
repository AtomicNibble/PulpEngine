#pragma once

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)

X_NAMESPACE_BEGIN(game)

class GameVars
{
public:
	GameVars();
	~GameVars() = default;

	void registerVars(void);



private:
	core::ICVar* pFovVar_;

	Vec3f cameraPos_;
	Vec3f cameraAngle_;
};

X_NAMESPACE_END

#include "GameVars.inl"