#pragma once

#include <IMaterial.h>
#include <IRender.h>

X_NAMESPACE_DECLARE(core,
	struct XFile;
);

X_NAMESPACE_BEGIN(engine)

class TechSetDefs;

class MaterialCompiler
{
	struct Tex
	{
		Tex();

		bool parse(core::json::Document& doc, const char* pName);
		bool write(core::XFile* pFile) const;
		bool writeName(core::XFile* pFile) const;

	public:
		core::string name;
		render::FilterType::Enum filterType_;
		render::TexRepeat::Enum texRepeat_;
	};

public:
	MaterialCompiler(TechSetDefs& techDefs);

	bool loadFromJson(core::string& str);
	bool writeToFile(core::XFile* pFile) const;

private:
	bool hasFlagAndTrue(core::json::Document& d, const char* pName);

	template<typename FlagClass, size_t Num>
	bool processFlagGroup(core::json::Document& doc, FlagClass& flags,
		const std::array<std::pair<const char*, typename FlagClass::Enum>, Num>& flagValues);


private:
	TechSetDefs& techDefs_;

	MaterialFlags flags_;
	MaterialCat::Enum cat_;
	MaterialUsage::Enum usage_;

	MaterialSurType::Enum surType_;
	MaterialPolygonOffset::Enum polyOffset_;
	MaterialCoverage::Enum coverage_;
	MaterialMountType::Enum mountType_;

	render::StateDesc stateDesc_;

	Vec2<int16_t> tiling_;
	Vec2f uvScroll_;


	Tex colMap_;
	Tex normalMap_;
	Tex detailNormalMap_;
	Tex specColMap_;
	core::string techType_;
};


X_NAMESPACE_END