#pragma once

#ifndef X_CORE_EVENT_DISPATCHER_H_
#define X_CORE_EVENT_DISPATCHER_H_

#include <ICore.h>
#include <Containers\Array.h>
#include <Containers\FixedFifo.h>

X_NAMESPACE_BEGIN(core)

class CoreVars;

class XCoreEventDispatcher : public ICoreEventDispatcher
{
    static const size_t EVENT_QUEUE_SIZE = 256;

    struct CoreEventData
    {
        CoreEvent::Enum event;
        uintptr_t wparam;
        uintptr_t lparam;
    };

    typedef core::FixedFifo<CoreEventData, EVENT_QUEUE_SIZE> CoreEventDataQueue;
    typedef core::Array<ICoreEventListener*> ListnersArr;

public:
    XCoreEventDispatcher(CoreVars& coreVars, core::MemoryArenaBase* arena);
    ~XCoreEventDispatcher() X_FINAL;

    void pumpEvents(void);

    bool RegisterListener(ICoreEventListener* pListener) X_FINAL;
    bool RemoveListener(ICoreEventListener* pListener) X_FINAL;

    void QueueCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_FINAL;

private:
    CoreVars& coreVars_;
    CoreEventDataQueue events_;
    ListnersArr listners_;
};

X_NAMESPACE_END

#endif // !X_CORE_EVENT_DISPATCHER_H_