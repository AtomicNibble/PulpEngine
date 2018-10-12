#pragma once

#ifndef _X_XFIND_DATA_H_
#define _X_XFIND_DATA_H_

X_NAMESPACE_BEGIN(core)

class xFileSys;
struct Directory;
struct Pak;
struct Search;
struct FindData;

struct XFindData
{
    XFindData(const wchar_t* path, xFileSys* pFileSys);
    XFindData(const char* path, xFileSys* pFileSys);
    ~XFindData();

    bool findnext(FindData& fi);
    bool getOSPath(core::Path<wchar_t>& path, FindData& fi);

private:
    bool searchDir(Directory* dir, FindData& fi);

    void updateFindInfo(FindData& fi);
    bool searchPak(FindData& fi);
    static bool returnFalse(FindData& fi);
    bool returnFindhNext(FindData& findinfo);

private:
    Path<wchar_t> path_;
    Path<wchar_t> folder_;

    intptr_t handle_;
    _wfinddata64_t fdw;

    Search* current_;
    xFileSys* pFileSys_;
};

X_NAMESPACE_END

#endif // !_X_XFIND_DATA_H_
