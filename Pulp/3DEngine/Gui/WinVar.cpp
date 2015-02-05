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

// the file loades.

void XWinBool::fromFile(core::XFile* pFile)
{
	pFile->readString(name);
	pFile->readObj(value);
}

void XWinStr::fromFile(core::XFile* pFile)
{
	pFile->readString(name);
	pFile->readString(value);
}

void XWinInt::fromFile(core::XFile* pFile)
{
	pFile->readString(name);
	pFile->readObj(value);
}

void XWinFloat::fromFile(core::XFile* pFile)
{
	pFile->readString(name);
	pFile->readObj(value);
}

void XWinVec2::fromFile(core::XFile* pFile)
{
	pFile->readString(name);
	pFile->readObj(value);
}

void XWinVec3::fromFile(core::XFile* pFile)
{
	pFile->readString(name);
	pFile->readObj(value);
}

void XWinVec4::fromFile(core::XFile* pFile)
{
	pFile->readString(name);
	pFile->readObj(value);
}

void XWinRect::fromFile(core::XFile* pFile)
{
	pFile->readString(name);
	pFile->readObj(value);
}

void XWinColor::fromFile(core::XFile* pFile)
{
	pFile->readString(name);
	pFile->readObj(value);
}
 
// write to file
void XWinBool::toFile(core::XFile* pFile)
{
	pFile->writeString(name);
	pFile->writeObj(value);
}

void XWinStr::toFile(core::XFile* pFile)
{
	pFile->writeString(name);
	pFile->writeString(value);
}


X_NAMESPACE_END