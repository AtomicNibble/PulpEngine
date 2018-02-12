#include "stdafx.h"
#include "Effect.h"

#include <IEffect.h>
#include <IPrimativeContext.h>

#include "EngineEnv.h"
#include "Material\MaterialManager.h"

X_NAMESPACE_BEGIN(engine)

namespace fx
{

	Effect::Effect(core::string& name, core::MemoryArenaBase* arena) :
		core::AssetBase(name, assetDb::AssetType::FX),
		numStages_(0),
		numIndex_(0),
		numFloats_(0),
		dataSize_(0)
	{
		X_UNUSED(arena);
	}

	Effect::~Effect()
	{

	}

	bool Effect::processData(core::UniquePointer<char[]> data, uint32_t dataSize)
	{
		if (dataSize < sizeof(EffectHdr)) {
			return false;
		}

		EffectHdr& hdr = *reinterpret_cast<EffectHdr*>(data.get());

		if (!hdr.isValid()) {
			X_ERROR("Fx", "\"%s\" invalid header", name_.c_str());
			return false;
		}

		numStages_ = hdr.numStages;
		numIndex_ = hdr.numIndex;
		numFloats_ = hdr.numFloats;

		dataSize_ = dataSize;
		data_ = std::move(data);
		return true;
	}

	const Stage* Effect::getStages(void) const
	{
		return reinterpret_cast<Stage*>(data_.ptr() + sizeof(EffectHdr));
	}

	const uint8_t* Effect::getIndexes(void) const
	{
		size_t offset = sizeof(EffectHdr);
		offset += sizeof(Stage) * numStages_;

		return reinterpret_cast<uint8_t*>(data_.ptr() + offset);
	}

	const float* Effect::getFloats(void) const
	{
		size_t offset = sizeof(EffectHdr);
		offset += sizeof(Stage) * numStages_;
		offset += sizeof(IndexType) * numIndex_;

		return reinterpret_cast<float*>(data_.ptr() + offset);
	}

	const char* Effect::getMaterialName(int32_t strOffset) const
	{
		size_t offset = sizeof(EffectHdr);
		offset += sizeof(Stage) * numStages_;
		offset += sizeof(IndexType) * numIndex_;
		offset += sizeof(float) * numFloats_;
		offset += strOffset;

		return reinterpret_cast<const char*>(data_.ptr() + offset);
	}

	// --------------------------------------------------

	Emitter::Emitter(const Effect& efx, core::MemoryArenaBase* arena) :
		efx_(efx),
		stages_(arena, efx_.getNumStages(), StageState(arena))
	{
	}


	void Emitter::update(core::TimeVal delta)
	{
		// for now lets just start emitting everything?
		// all stages lets go!
		auto* pStages = efx_.getStages();

		elapsed_ += delta;

		static float test = 0.f;
		test += 0.01f;

		if (test > 1.f) {
			test = 0.f;
		}

		for (int32_t i = 0; i < efx_.getNumStages(); i++)
		{
			auto& stage = pStages[i];
			auto& state = stages_[i];

			if (!state.pMaterial) {
				auto* pStr = efx_.getMaterialName(stage.materialStrOffset);
				state.pMaterial = gEngEnv.pMaterialMan_->loadMaterial(pStr);
			}

			// update elems?
//			float elapsedMS = elapsed_.GetMilliSeconds();
			for (auto& e : state.elems)
			{
				Vec3f vel;
				vel.x = fromGraph(stage.vel0X, test);
				vel.y = fromGraph(stage.vel0Y, test);
				vel.z = fromGraph(stage.vel0Z, test);

				float size = fromGraph(stage.size, test);
				float alpha = fromGraph(stage.alpha, test);

				e.col.a = CHANTRAIT<uint8_t>::convert(alpha);
				e.size = size;
				e.dir = vel;
			}


			if (stage.type == StageType::OrientedSprite)
			{
				// so how todo this?
				// we basically have to spawn elementes, for this this stage.
				// each elem we spawn maintain it's random properies for it's life.
				// we then need to keep track of how many elems we have spawned, and spawn more till we reach limit.


				// finished.
				if (state.currentLoop > stage.loopCount)
				{
					continue;
				}
				

				if (state.currentLoop > 0)
				{
					auto sinceSpawn = elapsed_ - state.lastSpawn;
					auto interval = core::TimeVal::fromMS(stage.interval);

					if (sinceSpawn < interval)
					{
						continue;
					}
				}
				else if(stage.delay.start > 0)
				{
					// initial delay logic.
					
				}

				// 'spawn'
				++state.currentLoop;

				// need to basically come up withsome values.
				Vec3f pos;
				pos.x = fromRange(stage.spawnOrgX);
				pos.y = fromRange(stage.spawnOrgY);
				pos.z = fromRange(stage.spawnOrgZ);

				float life = fromRange(stage.life);

				Vec3f vel;
				vel.x = fromGraph(stage.vel0X, 0.f);
				vel.y = fromGraph(stage.vel0Y, 0.f);
				vel.z = fromGraph(stage.vel0Z, 0.f);

				// uniform scale.
				float size = fromGraph(stage.size, 0.f);
				// don't use scale for most things.
				//float scale = fromGraph(stage.scale, 0.f);

				Vec3f col(1,0,0);
				float alpha = fromGraph(stage.alpha, 0.f);


				Elem e;
				e.pos = pos;
				e.dir = vel;
				e.size = size;
				e.col = Colorf(col, alpha);
				e.lifeMs = static_cast<int32_t>(life);

				state.elems.push_back(e);
			}
			else
			{
				X_ASSERT_NOT_IMPLEMENTED();
			}

		}

	}

	void Emitter::draw(IPrimativeContext* pPrim)
	{
		for (auto& s : stages_)
		{
			if (s.elems.isEmpty()) {
				continue;
			}

			gEngEnv.pMaterialMan_->waitForLoad(s.pMaterial);

			for (const auto& e : s.elems)
			{
				// basically need to make a quad.
				// position is the center.
				// the 4 points will just be courners.
				// *-----*
				// |     |
				// |  -  |
				// |     |
				// *-----*

				float size = e.size;
				float half = size * 0.5f;
				
				Vec3f tl = Vec3f(-half, -half, 0);
				Vec3f tr = Vec3f(half, -half, 0);
				Vec3f bl = Vec3f(-half, half, 0);
				Vec3f br = Vec3f(half, half, 0);
				
				Vec3f pos = e.pos + e.dir;

				tl += pos;
				tr += pos;
				bl += pos;
				br += pos;

				pPrim->drawQuad(tl, tr, bl, br, s.pMaterial, e.col);
			}
		}
	}


	inline float Emitter::fromRange(const Range& r) const
	{
		if (r.range == 0.f) {
			return r.start;
		}

		auto val = gEnv->xorShift.randRange(r.range);

		return r.start + val;
	}

	float Emitter::fromGraph(const Graph& g, float t) const
	{
		auto* pIndexes = efx_.getIndexes();
		auto* pFlts = efx_.getFloats();

		float scale = pFlts[g.scaleIdx];
		float result = 0.f;

		for (int32_t i = 0; i < g.numPoints; i++)
		{
			auto val0 = pFlts[pIndexes[g.timeStart + i]];

			if (val0 == t)
			{
				result = pFlts[pIndexes[g.valueStart + i]];
				break;
			}
			else if (val0 > t)
			{
				// blend.
				val0 = pFlts[pIndexes[g.timeStart + (i - 1)]];
				auto val1 = pFlts[pIndexes[g.timeStart + i]];

				auto res0 = pFlts[pIndexes[g.valueStart + (i - 1)]];
				auto res1 = pFlts[pIndexes[g.valueStart + i]];

				float offset = t - val0;
				float range = val1 - val0;
				float fraction = offset / range;

				result = lerp(res0, res1, fraction);
				break;
			}
		}

		return result * scale;
	}


} // namespace fx


X_NAMESPACE_END