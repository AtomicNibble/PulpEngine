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

    typedef core::FixedFifo<CoreEventData, EVENT_QUEUE_SIZE> CoreEventDataQueue;
    typedef core::Array<ICoreEventListener*> ListnersArr;

public:
    XCoreEventDispatcher(CoreVars& coreVars, core::MemoryArenaBase* arena);
    ~XCoreEventDispatcher() X_FINAL;

    void pumpEvents(void);

    bool RegisterListener(ICoreEventListener* pListener) X_FINAL;
    bool RemoveListener(ICoreEventListener* pListener) X_FINAL;

    void QueueCoreEvent(CoreEventData data) X_FINAL;

private:
    CoreVars& coreVars_;
    CoreEventDataQueue events_;
    ListnersArr listners_;

    CoreEventData resizeEvent_;
    CoreEventData moveEvent_;
};

X_NAMESPACE_END

#endif // !X_CORE_EVENT_DISPATCHER_H_