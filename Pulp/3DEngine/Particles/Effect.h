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
			Vec3f pos;
			Vec3f dir;
			float size;
			Color8u col;
			int32_t lifeMs;
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

		void update(core::TimeVal delta);
		void draw(IPrimativeContext* pPrim);

	private:
		float fromRange(const Range& r) const;
		float fromGraph(const Graph& r, float t) const;

	private:
		const Effect& efx_;
		StageStateArr stages_;

		core::TimeVal elapsed_;
	};


} // namespace fx

X_NAMESPACE_END