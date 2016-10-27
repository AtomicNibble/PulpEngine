#pragma once

#include <IMaterial.h>

X_NAMESPACE_DECLARE(core,
	struct XFile;
);

X_NAMESPACE_BEGIN(engine)

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
		MaterialFilterType::Enum filterType_;
		MaterialTexRepeat::Enum texRepeat_;
	};

public:
	MaterialCompiler();

	bool loadFromJson(core::string& str);
	bool writeToFile(core::XFile* pFile) const;

private:
	template<typename FlagClass, size_t Num>
	bool processFlagGroup(core::json::Document& doc, FlagClass& flags,
		const std::array<std::pair<const char*, typename FlagClass::Enum>, Num>& flagValues);


private:
	MaterialFlags flags_;
	MaterialStateFlags stateFlags_;
	MaterialCat::Enum cat_;
	MaterialUsage::Enum usage_;
	MaterialSurType::Enum surType_;
	MaterialPolygonOffset::Enum polyOffset_;
	MaterialCullType::Enum cullType_;
	MaterialCoverage::Enum coverage_;
	MaterialMountType::Enum mountType_;

	Vec2<int16_t> tiling_;
	Vec2f uvScroll_;

	// stencil
	StencilFunc::Enum depthTest_;

	// blend ops.
	MaterialBlendType::Enum srcBlendColor_;
	MaterialBlendType::Enum dstBlendColor_;
	MaterialBlendType::Enum srcBlendAlpha_;
	MaterialBlendType::Enum dstBlendAlpha_;

	Tex colMap_;
	Tex normalMap_;
	Tex detailNormalMap_;
	Tex specColMap_;
};


X_NAMESPACE_END