#include "stdafx.h"
#include "Effect.h"

#include <IEffect.h>

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

} // namespace fx


X_NAMESPACE_END