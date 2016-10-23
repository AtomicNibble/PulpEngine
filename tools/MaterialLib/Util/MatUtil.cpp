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



} // namespace Util

X_NAMESPACE_END