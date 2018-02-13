#include "stdafx.h"
#include "Compiler.h"

#include <IFileSys.h>
#include <Hashing\Fnva1Hash.h>
#include <String\StringTokenizer.h>

using namespace core::Hash::Fnv1Literals;

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

		for (auto& graph : colGraph["graphs"].GetArray())
		{
			// each graph is a array of objects
			if (!graph.IsArray()) {
				X_ERROR("Fx", "Graph is not of type array");
				return false;
			}

			GraphSetT::GraphT efxGraph(arena_);

			for (auto& g : graph.GetArray())
			{
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
		auto test = R"(
			{
				"stages" : [
					{
						"name" : "segment1",
						"type" : "OrientedSprite",
						"flags" : "RandGraphAlpha RandGraphSize RandGraphVel",

						"materials" : [
							"fx/test/raygun_ring"
						],

						"interval": 100,
						"loopCount": 0,

						"countStart": 0,
						"countRange": 0,
						"lifeStart": 300,
						"lifeRange": 200,
						"delayStart": 0,
						"delayRange": 0,
					
						"spawnOrgXStart" : 0,
						"spawnOrgXRange" : 0,
						"spawnOrgYStart" : 0,
						"spawnOrgYRange" : 0,
						"spawnOrgZStart" : 10,
						"spawnOrgZRange" : 2,

						"colorGraph" : {
							"scale" : 1,							
							"graphs": [
								[
									{
										"time": 0,
										"rgb": [
											0,
											1,
											1
										]
									},
									{
										"time": 1,
										"rgb": [
											0,
											1,
											0
										]
									}
								],
								[
									{
										"time": 0,
										"rgb": [
											0, 1, 1
										]
									},
									{
										"time": 1,
										"rgb": [
											1, 0, 0
										]
									}
								]
							]
						},
						"alphaGraph" : {
							"scale" : 1,							
							"graphs": [
								[
									{
										"time": 0,
										"val": 1
									},
									{
										"time": 0.09,
										"val": 0.57
									},
									{
										"time": 0.41,
										"val": 0.2
									},
									{
										"time": 0.71,
										"val": 0.028
									},
									{
										"time": 1,
										"val": 0
									}
								],
								[
									{
										"time": 0,
										"val": 1
									},
									{
										"time": 0.078,
										"val": 0.71
									},
									{
										"time": 0.24,
										"val": 0.39
									},
									{
										"time": 0.46,
										"val": 0.14
									},
									{
										"time": 0.81,
										"val": 0.004
									},									
									{
										"time": 1,
										"val": 0
									}
								]
							]
						},
						"sizeGraph" : {
							"scale" : 15,							
							"graphs": [
								[
									{
										"time": 0,
										"val": 0.227273
									},
									{
										"time": 1,
										"val": 0.75974
									}
								],
								[
									{
										"time": 0,
										"val": 0.233766
									},
									{
										"time": 1,
										"val": 1
									}
								]
							]
						},
						"scaleGraph" : {
							"scale" : 15,							
							"graphs": [
								[
									{
										"time": 0,
										"val": 0.5
									},
									{
										"time": 1,
										"val": 1
									}
								]
							]
						},
						"rotGraph" : {
							"scale" : 0,							
							"graphs": [
								[
									{
										"time": 0,
										"val": 0
									},
									{
										"time": 1,
										"val": 0
									}
								]
							]
						},
						"vel0XGraph" : {
							"scale" : 0,							
							"graphs": [
								[
									{
										"time": 0,
										"val": 0.5
									},
									{
										"time": 1,
										"val": -0.1
									}
								],
								[
									{
										"time": 0,
										"val": 0.5
									},
									{
										"time": 1,
										"val": 0.02
									}
								]
							]
						},
						"vel0YGraph" : {
							"scale" : 0,							
							"graphs": [
								[
									{
										"time": 0,
										"val": 0
									},
									{
										"time": 1,
										"val": 0
									}
								],
								[
									{
										"time": 0,
										"val": 0
									},
									{
										"time": 1,
										"val": 0
									}
								]
							]
						},
						"vel0ZGraph" : {
							"scale" : 10,							
							"graphs": [
								[
									{
										"time": 0,
										"val": 0
									},
									{
										"time": 1,
										"val": 1
									}
								],
								[
									{
										"time": 0,
										"val": 0
									},
									{
										"time": 1,
										"val": 0.98
									}
								]
							]
						}
					}
				]
			}
		)";

		str = test;

	
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

		for (auto& s : stages.GetArray())
		{
			StageBuilder stage(arena_);
			core::zero_object<Stage>(stage);
		
			const std::array<std::pair<const char*, Range&>, 6> ranges = { {
				{ "count", stage.count },
				{ "life", stage.life },
				{ "delay", stage.delay },
				{ "spawnOrgX", stage.spawnOrgX },
				{ "spawnOrgY", stage.spawnOrgY },
				{ "spawnOrgZ", stage.spawnOrgZ },
			} };

			if (!checkMember(s, "type", core::json::kStringType)) {
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

			auto& typeJson = s["type"];
			switch (core::Hash::Fnv1aHash(typeJson.GetString(), typeJson.GetStringLength()))
			{
				case "BillboardSprite"_fnv1a:
					stage.type = StageType::BillboardSprite;
					break;
				case "OrientedSprite"_fnv1a:
					stage.type = StageType::OrientedSprite;
					break;
				case "Tail"_fnv1a:
					stage.type = StageType::Tail;
					break;
				case "Line"_fnv1a:
					stage.type = StageType::Line;
					break;
				case "Sound"_fnv1a:
					stage.type = StageType::Sound;
					break;
				default:
					X_ERROR("Fx", "Unkonw type: \"%.*s\"", typeJson.GetStringLength(), typeJson.GetString());
					return false;
			}

			// space seperated flags.
			// basically need to check all the strings and see if it's a flag.
			{
				auto& flagsJson = s["flags"];

				core::StringRange<char> token(nullptr, nullptr);
				core::StringTokenizer<char> tokens(flagsJson.GetString(), 
					flagsJson.GetString() + flagsJson.GetStringLength(), ' ');

				while (tokens.ExtractToken(token))
				{
					switch (core::Hash::Fnv1aHash(token.GetStart(), token.GetLength()))
					{	
						case "Looping"_fnv1a:
							stage.flags.Set(StageFlag::Looping);
							break;
						case "RandGraphCol"_fnv1a:
							stage.flags.Set(StageFlag::RandGraphCol);
							break;
						case "RandGraphAlpha"_fnv1a:
							stage.flags.Set(StageFlag::RandGraphAlpha);
							break;
						case "RandGraphSize"_fnv1a:
							stage.flags.Set(StageFlag::RandGraphSize);
							break;
						case "RandGraphVel"_fnv1a:
							stage.flags.Set(StageFlag::RandGraphVel);
							break;
						default:
							X_ERROR("Fx", "Unkonw flag: \"%.*s\"", token.GetLength(), token.GetStart());
							return false;
					}
				}
			}

			{
				auto& matsJson = s["materials"];

				for (auto& m : matsJson.GetArray())
				{
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
			for (auto& r : ranges)
			{
				core::StackString<128> start, range;

				start.setFmt("%sStart", r.first);
				range.setFmt("%sRange", r.first);

				if (!s.HasMember(start.c_str()) || !s.HasMember(range.c_str())) {
					X_ERROR("Fx", "Missing required range values: \"%s\" \"%s\"", start.c_str(), range.c_str());
					return false;
				}

				if (!s[start.c_str()].IsNumber() || !s[range.c_str()].IsNumber()) {
					X_ERROR("Fx", "Incorrect type for range values: \"%s\" \"%s\"", start.c_str(), range.c_str());
					return false;
				}
				
				r.second.start = s[start.c_str()].GetFloat();
				r.second.range = s[range.c_str()].GetFloat();
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
			if (!parseGraphFloat(s, "sizeGraph", stage.size)) {
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

			stages_.emplace_back(std::move(stage));
		}

		return true;
	}


	bool EffectCompiler::writeToFile(core::XFile* pFile) const
	{
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

		auto processGraph = [&](Stage::GraphArr& graphsOut, const auto& srcGraph, auto addDataFunc) -> bool {

			for (size_t i = 0; i < graphsOut.size(); i++)
			{
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

				for (auto p : points)
				{
					auto idx = uniqueFloat(p.time);
					indexes.push_back(safe_static_cast<IndexType>(idx));
				}

				graphOut.valueStart = safe_static_cast<IndexOffset>(indexes.size());

				for (auto p : points)
				{
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
		for (const auto& stage : stages_)
		{
			Stage compiledStage = stage; // slice in the already set fields.

			processGraph(compiledStage.alpha, stage.alpha, addFloat);
			processGraph(compiledStage.size, stage.size, addFloat);
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
		for (const auto& stage : stages_)
		{
			for (auto& m : stage.materials)
			{
				bs.write(m.c_str(), core::strUtil::StringBytesIncNull(m));
			}
		}

		EffectHdr hdr;
		hdr.fourCC = EFFECT_FOURCC;
		hdr.version = EFFECT_VERSION;
		hdr.numStages = safe_static_cast<uint8_t>(stages_.size());
		hdr.numIndex = safe_static_cast<uint8_t>(indexes.size());
		hdr.numFloats = safe_static_cast<uint8_t>(fltPool.size());
		
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