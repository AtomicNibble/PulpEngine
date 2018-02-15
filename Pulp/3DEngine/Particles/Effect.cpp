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

	void Emitter::setPos(const Vec3f& pos)
	{
		pos_ = pos;
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

			{
				const auto atlas = state.pMaterial->getAtlas();
				const auto fps = stage.sequence.fps;
				const int32_t atlasCount = atlas.x * atlas.y;

				for (size_t j = 0; j < state.elems.size(); j++)
				{
					auto& e = state.elems[j];

					auto alive = (elapsed_ - e.spawnTime);
					auto aliveMs = alive.GetMilliSeconds();

					if (aliveMs >= e.lifeMs)
					{
						state.elems.removeIndex(j);
						continue;
					}

					float fraction = (aliveMs / e.lifeMs);
					updateElemForFraction(stage, e, fraction);

					if (fps == 0) {
						continue;
					}

					auto aliveSec = aliveMs * 0.001;

					int32_t atlasIdx;
					if (fps < 0)
					{
						atlasIdx = static_cast<int32_t>(atlasCount * fraction); // total frames over elem life.
					}
					else
					{
						atlasIdx = static_cast<int32_t>(fps * aliveSec); // 1 second == fps.
					}

					// offset from starting index.
					atlasIdx += e.atlasIdx;
					atlasIdx %= atlasCount;

					uvForIndex(e.uv, atlas, atlasIdx);
				}
			}

			// finished.
			if (stage.loopCount && state.currentLoop >= stage.loopCount)
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
			else if (stage.delay.start > 0)
			{
				// initial delay logic.

			}

			// 'spawn'
			++state.currentLoop;

			state.lastSpawn = elapsed_;

			if (stage.type == StageType::OrientedSprite)
			{
				int8_t colGraph = 0;
				int8_t alphaGraph = 0;
				int8_t sizeGraph = 0;
				int8_t velGraph = 0;
				
				if (stage.flags.IsSet(StageFlag::RandGraphCol)) {
					colGraph = gEnv->xorShift.rand() & 0x1;
				}
				if (stage.flags.IsSet(StageFlag::RandGraphAlpha)) {
					alphaGraph = gEnv->xorShift.rand() & 0x1;
				}
				if (stage.flags.IsSet(StageFlag::RandGraphSize)) {
					sizeGraph = gEnv->xorShift.rand() & 0x1;
				}
				if (stage.flags.IsSet(StageFlag::RandGraphVel)) {
					velGraph = gEnv->xorShift.rand() & 0x1;
				}

				// meow.
				Vec3f pos;
				pos.x = fromRange(stage.spawnOrgX);
				pos.y = fromRange(stage.spawnOrgY);
				pos.z = fromRange(stage.spawnOrgZ);

				float life = fromRange(stage.life);

				// so i need to support atlas.		
				auto atlas = state.pMaterial->getAtlas();

				Elem e;
				e.uv = Rectf(0.f, 0.f, 1.f, 1.f);

				if (atlas.x || atlas.y)
				{
					int32_t atlasIdx = stage.sequence.startFrame;
					int32_t count = atlas.x * atlas.y;
					if (atlasIdx < 0)
					{
						atlasIdx = static_cast<int32_t>(gEnv->xorShift.randIndex(count));
					}
					else
					{
						// user might of changed material so atlasIdx is no longer in range.
						if (atlasIdx >= count - 1) {
							X_WARNING("Fx", "Atlast index was out of range");
							atlasIdx = count - 1;
						}
					}

					uvForIndex(e.uv, atlas, atlasIdx);

					e.atlasIdx = atlasIdx;
				}
				
				e.colGraph = colGraph;
				e.alphaGraph = alphaGraph;
				e.sizeGraph = sizeGraph;
				e.velGraph = velGraph;
				e.pos = pos;
				e.spawnTime = elapsed_;
				e.lifeMs = life;

				updateElemForFraction(stage, e, 0.f);

				state.elems.push_back(e);
			}
			else
			{
				X_ASSERT_NOT_IMPLEMENTED();
			}

		}

	}

	inline void Emitter::uvForIndex(Rectf& uv, const Vec2<int16_t> atlas, int32_t idx)
	{
		int32_t col = idx % atlas.x;
		int32_t row = idx / atlas.x;

		if (atlas.x)
		{
			float range = 1.f / atlas.x;

			uv.x1 = range * col;
			uv.x2 = uv.x1 + range;
		}
		if (atlas.y)
		{
			float range = 1.f / atlas.y;

			uv.y1 = range * row;
			uv.y2 = uv.y1 + range;
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
				pos += pos_;

				tl += pos;
				tr += pos;
				bl += pos;
				br += pos;

				pPrim->drawQuad(tl, tr, bl, br, s.pMaterial, e.col, e.uv);
			}
		}
	}

	void Emitter::updateElemForFraction(const Stage& stage, Elem& e, float fraction) const
	{
		Vec3f vel;
		vel.x = fromGraph(stage.vel0X[e.velGraph], fraction);
		vel.y = fromGraph(stage.vel0Y[e.velGraph], fraction);
		vel.z = fromGraph(stage.vel0Z[e.velGraph], fraction);

		float size = fromGraph(stage.size[e.sizeGraph], fraction);

		Vec3f col = fromColorGraph(stage.color[e.colGraph], fraction);
		float alpha = fromGraph(stage.alpha[e.alphaGraph], fraction);

		e.dir = vel;
		e.size = size;
		e.col = Color8u(col, alpha);
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
		float scale = getFloat(g.scaleIdx);
		float result = 0.f;

		for (int32_t i = 0; i < g.numPoints; i++)
		{
			auto val0 = floatForIdx(g.timeStart + i);

			if (val0 == t)
			{
				result = floatForIdx(g.valueStart + i);
				break;
			}
			else if (val0 > t)
			{
				// blend.
				val0 = floatForIdx(g.timeStart + (i - 1));
				auto val1 = floatForIdx(g.timeStart + i);

				auto res0 = floatForIdx(g.valueStart + (i - 1));
				auto res1 = floatForIdx(g.valueStart + i);

				float offset = t - val0;
				float range = val1 - val0;
				float fraction = offset / range;

				result = lerp(res0, res1, fraction);
				break;
			}
		}

		return result * scale;
	}

	Vec3f Emitter::fromColorGraph(const Graph& g, float t) const
	{
		float scale = getFloat(g.scaleIdx);
		Vec3f result;

		for (int32_t i = 0; i < g.numPoints; i++)
		{
			auto val0 = floatForIdx(g.timeStart + i);

			if (val0 == t)
			{
				result = colorForIdx(g.valueStart, i);
				break;
			}
			else if (val0 > t)
			{
				// blend.
				val0 = floatForIdx(g.timeStart + (i - 1));
				auto val1 = floatForIdx(g.timeStart + i);

				auto res0 = colorForIdx(g.valueStart, i - 1);
				auto res1 = colorForIdx(g.valueStart, i);

				float offset = t - val0;
				float range = val1 - val0;
				float fraction = offset / range;

				result = res0.lerp(fraction, res1);
				break;
			}
		}

		return result * scale;
	}

	X_INLINE float Emitter::getFloat(int32_t idx) const
	{
		auto* pFlts = efx_.getFloats();

		return pFlts[idx];
	}

	X_INLINE float Emitter::floatForIdx(int32_t idx) const
	{
		auto* pIndexes = efx_.getIndexes();
		auto* pFlts = efx_.getFloats();

		idx = pIndexes[idx];
		return pFlts[idx];
	}

	X_INLINE Vec3f Emitter::colorForIdx(int32_t start, int32_t idx) const
	{
		idx = start + (idx * 3);
		return Vec3f(
			floatForIdx(idx),
			floatForIdx(idx + 1),
			floatForIdx(idx + 2)
		);
	}

} // namespace fx


X_NAMESPACE_END