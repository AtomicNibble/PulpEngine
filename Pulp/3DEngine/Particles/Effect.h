#pragma once

#include <Time\TimeVal.h>

X_NAMESPACE_BEGIN(engine)

namespace fx
{
	struct StageDsc;
	struct Range;
	struct Graph;

	class Effect : public core::AssetBase
	{
		X_NO_COPY(Effect);
		X_NO_ASSIGN(Effect);

	public:
		Effect(core::string& name, core::MemoryArenaBase* arena);
		~Effect();

		bool processData(core::UniquePointer<char[]> data, uint32_t dataSize);


		X_INLINE int32_t getNumStages(void) const;

	public:
		const StageDsc& getStageDsc(int32_t idx) const;
		const StageDsc* getStageDscs(void) const;
		const uint8_t* getIndexes(void) const;
		const float* getFloats(void) const;
		const char* getMaterialName(int32_t strOffset) const;

		float fromGraph(const Graph& r, float t) const;
		Vec3f fromColorGraph(const Graph& r, float t) const;
		Vec3f colorForIdx(int32_t start, int32_t idx) const;

		X_INLINE float getFloat(int32_t idx) const;
		X_INLINE float floatForIdx(int32_t idx) const;

	private:
		int32_t numStages_;
		int32_t numIndex_;
		int32_t numFloats_;

		uint32_t dataSize_; // don't really need?
		core::UniquePointer<char[]> data_;
	};


} // namespace fx

X_NAMESPACE_END

#include "Effect.inl"