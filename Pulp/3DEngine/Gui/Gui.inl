

X_INLINE const char* XGui::getName(void) const
{
    return name_.c_str();
}

X_INLINE void XGui::setName(const char* name)
{
    name_ = name;
}

X_INLINE void XGui::setCursorPos(float x, float y)
{
    cursorPos_.x = x;
    cursorPos_.y = y;
}

X_INLINE void XGui::setCursorPos(const Vec2f& pos)
{
    cursorPos_ = pos;
}

X_INLINE Vec2f XGui::getCursorPos(void)
{
    return cursorPos_;
}

X_INLINE float XGui::getCursorPosX(void)
{
    return cursorPos_.x;
}

X_INLINE float XGui::getCursorPosY(void)
{
    return cursorPos_.y;
}

// ------------------------

X_INLINE bool XGui::isDeskTopValid(void) const
{
    return pDesktop_ != nullptr;
}