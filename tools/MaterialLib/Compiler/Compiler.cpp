#include "stdafx.h"
#include "Compiler.h"

#include "Util\MatUtil.h"

X_NAMESPACE_BEGIN(engine)


bool MaterialCompiler::Tex::parse(core::json::Document& d, const char* pName)
{
	core::StackString<64, char> map("map");
	core::StackString<64, char> tile("tile");
	core::StackString<64, char> filter("filter");

	map.append(pName);
	tile.append(pName);
	filter.append(pName);

	if (!d.HasMember(map.c_str())) {
		X_ERROR("Mat", "Missing \"%s\" value", map.c_str());
		return false;
	}
	if (!d.HasMember(tile.c_str())) {
		X_ERROR("Mat", "Missing \"%s\" value", tile.c_str());
		return false;
	}
	if (!d.HasMember(filter.c_str())) {
		X_ERROR("Mat", "Missing \"%s\" value", filter.c_str());
		return false;
	}

	const char* pMapName = d[map.c_str()].GetString();
	const char* pTileMode = d[tile.c_str()].GetString();
	const char* pFilterType = d[filter.c_str()].GetString();

	name = pMapName;
	filterType_ = Util::MatFilterTypeFromStr(pFilterType);
	texRepeat_ = Util::MatTexRepeatFromStr(pTileMode);
	return true;
}

MaterialCompiler::MaterialCompiler()
{

}


bool MaterialCompiler::loadFromJson(core::string& str)
{
	core::json::Document d;
	d.Parse(str.c_str(), str.length());

	// find all the things.
	std::array<std::pair<const char*, core::json::Type>, 3> requiredValues = { {
			{ "cat", core::json::kStringType },
			{ "surface_type", core::json::kStringType },
			{ "usage", core::json::kStringType }
		}
	};

	for (size_t i = 0; i < requiredValues.size(); i++)
	{
		const auto& item = requiredValues[i];

		if (!d.HasMember(item.first)) {
			X_ERROR("Mat", "Missing required value: \"%s\"", item.first);
			return false;
		}

		if (d[item.first].GetType() != item.second) {
			return false;
		}
	}

	const char* pCat = d["cat"].GetString();
	const char* pUsage = d["usage"].GetString();
	const char* pSurfaceType = d["surface_type"].GetString();
	const char* pPolyOffset = d["polyOffset"].GetString();
	const char* pCullFace = d["cullFace"].GetString();

	type_ = Util::MatTypeFromStr(pCat);
	usage_ = Util::MatUsageFromStr(pUsage);
	surType_ = Util::MatSurfaceTypeFromStr(pSurfaceType);
	polyOffset_ = Util::MatPolyOffsetFromStr(pPolyOffset);
	cullType_ = Util::MatCullTypeFromStr(pCullFace);
	
	// now we do some flag parsing.
	flags_.Clear();

	std::array<std::pair<const char*, MaterialFlag::Enum>, 15> flags = { {
			{ "f_nodraw", MaterialFlag::NODRAW }, 
			{ "f_solid", MaterialFlag::SOLID },
			{ "f_structual", MaterialFlag::STRUCTURAL },
			{ "f_detail", MaterialFlag::DETAIL },
			{ "f_portal", MaterialFlag::PORTAL },
			{ "f_mount", MaterialFlag::MOUNT },
			{ "f_player_clip", MaterialFlag::PLAYER_CLIP },
			{ "f_ai_clip", MaterialFlag::AI_CLIP },
			{ "f_bullet_clip", MaterialFlag::BULLET_CLIP },
			{ "f_missile_clip", MaterialFlag::MISSLE_CLIP },
			{ "f_vehicle_clip", MaterialFlag::VEHICLE_CLIP },
			{ "f_no_fall_dmg", MaterialFlag::NO_FALL_DMG },
			{ "f_no_impact", MaterialFlag::NO_IMPACT },
			{ "f_no_pennetrate", MaterialFlag::NO_PENNETRATE },
			{ "f_no_steps", MaterialFlag::NO_STEPS }
		}
	};

	// process all the flags.
	for (size_t i = 0; i < flags.size(); i++)
	{
		const auto& flag = flags[i];

		if (d.HasMember(flag.first))
		{
			const auto& val = d[flags[i].first];

			switch (val.GetType())
			{
				case core::json::kFalseType:
					// do nothing
					break;
				case core::json::kTrueType:
					flags_.Set(flag.second);
					break;
				case core::json::kNumberType:
					if (val.IsBool()) {
						if (val.GetBool()) {
							flags_.Set(flag.second);
						}
						break;
					}
					// fall through if not bool
				default:
					X_ERROR("Mat", "Flag \"%s\" has a value with a incorrect type: %" PRIi32, flag.first, val.GetType());
					return false;
			}
		}
	}


	// col map.
	if (!colMap_.parse(d, "Color")) {
		X_ERROR("Mat", "Failed to parse texture info");
		return false;
	}
	if (!normalMap_.parse(d, "Normal")) {
		X_ERROR("Mat", "Failed to parse texture info");
		return false;
	}
	if (!detailNormalMap_.parse(d, "DetailNormal")) {
		X_ERROR("Mat", "Failed to parse texture info");
		return false;
	}
	if (!specColMap_.parse(d, "SpecCol")) {
		X_ERROR("Mat", "Failed to parse texture info");
		return false;
	}

	return true;
}


X_NAMESPACE_END