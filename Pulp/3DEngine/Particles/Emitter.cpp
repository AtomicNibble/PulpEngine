#include "stdafx.h"
#include "Emitter.h"
#include "Effect.h"

#include "EngineEnv.h"

#include "Material\MaterialManager.h"

#include <IPrimativeContext.h>
#include <IFrameData.h>

X_NAMESPACE_BEGIN(engine)

namespace fx
{


	Emitter::Emitter(const Effect& efx, core::MemoryArenaBase* arena) :
		efx_(efx),
		stages_(arena, efx_.getNumStages(), StageState(arena))
	{
	}

	void Emitter::setTrans(const Transformf& trans)
	{
		trans_ = trans;
	}

	void Emitter::setTrans(const Transformf& trans, const Vec3f& offset)
	{
		trans_ = trans;
		offset_ = offset;
	}


	void Emitter::update(core::TimeVal delta)
	{
		// for now lets just start emitting everything?
		// all stages lets go!
		auto* pStages = efx_.getStages();

		elapsed_ += delta;

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

					if (e.atlasIdx == atlasIdx) {
						continue;
					}

					e.atlasIdx = atlasIdx;

					auto idx = (e.atlasBaseIdx + atlasIdx) % atlasCount;

					uvForIndex(e.uv, atlas, idx);
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

			if (stage.type == StageType::OrientedSprite || stage.type == StageType::BillboardSprite)
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
				e.atlasIdx = 0;
				e.atlasBaseIdx = 0;

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
						if (atlasIdx >= count) {
							X_WARNING("Fx", "Atlast index was out of range");
							atlasIdx = count - 1;
						}
					}

					uvForIndex(e.uv, atlas, atlasIdx);

					e.atlasBaseIdx = atlasIdx;
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
		X_ASSERT(idx < atlas.x * atlas.y, "Index out of range")(idx, atlas.x, atlas.y, atlas.x * atlas.y);

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


	void Emitter::draw(core::FrameView& view, IPrimativeContext* pPrim)
	{
		Quatf viewQ(view.viewMatrix.transposed());
		viewQ.invert();


		for (size_t i = 0; i < stages_.size(); i++)
		{
			auto& stage = efx_.getStages()[i];
			auto& s = stages_[i];

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


				if (stage.type == StageType::BillboardSprite)
				{
					// i want the sprite to always face the camera.
					tl = tl * viewQ;
					tr = tr * viewQ;
					bl = bl * viewQ;
					br = br * viewQ;
				}
				else
				{
					tl = tl * trans_.quat;
					tr = tr * trans_.quat;
					bl = bl * trans_.quat;
					br = br * trans_.quat;
				}

				// i want the axis to follow the emmiter.
				// 
				{
					Transformf t = trans_;
					t.quat = viewQ;

					pPrim->drawAxis(t, Vec3f(20.f));
				}
				//pPrim->drawAxis(trans_, offset_, Vec3f(20.f));

				Vec3f pos = e.pos + e.dir + offset_;
				//	pos = pos * trans_.quat;
				pos += trans_.pos;

				tl += pos;
				tr += pos;
				bl += pos;
				br += pos;

				pPrim->drawQuad(tl, tr, bl, br, s.pMaterial, e.col, e.uv);
				pPrim->drawRect(tl, tr, bl, br, Col_Red, Col_Yellow, Col_Green, Col_Blue);

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
		X_ASSERT(g.numPoints > 0, "Hraph is empty")(g.numPoints);

		float scale = getFloat(g.scaleIdx);
		float result = 0.f;

		if (g.numPoints > 1)
		{
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
		}
		else
		{
			result = floatForIdx(g.valueStart);
		}

		return result * scale;
	}

	Vec3f Emitter::fromColorGraph(const Graph& g, float t) const
	{
		X_ASSERT(g.numPoints > 0, "Hraph is empty")(g.numPoints);

		float scale = getFloat(g.scaleIdx);
		Vec3f result;

		if (g.numPoints > 1)
		{
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
		}
		else
		{
			result = colorForIdx(g.valueStart, 0);
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