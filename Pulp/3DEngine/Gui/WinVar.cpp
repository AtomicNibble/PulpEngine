#include "stdafx.h"
#include "WinVar.h"

#include <IFileSys.h>

X_NAMESPACE_BEGIN(engine)

namespace gui
{
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
        X_UNUSED(win);
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

    void XWinInt::toFile(core::XFile* pFile)
    {
        pFile->writeString(name);
        pFile->writeObj(value);
    }

    void XWinFloat::toFile(core::XFile* pFile)
    {
        pFile->writeString(name);
        pFile->writeObj(value);
    }

    void XWinVec2::toFile(core::XFile* pFile)
    {
        pFile->writeString(name);
        pFile->writeObj(value);
    }

    void XWinVec3::toFile(core::XFile* pFile)
    {
        pFile->writeString(name);
        pFile->writeObj(value);
    }

    void XWinVec4::toFile(core::XFile* pFile)
    {
        pFile->writeString(name);
        pFile->writeObj(value);
    }

    void XWinRect::toFile(core::XFile* pFile)
    {
        pFile->writeString(name);
        pFile->writeObj(value);
    }

    void XWinColor::toFile(core::XFile* pFile)
    {
        pFile->writeString(name);
        pFile->writeObj(value);
    }

} // namespace gui

X_NAMESPACE_END