#pragma once

#ifndef X_CORE_EVENT_DISPATCHER_H_
#define X_CORE_EVENT_DISPATCHER_H_

#include <ICore.h>
X_DISABLE_WARNING(4702)
#include <set>
X_ENABLE_WARNING(4702)

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(core)

class XCoreEventDispatcher : public ICoreEventDispatcher
{
	typedef core::Array<ICoreEventListener*> ListnersArr;

public:
	XCoreEventDispatcher(core::MemoryArenaBase* arena);
	~XCoreEventDispatcher() X_OVERRIDE;

	virtual bool RegisterListener(ICoreEventListener* pListener) X_OVERRIDE;
	virtual bool RemoveListener(ICoreEventListener* pListener) X_OVERRIDE;

	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;

private:

	ListnersArr listners_;
};


X_NAMESPACE_END


#endif // !X_CORE_EVENT_DISPATCHER_H_