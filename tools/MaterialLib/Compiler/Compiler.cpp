#include "stdafx.h"
#include "Compiler.h"

#include "Util\MatUtil.h"

X_NAMESPACE_BEGIN(engine)


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

	for (int32_t i = 0; i < requiredValues.size(); i++)
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

	type_ = Util::MatTypeFromStr(pCat);
	usage_ = Util::MatUsageFromStr(pUsage);
	surType_ = Util::MatSurfaceTypeFromStr(pSurfaceType);

	
	// now we do some flag parsing.
	flags_.Clear();

	std::array<std::pair<const char*, MaterialFlag::Enum>, 11> flags = { {
			{ "f_nodraw", MaterialFlag::NODRAW }, 
			{ "f_solid", MaterialFlag::NODRAW },
			{ "f_structual", MaterialFlag::NODRAW },
			{ "f_detail", MaterialFlag::NODRAW },
			{ "f_portal", MaterialFlag::NODRAW },
			{ "f_player_clip", MaterialFlag::NODRAW },
			{ "f_ai_clip", MaterialFlag::NODRAW },
			{ "f_no_fall_dmg", MaterialFlag::NODRAW },
			{ "f_no_impact", MaterialFlag::NODRAW },
			{ "f_no_pennetrate", MaterialFlag::NODRAW },
			{ "f_no_steps", MaterialFlag::NODRAW }
		}
	};

	// process all the flags.
	for (int32_t i = 0; i < flags.size(); i++)
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



	return true;
}

X_NAMESPACE_END