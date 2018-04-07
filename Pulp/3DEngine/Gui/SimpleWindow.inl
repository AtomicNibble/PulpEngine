

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    const char* XWindowSimple::getName(void) const
    {
        return name_.c_str();
    }

    XWindow* XWindowSimple::getParent(void)
    {
        return pParent_;
    }

} // namespace gui

X_NAMESPACE_END
