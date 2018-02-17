#pragma once

#include <Time\TimeVal.h>

X_NAMESPACE_BEGIN(engine)

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


} // namespace fx

X_NAMESPACE_END