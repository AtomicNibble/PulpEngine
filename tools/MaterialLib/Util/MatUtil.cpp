#include "stdafx.h"
#include "MatUtil.h"

#include <Hashing\Fnva1Hash.h>


X_NAMESPACE_BEGIN(engine)

namespace Util
{
	MaterialMountType::Enum MatMountTypeFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(MaterialMountType::ENUM_COUNT == 6, "Added additional material mount types? this code needs updating.");

		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "none"_fnv1a:
			case "<none>"_fnv1a:
				return MaterialMountType::NONE;
			case "ladder"_fnv1a:
				return MaterialMountType::LADDER;
			case "mantleon"_fnv1a:
				return MaterialMountType::MANTLEON;
			case "mantleover"_fnv1a:
				return MaterialMountType::MANTLEOVER;
			case "climbwall"_fnv1a:
				return MaterialMountType::CLIMBWALL;
			case "climbpipe"_fnv1a:
				return MaterialMountType::CLIMBPIPE;

			default:
				X_ERROR("Mtl", "Unknown material mount type: '%.*s' (case-sen)", len, pBegin);
				return MaterialMountType::NONE;
		}
	}

	MaterialCat::Enum MatCatFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(MaterialCat::ENUM_COUNT == 9, "Added additional material cats? this code needs updating.");

		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "geo"_fnv1a:
				return MaterialCat::GEO;
			case "decal"_fnv1a:
				return MaterialCat::DECAL;
			case "ui"_fnv1a:
				return MaterialCat::UI;
			case "tool"_fnv1a:
				return MaterialCat::TOOL;
			case "code"_fnv1a:
				return MaterialCat::CODE;
			case "filters"_fnv1a:
				return MaterialCat::FILTERS;
			case "weapon"_fnv1a:
				return MaterialCat::WEAPON;
			case "effect"_fnv1a:
				return MaterialCat::EFFECT;

			default:
				X_ERROR("Mtl", "Unknown material cat: '%.*s' (case-sen)", len, pBegin);
				return MaterialCat::UNKNOWN;
		}
	}

	MaterialUsage::Enum MatUsageFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		core::StackString<96, char> strUpper(pBegin, pEnd);
		strUpper.toLower();

		static_assert(MaterialUsage::ENUM_COUNT == 14, "Added additional material usage? this code needs updating.");

		switch (core::Hash::Fnv1aHash(strUpper.c_str(), strUpper.length()))
		{
			case "none"_fnv1a:
			case "<none>"_fnv1a:
				return MaterialUsage::NONE;
			case "door"_fnv1a:
				return MaterialUsage::DOOR;
			case "floor"_fnv1a:
				return MaterialUsage::FLOOR;
			case "celing"_fnv1a:
				return MaterialUsage::CELING;
			case "roof"_fnv1a:
				return MaterialUsage::ROOF;
			case "wall_interior"_fnv1a:
				return MaterialUsage::WALL_INTERIOR;
			case "wall_exterior"_fnv1a:
				return MaterialUsage::WALL_EXTERIOR;
			case "trim_interior"_fnv1a:
				return MaterialUsage::TRIM_INTERIOR;
			case "trim_exterior"_fnv1a:
				return MaterialUsage::TRIM_EXTERIOR;
			case "window"_fnv1a:
				return MaterialUsage::WINDOW;
			case "foliage"_fnv1a:
				return MaterialUsage::FOLIAGE;
			case "water"_fnv1a:
				return MaterialUsage::WATER;
			case "sky"_fnv1a:
				return MaterialUsage::SKY;
			case "decal"_fnv1a:
				return MaterialUsage::DECAL;

			default:
				X_ERROR("Mtl", "Unknown material usage: '%.*s'", strUpper.length(), pBegin);
				return MaterialUsage::NONE;
		}
	}


	MaterialSurType::Enum MatSurfaceTypeFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		core::StackString<96, char> strUpper(pBegin, pEnd);
		strUpper.toLower();

		static_assert(MaterialSurType::ENUM_COUNT == 26, "Added additional surface types? this code needs updating.");

		switch (core::Hash::Fnv1aHash(strUpper.c_str(), strUpper.length()))
		{
			case "none"_fnv1a:
			case "<none>"_fnv1a:
				return MaterialSurType::NONE;

			case "brick"_fnv1a:
				return MaterialSurType::BRICK;
			case "concrete"_fnv1a:
				return MaterialSurType::CONCRETE;
			case "cloth"_fnv1a:
				return MaterialSurType::CLOTH;
			case "carpet"_fnv1a:
				return MaterialSurType::CARPET;
			case "ceramic"_fnv1a:
				return MaterialSurType::CERAMIC;

			case "dirt"_fnv1a:
				return MaterialSurType::DIRT;

			case "flesh"_fnv1a:
				return MaterialSurType::FLESH;
			case "foliage"_fnv1a:
				return MaterialSurType::FOLIAGE;


			case "glass"_fnv1a:
				return MaterialSurType::GLASS;
			case "grass"_fnv1a:
				return MaterialSurType::GRASS;
			case "gravel"_fnv1a:
				return MaterialSurType::GRAVEL;

			case "ice"_fnv1a:
				return MaterialSurType::ICE;

			case "metal"_fnv1a:
				return MaterialSurType::METAL;
			case "metal_thin"_fnv1a:
				return MaterialSurType::METAL_THIN;
			case "metal_hollow"_fnv1a:
				return MaterialSurType::METAL_HOLLOW;
			case "mud"_fnv1a:
				return MaterialSurType::MUD;

			case "plastic"_fnv1a:
				return MaterialSurType::PLASTIC;
			case "paper"_fnv1a:
				return MaterialSurType::PAPER;
			case "plaster"_fnv1a:
				return MaterialSurType::PLASTER;
			case "rock"_fnv1a:
				return MaterialSurType::ROCK;
			case "rubber"_fnv1a:
				return MaterialSurType::RUBBER;

			case "snow"_fnv1a:
				return MaterialSurType::SNOW;
			case "sand"_fnv1a:
				return MaterialSurType::SAND;

			case "wood"_fnv1a:
				return MaterialSurType::WOOD;
			case "water"_fnv1a:
				return MaterialSurType::WATER;

			default:
				X_ERROR("Mtl", "Unknown material surface type: '%.*s'", strUpper.length(), pBegin);
				return MaterialSurType::NONE;
		}
	}

	MaterialPolygonOffset::Enum MatPolyOffsetFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(MaterialPolygonOffset::ENUM_COUNT == 3, "Added additional polyoffsets? this code needs updating.");

		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "none"_fnv1a:
			case "<none>"_fnv1a:
				return MaterialPolygonOffset::NONE;
			case "decal"_fnv1a:
				return MaterialPolygonOffset::STATIC_DECAL;
			case "impact"_fnv1a:
				return MaterialPolygonOffset::WEAPON_IMPACT;

			default:
				X_ERROR("Mtl", "Unknown poly offset: '%.*s' (case-sen)", len, pBegin);
				return MaterialPolygonOffset::NONE;
		}
	}

	render::FilterType::Enum FilterTypeFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(render::FilterType::ENUM_COUNT == 10, "Added additional filter types? this code needs updating.");

		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "nearest (mip none)"_fnv1a:
				return render::FilterType::NEAREST_MIP_NONE;
			case "nearest (mip nearest)"_fnv1a:
				return render::FilterType::NEAREST_MIP_NEAREST;
			case "nearest (mip linear)"_fnv1a:
				return render::FilterType::NEAREST_MIP_LINEAR;

			case "linear (mip none)"_fnv1a:
				return render::FilterType::LINEAR_MIP_NONE;
			case "linear (mip nearest)"_fnv1a:
				return render::FilterType::LINEAR_MIP_NEAREST;
			case "linear (mip linear)"_fnv1a:
				return render::FilterType::LINEAR_MIP_LINEAR;

			case "anisotropicx2"_fnv1a:
				return render::FilterType::ANISOTROPIC_X2;
			case "anisotropicx4"_fnv1a:
				return render::FilterType::ANISOTROPIC_X4;
			case "anisotropicx8"_fnv1a:
				return render::FilterType::ANISOTROPIC_X8;
			case "anisotropicx16"_fnv1a:
				return render::FilterType::ANISOTROPIC_X16;

			default:
				X_ERROR("Mtl", "Unknown filter type: '%.*s' (case-sen)", len, pBegin);
				return render::FilterType::NEAREST_MIP_NEAREST;
		}
	}

	render::TexRepeat::Enum TexRepeatFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(render::TexRepeat::ENUM_COUNT == 4, "Added additional repeat types? this code needs updating.");

		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "no tile"_fnv1a:
				return render::TexRepeat::NO_TILE;
			case "tile both"_fnv1a:
				return render::TexRepeat::TILE_BOTH;
			case "tile horizontal"_fnv1a:
				return render::TexRepeat::TILE_HOZ;
			case "tile vertical"_fnv1a:
				return render::TexRepeat::TILE_VERT;

			default:
				X_ERROR("Mtl", "Unknown tex repeat: '%.*s' (case-sen)", len, pBegin);
				return render::TexRepeat::TILE_BOTH;
		}
	}

	render::CullType::Enum CullTypeFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(render::CullType::ENUM_COUNT == 3, "Added additional cull types? this code needs updating.");

		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "back"_fnv1a:
				return render::CullType::BACK_SIDED;
			case "front"_fnv1a:
				return render::CullType::FRONT_SIDED;
			case "none"_fnv1a:
				return render::CullType::NONE;

			default:
				X_ERROR("Mtl", "Unknown cull type: '%.*s' (case-sen)", len, pBegin);
				return render::CullType::BACK_SIDED;
		}
	}

	render::BlendType::Enum BlendTypeFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(render::BlendType::ENUM_COUNT == 18, "Added additional blend types? this code needs updating.");

		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "zero"_fnv1a:
				return render::BlendType::ZERO;
			case "one"_fnv1a:
				return render::BlendType::ONE;

			case "src_color"_fnv1a:
				return render::BlendType::SRC_COLOR;
			case "src_alpha"_fnv1a:
				return render::BlendType::SRC_ALPHA;
			case "src_alpha_sat"_fnv1a:
				return render::BlendType::SRC_ALPHA_SAT;
			case "src1_color"_fnv1a:
				return render::BlendType::SRC1_COLOR;
			case "src1_alpha"_fnv1a:
				return render::BlendType::SRC1_ALPHA;

			case "inv_src_color"_fnv1a:
				return render::BlendType::INV_SRC_COLOR;
			case "inv_src1_alpha"_fnv1a:
				return render::BlendType::INV_SRC1_ALPHA;

			case "dest_color"_fnv1a:
				return render::BlendType::DEST_COLOR;
			case "dest_alpha"_fnv1a:
				return render::BlendType::DEST_ALPHA;

			case "inv_dest_color"_fnv1a:
				return render::BlendType::INV_DEST_COLOR;
			case "inv_dest_alpha"_fnv1a:
				return render::BlendType::INV_DEST_ALPHA;

			case "blend_factor"_fnv1a:
				return render::BlendType::BLEND_FACTOR;
			case "inv_blend_factor"_fnv1a:
				return render::BlendType::INV_BLEND_FACTOR;


			default:
				X_ERROR("Mtl", "Unknown blend type: '%.*s' (case-sen)", len, pBegin);
				return render::BlendType::INVALID;
		}
	}

	render::BlendOp::Enum BlendOpFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(render::BlendOp::ENUM_COUNT == 5, "Added additional blend op? this code needs updating.");
		
		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "add"_fnv1a:
				return render::BlendOp::OP_ADD;
			case "sub"_fnv1a:
				return render::BlendOp::OP_SUB;
			case "reb_sub"_fnv1a:
				return render::BlendOp::OP_REB_SUB;
			case "min"_fnv1a:
				return render::BlendOp::OP_MIN;
			case "max"_fnv1a:
				return render::BlendOp::OP_MAX;

			default:
				X_ERROR("Mtl", "Unknown blend op: '%.*s' (case-sen)", len, pBegin);
				return render::BlendOp ::OP_ADD;
		}
	}


	render::StencilOperation::Enum StencilOpFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(render::StencilOperation::ENUM_COUNT == 8, "Added additional stencil op types? this code needs updating.");

		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "keep"_fnv1a:
				return render::StencilOperation::KEEP;
			case "zero"_fnv1a:
				return render::StencilOperation::ZERO;
			case "replace"_fnv1a:
				return render::StencilOperation::REPLACE;
			case "incr_sat"_fnv1a:
				return render::StencilOperation::INCR_SAT;
			case "decr_sat"_fnv1a:
				return render::StencilOperation::DECR_SAT;
			case "invert"_fnv1a:
				return render::StencilOperation::INVERT;
			case "incr"_fnv1a:
				return render::StencilOperation::INCR;
			case "decr"_fnv1a:
				return render::StencilOperation::DECR;

			default:
				X_ERROR("Mtl", "Unknown stencil op: '%.*s' (case-sen)", len, pBegin);
				return render::StencilOperation::KEEP;
		}
	}

	render::StencilFunc::Enum StencilFuncFromStr(const char* pBegin, const char* pEnd)
	{
		using namespace core::Hash::Fnva1Literals;

		static_assert(render::StencilFunc::ENUM_COUNT == 8, "Added additional stencil funcs? this code needs updating.");

		const size_t len = (pEnd - pBegin);
		switch (core::Hash::Fnv1aHash(pBegin, len))
		{
			case "never"_fnv1a:
				return render::StencilFunc::NEVER;
			case "less"_fnv1a:
				return render::StencilFunc::LESS;
			case "equal"_fnv1a:
				return render::StencilFunc::EQUAL;
			case "less_equal"_fnv1a:
				return render::StencilFunc::LESS_EQUAL;
			case "greater"_fnv1a:
				return render::StencilFunc::GREATER;
			case "not_equal"_fnv1a:
				return render::StencilFunc::NOT_EQUAL;
			case "greater_equal"_fnv1a:
				return render::StencilFunc::GREATER_EQUAL;
			case "always"_fnv1a:
				return render::StencilFunc::ALWAYS;

			default:
				X_ERROR("Mtl", "Unknown stencil func: '%.*s' (case-sen)", len, pBegin);
				return render::StencilFunc::ALWAYS;
		}
	}

	int16_t TilingSizeFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(str, core::strUtil::strlen(str)))
		{
			case "auto"_fnv1a:
			case "<auto>"_fnv1a:
				return engine::AUTO_TILING;
			
			default:
				break;
		}

		// should be power of two.
		const auto tile = core::strUtil::StringToInt<int32_t>(str);

		if (tile <= 0) {
			return engine::AUTO_TILING;
		}

		if (!core::bitUtil::IsPowerOfTwo(tile)) {
			X_ERROR("Mtl", "Tiling stride is not pow2: %" PRIi32, tile);
			return engine::AUTO_TILING;
		}

		if (tile > std::numeric_limits<int16_t>::max()) {
			X_ERROR("Mtl", "Tiling stride %" PRIi32 " exceeds limit of %" PRIi16, tile, std::numeric_limits<int16_t>::max());
			return engine::AUTO_TILING;
		}

		return tile;
	}

} // namespace Util

X_NAMESPACE_END