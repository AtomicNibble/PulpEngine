#include "stdafx.h"
#include "SimpleWindow.h"


X_NAMESPACE_BEGIN(engine)

XWindowSimple::XWindowSimple() :
	pParent_(nullptr)
{
	textScale_ = 1.0f;
	visible_ = true;
}

XWindowSimple::~XWindowSimple()
{

}


X_NAMESPACE_END