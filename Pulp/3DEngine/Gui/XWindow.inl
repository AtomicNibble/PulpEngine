

// Parent
void XWindow::setParent(XWindow* pParent)
{
    pParent_ = pParent;
}

XWindow* XWindow::getParent(void)
{
    return pParent_;
}

// Flags
void XWindow::setFlag(WindowFlag::Enum flag)
{
    flags_.Set(flag);
}
void XWindow::clearFlags(void)
{
    flags_.Clear();
}

XWindow::WindowFlags XWindow::getFlags(void) const
{
    return flags_;
}

// Children
void XWindow::addChild(XWindow* pChild)
{
    pChild->childId_ = safe_static_cast<uint32_t, size_t>(children_.append(pChild));
}

void XWindow::removeChild(XWindow* pChild)
{
    core::Array<XWindow*>::size_type idx = children_.find(pChild);

    if (idx != core::Array<XWindow*>::invalid_index) {
        children_.removeIndex(idx);
    }
}

XWindow* XWindow::getChild(int idx)
{
    return children_[idx];
}

size_t XWindow::getNumChildren(void) const
{
    return children_.size();
}

uint32_t XWindow::getIndexForChild(XWindow* pWindow) const
{
    return safe_static_cast<uint32_t, size_t>(
        children_.find(pWindow));
}

// Name
const char* XWindow::getName(void) const
{
    return name_.c_str();
}