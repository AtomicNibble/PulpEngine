#pragma once

#include <IMaterial.h>

X_NAMESPACE_BEGIN(engine)


class MaterialCompiler
{
	struct Tex
	{
		bool parse(core::json::Document& doc, const char* pName);

	public:
		core::string name;
		MaterialFilterType::Enum filterType_;
		MaterialTexRepeat::Enum texRepeat_;
	};

public:
	MaterialCompiler();

	bool loadFromJson(core::string& str);

private:

private:
	MaterialFlags flags_;
	MaterialType::Enum type_;
	MaterialUsage::Enum usage_;
	MaterialSurType::Enum surType_;

	MaterialPolygonOffset::Enum polyOffset_;
	MaterialCullType::Enum cullType_;


	Tex colMap_;
	Tex normalMap_;
	Tex detailNormalMap_;
	Tex specColMap_;
};


X_NAMESPACE_END