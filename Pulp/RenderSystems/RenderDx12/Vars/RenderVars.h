#pragma once


X_NAMESPACE_DECLARE(core,
	struct ICVar;
)

X_NAMESPACE_BEGIN(render)


class RenderVars
{
public:
	RenderVars();
	~RenderVars() = default;

	void registerVars(void);

	X_INLINE bool drawAux(void) const;
	X_INLINE const Colorf& getClearCol(void) const;

private:
	int32_t drawAux_;
	Colorf r_clear_color;
};



X_NAMESPACE_END


#include "RenderVars.inl"