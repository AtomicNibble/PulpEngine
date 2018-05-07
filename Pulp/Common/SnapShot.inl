
X_NAMESPACE_BEGIN(net)


X_INLINE size_t SnapShot::getNumObjects(void) const
{
    return objs_.size();
}

X_INLINE core::TimeVal SnapShot::getTime(void) const
{
    return time_;
}

X_INLINE void SnapShot::setTime(core::TimeVal time)
{
    time_ = time;
}

X_NAMESPACE_END
