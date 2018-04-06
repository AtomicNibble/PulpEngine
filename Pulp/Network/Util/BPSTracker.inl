
X_NAMESPACE_BEGIN(net)

X_INLINE void BPSTracker::add(core::TimeVal time, uint64_t val)
{
    total_ += val;
    lastSec_ += val;
    values_.push(std::make_pair(time, val));
}

X_INLINE uint64_t BPSTracker::getTotal(void) const
{
    return total_;
}

X_INLINE uint64_t BPSTracker::getBPS(void) const
{
    return lastSec_;
}

X_INLINE typename BPSTracker::size_type BPSTracker::size(void) const
{
    return values_.size();
}

X_INLINE typename BPSTracker::size_type BPSTracker::capacity(void) const
{
    return values_.capacity();
}

X_NAMESPACE_END