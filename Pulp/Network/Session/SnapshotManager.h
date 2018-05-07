#pragma once

#include <SnapShot.h>

X_NAMESPACE_BEGIN(net)

// This handles outgoing snapshots for a given peer.
// Also handles rebuiling snapshots deltas from server.
class SnapshotManager
{
    static const int32_t MAX_PENDING_DELTAS = 16;

public:
    SnapshotManager(core::MemoryArenaBase* arena);
    
    void setStateSnap(SnapShot&& snap);
    
    bool hasPendingSnap(void) const;

private:
    bool hasSnap_;
    SnapShot snap_;

    SnapShot baseSnap_;   // the latest ack'd snap on the client, we delta off this.

    core::MemoryArenaBase* arena_;
};

X_NAMESPACE_END