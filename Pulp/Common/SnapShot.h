#pragma once

#include <Containers\Array.h>
#include <Containers\FixedBitStream.h>
#include <Util\Span.h>
#include <Time\TimeVal.h>

#include <INetwork.h>

X_NAMESPACE_DECLARE(core,
    class FixedBitStreamBase
)

X_NAMESPACE_BEGIN(net)

typedef int32_t ObjectID;

class SnapShot
{
    struct ObjectState
    {
        ObjectState(ObjectID id_) :
            id(id_)
        {}

        ObjectState(ObjectID id_, core::span<uint8_t> buffer_) :
            id(id_),
            buffer(buffer_)
        {}

        ObjectID id;
        core::span<uint8_t> buffer; // pointer to slice
    };

    typedef core::ArrayGrowMultiply<ObjectState> ObjectStateArr;
    typedef core::ArrayGrowMultiply<ObjectState*> ObjectStatePtrArr;

    typedef core::FixedBitStreamNoneOwning MsgBitStream;

public:
    using PlayerTimeMSArr = std::array<int32_t, MAX_PLAYERS>;

public:
    SnapShot(core::MemoryArenaBase* arena);
    SnapShot(const SnapShot& oth) = delete;
    SnapShot(SnapShot&& oth) = default;
    ~SnapShot();

    SnapShot& operator=(SnapShot&& oth) = default;


    void writeToBitStream(core::FixedBitStreamBase& bs) const;
    void fromBitStream(core::FixedBitStreamBase& bs);

    void addObject(ObjectID id, core::FixedBitStreamBase& bs);
    void setUserCmdTimes(const PlayerTimeMSArr& userCmdTimes);
    X_INLINE const PlayerTimeMSArr& getUserCmdTimes(void) const;

    X_INLINE size_t getNumObjects(void) const;
    X_INLINE core::TimeVal getTime(void) const;

    X_INLINE void setTime(core::TimeVal time);

    MsgBitStream getMessageByIndex(size_t idx) const;

private:
    ObjectState& findOrMakeStateForId(ObjectID id);


private:
    core::MemoryArenaBase* arena_;
    core::TimeVal time_; // the creation or recival time;

    ObjectStateArr objs_;
    PlayerTimeMSArr userCmdTimes_;
};


X_NAMESPACE_END

#include "SnapShot.inl"
