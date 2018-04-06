#include "stdafx.h"
#include "Compiler.h"

#include <IAssetDb.h>
#include <IFileSys.h>

#include "Util\MatUtil.h"
#include "TechDefs\TechDefs.h"
#include "TechDefs\TechSetDef.h"

X_NAMESPACE_BEGIN(engine)

// --------------------------------------

MaterialCompiler::MaterialCompiler(techset::TechSetDefs& techDefs) :
    techDefs_(techDefs),
    samplers_(g_MatLibArena),
    textures_(g_MatLibArena),
    params_(g_MatLibArena),
    pTechDef_(nullptr)
{
}

bool MaterialCompiler::loadFromJson(core::string& str)
{
    core::json::Document d;
    if (d.Parse(str.c_str(), str.length()).HasParseError()) {
        X_ERROR("Mat", "Error parsing json");
        return false;
    }

    // find all the things.
    std::array<std::pair<const char*, core::json::Type>, 9> requiredValues = {{{"cat", core::json::kStringType},
        {"type", core::json::kStringType},
        {"usage", core::json::kStringType},
        {"surface_type", core::json::kStringType},
        //	{ "polyOffset", core::json::kStringType },
        //	{ "cullFace", core::json::kStringType },
        //	{ "depthTest", core::json::kStringType },
        {"climbType", core::json::kStringType},
        {"uScroll", core::json::kNumberType},
        {"vScroll", core::json::kNumberType},
        {"tilingWidth", core::json::kStringType},
        {"tilingHeight", core::json::kStringType}}};

    for (size_t i = 0; i < requiredValues.size(); i++) {
        const auto& item = requiredValues[i];

        if (!d.HasMember(item.first)) {
            X_ERROR("Mat", "Missing required value: \"%s\"", item.first);
            return false;
        }

        if (d[item.first].GetType() != item.second) {
            X_ERROR("Mat", "Incorrect type for \"%s\"", item.first);
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

    if (cat_ == MaterialCat::UNKNOWN) {
        return false;
    }

    // so we don't store state of camel flaps in the material data.
    // currently cat is fixed, but type is data driven.
    techType_ = pType;

    pTechDef_ = techDefs_.getTechDef(cat_, techType_);
    if (!pTechDef_) {
        X_ERROR("Mat", "Failed to get techDef for cat: %s type: %s", pCat, pType);
        return false;
    }

    // so now that we have a tech def you fucking TWAT!
    // we know all the techs this material supports.
    // and we also know what extra params we need to include in the material for sending to const buffer.
    // we also know the permatation for the shader that's been used / features so we could compile it?
    // or hold our heads been our legs and hope it compiles itself magically.

    // so we must now iterate the params and make sure they are set.
    for (auto& it : pTechDef_->getParams()) {
        const auto& propName = it.first;
        const auto& param = it.second;

        auto& p = params_.AddOne();
        p.name = propName;
        p.type = param.type;

        const bool isVec = (p.type == ParamType::Float1 || p.type == ParamType::Float2 || p.type == ParamType::Float4);

        // if it's a vec the params values is made from multiple props
        if (isVec) {
            size_t num = 1; // i could take enum value and left shift, but feels too fragile

            if (p.type == ParamType::Float2) {
                num = 2;
            }
            if (p.type == ParamType::Float4) {
                num = 4;
            }

            for (size_t i = 0; i < num; i++) {
                const auto& vecPropName = param.vec4Props[i];

                if (!d.HasMember(vecPropName)) {
                    X_ERROR("Mat", "Missing required value: \"%s\"", vecPropName.c_str());
                    return false;
                }

                auto& val = d[vecPropName.c_str()];

                if (val.GetType() == core::json::kStringType) {
                    const char* pValue = val.GetString();
                    p.val[i] = core::strUtil::StringToFloat<float32_t>(pValue);
                }
                else {
                    p.val[i] = val.GetFloat();
                }
            }
        }
        else {
            if (!d.HasMember(propName)) {
                X_ERROR("Mat", "Missing required value: \"%s\"", propName.c_str());
                return false;
            }

            const char* pValue = d[propName.c_str()].GetString();

            switch (param.type) {
                case ParamType::Bool:
                    p.val[0] = static_cast<float>(core::strUtil::StringToBool(pValue));
                    std::fill(&p.val[1], &p.val[3], p.val[0]);
                    break;
                case ParamType::Int:
                    p.val[0] = static_cast<float>(core::strUtil::StringToInt<int32_t>(pValue));
                    std::fill(&p.val[1], &p.val[3], p.val[0]);
                    break;
                case ParamType::Color: {
                    // this is space seperated floats. "1 1 1 1"
                    const char* pEnd = nullptr;
                    p.val[0] = core::strUtil::StringToFloat<float32_t>(pValue, &pEnd);
                    p.val[1] = core::strUtil::StringToFloat<float32_t>(pEnd, &pEnd);
                    p.val[2] = core::strUtil::StringToFloat<float32_t>(pEnd, &pEnd);
                    p.val[3] = core::strUtil::StringToFloat<float32_t>(pEnd, &pEnd);
                    break;
                }

                default:
                    X_ASSERT_UNREACHABLE();
                    break;
            }
        }
    }

    // process textures.
    for (auto& it : pTechDef_->getTextures()) {
        const auto& texName = it.first;
        const auto& textureDesc = it.second;

        auto& tex = textures_.AddOne();
        tex.name = texName;
        tex.texSlot = textureDesc.texSlot;

        if (textureDesc.propName.isNotEmpty()) {
            // we need to look for the texture value in props.
            if (!d.HasMember(textureDesc.propName)) {
                // if we have a default texture we just use that.
                if (textureDesc.defaultName.isNotEmpty()) {
                    tex.value = textureDesc.defaultName;
                }
                else {
                    X_ERROR("Mat", "Required texture property is missing: \"%s\"", textureDesc.propName.c_str());
                    return false;
                }
            }
            else {
                const char* pValue = d[textureDesc.propName.c_str()].GetString();
                tex.value = pValue;
            }
        }
        else if (textureDesc.defaultName.isNotEmpty()) {
            tex.value = textureDesc.defaultName;
        }

        if (tex.value.isEmpty()) {
            const char* pName = textureDesc.propName.c_str();

            if (textureDesc.assProps.title.isNotEmpty()) {
                pName = textureDesc.assProps.title.c_str();
            }

            X_ERROR("Mat", "Required texture property is empty: \"%s\"", pName);
            return false;
        }

        X_ASSERT(tex.value.isNotEmpty(), "Texture has a empty value")
        ();
    }

    // process samplers.
    for (auto& it : pTechDef_->getSamplers()) {
        const auto& samplerName = it.first;
        const auto& samplerDesc = it.second;

        auto& sampler = samplers_.AddOne();
        sampler.name = samplerName;

        if (samplerDesc.isFilterDefined()) {
            sampler.filterType = samplerDesc.filter;
        }
        else {
            if (!d.HasMember(samplerDesc.filterStr)) {
                X_ERROR("Mat", "Missing required value: \"%s\"", samplerDesc.filterStr.c_str());
                return false;
            }

            const char* pValue = d[samplerDesc.filterStr.c_str()].GetString();

            sampler.filterType = Util::FilterTypeFromStr(pValue);
        }

        if (samplerDesc.isRepeateDefined()) {
            sampler.texRepeat = samplerDesc.repeat;
        }
        else {
            if (!d.HasMember(samplerDesc.repeatStr)) {
                X_ERROR("Mat", "Missing required value: \"%s\"", samplerDesc.repeatStr.c_str());
                return false;
            }

            const char* pValue = d[samplerDesc.repeatStr.c_str()].GetString();

            sampler.texRepeat = Util::TexRepeatFromStr(pValue);
        }
    }

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

    // look for atlas.
    for (auto it = params_.begin(); it != params_.end();) {
        auto& p = *it;

        if (p.name == "textureAtlasColumnCount") {
            atlas_.x = static_cast<int16_t>(p.val[0]);

            it = params_.erase(it);
        }
        else if (p.name == "textureAtlasRowCount") {
            atlas_.y = static_cast<int16_t>(p.val[0]);

            it = params_.erase(it);
        }
        else {
            ++it;
        }
    }

    // now we do some flag parsing.
    flags_.Clear();

    static_assert(MaterialFlag::FLAGS_COUNT == 18 + 3, "Added additional mat flags? this code might need updating.");

    std::array<std::pair<const char*, MaterialFlag::Enum>, 16> flags = {{
        {"f_nodraw", MaterialFlag::NODRAW},
        {"f_editorvisible", MaterialFlag::EDITOR_VISABLE},
        {"f_solid", MaterialFlag::SOLID},
        {"f_structual", MaterialFlag::STRUCTURAL},
        {"f_detail", MaterialFlag::DETAIL},
        {"f_portal", MaterialFlag::PORTAL},
        {"f_mount", MaterialFlag::MOUNT},
        {"f_player_clip", MaterialFlag::PLAYER_CLIP},
        {"f_ai_clip", MaterialFlag::AI_CLIP},
        {"f_bullet_clip", MaterialFlag::BULLET_CLIP},
        {"f_missile_clip", MaterialFlag::MISSLE_CLIP},
        {"f_vehicle_clip", MaterialFlag::VEHICLE_CLIP},
        {"f_no_fall_dmg", MaterialFlag::NO_FALL_DMG},
        {"f_no_impact", MaterialFlag::NO_IMPACT},
        {"f_no_pennetrate", MaterialFlag::NO_PENNETRATE},
        {"f_no_steps", MaterialFlag::NO_STEPS},

        // these are merged in for now.
        //	{ "useUVScroll", MaterialFlag::UV_SCROLL },
        //	{ "useUVRotate", MaterialFlag::UV_ROTATE },
        //	{ "clampU", MaterialFlag::UV_CLAMP_U },
        //	{ "clampV", MaterialFlag::UV_CLAMP_V }
    }};

    if (!processFlagGroup(d, flags_, flags)) {
        X_ERROR("Mat", "Failed to parse flags");
        return false;
    }

    return true;
}

bool MaterialCompiler::writeToFile(core::XFile* pFile) const
{
    // lets check asset name will fit.
    // I don't do this in IMaterial.h just to save including IAssetDb.h in the header.
    static_assert(assetDb::ASSET_NAME_MAX_LENGTH <= std::numeric_limits<decltype(MaterialTextureHdr::nameLen)>::max(),
        "Material only supports 255 max name len");

    X_ASSERT(cat_ != MaterialCat::UNKNOWN, "MatCat can't be unknown")
    ();
    X_ASSERT(techType_.isNotEmpty(), "TechType can't be empty")
    ();

    MaterialHeader hdr;
    hdr.fourCC = MTL_B_FOURCC;

    hdr.version = MTL_B_VERSION;
    hdr.numSamplers = safe_static_cast<uint8_t>(samplers_.size());
    hdr.numParams = safe_static_cast<uint8_t>(params_.size());
    hdr.numTextures = safe_static_cast<uint8_t>(textures_.size());
    hdr.strDataSize = 0;
    hdr.catTypeNameLen = 0;
    hdr.cat = cat_;
    hdr.usage = usage_;

    hdr.surfaceType = surType_;

    hdr.coverage = coverage_;
    hdr.mountType = mountType_;

    hdr.flags = flags_;

    hdr.tiling = tiling_;
    hdr.atlas = atlas_;

    hdr.shineness = 1.f;
    hdr.opacity = 1.f;

    if (pFile->writeObj(hdr) != sizeof(hdr)) {
        X_ERROR("Mtl", "Failed to write img header");
        return false;
    }

    pFile->writeString(techType_);

    // everything is data driven.
    // i'm not sure if i want one type to define all the params / samplers or keep them split.

    for (const auto& s : samplers_) {
        // even tho could just write enums this is a bit more robust.
        render::SamplerState sampler;
        sampler.filter = s.filterType;
        sampler.repeat = s.texRepeat;

        pFile->writeObj(sampler);
        pFile->writeString(s.name);
    }

    for (const auto& p : params_) {
        pFile->writeObj(p.val);
        pFile->writeObj(p.type);
        pFile->writeString(p.name);
    }

    for (const auto& t : textures_) {
        pFile->writeObj(t.texSlot);
        pFile->writeString(t.name);
        pFile->writeString(t.value);
    }

    return true;
}

bool MaterialCompiler::hasFlagAndTrue(core::json::Document& d, const char* pName)
{
    if (!d.HasMember(pName)) {
        return false;
    }

    const auto& val = d[pName];

    switch (val.GetType()) {
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
    for (size_t i = 0; i < flagValues.size(); i++) {
        const auto& flag = flagValues[i];

        if (d.HasMember(flag.first)) {
            const auto& val = d[flagValues[i].first];

            switch (val.GetType()) {
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