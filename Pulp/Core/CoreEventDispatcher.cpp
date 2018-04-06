#include "stdafx.h"
#include "CoreEventDispatcher.h"

#include "Vars\CoreVars.h"

X_NAMESPACE_BEGIN(core)

XCoreEventDispatcher::XCoreEventDispatcher(CoreVars& coreVars, core::MemoryArenaBase* arena) :
    coreVars_(coreVars),
    listners_(arena)
{
}

XCoreEventDispatcher::~XCoreEventDispatcher()
{
    // the core listner is static and left.
    X_ASSERT(listners_.size() <= 1, "Core listners are still registerd")
    (listners_.size());

    listners_.clear();
}

bool XCoreEventDispatcher::RegisterListener(ICoreEventListener* pListener)
{
    X_ASSERT_NOT_NULL(pListener);

    auto it = std::find(listners_.begin(), listners_.end(), pListener);

    if (it != listners_.end()) {
        X_ERROR("Core", "Event listner registered twice");
        return false;
    }

    listners_.append(pListener);
    return true;
}

bool XCoreEventDispatcher::RemoveListener(ICoreEventListener* pListener)
{
    X_ASSERT_NOT_NULL(pListener);

    auto idx = listners_.find(pListener);

    if (idx == ListnersArr::invalid_index) {
        X_ERROR("Core", "Failed to Unregistered Event listner, it is not currently registered");
        return false;
    }

    listners_.removeIndex(idx);
    return true;
}

void XCoreEventDispatcher::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
    if (coreVars_.coreEventDebug_) {
        X_LOG0("CoreEvent", "---- CoreEvent: %s ----", CoreEvent::ToString(event));
    }

    for (auto* pList : listners_) {
        pList->OnCoreEvent(event, wparam, lparam);
    }
}

X_NAMESPACE_END