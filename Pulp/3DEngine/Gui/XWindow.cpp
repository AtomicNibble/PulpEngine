#include "stdafx.h"
#include "XWindow.h"

X_NAMESPACE_BEGIN(engine)


XWindow::XWindow() :
	children_(g_3dEngineArena)
{

}

XWindow::~XWindow()
{

}

// ----------------------------------------

// Drawing
void XWindow::reDraw(void)
{

}

void XWindow::drawDebug(void)
{

}

void XWindow::drawCaption(void)
{

}


// -------------- Overrides ---------------


void XWindow::draw(int time, float x, float y)
{

}

void XWindow::activate(bool activate)
{

}

void XWindow::gainFocus(void)
{

}

void XWindow::loseFocus(void)
{

}

void XWindow::gainCapture(void)
{

}

void XWindow::loseCapture(void)
{

}

void XWindow::sized(void)
{

}

void XWindow::moved(void)
{

}

void XWindow::mouseExit(void)
{

}

void XWindow::mouseEnter(void)
{

}



X_NAMESPACE_END