#pragma once

#include <Time\TimeVal.h>
#include <IEffect.h> // only for IEmitter

X_NAMESPACE_DECLARE(core,
	struct FrameView
)

X_NAMESPACE_BEGIN(engine)

class EffectVars;

class IPrimativeContext;

namespace fx
{
	struct Stage;
	struct Range;
	struct Graph;

	// a emmtier plays a FX.
	class Emitter : public IEmitter
	{
		struct Elem
		{
			int8_t colGraph;
			int8_t alphaGraph;
			int8_t sizeGraph;
			int8_t velGraph;

			Vec3f pos;
			Vec3f dir;
			float size;
			Color8u col;

			Rectf uv;
			int32_t atlasBaseIdx;
			int32_t atlasIdx;
			// int32_t loop;

			float lifeMs;
			core::TimeVal spawnTime;
		};

		// ideally keep this below 64.
		// X_ENSURE_LE(sizeof(Elem), 64, "Keep elem below single cache lane");

		template <typename T>
		using ArrayType = core::Array<T, core::ArrayAllocator<T>, core::growStrat::Multiply>;

		typedef ArrayType<Elem> ElemArr;

		struct Stage
		{
			Stage(const Effect* pEfx, const StageDsc* pDesc, engine::Material* pMaterial, const Transformf& spawnTrans, int32_t maxElems, core::MemoryArenaBase* arena) :
				pEfx(X_ASSERT_NOT_NULL(pEfx)),
				pDesc(X_ASSERT_NOT_NULL(pDesc)),
				pMaterial(X_ASSERT_NOT_NULL(pMaterial)),
				currentLoop(0),
				spawnTrans(spawnTrans),
				elems(arena, maxElems)
			{

			}

			Stage(Stage&& oth) = default;
			Stage& operator=(Stage&& oth) = default;

			const Effect* pEfx;
			const StageDsc* pDesc;
			engine::Material* pMaterial;

			core::TimeVal lastSpawn; 
			int32_t currentLoop;

			Transformf spawnTrans;

			ElemArr elems;
		};

		typedef core::Array<Stage> StageArr;


	public:
		Emitter(const EffectVars& effectVars, core::MemoryArenaBase* arena);

		void play(const Effect* pEfx, bool clear = false) X_FINAL;
		bool isPlaying(void) const;

		void setTrans(const Transformf& trans) X_FINAL;
		void setTrans(const Transformf& trans, const Vec3f& offset) X_FINAL;

		void update(core::TimeVal delta);
		void draw(core::FrameView& view, IPrimativeContext* pPrim);

	private:
		void updateElemForFraction(const Stage& stage, Elem& e, float fraction) const;

		static void uvForIndex(Rectf& uv, const Vec2<int16_t> atlas, int32_t idx);

		float fromRange(const Range& r) const;

	private:
		const EffectVars& vars_;
		core::MemoryArenaBase* arena_;

		const Effect* pEfx_;
		core::TimeVal elapsed_; 
		int32_t curStage_;

		StageArr activeStages_;
		Transformf trans_;
		Vec3f offset_;
	};


} // namespace fx

X_NAMESPACE_END