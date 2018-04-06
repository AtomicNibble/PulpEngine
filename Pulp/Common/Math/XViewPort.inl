#pragma once

X_INLINE void XViewPort::setZ(float32_t near_, float32_t far_)
{
    z_.x = near_;
    z_.y = far_;
}

X_INLINE void XViewPort::set(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
    view_.set(left, top, right, bottom);
    viewf_.set(
        static_cast<float>(left),
        static_cast<float>(top),
        static_cast<float>(right),
        static_cast<float>(bottom));
}

X_INLINE void XViewPort::set(uint32_t width, uint32_t height)
{
    set(0, 0, width, height);
}

X_INLINE void XViewPort::set(const Vec2<uint32_t>& wh)
{
    set(wh.x, wh.y);
}

X_INLINE uint32_t XViewPort::getX(void) const
{
    return view_.getX1();
}

X_INLINE uint32_t XViewPort::getY(void) const
{
    return view_.getY1();
}

X_INLINE uint32_t XViewPort::getWidth(void) const
{
    return view_.getWidth();
}

X_INLINE uint32_t XViewPort::getHeight(void) const
{
    return view_.getHeight();
}

X_INLINE float XViewPort::getXf(void) const
{
    return viewf_.getX1();
}

X_INLINE float XViewPort::getYf(void) const
{
    return viewf_.getY1();
}

X_INLINE float XViewPort::getWidthf(void) const
{
    return viewf_.getWidth();
}

X_INLINE float XViewPort::getHeightf(void) const
{
    return viewf_.getHeight();
}

X_INLINE Recti XViewPort::getRect(void)
{
    return view_;
}

X_INLINE const Recti& XViewPort::getRect(void) const
{
    return view_;
}

X_INLINE float XViewPort::getZNear(void) const
{
    return z_.x;
}

X_INLINE float XViewPort::getZFar(void) const
{
    return z_.y;
}