#pragma once

#include <IMaterial.h>
#include <IRender.h>

X_NAMESPACE_DECLARE(core,
	struct XFile;
);

X_NAMESPACE_BEGIN(engine)

class TechSetDefs;
class TechSetDef;

class MaterialCompiler
{
	struct Sampler
	{
		core::string name;
		render::FilterType::Enum filterType;
		render::TexRepeat::Enum texRepeat;
	};

	struct Param
	{
		core::string name;
		core::string val;
	};

	typedef core::Array<Sampler> SamplerArr;
	typedef core::Array<Param> ParamArr;

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

	core::string techType_;

	SamplerArr samplers_;
	ParamArr params_;

	engine::TechSetDef* pTechDef_;
};


X_NAMESPACE_END