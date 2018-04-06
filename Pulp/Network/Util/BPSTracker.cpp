#include "stdafx.h"
#include "BPSTracker.h"

X_NAMESPACE_BEGIN(net)

BPSTracker::BPSTracker(core::MemoryArenaBase* arena) :
    total_(0),
    lastSec_(0),
    values_(arena)
{
}

void BPSTracker::update(core::TimeVal time)
{
    core::TimeVal thresh = time - core::TimeVal::fromMS(1000);

    while (values_.isNotEmpty() && values_.peek().first < thresh) {
        lastSec_ -= values_.peek().second;
        values_.pop();
    }
}

void BPSTracker::reset(void)
{
    total_ = 0;
    lastSec_ = 0;

    values_.clear();
}

void BPSTracker::free(void)
{
    total_ = 0;
    lastSec_ = 0;

    values_.free();
}

X_NAMESPACE_END