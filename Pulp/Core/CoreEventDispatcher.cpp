#include "stdafx.h"
#include "CoreEventDispatcher.h"
#include "Core.h"

X_NAMESPACE_BEGIN(core)


XCoreEventDispatcher::XCoreEventDispatcher()
{

}

XCoreEventDispatcher::~XCoreEventDispatcher()
{
	// the core listner is static and left.
	X_ASSERT(Listners_.size() <= 1, "Core listners are still registerd")(Listners_.size());

	Listners_.clear();
}

bool XCoreEventDispatcher::RegisterListener(ICoreEventListener* pListener)
{
	X_ASSERT_NOT_NULL(pListener);

	if (Listners_.find(pListener) != Listners_.end()) {
		X_ERROR("Core", "Event listner registered twice");
		return false;
	}

	Listners_.insert(pListener);
	return true;
}

bool XCoreEventDispatcher::RemoveListener(ICoreEventListener* pListener)
{
	X_ASSERT_NOT_NULL(pListener);

	Listners::iterator it = Listners_.find(pListener);

	if (it == Listners_.end()) {
		X_ERROR("Core", "Failed to Unregistered Event listner, it is not currently registered");
		return false;
	}

	Listners_.erase(it);
	return true;
}

void XCoreEventDispatcher::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
	if (g_coreVars.core_event_debug) {
		X_LOG0("CoreEvent", "---- CoreEvent: %s ----", CoreEvent::ToString(event));
	}

	Listners::iterator it = Listners_.begin();
	for (; it != Listners_.end(); ++it)
	{
		(*it)->OnCoreEvent(event, wparam, lparam);
	}
}



X_NAMESPACE_END