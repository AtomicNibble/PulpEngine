#pragma once



X_NAMESPACE_DECLARE(core,
	struct ICVar;
)

X_NAMESPACE_BEGIN(net)


class NetVars
{
public:
	NetVars();
	~NetVars() = default;

	void registerVars(void);

	X_INLINE int32_t debugEnabled(void) const;


private:
	int32_t debug_;

};


X_NAMESPACE_END


#include "NetVars.inl"