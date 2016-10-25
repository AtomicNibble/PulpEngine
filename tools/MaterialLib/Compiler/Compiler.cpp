#include "stdafx.h"
#include "Compiler.h"

#include <IAssetDb.h>
#include <IFileSys.h>

#include "Util\MatUtil.h"

X_NAMESPACE_BEGIN(engine)

MaterialCompiler::Tex::Tex() :
	filterType_(MaterialFilterType::LINEAR_MIP_LINEAR),
	texRepeat_(MaterialTexRepeat::TILE_BOTH)
{
}


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


bool MaterialCompiler::Tex::write(core::XFile* pFile) const
{
	MaterialTexture tex;
	tex.filterType = filterType_;
	tex.texRepeat = texRepeat_;
	tex.nameLen = safe_static_cast<decltype(MaterialTexture::nameLen), size_t>(name.length());

	if (pFile->writeObj(tex) != sizeof(tex)) {
		return false;
	}

	return true;
}

bool MaterialCompiler::Tex::writeName(core::XFile* pFile) const
{
	if (name.isEmpty()) {
		return true;
	}

	return pFile->writeString(name.c_str()) == name.length();
}

// --------------------------------------

MaterialCompiler::MaterialCompiler()
{

}


bool MaterialCompiler::loadFromJson(core::string& str)
{
	core::json::Document d;
	d.Parse(str.c_str(), str.length());

	// find all the things.
	std::array<std::pair<const char*, core::json::Type>, 5> requiredValues = { {
			{ "cat", core::json::kStringType },
			{ "usage", core::json::kStringType },
			{ "surface_type", core::json::kStringType },
			{ "polyOffset", core::json::kStringType },
			{ "cullFace", core::json::kStringType }
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

	// shieezz
	const char* pCat = d["cat"].GetString();
	const char* pUsage = d["usage"].GetString();
	const char* pSurfaceType = d["surface_type"].GetString();
	const char* pPolyOffset = d["polyOffset"].GetString();
	const char* pCullFace = d["cullFace"].GetString();

	cat_ = Util::MatCatFromStr(pCat);
	usage_ = Util::MatUsageFromStr(pUsage);
	surType_ = Util::MatSurfaceTypeFromStr(pSurfaceType);
	polyOffset_ = Util::MatPolyOffsetFromStr(pPolyOffset);
	cullType_ = Util::MatCullTypeFromStr(pCullFace);
	coverage_ = MaterialCoverage::OPAQUE;
	
	// blend ops.
	const char* pSrcCol = d["srcBlendColor"].GetString();
	const char* pDstCol = d["dstBlendColor"].GetString();
	const char* pSrcAlpha = d["srcBlendAlpha"].GetString();
	const char* pDstAlpha = d["dstBlendAlpha"].GetString();

	srcBlendColor_ = Util::MatBlendTypeFromStr(pSrcCol);
	dstBlendColor_ = Util::MatBlendTypeFromStr(pDstCol);
	srcBlendAlpha_ = Util::MatBlendTypeFromStr(pSrcAlpha);
	dstBlendAlpha_ = Util::MatBlendTypeFromStr(pDstAlpha);


	// tilling shit.
	// how many goats for a given N pickles
	const auto tillingWidthTyep = d["tilingWidth"].GetType();
	if (tillingWidthTyep != core::json::kNumberType) {
		tiling_.x = -1;
	}
	else {
		const int32_t tillingWidth = d["tilingWidth"].GetInt();
		tiling_.x = safe_static_cast<int16_t>(tillingWidth);
	}
	
	const auto tillingHeightType = d["tilingHeight"].GetType();
	if (tillingHeightType != core::json::kNumberType) {
		tiling_.y = -1;
	}
	else {
		const int32_t tillingHeight = d["tilingHeight"].GetInt();
		tiling_.y = safe_static_cast<int16_t>(tillingHeight);
	}



	// now we do some flag parsing.
	flags_.Clear();
	stateFlags_.Clear();

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

	std::array<std::pair<const char*, MaterialStateFlag::Enum>, 2> stateFlags = { {
			{ "depthWrite", MaterialStateFlag::DEPTHWRITE },
			{ "wireFrame", MaterialStateFlag::WIREFRAME }
		}
	};

	if (!processFlagGroup(d, flags_, flags)) {
		X_ERROR("Mat", "Failed to parse flags");
		return false;
	}
	if (!processFlagGroup(d, stateFlags_, stateFlags)) {
		X_ERROR("Mat", "Failed to parse state flags");
		return false;
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

bool MaterialCompiler::writeToFile(core::XFile* pFile) const
{
	// lets check asset will fit.
	// I don't do this in IMaterial just to save including IAssetDb.h in the header.
	static_assert(assetDb::ASSET_NAME_MAX_LENGTH <= std::numeric_limits<decltype(MaterialTexture::nameLen)>::max(),
		"Material only supports 255 max name len");

	MaterialHeader hdr;
	hdr.fourCC = MTL_B_FOURCC;
	hdr.version = MTL_B_VERSION;

	hdr.flags = flags_;
	hdr.stateFlags = stateFlags_;
	hdr.cat = cat_;
	hdr.usage = usage_;
	hdr.surfaceType = surType_;
	hdr.polyOffsetType = polyOffset_;
	hdr.cullType = cullType_;
	hdr.coverage = coverage_;

	hdr.depthTest = depthTest_;

	hdr.srcBlendColor = srcBlendColor_;
	hdr.dstBlendColor = dstBlendColor_;
	hdr.srcBlendAlpha = srcBlendAlpha_;
	hdr.dstBlendAlpha = dstBlendAlpha_;

	// textures.
	hdr.numTextures = 1;
	
	if (pFile->writeObj(hdr) != sizeof(hdr)) {
		return false;
	}

	// i want to just write all the tex blocks regardless if they are set or not.
	std::array<const Tex* const, 4> textures = {
		&colMap_,
		&normalMap_,
		&detailNormalMap_,
		&specColMap_
	};

	for (const auto& t : textures)
	{
		if (!t->write(pFile)) {
			return false;
		}
	}

	// now write the names.
	for (const auto& t : textures)
	{
		if (!t->writeName(pFile)) {
			return false;
		}
	}

	return true;
}

template<typename FlagClass, size_t Num>
bool MaterialCompiler::processFlagGroup(core::json::Document& d, FlagClass& flags, 
	const std::array<std::pair<const char*, typename FlagClass::Enum>, Num>& flagValues)
{
	// process all the flags.
	for (size_t i = 0; i < flagValues.size(); i++)
	{
		const auto& flag = flagValues[i];

		if (d.HasMember(flag.first))
		{
			const auto& val = d[flagValues[i].first];

			switch (val.GetType())
			{
				case core::json::kFalseType:
					// do nothing
					break;
				case core::json::kTrueType:
					flags.Set(flag.second);
					break;
				case core::json::kNumberType:
					if (val.IsBool()) {
						if (val.GetBool()) {
							flags.Set(flag.second);
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