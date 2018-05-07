#include "EngineCommon.h"
#include "SnapShot.h"


X_NAMESPACE_BEGIN(net)


SnapShot::SnapShot(core::MemoryArenaBase* arena) :
    arena_(arena),
    objs_(arena)
{

}

SnapShot::~SnapShot()
{
    for (auto& obj : objs_)
    {
        X_DELETE_ARRAY(obj.buffer.data(), arena_);
    }
}


void SnapShot::writeToBitStream(core::FixedBitStreamBase& bs) const
{
    auto num = objs_.size();
    bs.write(safe_static_cast<uint16_t>(num));

    for (auto& obj : objs_)
    {
        auto size = obj.buffer.size_bytes();

        bs.write(obj.id);
        bs.write(safe_static_cast<uint16_t>(size));
        bs.write(obj.buffer.data(), obj.buffer.size_bytes());
    }
}

void SnapShot::fromBitStream(core::FixedBitStreamBase& bs)
{
    objs_.clear();

    auto num = bs.read<uint16_t>();
    objs_.reserve(num);

    for (size_t i=0; i<num; i++)
    {
        auto id = bs.read<ObjectID>();
        auto sizeInBytes = bs.read<uint16_t>();

        auto* pData = X_NEW_ARRAY(uint8_t, sizeInBytes, arena_, "SnapObjectData");

        bs.read(pData, sizeInBytes);

        objs_.emplace_back(id, core::make_span(pData, bs.sizeInBytes()));
    }
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



SnapShot::MsgBitStream SnapShot::getMessageByIndex(size_t idx) const
{
    X_ASSERT(idx < objs_.size(), "Index out of range")(objs_.size(), idx);

    auto& state = objs_[idx];


    // can't see a nice way to remove this cast.
    // since bit stream supports writing.
    // if the user did actually write it would not break anything, but kinda breaks the 'const'
    auto* pBegin = const_cast<uint8_t*>(state.buffer.data());

    return MsgBitStream(pBegin, pBegin + state.buffer.size(), true);
}

SnapShot::ObjectState& SnapShot::findOrMakeStateForId(ObjectID id)
{
    for (size_t i = 0; i < objs_.size(); i++)
    {
        if (objs_[i].id == id) {
            return objs_[i];
        }
    }

    objs_.emplace_back(id);
    return objs_.back();
}



X_NAMESPACE_END
