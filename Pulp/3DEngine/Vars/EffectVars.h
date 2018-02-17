#pragma once

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)


X_NAMESPACE_BEGIN(engine)


class EffectVars
{
public:
	EffectVars();
	~EffectVars() = default;

	void registerVars(void);

	X_INLINE int32_t drawDebug(void) const;
	X_INLINE int32_t drawElemRect(void) const;
	X_INLINE float axisExtent(void) const;


private:
	int32_t drawDebug_;
	int32_t drawElemRect_;
	float axisExtent_;
};


X_NAMESPACE_END

#include "EffectVars.inl"