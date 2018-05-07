#include "stdafx.h"
#include "SnapshotManager.h"

X_NAMESPACE_BEGIN(net)

SnapshotManager::SnapshotManager(core::MemoryArenaBase* arena) :
    arena_(arena),
    snap_(arena),
    baseSnap_(arena)
{
    hasSnap_ = false;
}


void SnapshotManager::setStateSnap(SnapShot&& snap)
{
    snap_ = std::move(snap);
    hasSnap_ = true;
}

bool SnapshotManager::hasPendingSnap(void) const
{
    return hasSnap_;
}


X_NAMESPACE_END