#pragma once

#include <IMaterial.h>

X_NAMESPACE_BEGIN(engine)


class MaterialCompiler
{
public:
	MaterialCompiler();

	bool loadFromJson(core::string& str);

private:
	MaterialFlags flags_;
	MaterialType::Enum type_;
	MaterialUsage::Enum usage_;
	MaterialSurType::Enum surType_;
};


X_NAMESPACE_END