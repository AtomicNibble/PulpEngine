#include "EngineCommon.h"
#include "SnapShot.h"

#include "Containers\FixedBitStream.h"

X_NAMESPACE_BEGIN(net)


SnapShot::SnapShot(core::MemoryArenaBase* arena) :
    arena_(arena),
    objs_(arena)
{

}


void SnapShot::writeToBitStream(core::FixedBitStreamBase& bs) const
{
    X_UNUSED(bs);
}

void SnapShot::fromBitStream(core::FixedBitStreamBase& bs)
{
    X_UNUSED(bs);
}

void SnapShot::addObject(ObjectID id, core::FixedBitStreamBase& bs)
{
    ObjectState& state = findOrMakeStateForId(id);

    if (safe_static_cast<size_t>(state.buffer.size()) != bs.sizeInBytes())
    {
        // leaning towards making this allocated from a linera arena.

        auto* pData = X_NEW_ARRAY(uint8_t, bs.sizeInBytes(), arena_, "SnapObjectData");

        state.buffer = core::make_span(pData, bs.sizeInBytes());
    }

    std::memcpy(state.buffer.data(), bs.data(), bs.sizeInBytes());
}



bool SnapShot::getMessageByIndex(size_t idx, core::FixedBitStreamBase& bs) const
{
    X_ASSERT(idx < objs_.size(), "Index out of range")(objs_.size(), idx);

    auto& state = *objs_[idx];


    // can't see a nice way to remove this cast.
    // since bit stream supports writing.
    // if the user did actually write it would not break anything, but kinda breaks the 'const'
    auto* pBegin = const_cast<uint8_t*>(state.buffer.data());

    bs = core::FixedBitStreamNoneOwning(pBegin, pBegin + state.buffer.size(), true);
    return true;
}

SnapShot::ObjectState& SnapShot::findOrMakeStateForId(ObjectID id)
{
    for (size_t i = 0; i < objs_.size(); i++)
    {
        if (objs_[i]->id == id) {
            return *objs_[i];
        }
    }

    objs_.push_back(X_NEW(ObjectState, arena_, "ObjectStat"));
    return *objs_.back();
}



X_NAMESPACE_END
