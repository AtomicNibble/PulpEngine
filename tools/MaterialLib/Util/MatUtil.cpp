#include "stdafx.h"
#include "MatUtil.h"

#include <Hashing\Fnva1Hash.h>


X_NAMESPACE_BEGIN(engine)

namespace Util
{

	MaterialType::Enum MatTypeFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(str, core::strUtil::strlen(str)))
		{
			case "world"_fnv1a:
				return MaterialType::WORLD;
			case "ui"_fnv1a:
				return MaterialType::UI;
			case "model"_fnv1a:
				return MaterialType::MODEL;
			case "tool"_fnv1a:
				return MaterialType::WORLD;

			default:
				X_ERROR("Mtl", "Unknown material type: '%s' (case-sen)", str);
				return MaterialType::UNKNOWN;
		}
	}

	MaterialUsage::Enum MatUsageFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		core::StackString<96, char> strUpper(str);
		strUpper.toLower();

		switch (core::Hash::Fnv1aHash(strUpper.c_str(), strUpper.length()))
		{
			case "none"_fnv1a:
				return MaterialUsage::NONE;
			case "door"_fnv1a:
				return MaterialUsage::DOOR;
			case "floor"_fnv1a:
				return MaterialUsage::FLOOR;
			case "celing"_fnv1a:
				return MaterialUsage::CELING;
			case "roof"_fnv1a:
				return MaterialUsage::ROOF;
			case "window"_fnv1a:
				return MaterialUsage::WINDOW;
			case "sky"_fnv1a:
				return MaterialUsage::SKY;

			default:
				X_ERROR("Mtl", "Unknown material usage: '%s'", str);
				return MaterialUsage::NONE;
		}
	}


	MaterialSurType::Enum MatSurfaceTypeFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		core::StackString<96, char> strUpper(str);
		strUpper.toLower();

		switch (core::Hash::Fnv1aHash(strUpper.c_str(), strUpper.length()))
		{
			case "none"_fnv1a:
				return MaterialSurType::NONE;
			case "brick"_fnv1a:
				return MaterialSurType::BRICK;
			case "concrete"_fnv1a:
				return MaterialSurType::CONCRETE;
			case "cloth"_fnv1a:
				return MaterialSurType::CLOTH;
			case "flesh"_fnv1a:
				return MaterialSurType::FLESH;
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
			case "mud"_fnv1a:
				return MaterialSurType::MUD;
			case "plastic"_fnv1a:
				return MaterialSurType::PLASTIC;
			case "paper"_fnv1a:
				return MaterialSurType::PAPER;
			case "rock"_fnv1a:
				return MaterialSurType::ROCK;
			case "snow"_fnv1a:
				return MaterialSurType::SNOW;
			case "sand"_fnv1a:
				return MaterialSurType::SAND;
			case "wood"_fnv1a:
				return MaterialSurType::WOOD;
			case "water"_fnv1a:
				return MaterialSurType::WATER;

			default:
				X_ERROR("Mtl", "Unknown material surface type: '%s'", str);
				return MaterialSurType::NONE;
		}
	}

	MaterialFilterType::Enum MatFilterTypeFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(str, core::strUtil::strlen(str)))
		{
			case "nearest (mip none)"_fnv1a:
				return MaterialFilterType::NEAREST_MIP_NONE;
			case "nearest (mip nearest)"_fnv1a:
				return MaterialFilterType::NEAREST_MIP_NEAREST;
			case "nearest (mip linear)"_fnv1a:
				return MaterialFilterType::NEAREST_MIP_LINEAR;

			case "linear (mip none)"_fnv1a:
				return MaterialFilterType::LINEAR_MIP_NONE;
			case "linear (mip nearest)"_fnv1a:
				return MaterialFilterType::LINEAR_MIP_NEAREST;
			case "linear (mip linear)"_fnv1a:
				return MaterialFilterType::LINEAR_MIP_LINEAR;

			case "anisotropicx2"_fnv1a:
				return MaterialFilterType::ANISOTROPIC_X2;
			case "anisotropicx4"_fnv1a:
				return MaterialFilterType::ANISOTROPIC_X4;
			case "anisotropicx8"_fnv1a:
				return MaterialFilterType::ANISOTROPIC_X8;
			case "anisotropicx16"_fnv1a:
				return MaterialFilterType::ANISOTROPIC_X16;

			default:
				X_ERROR("Mtl", "Unknown filter type: '%s' (case-sen)", str);
				return MaterialFilterType::NEAREST_MIP_NEAREST;
		}
	}

	MaterialTexRepeat::Enum MatTexRepeatFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(str, core::strUtil::strlen(str)))
		{
			case "no tile"_fnv1a:
				return MaterialTexRepeat::NO_TILE;
			case "tile both"_fnv1a:
				return MaterialTexRepeat::TILE_BOTH;
			case "tile horizontal"_fnv1a:
				return MaterialTexRepeat::TILE_HOZ;
			case "tile vertical"_fnv1a:
				return MaterialTexRepeat::TILE_VERT;

			default:
				X_ERROR("Mtl", "Unknown tex repeat: '%s' (case-sen)", str);
				return MaterialTexRepeat::TILE_BOTH;
		}
	}

	MaterialPolygonOffset::Enum MatPolyOffsetFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(str, core::strUtil::strlen(str)))
		{
			case "none"_fnv1a:
				return MaterialPolygonOffset::NONE;
			case "decal"_fnv1a:
				return MaterialPolygonOffset::STATIC_DECAL;
			case "impact"_fnv1a:
				return MaterialPolygonOffset::WEAPON_IMPACT;

			default:
				X_ERROR("Mtl", "Unknown poly offset: '%s' (case-sen)", str);
				return MaterialPolygonOffset::NONE;
		}
	}

	MaterialCullType::Enum MatCullTypeFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(str, core::strUtil::strlen(str)))
		{
			case "back"_fnv1a:
				return MaterialCullType::BACK_SIDED;
			case "front"_fnv1a:
				return MaterialCullType::FRONT_SIDED;
			case "none"_fnv1a:
				return MaterialCullType::TWO_SIDED;

			default:
				X_ERROR("Mtl", "Unknown cull type: '%s' (case-sen)", str);
				return MaterialCullType::BACK_SIDED;
		}
	}

	MaterialBlendType::Enum MatBlendTypeFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(str, core::strUtil::strlen(str)))
		{
			case "zero"_fnv1a:
				return MaterialBlendType::ZERO;
			case "one"_fnv1a:
				return MaterialBlendType::ONE;

			case "src_color"_fnv1a:
				return MaterialBlendType::SRC_COLOR;
			case "src_alpha"_fnv1a:
				return MaterialBlendType::SRC_ALPHA;
			case "src_alpha_sat"_fnv1a:
				return MaterialBlendType::SRC_ALPHA_SAT;
			case "src1_color"_fnv1a:
				return MaterialBlendType::SRC1_COLOR;
			case "src1_alpha"_fnv1a:
				return MaterialBlendType::SRC1_ALPHA;

			case "inv_src_color"_fnv1a:
				return MaterialBlendType::INV_SRC_COLOR;
			case "inv_src1_alpha"_fnv1a:
				return MaterialBlendType::INV_SRC1_ALPHA;

			case "dest_color"_fnv1a:
				return MaterialBlendType::DEST_COLOR;
			case "dest_alpha"_fnv1a:
				return MaterialBlendType::DEST_ALPHA;

			case "inv_dest_color"_fnv1a:
				return MaterialBlendType::INV_DEST_COLOR;
			case "inv_dest_alpha"_fnv1a:
				return MaterialBlendType::INV_DEST_ALPHA;

			case "blend_factor"_fnv1a:
				return MaterialBlendType::BLEND_FACTOR;
			case "inv_blend_factor"_fnv1a:
				return MaterialBlendType::INV_BLEND_FACTOR;


			default:
				X_ERROR("Mtl", "Unknown blend type: '%s' (case-sen)", str);
				return MaterialBlendType::INVALID;
		}
	}

	StencilOperation::Enum StencilOpFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(str, core::strUtil::strlen(str)))
		{
			case "keep"_fnv1a:
				return StencilOperation::KEEP;
			case "zero"_fnv1a:
				return StencilOperation::ZERO;
			case "replace"_fnv1a:
				return StencilOperation::REPLACE;
			case "incr_sat"_fnv1a:
				return StencilOperation::INCR_SAT;
			case "decr_sat"_fnv1a:
				return StencilOperation::DECR_SAT;
			case "invert"_fnv1a:
				return StencilOperation::INVERT;
			case "incr"_fnv1a:
				return StencilOperation::INCR;
			case "decr"_fnv1a:
				return StencilOperation::DECR;

			default:
				X_ERROR("Mtl", "Unknown stencil op: '%s' (case-sen)", str);
				return StencilOperation::KEEP;
		}
	}

	StencilFunc::Enum StencilFuncFromStr(const char* str)
	{
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(str, core::strUtil::strlen(str)))
		{
			case "never"_fnv1a:
				return StencilFunc::NEVER;
			case "less"_fnv1a:
				return StencilFunc::LESS;
			case "equal"_fnv1a:
				return StencilFunc::EQUAL;
			case "less_equal"_fnv1a:
				return StencilFunc::LESS_EQUAL;
			case "greater"_fnv1a:
				return StencilFunc::GREATER;
			case "not_equal"_fnv1a:
				return StencilFunc::NOT_EQUAL;
			case "greater_equal"_fnv1a:
				return StencilFunc::GREATER_EQUAL;
			case "always"_fnv1a:
				return StencilFunc::ALWAYS;

			default:
				X_ERROR("Mtl", "Unknown stencil func: '%s' (case-sen)", str);
				return StencilFunc::ALWAYS;
		}
	}

} // namespace Util

X_NAMESPACE_END