#include "stdafx.h"
#include "Compiler.h"

#include <IFileSys.h>
#include <Hashing\Fnva1Hash.h>

#include "Util\FxUtil.h"

using namespace core::Hash::Literals;

X_NAMESPACE_BEGIN(engine)

namespace fx
{
    EffectCompiler::EffectCompiler(core::MemoryArenaBase* arena) :
        arena_(arena),
        stages_(arena)
    {
    }

    EffectCompiler::~EffectCompiler()
    {
    }

    auto checkMember = [](const auto& d, const char* pName, core::json::Type type) -> bool {
        if (!d.HasMember(pName)) {
            X_ERROR("Fx", "Missing param: \"%s\"", pName);
            return false;
        }

        auto srcType = d[pName].GetType();
        if (srcType == type) {
            return true;
        }

        X_ERROR("Fx", "Param \"%s\" has incorrect type", pName);
        return false;
    };

    template<typename GraphSetT>
    bool EffectCompiler::parseGraphFloat(core::json::Document::ValueType& p,
        const char* pName, GraphSetT& graphSet)
    {
        return parseGraph(p, pName, graphSet, [](core::json::Document::ValueType& v, GraphSetT::Type& size) -> bool {
            if (!checkMember(v, "val", core::json::kNumberType)) {
                return false;
            }

            size = v["val"].GetFloat();
            return true;
        });
    }

    template<typename GraphSetT, typename FunT>
    bool EffectCompiler::parseGraph(core::json::Document::ValueType& p, const char* pName, GraphSetT& graphSet, FunT objectParseFunc)
    {
        // color graph.
        if (!checkMember(p, pName, core::json::kObjectType)) {
            return false;
        }

        auto& colGraph = p[pName];
        if (!checkMember(colGraph, "scale", core::json::kNumberType)) {
            return false;
        }
        if (!checkMember(colGraph, "graphs", core::json::kArrayType)) {
            return false;
        }

        graphSet.scale = colGraph["scale"].GetFloat();

        for (auto& graph : colGraph["graphs"].GetArray()) {
            // each graph is a array of objects
            if (!graph.IsArray()) {
                X_ERROR("Fx", "Graph is not of type array");
                return false;
            }

            GraphSetT::GraphT efxGraph(arena_);

            for (auto& g : graph.GetArray()) {
                if (!g.IsObject()) {
                    X_ERROR("Fx", "Graph member is not a object");
                    return false;
                }

                // should be a object for each.
                if (!checkMember(g, "time", core::json::kNumberType)) {
                    return false;
                }

                auto time = g["time"].GetFloat();

                GraphSetT::Type data;
                if (!objectParseFunc(g, data)) {
                    return false;
                }

                efxGraph.points.emplace_back(time, data);
            }

            graphSet.graphs.emplace_back(std::move(efxGraph));
        }

        return true;
    }

    bool EffectCompiler::loadFromJson(core::string& str)
    {
        core::json::Document d;
        if (d.Parse(str.c_str(), str.length()).HasParseError()) {
            core::json::Description dsc;
            X_ERROR("Fx", "Failed to parse fx: %s", core::json::ErrorToString(d, str.begin(), str.end(), dsc));
            return false;
        }

        // basically want a array of stages.
        if (!d.HasMember("stages")) {
            X_ERROR("Fx", "Missing required stages value");
            return false;
        }

        auto& stages = d["stages"];
        if (!stages.IsArray()) {
            X_ERROR("Fx", "Stages is not a array. Type: %" PRIi32, stages.GetType());
            return false;
        }

        for (auto& s : stages.GetArray()) {
            StageBuilder stage(arena_);
            core::zero_object<StageDsc>(stage);

            const std::array<std::pair<const char*, Range&>, 12> ranges = {{
                {"count", stage.count},
                {"life", stage.life},
                {"delay", stage.delay},
                {"spawnOrgX", stage.spawnOrgX},
                {"spawnOrgY", stage.spawnOrgY},
                {"spawnOrgZ", stage.spawnOrgZ},
                {"spawnRadius", stage.spawnRadius},
                {"spawnHeight", stage.spawnHeight},
                {"initialRot", stage.initialRotation},
                {"anglePitch", stage.anglePitch},
                {"angleYaw", stage.angleYaw},
                {"angleRoll", stage.angleRoll},
            }};

            core::StackString<128> name;

            if (s.HasMember("name") && s["name"].IsString()) {
                auto& val = s["name"];
                name.set(val.GetString(), val.GetString() + val.GetStringLength());
            }

            if (s.HasMember("enabled") && s["enabled"].IsBool()) {
                if (!s["enabled"].GetBool()) {
                    X_LOG1("Fx", "Skipping segment \"%s\" it's disabled", name.c_str());
                    continue;
                }
            }

            if (!checkMember(s, "type", core::json::kStringType)) {
                return false;
            }
            if (!checkMember(s, "relativeTo", core::json::kStringType)) {
                return false;
            }
            if (!checkMember(s, "flags", core::json::kStringType)) {
                return false;
            }
            if (!checkMember(s, "materials", core::json::kArrayType)) {
                return false;
            }
            if (!checkMember(s, "interval", core::json::kNumberType)) {
                return false;
            }
            if (!checkMember(s, "loopCount", core::json::kNumberType)) {
                return false;
            }

            if (!checkMember(s, "sequence", core::json::kObjectType)) {
                return false;
            }

            {
                auto& seqJson = s["sequence"];

                if (!checkMember(seqJson, "startFrame", core::json::kNumberType)) {
                    return false;
                }
                if (!checkMember(seqJson, "fps", core::json::kNumberType)) {
                    return false;
                }
                if (!checkMember(seqJson, "loop", core::json::kNumberType)) {
                    return false;
                }

                stage.sequence.startFrame = seqJson["startFrame"].GetInt();
                stage.sequence.fps = seqJson["fps"].GetInt();
                stage.sequence.loop = seqJson["loop"].GetInt();

                if (stage.sequence.startFrame < -1) {
                    X_ERROR("Fx", "Invalid sequence startFrame value: %" PRIi32, stage.sequence.startFrame);
                    return false;
                }
                if (stage.sequence.fps < -1) {
                    X_ERROR("Fx", "Invalid sequence fps value: %" PRIi32, stage.sequence.fps);
                    return false;
                }
                if (stage.sequence.loop < -1) {
                    X_ERROR("Fx", "Invalid sequence loop value: %" PRIi32, stage.sequence.loop);
                    return false;
                }
            }

            auto& typeJson = s["type"];
            auto& relativeToJson = s["relativeTo"];

            stage.type = Util::TypeFromStr(typeJson.GetString(), typeJson.GetString() + typeJson.GetStringLength());
            stage.postionType = Util::RelativeToFromStr(relativeToJson.GetString(), relativeToJson.GetString() + relativeToJson.GetStringLength());

            // space seperated flags.
            // basically need to check all the strings and see if it's a flag.
            {
                auto& flagsJson = s["flags"];

                stage.flags = Util::FlagsFromStr(flagsJson.GetString(), flagsJson.GetString() + flagsJson.GetStringLength());
            }

            {
                auto& matsJson = s["materials"];

                for (auto& m : matsJson.GetArray()) {
                    if (!m.IsString()) {
                        X_ERROR("Fx", "material entry is not type string");
                        return false;
                    }

                    stage.materials.emplace_back(core::string(m.GetString(), m.GetStringLength()));
                }
            }

            stage.interval = s["interval"].GetInt();
            stage.loopCount = s["loopCount"].GetInt();

            // need to basically parse a fuck ton of shit :D
            for (auto& r : ranges) {
                core::StackString<128> startStr, rangeStr;

                startStr.setFmt("%sStart", r.first);
                rangeStr.setFmt("%sRange", r.first);

                if (!s.HasMember(startStr.c_str()) || !s.HasMember(rangeStr.c_str())) {
                    X_ERROR("Fx", "Missing required range values: \"%s\" \"%s\"", startStr.c_str(), rangeStr.c_str());
                    return false;
                }

                if (!s[startStr.c_str()].IsNumber() || !s[rangeStr.c_str()].IsNumber()) {
                    X_ERROR("Fx", "Incorrect type for range values: \"%s\" \"%s\"", startStr.c_str(), rangeStr.c_str());
                    return false;
                }

                auto& range = r.second;
                range.start = s[startStr.c_str()].GetFloat();
                range.range = s[rangeStr.c_str()].GetFloat();
            }

            if (!parseGraph(s, "colorGraph", stage.color, [](core::json::Document::ValueType& v, ColGraphSet::Type& col) -> bool {
                    if (!checkMember(v, "rgb", core::json::kArrayType)) {
                        return false;
                    }
                    auto rgb = v["rgb"].GetArray();
                    if (rgb.Size() != 3) {
                        return false;
                    }

                    col[0] = rgb[0].GetFloat();
                    col[1] = rgb[1].GetFloat();
                    col[2] = rgb[2].GetFloat();
                    return true;
                })) {
                return false;
            }

            if (!parseGraphFloat(s, "alphaGraph", stage.alpha)) {
                return false;
            }
            if (!parseGraphFloat(s, "size0Graph", stage.size0)) {
                return false;
            }
            if (!parseGraphFloat(s, "size1Graph", stage.size1)) {
                return false;
            }
            if (!parseGraphFloat(s, "scaleGraph", stage.scale)) {
                return false;
            }
            if (!parseGraphFloat(s, "rotGraph", stage.rot)) {
                return false;
            }
            if (!parseGraphFloat(s, "vel0XGraph", stage.vel0X)) {
                return false;
            }
            if (!parseGraphFloat(s, "vel0YGraph", stage.vel0Y)) {
                return false;
            }
            if (!parseGraphFloat(s, "vel0ZGraph", stage.vel0Z)) {
                return false;
            }
            if (!parseGraphFloat(s, "vel1XGraph", stage.vel1X)) {
                return false;
            }
            if (!parseGraphFloat(s, "vel1YGraph", stage.vel1Y)) {
                return false;
            }
            if (!parseGraphFloat(s, "vel1ZGraph", stage.vel1Z)) {
                return false;
            }

            if (stage.flags.IsSet(StageFlag::RandGraphCol)) {
                if (stage.color.graphs.size() < 2) {
                    stage.flags.Remove(StageFlag::RandGraphCol);
                    X_WARNING("Fx", "RandCol requires atleast two graphs");
                }
            }
            if (stage.flags.IsSet(StageFlag::RandGraphAlpha)) {
                if (stage.alpha.graphs.size() < 2) {
                    stage.flags.Remove(StageFlag::RandGraphAlpha);
                    X_WARNING("Fx", "RandAlpha requires atleast two graphs");
                }
            }
            if (stage.flags.IsSet(StageFlag::RandGraphSize0)) {
                if (stage.size0.graphs.size() < 2) {
                    stage.flags.Remove(StageFlag::RandGraphSize0);
                    X_WARNING("Fx", "RandSize requires atleast two graphs");
                }
            }
            if (stage.flags.IsSet(StageFlag::RandGraphSize1)) {
                if (stage.size1.graphs.size() < 2) {
                    stage.flags.Remove(StageFlag::RandGraphSize1);
                    X_WARNING("Fx", "RandSize requires atleast two graphs");
                }
            }
            if (stage.flags.IsSet(StageFlag::RandGraphVel0)) {
                if (stage.vel0X.graphs.size() < 2 || stage.vel0Y.graphs.size() < 2 || stage.vel0Z.graphs.size() < 2) {
                    stage.flags.Remove(StageFlag::RandGraphVel0);
                    X_WARNING("Fx", "RandVel0 requires atleast two graphs");
                }
            }
            if (stage.flags.IsSet(StageFlag::RandGraphVel1)) {
                if (stage.vel1X.graphs.size() < 2 || stage.vel1Y.graphs.size() < 2 || stage.vel1Z.graphs.size() < 2) {
                    stage.flags.Remove(StageFlag::RandGraphVel1);
                    X_WARNING("Fx", "RandVel1 requires atleast two graphs");
                }
            }

            // processing.
            {
                if (stage.materials.isEmpty()) {
                    X_LOG0("Fx", "Skipping segment \"%s\" no materials specified", name.c_str());
                    continue;
                }

                if (stage.flags.IsSet(StageFlag::Looping)) {
                    // 0 is infinate, negative values not allowed tho.
                    if (stage.loopCount < 0) {
                        X_ERROR("Fx", "Segment \"%s\" Loop count is negative", name.c_str());
                        return false;
                    }
                }
                else {
                    if (stage.count.range < 0) {
                        X_ERROR("Fx", "Segment \"%s\" count range can't be negative", name.c_str());
                        return false;
                    }

                    // make sure we will spawn something.
                    auto maxCount = stage.count.start + stage.count.range;
                    if (stage.count.start <= 0) {
                        X_LOG0("Fx", "Skipping none looping segment \"%s\" with a max count of zero", name.c_str());
                        continue;
                    }
                }

                // check things that would result in all elements been invisible
                if (stage.size0.scale == 0.f) {
                    X_WARNING("Fx", "Skipping segment \"%s\" elem scale is zero", name.c_str());
                    continue;
                }

                if (stage.flags.IsSet(StageFlag::NonUniformScale)) {
                    if (stage.size1.scale == 0.f) {
                        X_WARNING("Fx", "Skipping segment \"%s\" elem scale1 is zero, with none uniform scale enabled", name.c_str());
                        continue;
                    }
                }
                else {
                    // house keeping
                    stage.flags.Remove(StageFlag::RandGraphSize1);
                    stage.size1.graphs.clear();
                }

                auto removeUnusedGraphs = [](FloatGraphSet& g, bool remove) {
                    if (remove && g.graphs.size() > 1) {
                        g.graphs.erase(g.graphs.begin() + 1, g.graphs.end());
                    }
                };

                // if we are not going to belnd with 2nd graph remove it.
                if (!stage.flags.IsSet(StageFlag::RandGraphCol) && stage.color.graphs.size() > 1) {
                    stage.color.graphs.erase(stage.color.graphs.begin() + 1, stage.color.graphs.end());
                }

                removeUnusedGraphs(stage.size0, !stage.flags.IsSet(StageFlag::RandGraphSize0));
                removeUnusedGraphs(stage.size1, !stage.flags.IsSet(StageFlag::RandGraphSize1));
                removeUnusedGraphs(stage.alpha, !stage.flags.IsSet(StageFlag::RandGraphAlpha));
                removeUnusedGraphs(stage.vel0X, !stage.flags.IsSet(StageFlag::RandGraphVel0));
                removeUnusedGraphs(stage.vel0Y, !stage.flags.IsSet(StageFlag::RandGraphVel0));
                removeUnusedGraphs(stage.vel0Z, !stage.flags.IsSet(StageFlag::RandGraphVel0));
                removeUnusedGraphs(stage.vel1X, !stage.flags.IsSet(StageFlag::RandGraphVel1));
                removeUnusedGraphs(stage.vel1Y, !stage.flags.IsSet(StageFlag::RandGraphVel1));
                removeUnusedGraphs(stage.vel1Z, !stage.flags.IsSet(StageFlag::RandGraphVel1));

                auto clearIfZeroScale = [](FloatGraphSet& g) {
                    if (g.scale == 0.f) {
                        g.graphs.clear();
                    }
                };

                clearIfZeroScale(stage.rot);

                clearIfZeroScale(stage.vel0X);
                clearIfZeroScale(stage.vel0Y);
                clearIfZeroScale(stage.vel0Z);

                clearIfZeroScale(stage.vel1X);
                clearIfZeroScale(stage.vel1Y);
                clearIfZeroScale(stage.vel1Z);

                // handle negative ranges.
                auto shiftRange = [](Range& r) {
                    if (r.range < 0.f) {
                        auto range = r.start - r.range;

                        r.start -= range;
                        r.range = range;

                        X_ASSERT(range > 0, "Range is negative")
                        (range, r.start, r.range);
                    }
                };

                auto absToRelativeRange = [](Range& r) {
                    auto diff = r.range - r.start;
                    r.range = diff;
                };

                shiftRange(stage.spawnOrgX);
                shiftRange(stage.spawnOrgY);
                shiftRange(stage.spawnOrgZ);

                // some ranges we want to actually be start and end.
                // the are both same thing no?
                // one is a absolute range the other relative.
                // but if i have rotations of 90 and -90.
                // how todo :/
                absToRelativeRange(stage.initialRotation);

                // currently scale graph is unused.
                stage.scale.graphs.clear();

                //vel1 not used.
                stage.flags.Remove(StageFlag::RandGraphVel1);
                stage.vel1X.graphs.clear();
                stage.vel1Y.graphs.clear();
                stage.vel1Z.graphs.clear();

                for (const auto& r : ranges) {
                    auto& range = r.second;
                    if (range.range < 0) {
                        X_ERROR("Fx", "Negative range for: \"%s\"", r.first);
                        return false;
                    }
                }
            }

            stages_.emplace_back(std::move(stage));
        }

        return true;
    }

    bool EffectCompiler::writeToFile(core::XFile* pFile) const
    {
        if (stages_.isEmpty()) {
            X_ERROR("Fx", "Effect has no acitive segments, skipping compile");
            return false;
        }

        core::ByteStream bs(arena_);
        bs.reserve(2048);

        core::Array<float> fltPool(arena_);
        core::Array<IndexType> indexes(arena_);

        fltPool.setGranularity(64);
        indexes.setGranularity(64);

        auto uniqueFloat = [&](float val) -> size_t {
            auto it = std::find_if(fltPool.begin(), fltPool.end(), [val](float f) {
                return math<float>::abs(f - val) < 0.0001f;
            });
            if (it != fltPool.end()) {
                return std::distance(fltPool.begin(), it);
            }

            return fltPool.push_back(val);
        };

        auto processGraph = [&](StageDsc::GraphArr& graphsOut, const auto& srcGraph, auto addDataFunc) -> bool {
            for (size_t i = 0; i < graphsOut.size(); i++) {
                auto& graphOut = graphsOut[i];

                if (i >= srcGraph.graphs.size()) {
                    core::zero_object(graphOut);
                    continue;
                }

                const auto& graph = srcGraph.graphs[i];
                const auto& points = graph.points;

                graphOut.numPoints = safe_static_cast<decltype(graphOut.numPoints)>(points.size());
                graphOut.timeStart = safe_static_cast<IndexOffset>(indexes.size());
                graphOut.scaleIdx = safe_static_cast<IndexType>(uniqueFloat(srcGraph.scale));

                for (auto p : points) {
                    auto idx = uniqueFloat(p.time);
                    indexes.push_back(safe_static_cast<IndexType>(idx));
                }

                graphOut.valueStart = safe_static_cast<IndexOffset>(indexes.size());

                for (auto p : points) {
                    addDataFunc(p.data);
                }
            }

            return true;
        };

        auto addFloat = [&](float val) {
            auto idx = uniqueFloat(val);
            indexes.push_back(safe_static_cast<IndexType>(idx));
        };
        auto addColor = [&](const Vec3f& val) {
            for (int32_t i = 0; i < 3; i++) {
                auto idx = uniqueFloat(val[i]);
                indexes.push_back(safe_static_cast<IndexType>(idx));
            }
        };

        size_t matOffset = 0;
        for (const auto& stage : stages_) {
            StageDsc compiledStage = stage; // slice in the already set fields.

            processGraph(compiledStage.alpha, stage.alpha, addFloat);
            processGraph(compiledStage.size0, stage.size0, addFloat);
            processGraph(compiledStage.size1, stage.size1, addFloat);
            processGraph(compiledStage.scale, stage.scale, addFloat);
            processGraph(compiledStage.rot, stage.rot, addFloat);
            processGraph(compiledStage.vel0X, stage.vel0X, addFloat);
            processGraph(compiledStage.vel0Y, stage.vel0Y, addFloat);
            processGraph(compiledStage.vel0Z, stage.vel0Z, addFloat);
            processGraph(compiledStage.color, stage.color, addColor);

            compiledStage.materialStrOffset = safe_static_cast<int32_t>(matOffset);

            matOffset += core::strUtil::StringBytesIncNull(stage.materials.front());

            bs.write(compiledStage);
        }

        // now we write the indexes and points.
        bs.write(indexes.data(), indexes.size());
        bs.write(fltPool.data(), fltPool.size());

        // write strings at end.
        for (const auto& stage : stages_) {
            for (auto& m : stage.materials) {
                bs.write(m.c_str(), core::strUtil::StringBytesIncNull(m));
            }
        }

        EffectHdr hdr;
        hdr.fourCC = EFFECT_FOURCC;
        hdr.version = EFFECT_VERSION;
        hdr.numStages = safe_static_cast<decltype(hdr.numStages)>(stages_.size());
        hdr.numIndex = safe_static_cast<decltype(hdr.numIndex)>(indexes.size());
        hdr.numFloats = safe_static_cast<decltype(hdr.numFloats)>(fltPool.size());

        if (pFile->writeObj(hdr) != sizeof(hdr)) {
            X_ERROR("Fx", "Failed to write header");
            return false;
        }

        if (pFile->writeObj(bs.data(), bs.size()) != bs.size()) {
            X_ERROR("Fx", "Failed to write data");
            return false;
        }

        return true;
    }

} // namespace fx

X_NAMESPACE_END