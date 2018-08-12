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
    X_ASSERT(listners_.isEmpty(), "Core listners are still registerd")(listners_.size());

    listners_.clear();
}

void XCoreEventDispatcher::pumpEvents(void)
{
    if (events_.isEmpty()) {
        return;
    }

    while (events_.isNotEmpty()) {
        auto& ed = events_.peek();

        if (coreVars_.coreEventDebug_) {
            X_LOG0("CoreEvent", "CoreEvent: \"%s\"", CoreEvent::ToString(ed.event));
        }

        for (auto* pList : listners_) {
            pList->OnCoreEvent(ed.event, ed.wparam, ed.lparam);
        }

        events_.pop();
    }
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

void XCoreEventDispatcher::QueueCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
    if (coreVars_.coreEventDebug_) {
        X_LOG0("CoreEvent", "CoreEvent Queued: \"%s\"", CoreEvent::ToString(event));
    }

    CoreEventData ed;
    ed.event = event;
    ed.wparam = wparam;
    ed.lparam = lparam;

    if (!events_.freeSpace()) {
        X_ERROR("CoreEvent", "Event queue overflow!");
        events_.pop();
    }

    events_.push(ed);
}

X_NAMESPACE_END