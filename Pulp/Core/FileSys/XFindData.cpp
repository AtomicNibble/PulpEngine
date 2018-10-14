#include "stdafx.h"
#include "XFindData.h"

#include <io.h>
#include "xFileSys.h"

X_NAMESPACE_BEGIN(core)

XFindData::XFindData(const wchar_t* path, xFileSys* pFileSys) :
    path_(path),
    handle_(-1),
    current_(pFileSys->searchPaths_),
    pFileSys_(pFileSys)
{
    folder_ = path_;
    folder_.removeFileName();
    folder_.replaceSeprators();
    folder_.ensureSlash();
}

XFindData::XFindData(const char* path, xFileSys* pFileSys) :
    handle_(-1),
    current_(pFileSys->searchPaths_),
    pFileSys_(pFileSys)
{
    wchar_t pathW[core::Path<wchar_t>::BUF_SIZE];

    path_ = strUtil::Convert(path, pathW, sizeof(pathW));

    folder_ = path_;
    folder_.removeFileName();
    folder_.replaceSeprators();
    folder_.ensureSlash();
}

XFindData::~XFindData()
{
}

bool XFindData::findnext(FindData& fi)
{
    if (!current_) {
        return returnFalse(fi);
    }

    if (current_->pDir) {
        return searchDir(current_->pDir, fi);
    }

    if (current_->pPak) {
        // return searchPak(current_->pPak, fi);
    }

    return returnFindhNext(fi);
}

bool XFindData::getOSPath(core::Path<wchar_t>& path, FindData& fi)
{
    if (current_->pDir) {
        path /= current_->pDir->path;
        path.ensureSlash();
        path.append(fi.name.begin(), fi.name.end());
        return true;
    }

    return false;
}

bool XFindData::searchDir(Directory* dir, FindData& fi)
{
    if (handle_ == -1) {
        // new search dir.
        Path<wchar_t> temp;

        core::zero_object(fdw);
        pFileSys_->createOSPath(dir, path_, temp);

        // we have:
        // C:\\Users\\Tom\\Documents\\Visual Studio 2013\\Projects\\WinEngine\\code\\game_folder\\
		// core_assets/white.dds

        // then gives us:
        // white.dds
        // we need to make it back into relative path.

        handle_ = _wfindfirst64(temp.c_str(), &fdw);

        if (handle_ == -1) { // if nothing found in this dir, move on.
            return returnFindhNext(fi);
        }

        // we have found somthing.
        updateFindInfo(fi);
        return true;
    }

    // find next.
    if (_wfindnext64(handle_, &fdw) == 0) {
        updateFindInfo(fi);
        return true;
    }

    // no more files or a error.
    // close the handle.
    _findclose(handle_);
    handle_ = -1;

    // search next folder.
    return returnFindhNext(fi);
}

inline void XFindData::updateFindInfo(FindData& fi)
{
    fi.attrib.Clear();
    if ((fdw.attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        fi.attrib.Set(FindData::AttrFlag::DIRECTORY);
    }

    fi.size = fdw.size;
    fi.name.set(folder_.begin(), folder_.end());

    char buf[core::Path<>::BUF_SIZE] = {};
    fi.name.append(core::strUtil::Convert(fdw.name, buf));
}

inline bool XFindData::searchPak(FindData& fi)
{
    return returnFalse(fi);
}

inline bool XFindData::returnFalse(FindData& fi)
{
    core::zero_object(fi);
    return false;
}

inline bool XFindData::returnFindhNext(FindData& findinfo)
{
    current_ = current_->pNext;
    return findnext(findinfo);
}

X_NAMESPACE_END
