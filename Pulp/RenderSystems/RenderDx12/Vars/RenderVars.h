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

	X_INLINE bool varsRegisterd(void) const; // check if vars are init
	X_INLINE bool enableDebugLayer(void) const;
	X_INLINE bool drawAux(void) const;
	X_INLINE const Colorf& getClearCol(void) const;

	void setNativeRes(const Vec2<uint32_t>& res);
	void setRes(const Vec2<uint32_t>& res);

private:
	bool varsRegisterd_;
	bool _pad[3];

	int32_t debugLayer_;
	int32_t drawAux_;
	Colorf clearColor_;

	core::ICVar* pNativeRes_;
	core::ICVar* pRes_;
};



X_NAMESPACE_END


#include "RenderVars.inl"