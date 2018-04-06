#pragma once

#ifndef X_CORE_EVENT_DISPATCHER_H_
#define X_CORE_EVENT_DISPATCHER_H_

#include <ICore.h>
#include <Containers\Array.h>

X_NAMESPACE_BEGIN(core)

class CoreVars;

class XCoreEventDispatcher : public ICoreEventDispatcher
{
    typedef core::Array<ICoreEventListener*> ListnersArr;

public:
    XCoreEventDispatcher(CoreVars& coreVars, core::MemoryArenaBase* arena);
    ~XCoreEventDispatcher() X_OVERRIDE;

    virtual bool RegisterListener(ICoreEventListener* pListener) X_OVERRIDE;
    virtual bool RemoveListener(ICoreEventListener* pListener) X_OVERRIDE;

    virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;

private:
    CoreVars& coreVars_;
    ListnersArr listners_;
};

X_NAMESPACE_END

#endif // !X_CORE_EVENT_DISPATCHER_H_