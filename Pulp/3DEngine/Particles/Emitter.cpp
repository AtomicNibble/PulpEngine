#include "stdafx.h"
#include "Emitter.h"
#include "Effect.h"

#include "EngineEnv.h"
#include "Material\MaterialManager.h"
#include "Vars\EffectVars.h"

#include <IPrimativeContext.h>
#include <IFrameData.h>

X_NAMESPACE_BEGIN(engine)

namespace fx
{


	Emitter::Emitter(const EffectVars& effectVars, core::MemoryArenaBase* arena) :
		vars_(effectVars),
		arena_(arena),
		pEfx_(nullptr),
		curStage_(0),
		activeStages_(arena)
	{
	}

	void Emitter::play(const Effect* pEfx, bool clear)
	{
		pEfx_ = pEfx;
		elapsed_.SetValue(0);
		curStage_ = 0;

		if (clear) {
			activeStages_.clear();
		}
	}

	bool Emitter::isPlaying(void) const
	{
		return pEfx_ || activeStages_.isNotEmpty();
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
		// so how this needs to work is.
		// when we play a fx, we keep looping throught it's stages untill they are all 'active'.
		// then we just clear the fx.
		// any active stages live on, and die out as defined.
		// this way if you change the active fx half way through any active stages will continue.
		// TODO: potentially I want to prevent new elems been spawned also.
		

		elapsed_ += delta;

		auto elapsedMS = elapsed_.GetMilliSeconds();

		if (pEfx_)
		{
			// TODO: make start time from loaded.
			if (!pEfx_->isLoaded()) {
				return;
			}

			auto& efx = *pEfx_;
			X_ASSERT(curStage_ < efx.getNumStages(), "Stage index out of bounds")(curStage_, efx.getNumStages());

			auto numStages = efx.getNumStages();

			// stages are sorted by delay.
			for (; curStage_ < numStages; ++curStage_)
			{
				auto& stageDesc = efx.getStageDsc(curStage_);

				auto delay = fromRange(stageDesc.delay);
				if (delay > elapsedMS) {
					break;
				}

				// play this stage.
				auto* pStr = efx.getMaterialName(stageDesc.materialStrOffset);
				auto* pMaterial = gEngEnv.pMaterialMan_->loadMaterial(pStr);

				int32_t maxElems;
				
				if (stageDesc.flags.IsSet(StageFlag::Looping))
				{
					auto maxLife = stageDesc.life.start + stageDesc.life.range;
					auto maxAlive = static_cast<int32_t>(maxLife / stageDesc.interval);

					maxAlive = core::Min(maxAlive, stageDesc.loopCount);
					maxElems = maxAlive;
				}
				else
				{
					maxElems = static_cast<int32_t>(stageDesc.count.start + stageDesc.count.range);
				}

				if (maxElems < 1) {
					X_WARNING("Fx", "Calculated max elems is less than one: %" PRId32, maxElems);
				}

				maxElems = core::Max(maxElems, 1);

				activeStages_.emplace_back(pEfx_, &stageDesc, pMaterial, trans_, maxElems, arena_);
			}

			// finished all the stages for this efx?
			if (curStage_ == numStages) {
				pEfx_ = nullptr;
			}
		}

		// so now just need to update any active stages.
		if (activeStages_.isEmpty()) {
			return;
		}


		// update elems.
		for (size_t i = 0; i < activeStages_.size(); i++)
		{
			auto& stage = activeStages_[i];
			auto& elems = stage.elems;

			// update any active elems.
			if (elems.isEmpty()) {
				continue;
			}

			auto& desc = *stage.pDesc;

			for (auto it = elems.begin(); it != elems.end();)
			{
				auto& e = *it;

				auto alive = (elapsed_ - e.spawnTime);
				auto aliveMs = alive.GetMilliSeconds();

				if (aliveMs >= e.lifeMs)
				{
					it = elems.erase(it);
					continue;
				}

				float fraction = (aliveMs / e.lifeMs);
				updateElemForFraction(stage, e, fraction);

				++it;
			}

			// handle atlas updates.
			const auto fps = desc.sequence.fps;
			if (fps != 0)
			{
				const auto atlas = stage.pMaterial->getAtlas();
				const int32_t atlasCount = atlas.x * atlas.y;

				for (size_t j = 0; j < elems.size(); j++)
				{
					auto& e = elems[j];

					auto alive = (elapsed_ - e.spawnTime);
					auto aliveMs = alive.GetMilliSeconds();

					int32_t atlasIdx;
					if (fps < 0)
					{
						float fraction = (aliveMs / e.lifeMs);
						atlasIdx = static_cast<int32_t>(atlasCount * fraction); // total frames over elem life.
					}
					else
					{
						auto aliveSec = aliveMs * 0.001;
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
		}

		// process the stages: spawn more elems, remove finsihed, etc..
		for (auto it = activeStages_.begin(); it != activeStages_.end(); )
		{
			auto& stage = *it;
			auto& desc = *stage.pDesc;

			if (desc.loopCount && stage.currentLoop >= desc.loopCount) {
				// remove the stage?
				it = activeStages_.erase(it);
				continue;
			}

			++it;

			if (stage.currentLoop > 0)
			{
				auto sinceSpawn = elapsed_ - stage.lastSpawn;
				auto interval = core::TimeVal::fromMS(desc.interval);

				if (sinceSpawn < interval)
				{
					continue;
				}
			}

			// 'spawn'
			++stage.currentLoop;
			stage.lastSpawn = elapsed_;

			if (desc.type == StageType::OrientedSprite || desc.type == StageType::BillboardSprite)
			{
				int8_t colGraph = 0;
				int8_t alphaGraph = 0;
				int8_t sizeGraph = 0;
				int8_t velGraph = 0;

				if (desc.flags.IsSet(StageFlag::RandGraphCol)) {
					colGraph = gEnv->xorShift.rand() & 0x1;
				}
				if (desc.flags.IsSet(StageFlag::RandGraphAlpha)) {
					alphaGraph = gEnv->xorShift.rand() & 0x1;
				}
				if (desc.flags.IsSet(StageFlag::RandGraphSize)) {
					sizeGraph = gEnv->xorShift.rand() & 0x1;
				}
				if (desc.flags.IsSet(StageFlag::RandGraphVel)) {
					velGraph = gEnv->xorShift.rand() & 0x1;
				}

				// meow.
				Vec3f pos;
				pos.x = fromRange(desc.spawnOrgX);
				pos.y = fromRange(desc.spawnOrgY);
				pos.z = fromRange(desc.spawnOrgZ);

				float life = fromRange(desc.life);

				// so i need to support atlas.		
				auto atlas = stage.pMaterial->getAtlas();

				Elem e;
				e.uv = Rectf(0.f, 0.f, 1.f, 1.f);
				e.atlasIdx = 0;
				e.atlasBaseIdx = 0;

				if (atlas.x || atlas.y)
				{
					int32_t atlasIdx = desc.sequence.startFrame;
					int32_t atlasNum = atlas.x * atlas.y;
					if (atlasIdx < 0)
					{
						atlasIdx = static_cast<int32_t>(gEnv->xorShift.randIndex(atlasNum));
					}
					else
					{
						// user might of changed material so atlasIdx is no longer in range.
						if (atlasIdx >= atlasNum) {
							X_WARNING("Fx", "Atlast index was out of range");
							atlasIdx = atlasNum - 1;
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

				stage.elems.push_back(e);
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
		auto lookatCam(view.viewMatrix);
		lookatCam.rotate(Vec3f::xAxis(), ::toRadians(180.f));

		Quatf lookatCamQ(lookatCam);

		for (size_t i = 0; i < activeStages_.size(); i++)
		{
			auto& stage = activeStages_[i];

			if (stage.elems.isEmpty()) {
				continue;
			}

			gEngEnv.pMaterialMan_->waitForLoad(stage.pMaterial);

			for (const auto& e : stage.elems)
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

				if (stage.pDesc->type == StageType::BillboardSprite)
				{
					// i want the sprite to always face the camera.
					tl = tl * lookatCamQ;
					tr = tr * lookatCamQ;
					bl = bl * lookatCamQ;
					br = br * lookatCamQ;
				}
				else
				{
					tl = tl * trans_.quat;
					tr = tr * trans_.quat;
					bl = bl * trans_.quat;
					br = br * trans_.quat;
				}

				Vec3f pos = e.pos + e.dir + offset_;
				// do the rotation including  offset.
				// so the effects point of origin is at trans_.pos;
				pos = pos * trans_.quat; 
				pos += trans_.pos;

				tl += pos;
				tr += pos;
				bl += pos;
				br += pos;

				pPrim->drawQuad(tl, tr, bl, br, stage.pMaterial, e.col, e.uv);

				if (vars_.drawDebug() && vars_.drawElemRect())
				{
					pPrim->drawRect(tl, tr, bl, br, Col_Red, Col_Red, Col_Blue, Col_Blue);
				}
			}
		}

		if (vars_.drawDebug() && vars_.axisExtent() > 0.f)
		{
			pPrim->drawAxis(trans_, offset_, Vec3f(vars_.axisExtent()));
		}
	}

	void Emitter::updateElemForFraction(const Stage& stage, Elem& e, float fraction) const
	{
		auto& efx = *stage.pEfx;
		auto& desc = *stage.pDesc;

		Vec3f vel;
		vel.x = efx.fromGraph(desc.vel0X[e.velGraph], fraction);
		vel.y = efx.fromGraph(desc.vel0Y[e.velGraph], fraction);
		vel.z = efx.fromGraph(desc.vel0Z[e.velGraph], fraction);

		float size = efx.fromGraph(desc.size[e.sizeGraph], fraction);

		Vec3f col = efx.fromColorGraph(desc.color[e.colGraph], fraction);
		float alpha = efx.fromGraph(desc.alpha[e.alphaGraph], fraction);

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


} // namespace fx


X_NAMESPACE_END