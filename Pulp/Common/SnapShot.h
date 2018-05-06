#pragma once

#include <Containers\Array.h>
#include <Util\Span.h>

X_NAMESPACE_DECLARE(core,
    class FixedBitStreamBase)

    X_NAMESPACE_BEGIN(net)

    typedef int32_t ObjectID;


class SnapShot
{
    struct ObjectState
    {
        ObjectID id;
        core::span<uint8_t> buffer; // pointer to slice
    };

    typedef core::Array<ObjectState*> ObjectStatePtrArr;

public:
    SnapShot(core::MemoryArenaBase* arena);

    void writeToBitStream(core::FixedBitStreamBase& bs) const;
    void fromBitStream(core::FixedBitStreamBase& bs);

    void addObject(ObjectID id, core::FixedBitStreamBase& bs);

    X_INLINE size_t getNumObjects(void) const;

    bool getMessageByIndex(size_t idx, core::FixedBitStreamBase& bs) const;

private:
    ObjectState& findOrMakeStateForId(ObjectID id);


private:
    core::MemoryArenaBase* arena_;
    ObjectStatePtrArr objs_;
};


X_NAMESPACE_END

#include "SnapShot.inl"
