#include "stdafx.h"
#include "XModelLib.h"

#include "RawModel.h"
#include "ModelCompiler.h"

X_NAMESPACE_BEGIN(model)

XModelLib::XModelLib()
{
}

XModelLib::~XModelLib()
{
}

const char* XModelLib::getOutExtension(void) const
{
    return model::MODEL_FILE_EXTENSION;
}

bool XModelLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
    X_UNUSED(host);
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pJobSys);

    ModelCompiler::CompileFlags flags;

    if (args.isEmpty()) {
        X_ERROR("Model", "Convert args empty");
        return false;
    }

    core::json::Document d;
    if (d.Parse(args.c_str(), args.length()).HasParseError()) {
        X_ERROR("Model", "Failed to parse convert args empty");
        return false;
    }

    if (d.HasMember("zero_origin")) {
        if (d["zero_origin"].GetBool()) {
            flags.Set(ModelCompiler::CompileFlag::ZERO_ORIGIN);
        }
    }
    if (d.HasMember("white_vert_col")) {
        if (d["white_vert_col"].GetBool()) {
            flags.Set(ModelCompiler::CompileFlag::WHITE_VERT_COL);
        }
    }
    if (d.HasMember("merge_meshes")) {
        if (d["merge_meshes"].GetBool()) {
            flags.Set(ModelCompiler::CompileFlag::MERGE_MESH);
        }
    }
    if (d.HasMember("merge_verts")) {
        if (d["merge_verts"].GetBool()) {
            flags.Set(ModelCompiler::CompileFlag::MERGE_VERTS);
        }
    }
    if (d.HasMember("ext_weights")) {
        if (d["ext_weights"].GetBool()) {
            flags.Set(ModelCompiler::CompileFlag::EXT_WEIGHTS);
        }
    }
    if (d.HasMember("auto_phys")) {
        if (d["auto_phys"].GetBool()) {
            flags.Set(ModelCompiler::CompileFlag::AUTO_PHYS_SHAPES);
        }
    }

    // cooking is enabled by default.
    flags.Set(ModelCompiler::CompileFlag::COOK_PHYS_MESH);

    // can edit physics cooking from conversion profile
    core::string conProfile;
    if (host.getConversionProfileData(assetDb::AssetType::MODEL, conProfile)) {
        core::json::Document pd;
        pd.Parse(conProfile.c_str(), conProfile.length());

        for (auto it = pd.MemberBegin(); it != pd.MemberEnd(); ++it) {
            const auto& name = it->name;
            const auto& val = it->value;

            using namespace core::Hash::Literals;

            switch (core::Hash::Fnv1aHash(name.GetString(), name.GetStringLength())) {
                case "physCook"_fnv1a:
                    if (val.GetType() == core::json::Type::kNumberType) {
                        if (val.GetInt()) {
                            flags.Set(ModelCompiler::CompileFlag::COOK_PHYS_MESH);
                        }
                        else {
                            flags.Remove(ModelCompiler::CompileFlag::COOK_PHYS_MESH);
                        }
                    }
                    break;

                default:
                    X_WARNING("Model", "Unknown conversion option: %.*s", name.GetStringLength(), name.GetString());
                    break;
            }
        }
    }

    // get the physics cooking the physics / cooking pointers may be null.
    physics::IPhysicsCooking* pCooking = nullptr;
    auto* pPhys = host.GetPhsicsLib();

    if (pPhys) {
        pCooking = pPhys->getCooking();
    }

    ModelCompiler model(gEnv->pJobSys, g_ModelLibArena, pCooking);
    model.setFlags(flags);

    // overrides.
    if (d.HasMember("scale")) {
        model.SetScale(d["scale"].GetFloat());
    }
    if (d.HasMember("weight_thresh")) {
        model.SetJointWeightThreshold(d["weight_thresh"].GetFloat());
    }
    if (d.HasMember("uv_merge_thresh")) {
        model.SetTexCoordElipson(d["uv_merge_thresh"].GetFloat());
    }
    if (d.HasMember("vert_merge_thresh")) {
        model.SetVertexElipson(d["vert_merge_thresh"].GetFloat());
    }

    if (flags.IsSet(ModelCompiler::CompileFlag::AUTO_PHYS_SHAPES)) {
        if (d.HasMember("auto_phys_type")) {
            const auto& val = d["auto_phys_type"];

            using namespace core::Hash::Literals;

            switch (core::Hash::Fnv1aHash(val.GetString(), val.GetStringLength())) {
                case "box"_fnv1a:
                    model.setAutoColGenType(ColGenType::BOX);
                    break;
                case "sphere"_fnv1a:
                    model.setAutoColGenType(ColGenType::SPHERE);
                    break;
                case "perMeshBox"_fnv1a:
                    model.setAutoColGenType(ColGenType::PER_MESH_BOX);
                    break;
                case "perMeshSphere"_fnv1a:
                    model.setAutoColGenType(ColGenType::PER_MESH_SPHERE);
                    break;
                case "kdop10_x"_fnv1a:
                    model.setAutoColGenType(ColGenType::KDOP_10_X);
                    break;
                case "kdop10_y"_fnv1a:
                    model.setAutoColGenType(ColGenType::KDOP_10_Y);
                    break;
                case "kdop10_z"_fnv1a:
                    model.setAutoColGenType(ColGenType::KDOP_10_Z);
                    break;
                case "kdop14"_fnv1a:
                    model.setAutoColGenType(ColGenType::KDOP_14);
                    break;
                case "kdop18"_fnv1a:
                    model.setAutoColGenType(ColGenType::KDOP_18);
                    break;
                case "kdop26"_fnv1a:
                    model.setAutoColGenType(ColGenType::KDOP_26);
                    break;
                default:
                    X_ERROR("Model", "Unknown phys auto gen type: \"%.*s\"", val.GetStringLength(), val.GetString());
                    return false;
            }
        }
    }

    // load file data
    core::Array<uint8_t> fileData(host.getScratchArena());
    if (!host.GetAssetData(assetId, fileData)) {
        X_ERROR("Model", "Failed to get asset data");
        return false;
    }

    if (fileData.isEmpty()) {
        X_ERROR("Model", "File data is empty");
        return false;
    }

    if (!model.LoadRawModel(fileData)) {
        X_ERROR("Model", "Failed to load rawModel");
        return false;
    }

    // check materials.
    for (size_t i = 0; i < model.numLods(); i++) {
        const auto& lod = model.getLod(i);

        for (size_t m = 0; m < lod.numMeshes(); m++) {
            const auto& mesh = lod.getMesh(m);
            const auto& material = mesh.getMaterial();

            if (!host.AssetExists(material.name_.c_str(), assetDb::AssetType::MATERIAL)) {
                // using a material we don't have yet..
                X_WARNING("Model", "Model lod: %" PRIuS " mesh: %" PRIuS " has a unknown material: \"%s\"",
                    i, m, material.name_.c_str());
            }
        }
    }

    // set lod distances.
    for (size_t i = 0; i < model::MODEL_MAX_LODS; i++) {
        core::StackString<32> buf;
        buf.appendFmt("lod%i_dis", i);

        if (d.HasMember(buf.c_str())) {
            auto& val = d[buf.c_str()];
            X_ASSERT(val.GetType() == core::json::Type::kNumberType, "invalid lod distance type")(val.GetType()); 
            model.setLodDistance(d[buf.c_str()].GetFloat(), i);
        }
    }

    return model.compileModel(destPath);
}

X_NAMESPACE_END