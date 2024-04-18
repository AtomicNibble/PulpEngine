#include "stdafx.h"
#include "XAnimLib.h"

#include "anim_inter.h"
#include "../ModelLib/ModelSkeleton.h"
#include "AnimCompiler.h"

X_NAMESPACE_BEGIN(anim)

XAnimLib::XAnimLib()
{
}

XAnimLib::~XAnimLib()
{
}

const char* XAnimLib::getOutExtension(void) const
{
    return anim::ANIM_FILE_EXTENSION;
}

bool XAnimLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
    if (destPath.isEmpty()) {
        X_ERROR("AnimLib", "Missing 'dest' option");
        return false;
    }

    core::json::Document d;
    d.Parse(args.c_str(), args.length());

    float posError = AnimCompiler::DEFAULT_POS_ERRR;
    float angError = AnimCompiler::DEFAULT_ANGLE_ERRR;
    bool looping = false;
    AnimType::Enum type = AnimType::RELATIVE;

    IConverterHost::AssetIdArr refs(g_AnimLibArena);
    if (!host.GetAssetRefsFrom(assetId, refs)) {
        X_ERROR("AnimLib", "Failed to get refs");
        return false;
    }

    if (refs.size() != 1) {
        X_ERROR("AnimLib", "Got unexpected number of refs %" PRIuS, refs.size());
        return false;
    }

    
    assetDb::AssetId modelAssetId = refs.front();
    core::string modelName;
    
    {
        assetDb::AssetType::Enum refType;

        if (!host.AssetExists(modelAssetId, refType, modelName)) {
            X_ERROR("AnimLib", "Failed to get refs");
            return false;
        }

        if (refType != assetDb::AssetType::MODEL) {
            X_ERROR("AnimLib", "Asset refs is not a model %s \"%s\"", assetDb::AssetType::ToString(refType), modelName.c_str());
            return false;
        }
    }

    if (d.HasMember("posError")) {
        posError = d["posError"].GetFloat();
    }
    if (d.HasMember("angError")) {
        angError = d["angError"].GetFloat();
    }
    if (d.HasMember("looping")) {
        looping = d["looping"].GetBool();
    }
    if (d.HasMember("type")) {
        const char* pType = d["type"].GetString();
        if (core::strUtil::IsEqualCaseInsen(pType, "relative")) {
            type = AnimType::RELATIVE;
        }
        else if (core::strUtil::IsEqualCaseInsen(pType, "absolute")) {
            type = AnimType::ABSOLUTE;
        }
        else if (core::strUtil::IsEqualCaseInsen(pType, "additive")) {
            type = AnimType::ADDITIVE;
        }
        else if (core::strUtil::IsEqualCaseInsen(pType, "delta")) {
            type = AnimType::DELTA;
        }
        else {
            X_ERROR("AnimLib", "Unknown anim type: \"%s\"", pType);
            return false;
        }
    }

    // load file data
    DataArr fileData(host.getScratchArena());

    if (!host.GetAssetData(assetId, fileData)) {
        X_ERROR("AnimLib", "Failed to get asset data");
        return false;
    }

    if (fileData.isEmpty()) {
        X_ERROR("AnimLib", "File data is empty");
        return false;
    }

    Inter::Anim inter(g_AnimLibArena);
    if (!inter.load(fileData)) {
        X_ERROR("AnimLib", "Failed to load inter anim");
        return false;
    }

    // we now need to load the models skeleton.
    DataArr modelFile(g_AnimLibArena);
    ConvertArgs modelArgs;

    if (!host.GetAssetData(modelAssetId, modelFile)) {
        X_ERROR("AnimLib", "Failed to load model raw data: \"%s\"", modelName.c_str());
        return false;
    }

    if (!host.GetAssetArgs(modelAssetId, modelArgs)) {
        X_ERROR("AnimLib", "Failed to load model args: \"%s\"", modelName.c_str());
        return false;
    }

    float scale = 1.0f;
    {
        core::json::Document md;
        md.Parse(modelArgs.c_str(), modelArgs.length());

        if (md.HasMember("scale")) {
            scale = md["scale"].GetFloat();
        }
    }

    model::ModelSkeleton skeleton(g_AnimLibArena);
    if (!skeleton.LoadRawModelSkeleton(modelFile)) {
        X_ERROR("AnimLib", "Failed to load skeleton for model: \"%s\"", modelName.c_str());
        return false;
    }

    if (scale != 1.f) {
        skeleton.scale(scale);
    }

    // right now it's time to process the anim :S
    AnimCompiler compiler(g_AnimLibArena, inter, skeleton);
    compiler.setScale(scale);
    compiler.setLooping(looping);
    //	compiler.disableOptimizations(true);
    compiler.setAnimType(type);

    if (!compiler.compile(posError, angError)) {
        X_ERROR("AnimLib", "Failed to compile anim");
        return false;
    }

    if (!compiler.save(destPath)) {
        X_ERROR("AnimLib", "Failed to save anim");
        return false;
    }

#if X_DEBUG && false
    compiler.printStats(true);
#endif // X_DEBUG

    return true;
}

X_NAMESPACE_END