
X_NAMESPACE_BEGIN(render)

X_INLINE int XRender::getWidth(void) const
{
    return ViewPort_.getWidth();
}

X_INLINE int XRender::getHeight(void) const
{
    return ViewPort_.getHeight();
}

X_INLINE float XRender::getWidthf(void) const
{
    return ViewPort_.getWidthf();
}

X_INLINE float XRender::getHeightf(void) const
{
    return ViewPort_.getHeightf();
}

X_INLINE float XRender::ScaleCoordX(float value) const
{
    return ScaleCoordXInternal(value);
}

X_INLINE float XRender::ScaleCoordY(float value) const
{
    return ScaleCoordYInternal(value);
}

X_INLINE void XRender::ScaleCoord(float& x, float& y) const
{
    ScaleCoordInternal(x, y);
}

X_INLINE void XRender::ScaleCoord(Vec2f& xy) const
{
    ScaleCoordInternal(xy);
}

X_INLINE float XRender::ScaleCoordXInternal(float value) const
{
    value *= ViewPort_.getWidthf() / 800.0f;
    return (value);
}

X_INLINE float XRender::ScaleCoordYInternal(float value) const
{
    value *= ViewPort_.getHeightf() / 600.0f;
    return (value);
}

X_INLINE void XRender::ScaleCoordInternal(float& x, float& y) const
{
    x = ScaleCoordXInternal(x);
    y = ScaleCoordYInternal(y);
}

X_INLINE void XRender::ScaleCoordInternal(Vec2f& xy) const
{
    xy.x = ScaleCoordXInternal(xy.x);
    xy.y = ScaleCoordYInternal(xy.y);
}

X_INLINE const XCamera& XRender::GetCamera(void)
{
    return cam_;
}

X_INLINE VidMemManager* XRender::VidMemMng(void)
{
    return &vidMemMng_;
}

X_INLINE Matrix44f* XRender::pViewMatrix(void)
{
    return &ViewMatrix_;
}

X_INLINE Matrix44f* XRender::pProjMatrix(void)
{
    return &ProjMatrix_;
}

X_INLINE Matrix44f* XRender::pViewProjMatrix(void)
{
    return &ViewProjMatrix_;
}

X_INLINE Matrix44f* XRender::pViewProjInvMatrix(void)
{
    return &ViewProjInvMatrix_;
}

X_NAMESPACE_END
