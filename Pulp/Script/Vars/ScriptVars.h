#pragma once

X_NAMESPACE_DECLARE(core,
	struct ICVar;
)

X_NAMESPACE_BEGIN(script)


class ScriptVars
{
public:
	ScriptVars();
	~ScriptVars() = default;

	void registerVars(void);

	X_INLINE int32_t debugEnabled(void) const;
	X_INLINE int32_t gcStepSize(void) const;

private:
	int32_t debug_;
	int32_t gcStepSize_;
};

X_NAMESPACE_END

#include "ScriptVars.inl"