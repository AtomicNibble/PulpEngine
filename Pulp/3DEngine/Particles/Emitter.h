#pragma once

#include <Time\TimeVal.h>
#include <IEffect.h> // only for IEmitter

X_NAMESPACE_DECLARE(core,
	struct FrameView
)

X_NAMESPACE_BEGIN(engine)

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

		typedef core::Array<Elem> ElemArr;

		struct StageState
		{
			StageState(core::MemoryArenaBase* arena) :
				elems(arena)
			{
				pMaterial = nullptr;
				currentLoop = 0;
			}


			engine::Material* pMaterial;

			core::TimeVal lastSpawn;
			int32_t currentLoop;

			ElemArr elems;
		};

		typedef core::Array<StageState> StageStateArr;


	public:
		Emitter(const Effect& efx, core::MemoryArenaBase* arena);

		void setTrans(const Transformf& trans) X_FINAL;
		void setTrans(const Transformf& trans, const Vec3f& offset) X_FINAL;

		void update(core::TimeVal delta);
		void draw(core::FrameView& view, IPrimativeContext* pPrim);

	private:
		void updateElemForFraction(const Stage& stage, Elem& e, float fraction) const;

		static void uvForIndex(Rectf& uv, const Vec2<int16_t> atlas, int32_t idx);


		float fromRange(const Range& r) const;
		float fromGraph(const Graph& r, float t) const;
		Vec3f fromColorGraph(const Graph& r, float t) const;

		float getFloat(int32_t idx) const;
		float floatForIdx(int32_t idx) const;
		Vec3f colorForIdx(int32_t start, int32_t idx) const;

	private:
		const Effect& efx_;
		StageStateArr stages_;
		Transformf trans_;
		Vec3f offset_;
		core::TimeVal elapsed_;
	};


} // namespace fx

X_NAMESPACE_END