#pragma once

#ifndef X_CORE_EVENT_DISPATCHER_H_
#define X_CORE_EVENT_DISPATCHER_H_

#include <ICore.h>
#include <set>

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(core)

class XCoreEventDispatcher : public ICoreEventDispatcher
{
public:
	XCoreEventDispatcher();
	~XCoreEventDispatcher() X_OVERRIDE;

	virtual bool RegisterListener(ICoreEventListener* pListener) X_OVERRIDE;
	virtual bool RemoveListener(ICoreEventListener* pListener) X_OVERRIDE;

	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;

private:
	typedef std::set<ICoreEventListener*> Listners;

	Listners Listners_;
};


X_NAMESPACE_END


#endif // !X_CORE_EVENT_DISPATCHER_H_