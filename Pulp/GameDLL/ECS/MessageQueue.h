#pragma once

#include "EntityId.h"

#include <Time/TimeVal.h>
#include <Util/Function.h>
#include <Containers/PriorityQueue.h>

X_NAMESPACE_BEGIN(game)

namespace ecs
{
    using EntityId = entity::EntityId;

    template<typename MessageType>
    class MessageQueue
    {
    public:

    public:
        static constexpr size_t NUM_MSG = typename MessageType::ENUM_COUNT;
        using MessageTypeEnum = typename MessageType::Enum;
        using TimeType = core::TimeVal;

        // want a list of all the handlers yo.
        struct Message
        {
            static constexpr size_t MSG_SIZE = 64;
            static constexpr size_t PAYLOAD_SIZE = MSG_SIZE - sizeof(MessageTypeEnum) - sizeof(TimeType);

            Message() = default;

            TimeType time;
            MessageTypeEnum type;
            char payload[PAYLOAD_SIZE];
        };

    private:
        static_assert(sizeof(Message) == Message::MSG_SIZE, "Incorrect size");

        using MessageFunc = core::Function<void(const Message&), 32>;

        // how do i make these sluts.
        using MessageSink = core::Array<MessageFunc>;
        using MessageSinkArr = core::FixedArray<MessageSink, NUM_MSG>;

        struct MessageCompTime
        {
            bool operator()(const Message& l, const Message& r)
            {
                return l.time > r.time;
            }
        };

        using MessagePriorityQueue = core::PriorityQueue<Message, core::ArrayGrowMultiply<Message>, MessageCompTime>;

    public:
        MessageQueue(core::MemoryArenaBase* arena) :
            time_(0ll),
            msgSinks_(MessageSink(arena)),
            queue_(arena)
        {
        }

        bool hasPendingEvents(void) const
        {
            return queue_.isNotEmpty();
        }

        size_t numPendingEvents(void) const
        {
            return queue_.size();
        }

        void update(TimeType dt)
        {
            if (dt == TimeType(0ll)) {
                return;
            }

            time_ += dt;

            while (queue_.isNotEmpty() && queue_.peek().time <= time_) {
                auto msg = queue_.peek();
                queue_.pop();
                dispatch(msg);
            }
        }

        void register_callback(MessageTypeEnum msg, MessageFunc&& handler)
        {
            msgSinks_[msg].emplace_back(handler);
        }

        void dispatch(const Message& m)
        {
            for (auto& h : msgSinks_[m.type]) {
                h(m);
            }
        }

        template<typename Payload, typename ...Args>
        void dispatch(Args&&... args)
        {
            Message m{ TimeType(0ll), Payload::MSG_ID };
            new(&m.payload) Payload{ std::forward<Args>(args)... };
            dispatch(m);
        }

        bool queue(const Message& m)
        {
            queue_.push(m);
            return true;
        }

        template < typename Payload, typename ...Args >
        bool queue(TimeType delay, Args&&... args)
        {
            Message m{ time_ + delay , Payload::MSG_ID};
            new(&m.payload) Payload{ std::forward<Args>(args)... };
            return queue(m);
        }

    protected:
        template<typename Payload, typename Message>
        static const Payload& message_cast(const Message& m)
        {
            X_ASSERT(Payload::MSG_ID == m.type, "Msg type missmatch")(m.type);
            static_assert(sizeof(Payload) < sizeof(Message::payload), "Payload size over limit!");
            return *reinterpret_cast<const Payload*>(&(m.payload));
        }

    protected:
        TimeType time_;
        MessageSinkArr msgSinks_;
        MessagePriorityQueue queue_;
    };


} // namespace ecs

X_NAMESPACE_END