#pragma once


X_NAMESPACE_DECLARE(core,
	struct ICVar;
)


X_NAMESPACE_BEGIN(engine)


class MaterialVars
{
public:
	MaterialVars();
	~MaterialVars() = default;

	void registerVars(void);


private:

};


X_NAMESPACE_END

#include "MaterialVars.inl"