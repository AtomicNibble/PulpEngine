#pragma once

#include <IMaterial.h>

X_NAMESPACE_BEGIN(engine)


class MaterialCompiler
{
	struct Tex
	{
		core::string name;
		MaterialFilterType::Enum filterType_;
		MaterialTexRepeat::Enum texRepeat_;
	};

public:
	MaterialCompiler();

	bool loadFromJson(core::string& str);

private:
	bool parseTexInfo(core::json::Document& doc, Tex& tex, const char* pName);

private:
	MaterialFlags flags_;
	MaterialType::Enum type_;
	MaterialUsage::Enum usage_;
	MaterialSurType::Enum surType_;

	Tex colMap_;
	Tex normalMap_;
	Tex detailNormalMap_;
	Tex specColMap_;
};


X_NAMESPACE_END