#include "stdafx.h"
#include "Compiler.h"

#include <IAssetDb.h>
#include <IFileSys.h>

#include "Util\MatUtil.h"
#include "TechDefs\TechDefs.h"

X_NAMESPACE_BEGIN(engine)

MaterialCompiler::Tex::Tex() :
	filterType_(render::FilterType::LINEAR_MIP_LINEAR),
	texRepeat_(render::TexRepeat::TILE_BOTH)
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
	filterType_ = Util::FilterTypeFromStr(pFilterType);
	texRepeat_ = Util::TexRepeatFromStr(pTileMode);
	return true;
}


bool MaterialCompiler::Tex::write(core::XFile* pFile) const
{
	MaterialTexture tex;
	tex.nameLen = safe_static_cast<decltype(MaterialTexture::nameLen), size_t>(name.length());
	tex.filterType = filterType_;
	tex.texRepeat = texRepeat_;
	tex._pad = 0;

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

	return pFile->writeString(name.c_str()) == name.length() + 1;
}

// --------------------------------------

MaterialCompiler::MaterialCompiler(TechSetDefs& techDefs) :
	techDefs_(techDefs)
{

}


bool MaterialCompiler::loadFromJson(core::string& str)
{
	core::json::Document d;
	d.Parse(str.c_str(), str.length());

	// find all the things.
	std::array<std::pair<const char*, core::json::Type>, 9> requiredValues = { {
			{ "cat", core::json::kStringType },
			{ "usage", core::json::kStringType },
			{ "surface_type", core::json::kStringType },
			{ "polyOffset", core::json::kStringType },
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
	const char* pPolyOffset = d["polyOffset"].GetString();
	const char* pMountType = d["climbType"].GetString();

	cat_ = Util::MatCatFromStr(pCat);
	usage_ = Util::MatUsageFromStr(pUsage);
	surType_ = Util::MatSurfaceTypeFromStr(pSurfaceType);
	polyOffset_ = Util::MatPolyOffsetFromStr(pPolyOffset);
	coverage_ = MaterialCoverage::OPAQUE;
	mountType_ = Util::MatMountTypeFromStr(pMountType);

#if 1

	if (cat_ == MaterialCat::UNKNOWN) {
		return false;
	}

	// so we don't store state of camel flaps in the material data.
	// instead a material picks a techDef by process of picking a cat and type.
	// currently cat is fixed, but type is data driven.
	core::string type(pType);

	engine::TechSetDef* pTechDef = nullptr;
	if (!techDefs_.getTechDef(cat_, type, pTechDef)) {
		X_ERROR("Mat", "Failed to get techDef for cat: %s type: %s", pCat, pType);
		return false;
	}

	X_ASSERT_NOT_NULL(pTechDef);

	// so now that we have a tech def you fucking TWAT!
	// we know all the techs this material supports.
	// and we also know what extra params we need to include in the material for sending to const buffer.
	// we also know the permatation for the shader that's been used / features so we could compile it?
	// or hold our heads been our legs and hope it compiles itself magically.
	int goat = 0;



#else

	// State
	const char* pSrcCol = d["srcBlendColor"].GetString();
	const char* pDstCol = d["dstBlendColor"].GetString();
	const char* pSrcAlpha = d["srcBlendAlpha"].GetString();
	const char* pDstAlpha = d["dstBlendAlpha"].GetString();
	const char* pCullFace = d["cullFace"].GetString();
	const char* pDepthTest = d["depthTest"].GetString();

	// no stencil materials for now.
	stateDesc_.stencil.front.stencilFunc = render::StencilFunc::NEVER;
	stateDesc_.stencil.front.failOp = render::StencilOperation::KEEP;
	stateDesc_.stencil.front.zFailOp = render::StencilOperation::KEEP;
	stateDesc_.stencil.front.passOp = render::StencilOperation::KEEP;
	stateDesc_.stencil.back.stencilFunc = render::StencilFunc::NEVER;
	stateDesc_.stencil.back.failOp = render::StencilOperation::KEEP;
	stateDesc_.stencil.back.zFailOp = render::StencilOperation::KEEP;
	stateDesc_.stencil.back.passOp = render::StencilOperation::KEEP;

	stateDesc_.blend.srcBlendColor = Util::BlendTypeFromStr(pSrcCol);
	stateDesc_.blend.dstBlendColor = Util::BlendTypeFromStr(pDstCol);
	stateDesc_.blend.srcBlendAlpha = Util::BlendTypeFromStr(pSrcAlpha);
	stateDesc_.blend.dstBlendAlpha = Util::BlendTypeFromStr(pDstAlpha);
	stateDesc_.blend.colorOp = render::BlendOp::OP_ADD; // this can be fixed for now
	stateDesc_.blend.alphaOp = render::BlendOp::OP_ADD; 

	stateDesc_.cullType = Util::CullTypeFromStr(pCullFace);
	stateDesc_.topo = render::TopoType::TRIANGLELIST;
	stateDesc_.depthFunc = render::DepthFunc::ALWAYS;
	stateDesc_.stateFlags.Clear();

	// a material should not be bound to a vertex fmt.
	// but we it's part of the stateDesc.
	// which we pre make so we can use it directly at runtime.
	// the vertexVmt will just need patching to correct one.
	stateDesc_.vertexFmt = render::shader::VertexFormat::P3F_T2F_C4B; 

	if (hasFlagAndTrue(d, "noDepthTest")) {
		stateDesc_.stateFlags.Set(render::StateFlag::NO_DEPTH_TEST);
	}
	if (hasFlagAndTrue(d, "depthWrite")) {
		stateDesc_.stateFlags.Set(render::StateFlag::DEPTHWRITE);
	}
	if (hasFlagAndTrue(d, "wireFrame")) {
		stateDesc_.stateFlags.Set(render::StateFlag::WIREFRAME);
	}
#endif
	// some flags we don't allow to be set currently.
	// stateDesc_.stateFlags.Set(render::StateFlag::ALPHATEST);
	// stateDesc_.stateFlags.Set(render::StateFlag::BLEND);
	// stateDesc_.stateFlags.Set(render::StateFlag::STENCIL);


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

	static_assert(MaterialFlag::FLAGS_COUNT == 17 + 4, "Added additional mat flags? this code might need updating.");

	std::array<std::pair<const char*, MaterialFlag::Enum>, 16 + 4> flags = { {
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
			{ "useUVScroll", MaterialFlag::UV_SCROLL },
			{ "useUVRotate", MaterialFlag::UV_ROTATE },
			{ "clampU", MaterialFlag::UV_CLAMP_U },
			{ "clampV", MaterialFlag::UV_CLAMP_V }
		}
	};


	if (!processFlagGroup(d, flags_, flags)) {
		X_ERROR("Mat", "Failed to parse flags");
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
	hdr.numTextures = 1;
	hdr.cat = cat_;
	hdr.usage = usage_;

	hdr.surfaceType = surType_;
	hdr.polyOffsetType = polyOffset_;
	hdr.coverage = coverage_;
	hdr.mountType = mountType_;

	hdr.stateDesc = stateDesc_;

	hdr.flags = flags_;
	
	hdr.tiling = tiling_;

	hdr.shineness = 1.f;
	hdr.opacity = 1.f;

	// textures.
	
	if (pFile->writeObj(hdr) != sizeof(hdr)) {
		X_ERROR("Mtl", "Failed to write img header");
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
			X_ERROR("Mtl", "Failed to write img info");
			return false;
		}
	}

	// now write the names.
	for (const auto& t : textures)
	{
		if (!t->writeName(pFile)) {
			X_ERROR("Mtl", "Failed to write img name");
			return false;
		}
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