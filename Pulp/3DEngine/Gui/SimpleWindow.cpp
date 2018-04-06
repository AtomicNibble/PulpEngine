#include "stdafx.h"
#include "SimpleWindow.h"

X_NAMESPACE_BEGIN(engine)

namespace gui
{
    XWindowSimple::XWindowSimple() :
        pParent_(nullptr)
    {
        textScale_ = 1.0f;
        visible_ = true;
    }

    XWindowSimple::~XWindowSimple()
    {
    }

} // namespace gui

X_NAMESPACE_END