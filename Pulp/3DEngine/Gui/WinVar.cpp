#include "stdafx.h"
#include "WinVar.h"

X_NAMESPACE_BEGIN(gui)

XWinVar::XWinVar()
{
	eval = true;
}

XWinVar::~XWinVar()
{

}

void XWinVar::Init(const char* _name, XWindow* win)
{
	Set(_name);
}


X_NAMESPACE_END