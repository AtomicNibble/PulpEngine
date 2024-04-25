#include "stdafx.h"
#include "CoreEventDispatcher.h"

#include "Vars\CoreVars.h"

X_NAMESPACE_BEGIN(core)

XCoreEventDispatcher::XCoreEventDispatcher(CoreVars& coreVars, core::MemoryArenaBase* arena) :
    coreVars_(coreVars),
    listeners_(arena)
{
    core::zero_object(resizeEvent_);
    core::zero_object(moveEvent_);
}

XCoreEventDispatcher::~XCoreEventDispatcher()
{
    X_ASSERT(listeners_.isEmpty(), "Core listners are still registered")(listeners_.size());

    listeners_.clear();
}

void XCoreEventDispatcher::pumpEvents(void)
{
    auto dispatchEvent = [&](const CoreEventData& ed) {
        if (coreVars_.getCoreEventDebug()) {
            X_LOG0("CoreEvent", "CoreEvent: \"%s\"", CoreEvent::ToString(ed.event));
        }

        for (auto* pList : listeners_) {
            pList->OnCoreEvent(ed);
        }
    };

    // only send the last merge and resize event since last frame?
    if (resizeEvent_.event == CoreEvent::RESIZE) {
        dispatchEvent(resizeEvent_);
        core::zero_object(resizeEvent_);
    }
    if (moveEvent_.event == CoreEvent::MOVE) {
        dispatchEvent(moveEvent_);
        core::zero_object(moveEvent_);
    }

    while (events_.isNotEmpty()) {
        auto& ed = events_.peek();

        dispatchEvent(ed);

        events_.pop();
    }
}

bool XCoreEventDispatcher::RegisterListener(ICoreEventListener* pListener)
{
    X_ASSERT_NOT_NULL(pListener);

    auto it = std::find(listeners_.begin(), listeners_.end(), pListener);

    if (it != listeners_.end()) {
        X_ERROR("Core", "Event listener registered twice");
        return false;
    }

    listeners_.append(pListener);
    return true;
}

bool XCoreEventDispatcher::RemoveListener(ICoreEventListener* pListener)
{
    X_ASSERT_NOT_NULL(pListener);

    auto idx = listeners_.find(pListener);

    if (idx == ListnersArr::invalid_index) {
        X_ERROR("Core", "Failed to Unregistered Event listener, it is not currently registered");
        return false;
    }

    listeners_.removeIndex(idx);
    return true;
}

void XCoreEventDispatcher::QueueCoreEvent(CoreEventData data)
{
    X_ASSERT(data.event != CoreEvent::NONE, "Event not set")();

    if (coreVars_.getCoreEventDebug()) {
        X_LOG0("CoreEvent", "CoreEvent Queued: \"%s\"", CoreEvent::ToString(data.event));
    }

    if (data.event == CoreEvent::RESIZE)
    {
        resizeEvent_ = data;
        return;
    }
    if (data.event == CoreEvent::MOVE)
    {
        moveEvent_ = data;
        return;
    }

    if (!events_.freeSpace()) {
        X_ERROR("CoreEvent", "Event queue overflow!");
        events_.pop();
    }

    events_.push(data);
}

X_NAMESPACE_END
