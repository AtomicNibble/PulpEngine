

const char* XWindowSimple::getName(void) const
{
    return name_.c_str();
}

XWindow* XWindowSimple::getParent(void)
{
    return pParent_;
}