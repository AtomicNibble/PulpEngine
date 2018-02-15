#pragma once

#include <Time\TimeVal.h>

X_NAMESPACE_BEGIN(engine)

class IPrimativeContext;

namespace fx
{
	struct Stage;
	struct Range;
	struct Graph;

	class Effect : public core::AssetBase
	{
		X_NO_COPY(Effect);
		X_NO_ASSIGN(Effect);

	public:
		Effect(core::string& name, core::MemoryArenaBase* arena);
		~Effect();

		X_INLINE int32_t getNumStages(void) const;

		bool processData(core::UniquePointer<char[]> data, uint32_t dataSize);

	public:
		const Stage* getStages(void) const;
		const uint8_t* getIndexes(void) const;
		const float* getFloats(void) const;
		const char* getMaterialName(int32_t strOffset) const;

	private:
		int32_t numStages_;
		int32_t numIndex_;
		int32_t numFloats_;

		uint32_t dataSize_; // don't really need?
		core::UniquePointer<char[]> data_;
	};


	int32_t Effect::getNumStages(void) const
	{
		return numStages_;
	}

	// a emmtier plays a FX.
	class Emitter
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

			core::TimeVal spawnTime;
			float lifeMs;
		};

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

		void setPos(const Vec3f& pos);

		void update(core::TimeVal delta);
		void draw(IPrimativeContext* pPrim);

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
		Vec3f pos_;
		core::TimeVal elapsed_;
	};


} // namespace fx

X_NAMESPACE_END