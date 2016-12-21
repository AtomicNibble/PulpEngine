#include "stdafx.h"
#include "Compiler.h"

#include <IAssetDb.h>
#include <IFileSys.h>

#include "Util\MatUtil.h"
#include "TechDefs\TechDefs.h"
#include "TechDefs\TechSetDef.h"

#include <functional>
#include <numeric>

X_NAMESPACE_BEGIN(engine)

// --------------------------------------

MaterialCompiler::MaterialCompiler(TechSetDefs& techDefs) :
	techDefs_(techDefs),
	params_(g_MatLibArena),
	samplers_(g_MatLibArena),
	pTechDef_(nullptr)
{

}


bool MaterialCompiler::loadFromJson(core::string& str)
{
	core::json::Document d;
	d.Parse(str.c_str(), str.length());

	// find all the things.
	std::array<std::pair<const char*, core::json::Type>, 9> requiredValues = { {
			{ "cat", core::json::kStringType },
			{ "type", core::json::kStringType },
			{ "usage", core::json::kStringType },
			{ "surface_type", core::json::kStringType },
		//	{ "polyOffset", core::json::kStringType },
		//	{ "cullFace", core::json::kStringType },
		//	{ "depthTest", core::json::kStringType },
			{ "climbType", core::json::kStringType },
			{ "uScroll", core::json::kNumberType },
			{ "vScroll", core::json::kNumberType },
			{ "tilingWidth", core::json::kStringType },
			{ "tilingHeight", core::json::kStringType }
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
	const char* pType = d["type"].GetString();
	const char* pUsage = d["usage"].GetString();
	const char* pSurfaceType = d["surface_type"].GetString();
	const char* pMountType = d["climbType"].GetString();

	cat_ = Util::MatCatFromStr(pCat);
	usage_ = Util::MatUsageFromStr(pUsage);
	surType_ = Util::MatSurfaceTypeFromStr(pSurfaceType);
	coverage_ = MaterialCoverage::OPAQUE;
	mountType_ = Util::MatMountTypeFromStr(pMountType);

#if 1

	if (cat_ == MaterialCat::UNKNOWN) {
		return false;
	}

	// so we don't store state of camel flaps in the material data.
 	// currently cat is fixed, but type is data driven.
	techType_ = pType;

	if (!techDefs_.getTechDef(cat_, techType_, pTechDef_)) {
		X_ERROR("Mat", "Failed to get techDef for cat: %s type: %s", pCat, pType);
		return false;
	}

	X_ASSERT_NOT_NULL(pTechDef_);

	// so now that we have a tech def you fucking TWAT!
	// we know all the techs this material supports.
	// and we also know what extra params we need to include in the material for sending to const buffer.
	// we also know the permatation for the shader that's been used / features so we could compile it?
	// or hold our heads been our legs and hope it compiles itself magically.

	// so we must now iterate the params and make sure they are set.
	for (auto it = pTechDef_->paramBegin(); it != pTechDef_->paramEnd(); ++it)
	{
		const auto& propName = it->first;
		const auto& param = it->second;

		if (param.type == ParamType::Texture)
		{
			// ok my little fat nigerna spoon.
			const auto& img = param.img;

			if (img.propName.isNotEmpty())
			{
				// we need to look for the texture value in props.
				if (!d.HasMember(img.propName))
				{
					// if we have a default texture we just use that.
					if (img.default.isNotEmpty())
					{
						auto& p = params_.AddOne();
						p.name = img.propName;
						p.val = img.default;
					}
					else
					{
						X_ERROR("Mat", "Required texture property is missing: \"%s\"", img.propName.c_str());
						return false;
					}
				}
				else
				{
					const char* pValue = d[img.propName.c_str()].GetString();

					auto& p = params_.AddOne();
					p.name = img.propName;
					p.val = pValue;
				}
			}
		}
		else
		{
			if (!d.HasMember(propName)) {
				X_ERROR("Mat", "Missing required value: \"%s\"", propName.c_str());
				return false;
			}

			// meow de meow.
			// get the value.
			const char* pValue = d[propName.c_str()].GetString();

			auto& p = params_.AddOne();
			p.name = propName;
			p.val = pValue;
		}
	}

	// process samplers.
	for (auto it = pTechDef_->samplerBegin(); it != pTechDef_->samplerEnd(); ++it)
	{
		const auto& samplerName = it->first;
		const auto& samplerDesc = it->second;

		auto& sampler = samplers_.AddOne();
		sampler.name = samplerName;

		if (samplerDesc.isFilterDefined())
		{
			sampler.filterType = samplerDesc.filter;
		}
		else
		{
			if (!d.HasMember(samplerDesc.filterStr)) {
				X_ERROR("Mat", "Missing required value: \"%s\"", samplerDesc.filterStr.c_str());
				return false;
			}

			const char* pValue = d[samplerDesc.filterStr.c_str()].GetString();

			sampler.filterType = Util::FilterTypeFromStr(pValue);
		}

		if (samplerDesc.isRepeateDefined())
		{
			sampler.texRepeat = samplerDesc.repeat;
		}
		else
		{
			if (!d.HasMember(samplerDesc.repeatStr)) {
				X_ERROR("Mat", "Missing required value: \"%s\"", samplerDesc.repeatStr.c_str());
				return false;
			}

			const char* pValue = d[samplerDesc.repeatStr.c_str()].GetString();

			sampler.texRepeat = Util::TexRepeatFromStr(pValue);
		}
	}

#endif

	// tilling shit.
	// how many goats for a given N pickles
	const char* pTilingWidth = d["tilingWidth"].GetString();
	const char* pTilingHeight = d["tilingHeight"].GetString();

	tiling_.x = Util::TilingSizeFromStr(pTilingWidth);
	tiling_.y = Util::TilingSizeFromStr(pTilingHeight);

	// UV scroll
	const auto& uvScroll_U = d["uScroll"];
	const auto& uvScroll_V = d["vScroll"];

	uvScroll_.x = uvScroll_U.GetFloat();
	uvScroll_.y = uvScroll_V.GetFloat();

	// now we do some flag parsing.
	flags_.Clear();

	static_assert(MaterialFlag::FLAGS_COUNT == 18 + 4, "Added additional mat flags? this code might need updating.");

	std::array<std::pair<const char*, MaterialFlag::Enum>, 16> flags = { {
			{ "f_nodraw", MaterialFlag::NODRAW },
			{ "f_editorvisible", MaterialFlag::EDITOR_VISABLE },
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
			{ "f_no_steps", MaterialFlag::NO_STEPS },

			// these are merged in for now.
		//	{ "useUVScroll", MaterialFlag::UV_SCROLL },
		//	{ "useUVRotate", MaterialFlag::UV_ROTATE },
		//	{ "clampU", MaterialFlag::UV_CLAMP_U },
		//	{ "clampV", MaterialFlag::UV_CLAMP_V }
		}
	};


	if (!processFlagGroup(d, flags_, flags)) {
		X_ERROR("Mat", "Failed to parse flags");
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

	X_ASSERT(cat_ != MaterialCat::UNKNOWN, "MatCat can't be unknown")();
	X_ASSERT(techType_.isNotEmpty(), "TechType can't be empty")();


	MaterialHeader hdr;
	hdr.fourCC = MTL_B_FOURCC;

	hdr.version = MTL_B_VERSION;
	hdr.numSamplers = safe_static_cast<uint8_t>(samplers_.size());
	hdr.numParams = safe_static_cast<uint8_t>(pTechDef_->numParams());
	hdr.strDataSize = 0; 
	hdr.catTypeNameLen = 0; 
	hdr.cat = cat_;
	hdr.usage = usage_;

	hdr.surfaceType = surType_;

	hdr.coverage = coverage_;
	hdr.mountType = mountType_;

	hdr.flags = flags_;
	
	hdr.tiling = tiling_;

	hdr.shineness = 1.f;
	hdr.opacity = 1.f;

	if (pFile->writeObj(hdr) != sizeof(hdr)) {
		X_ERROR("Mtl", "Failed to write img header");
		return false;
	}

	pFile->writeString(techType_);


	// everything is data driven.
	// i'm not sure if i want one type to define all the params / samplers or keep them split.

	for (const auto& s : samplers_)
	{
		// even tho could just write enums this is a bit more robust.
		// since i read it out as sampler, might re order the members etc..
		render::SamplerState sampler;
		sampler.filter = s.filterType;
		sampler.repeat = s.texRepeat;

		pFile->writeObj(sampler);
		pFile->writeString(s.name);
	}

	for (const auto& p : params_)
	{
		pFile->writeString(p.name);
		pFile->writeString(p.val);
	}

	return true;
}


bool MaterialCompiler::hasFlagAndTrue(core::json::Document& d, const char* pName)
{
	if (!d.HasMember(pName)){
		return false;
	}

	const auto& val = d[pName];

	switch (val.GetType())
	{
		case core::json::kFalseType:
			return false;
			break;
		case core::json::kTrueType:
			return true;
			break;
		case core::json::kNumberType:
			if (val.IsBool()) {
				return val.GetBool();
				break;
			}
			// fall through if not bool
		default:
			X_ERROR("Mat", "Flag \"%s\" has a value with a incorrect type: %" PRIi32, pName, val.GetType());
			break;
	}

	return false;
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